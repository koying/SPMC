/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//#define DEBUG_VERBOSE 1

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
#include "config.h"
#elif defined(TARGET_WINDOWS)
#include "system.h"
#endif

#if defined(HAS_LIBSTAGEFRIGHT)
#include "DVDClock.h"
#include "settings/Settings.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecRKStageFright.h"
#include "utils/log.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "android/jni/Build.h"
#include "utils/StringUtils.h"
#include "DVDCodecs/DVDCodecInterface.h"
#include "utils/BitstreamConverter.h"

#include "DllLibStageFrightCodec.h"

CCriticalSection            RKSTF_valid_mutex;
bool                        CDVDVideoCodecRKStageFright::m_isvalid = false;
void*                       CDVDVideoCodecRKStageFright::m_stf_handle = NULL;

#define CLASSNAME "CDVDVideoCodecStageFright"
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

DllLibStageFrightCodec*     CDVDVideoCodecRKStageFright::m_stf_dll = NULL;

CDVDVideoCodecRKStageFright::CDVDVideoCodecRKStageFright()
  : CDVDVideoCodec()
  , m_convert_bitstream(false),  m_converter(NULL)
{
  if (!m_stf_dll)
  {
    m_stf_dll = new DllLibStageFrightCodec;
    m_stf_dll->SetFile(DLL_PATH_LIBSTAGEFRIGHTRK);
  }
  m_pFormatName = "rkstf";
}

CDVDVideoCodecRKStageFright::~CDVDVideoCodecRKStageFright()
{
  Dispose();
}

bool CDVDVideoCodecRKStageFright::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
#if defined(HAS_RKSTF)
  if (!StringUtils::StartsWithNoCase(CJNIBuild::HARDWARE, "rk3"))  // Rockchip
    return false;
#else
  return false;
#endif

// we always qualify even if DVDFactoryCodec does this too.
  if (CSettings::Get().GetBool("videoplayer.userkstagefright") && !hints.software)
  {
    m_convert_bitstream = false;
    CLog::Log(LOGDEBUG,
          "%s::%s - trying to open, codec(%d), profile(%d), level(%d)",
          CLASSNAME, __func__, hints.codec, hints.profile, hints.level);

    m_hints = hints;
    switch (m_hints.codec)
    {
      case AV_CODEC_ID_H264:
//      case AV_CODEC_ID_H264MVC:
        m_pFormatName = "rkstf-h264";
        if (m_hints.extradata)
        {
          m_converter     = new CBitstreamConverter();
          m_convert_bitstream = m_converter->Open(m_hints.codec, (uint8_t *)m_hints.extradata, m_hints.extrasize, true);
          free(m_hints.extradata);
          m_hints.extrasize = m_converter->GetExtraSize();
          m_hints.extradata = malloc(m_hints.extrasize);
          memcpy(m_hints.extradata, m_converter->GetExtraData(), m_converter->GetExtraSize());
        }

        break;
      case AV_CODEC_ID_HEVC:
        m_pFormatName = "rkstf-h265";
        if (m_hints.extradata)
        {
          m_converter     = new CBitstreamConverter();
          m_convert_bitstream = m_converter->Open(m_hints.codec, (uint8_t *)m_hints.extradata, m_hints.extrasize, true);
          free(m_hints.extradata);
          m_hints.extrasize = m_converter->GetExtraSize();
          m_hints.extradata = malloc(m_hints.extrasize);
          memcpy(m_hints.extradata, m_converter->GetExtraData(), m_converter->GetExtraSize());
        }

        break;
      case AV_CODEC_ID_MPEG2VIDEO:
        m_pFormatName = "rkstf-mpeg2";
        break;
      case AV_CODEC_ID_MPEG4:
        m_pFormatName = "rkstf-mpeg4";
        break;
      case AV_CODEC_ID_VP3:
      case AV_CODEC_ID_VP6:
      case AV_CODEC_ID_VP6F:
      case AV_CODEC_ID_VP8:
        m_pFormatName = "rkstf-vpx";
        break;
      case AV_CODEC_ID_WMV3:
      case AV_CODEC_ID_VC1:
        m_pFormatName = "rkstf-wmv";
        break;
      default:
        return false;
        break;
    }

    if (!(m_stf_dll && m_stf_dll->Load()))
      return false;
    m_stf_dll->EnableDelayedUnload(false);

    m_stf_handle = m_stf_dll->create_stf(&g_dvdcodecinterface);

    if (!m_stf_dll->stf_Open(m_stf_handle, m_hints))
    {
      CLog::Log(LOGERROR,
          "%s::%s - failed to open, codec(%d), profile(%d), level(%d)",
          CLASSNAME, __func__, m_hints.codec, m_hints.profile, m_hints.level);
      Dispose();
      return false;
    }

    CSingleLock lock (RKSTF_valid_mutex);
    m_isvalid = true;
    return true;
  }

  return false;
}

void CDVDVideoCodecRKStageFright::Dispose()
{
  if (m_converter)
  {
    m_converter->Close();
    delete m_converter;
    m_converter = NULL;
  }

  CSingleLock lock (RKSTF_valid_mutex);
  m_isvalid = false;

  if (m_stf_handle)
  {
    m_stf_dll->stf_Dispose(m_stf_handle);
    m_stf_dll->destroy_stf(m_stf_handle);
    m_stf_handle = NULL;
  }
}

void CDVDVideoCodecRKStageFright::SetDropState(bool bDrop)
{
  m_stf_dll->stf_SetDropState(m_stf_handle, bDrop);
}

int CDVDVideoCodecRKStageFright::Decode(uint8_t *pData, int iSize, double dts, double pts)
{
#if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  int rtn;
  int demuxer_bytes = iSize;
  uint8_t *demuxer_content = pData;

  if (m_convert_bitstream && demuxer_content)
  {
    // convert demuxer packet from bitstream to bytestream (AnnexB)
    if (m_converter->Convert(demuxer_content, demuxer_bytes))
    {
      demuxer_content = m_converter->GetConvertBuffer();
      demuxer_bytes = m_converter->GetConvertSize();
    }
    else
      CLog::Log(LOGERROR,"%s::%s - bitstream_convert error", CLASSNAME, __func__);
  }
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, ">>> decode conversion - tm:%d\n", XbmcThreads::SystemClockMillis() - time);
#endif

  rtn = m_stf_dll->stf_Decode(m_stf_handle, demuxer_content, demuxer_bytes, dts, pts);

  return rtn;
}

void CDVDVideoCodecRKStageFright::Reset(void)
{
  m_stf_dll->stf_Reset(m_stf_handle);
}

bool CDVDVideoCodecRKStageFright::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_stf_dll->stf_GetPicture(m_stf_handle, pDvdVideoPicture);
}

bool CDVDVideoCodecRKStageFright::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_stf_dll->stf_ClearPicture(m_stf_handle, pDvdVideoPicture);
}

void CDVDVideoCodecRKStageFright::SetSpeed(int iSpeed)
{
  m_stf_dll->stf_SetSpeed(m_stf_handle, iSpeed);
}

int CDVDVideoCodecRKStageFright::GetDataSize(void)
{
  return 0;
}

double CDVDVideoCodecRKStageFright::GetTimeSize(void)
{
  return 0;
}

#endif


