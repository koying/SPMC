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
#include "DVDVideoCodecStageFright.h"
#include "utils/log.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "android/jni/Build.h"
#include "utils/StringUtils.h"
#include "DVDCodecs/DVDCodecInterface.h"

#include "DllLibStageFrightCodec.h"

CCriticalSection            valid_mutex;
bool                        CDVDVideoCodecStageFright::m_isvalid = false;
void*                       CDVDVideoCodecStageFright::m_stf_handle = NULL;
std::string                 CDVDVideoCodecStageFright::m_pFormatSource;

#define CLASSNAME "CDVDVideoCodecStageFright"
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

DllLibStageFrightCodec*     CDVDVideoCodecStageFright::m_stf_dll = NULL;

CDVDVideoCodecStageFright::CDVDVideoCodecStageFright()
  : CDVDVideoCodec()
  , m_convert_bitstream(false),  m_converter(NULL)
{
  if (!m_stf_dll)
  {
    m_stf_dll = new DllLibStageFrightCodec;
#if defined(HAS_RKSTF)
    if (StringUtils::StartsWithNoCase(CJNIBuild::HARDWARE, "rk3"))  // Rockchip
    {
      m_pFormatSource = "rkstf";
      m_stf_dll->SetFile(DLL_PATH_LIBSTAGEFRIGHTRK);
    }
    else
#endif
    {
      m_pFormatSource = "stf";
      m_stf_dll->SetFile(DLL_PATH_LIBSTAGEFRIGHTICS);
    }
  }
}

CDVDVideoCodecStageFright::~CDVDVideoCodecStageFright()
{
  Dispose();
}

bool CDVDVideoCodecStageFright::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  // we always qualify even if DVDFactoryCodec does this too.
  if (CSettings::Get().GetBool("videoplayer.usestagefright") && !hints.software)
  {
    m_convert_bitstream = false;
    CLog::Log(LOGDEBUG,
          "%s::%s - trying to open, codec(%d), profile(%d), level(%d)",
          CLASSNAME, __func__, hints.codec, hints.profile, hints.level);

    switch (hints.codec)
    {
      case CODEC_ID_H264:
        m_pFormatName = m_pFormatSource + "-h264";
        if (hints.extrasize < 7 || hints.extradata == NULL)
        {
          CLog::Log(LOGNOTICE,
              "%s::%s - avcC data too small or missing", CLASSNAME, __func__);
          return false;
        }
        m_converter     = new CBitstreamConverter();
        m_convert_bitstream = m_converter->Open(hints.codec, (uint8_t *)hints.extradata, hints.extrasize, true);

        break;
      case CODEC_ID_MPEG2VIDEO:
        m_pFormatName = m_pFormatSource + "-mpeg2";
        break;
      case CODEC_ID_MPEG4:
        m_pFormatName = m_pFormatSource + "-mpeg4";
        break;
      case CODEC_ID_VP3:
      case CODEC_ID_VP6:
      case CODEC_ID_VP6F:
      case CODEC_ID_VP8:
        m_pFormatName = m_pFormatSource + "-vpx";
        break;
      case CODEC_ID_WMV3:
      case CODEC_ID_VC1:
        m_pFormatName = m_pFormatSource + "-wmv";
        break;
      default:
        return false;
        break;
    }

    if (!(m_stf_dll && m_stf_dll->Load()))
      return false;
    m_stf_dll->EnableDelayedUnload(false);

    m_stf_handle = m_stf_dll->create_stf(&g_dvdcodecinterface);

    if (!m_stf_dll->stf_Open(m_stf_handle, hints))
    {
      CLog::Log(LOGERROR,
          "%s::%s - failed to open, codec(%d), profile(%d), level(%d)",
          CLASSNAME, __func__, hints.codec, hints.profile, hints.level);
      Dispose();
      return false;
    }

    CSingleLock lock (valid_mutex);
    m_isvalid = true;
    return true;
  }

  return false;
}

void CDVDVideoCodecStageFright::Dispose()
{
  if (m_converter)
  {
    m_converter->Close();
    delete m_converter;
    m_converter = NULL;
  }

  CSingleLock lock (valid_mutex);
  m_isvalid = false;

  if (m_stf_handle)
  {
    m_stf_dll->stf_Dispose(m_stf_handle);
    m_stf_dll->destroy_stf(m_stf_handle);
    m_stf_handle = NULL;
  }
}

void CDVDVideoCodecStageFright::SetDropState(bool bDrop)
{
  m_stf_dll->stf_SetDropState(m_stf_handle, bDrop);
}

int CDVDVideoCodecStageFright::Decode(uint8_t *pData, int iSize, double dts, double pts)
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

void CDVDVideoCodecStageFright::Reset(void)
{
  m_stf_dll->stf_Reset(m_stf_handle);
}

bool CDVDVideoCodecStageFright::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_stf_dll->stf_GetPicture(m_stf_handle, pDvdVideoPicture);
}

bool CDVDVideoCodecStageFright::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_stf_dll->stf_ClearPicture(m_stf_handle, pDvdVideoPicture);
}

void CDVDVideoCodecStageFright::SetSpeed(int iSpeed)
{
  m_stf_dll->stf_SetSpeed(m_stf_handle, iSpeed);
}

int CDVDVideoCodecStageFright::GetDataSize(void)
{
  return 0;
}

double CDVDVideoCodecStageFright::GetTimeSize(void)
{
  return 0;
}

/********************************************************/

void CDVDVideoCodecStageFrightBuffer::Lock()
{
  if (CDVDVideoCodecStageFright::m_stf_dll && CDVDVideoCodecStageFright::m_stf_handle)
    CDVDVideoCodecStageFright::m_stf_dll->stf_LockBuffer(CDVDVideoCodecStageFright::m_stf_handle, this);
}

long CDVDVideoCodecStageFrightBuffer::Release()
{
  if (CDVDVideoCodecStageFright::m_stf_dll && CDVDVideoCodecStageFright::m_stf_handle)
    CDVDVideoCodecStageFright::m_stf_dll->stf_ReleaseBuffer(CDVDVideoCodecStageFright::m_stf_handle, this);
}

bool CDVDVideoCodecStageFrightBuffer::IsValid()
{
  CSingleLock lock (valid_mutex);
  return CDVDVideoCodecStageFright::m_isvalid;

}

#endif


