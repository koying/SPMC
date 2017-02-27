/*
 *      Copyright (C) 2016 Christian Browet
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// http://developer.android.com/reference/android/media/MediaCodec.html
//
// Android MediaCodec class can be used to access low-level media codec,
// i.e. encoder/decoder components. (android.media.MediaCodec). Requires
// SDK16+ which is 4.1 Jellybean and above.
//

#include "DVDAudioCodecAndroidMediaCodec.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDDemuxers/DemuxCrypto.h"
#include "utils/log.h"
#include "androidjni/ByteBuffer.h"
#include "androidjni/MediaCodec.h"
#include "androidjni/MediaCrypto.h"
#include "androidjni/MediaFormat.h"
#include "androidjni/MediaCodecList.h"
#include "androidjni/MediaCodecInfo.h"
#include "android/activity/AndroidFeatures.h"
#include "androidjni/Surface.h"

#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

#include <cassert>

static const AEChannel KnownChannels[] = { AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR, AE_CH_BL, AE_CH_BR, AE_CH_BC, AE_CH_BLOC, AE_CH_BROC, AE_CH_NULL };
static const AMediaUUID WIDEVINE_UUID = { 0xED, 0xEF, 0x8B, 0xA9, 0x79, 0xD6, 0x4A, 0xCE, 0xA3, 0xC8, 0x27, 0xDC, 0xD5, 0x1D, 0x21, 0xED };
static const AMediaUUID PLAYREADY_UUID = { 0x9A, 0x04, 0xF0, 0x79, 0x98, 0x40, 0x42, 0x86, 0xAB, 0x92, 0xE6, 0x5B, 0xE0, 0x88, 0x5F, 0x95 };

/****************************/

CDVDAudioCodecAndroidMediaCodec::CDVDAudioCodecAndroidMediaCodec()
: m_formatname("mediacodec")
, m_opened(false)
, m_buffer    (NULL)
, m_bufferSize(0)
, m_bufferUsed(0)
, m_samplerate(0)
, m_channels(0)
, m_codec(nullptr)
, m_crypto(nullptr)
{
}

CDVDAudioCodecAndroidMediaCodec::~CDVDAudioCodecAndroidMediaCodec()
{
  Dispose();

  if (m_crypto)
    AMediaCrypto_delete(m_crypto);
}

bool CDVDAudioCodecAndroidMediaCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  m_hints = hints;

  CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::Open codec(%d), profile(%d), tag(%d), extrasize(%d)", hints.codec, hints.profile, hints.codec_tag, hints.extrasize);

  switch(m_hints.codec)
  {
    case AV_CODEC_ID_AAC:
    case AV_CODEC_ID_AAC_LATM:
      if (!m_hints.extrasize)
      {
        //TODO Support adts
        return false;
      }
      m_mime = "audio/mp4a-latm";
      m_formatname = "amc-aac";
      break;

    case AV_CODEC_ID_MP2:
      m_mime = "audio/mpeg-L2";
      m_formatname = "amc-mp2";
      break;

    case AV_CODEC_ID_MP3:
      m_mime = "audio/mpeg";
      m_formatname = "amc-mp3";
      break;

    case AV_CODEC_ID_VORBIS:
      m_mime = "audio/vorbis";
      m_formatname = "amc-ogg";

      //TODO
      return false;

      break;

    case AV_CODEC_ID_WMAPRO:
      m_mime = "audio/wmapro";
      m_formatname = "amc-wma";

      //TODO
      return false;

      break;

    case AV_CODEC_ID_WMAV1:
    case AV_CODEC_ID_WMAV2:
      m_mime = "audio/x-ms-wma";
      m_formatname = "amc-wma";
      //TODO
      return false;

      break;

    case AV_CODEC_ID_AC3:
      m_mime = "audio/ac3";
      m_formatname = "amc-ac3";
      break;

    case AV_CODEC_ID_EAC3:
      m_mime = "audio/eac3";
      m_formatname = "amc-eac3";
      break;

    default:
      CLog::Log(LOGNOTICE, "CDVDAudioCodecAndroidMediaCodec:: Unknown hints.codec(%d)", hints.codec);
      return false;
      break;
  }

  if (m_crypto)
    AMediaCrypto_delete( m_crypto);

  bool needSecureDecoder = false;
  if (m_hints.cryptoSession)
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::Open Initializing MediaCrypto");

    if (m_hints.cryptoSession->keySystem == CRYPTO_SESSION_SYSTEM_WIDEVINE)
      m_crypto = AMediaCrypto_new(WIDEVINE_UUID, m_hints.cryptoSession->sessionId, m_hints.cryptoSession->sessionIdSize);
    else if (m_hints.cryptoSession->keySystem == CRYPTO_SESSION_SYSTEM_PLAYREADY)
      m_crypto = AMediaCrypto_new(PLAYREADY_UUID, m_hints.cryptoSession->sessionId, m_hints.cryptoSession->sessionIdSize);
    else
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::Open Unsupported crypto-keysystem:%u", m_hints.cryptoSession->keySystem);
      return false;
    }

    if (!m_crypto)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec: Cannot initilize crypto");
      return false;
    }
    needSecureDecoder = AMediaCrypto_requiresSecureDecoderComponent(m_mime.c_str());
  }
  else
    m_crypto = nullptr;

  m_codec = AMediaCodec_createDecoderByType(m_mime.c_str());
  if (!m_codec)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec:: Failed to create Android MediaCodec");
    return false;
  }

  if (!ConfigureMediaCodec())
  {
    AMediaCodec_delete(m_codec);
    m_codec = nullptr;
    return false;
  }

  CLog::Log(LOGINFO, "CDVDAudioCodecAndroidMediaCodec:: "
    "Open Android MediaCodec %s", m_codecname.c_str());

  m_opened = true;

  return m_opened;
}

void CDVDAudioCodecAndroidMediaCodec::Dispose()
{
  if (!m_opened)
    return;

  m_opened = false;

  if (m_codec)
  {
    AMediaCodec_stop(m_codec);
    AMediaCodec_delete(m_codec);
    m_codec = nullptr;
  }
}

int CDVDAudioCodecAndroidMediaCodec::Decode(const DemuxPacket &packet)
{
  int rtn = 0;

  unsigned char* pData = packet.pData;
  int iSize = packet.iSize;
  double pts = packet.pts;
  double dts = packet.dts;

  if (!pData)
  {
    // Check if we have a saved buffer
    if (m_demux_pkt.pData)
    {
      pData = m_demux_pkt.pData;
      iSize = m_demux_pkt.iSize;
    }
  }

  if (pData)
  {
    // try to fetch an input buffer
    int64_t timeout_us = 5000;
    int index = AMediaCodec_dequeueInputBuffer(m_codec, timeout_us);
    if (index >= 0)
    {
      size_t out_size;
      uint8_t* dst_ptr = AMediaCodec_getInputBuffer(m_codec, index, &out_size);
      if ((size_t)iSize > out_size)
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::Decode, iSize(%d) > size(%d)", iSize, out_size);
        iSize = out_size;
      }
      if (dst_ptr)
      {
        // Codec specifics
        switch(m_hints.codec)
        {
          default:
            memcpy(dst_ptr, pData, iSize);
            break;
        }
        rtn = iSize;
      }


#ifdef DEBUG_VERBOSE
      CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::Decode iSize(%d)", iSize);
#endif

      int64_t presentationTimeUs = 0;
      if (pts != DVD_NOPTS_VALUE)
        presentationTimeUs = pts;
      else if (dts != DVD_NOPTS_VALUE)
        presentationTimeUs = dts;

      AMediaCodecCryptoInfo* cryptoInfo = nullptr;
      if (m_crypto && packet.cryptoInfo)
      {
        size_t clearBytes = packet.cryptoInfo->clearBytes[0];
        size_t cipherBytes = packet.cryptoInfo->cipherBytes[0];
        if (g_advancedSettings.CanLogComponent(LOGAUDIO))
        {
          CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::Decode Crypto, numSamples(%d) - clearBytes(%d) - cipherBytes(%d)", packet.cryptoInfo->numSubSamples, clearBytes, cipherBytes);
          CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::Decode Crypto  kid");
          CLog::MemDump(reinterpret_cast<char*>(packet.cryptoInfo->kid), 16);
          CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::Decode Crypto  iv");
          CLog::MemDump(reinterpret_cast<char*>(packet.cryptoInfo->iv), 16);
        }
        cryptoInfo = AMediaCodecCryptoInfo_new(
              packet.cryptoInfo->numSubSamples,
              packet.cryptoInfo->kid,
              packet.cryptoInfo->iv,
              AMEDIACODECRYPTOINFO_MODE_AES_CTR,
              &clearBytes,
              &cipherBytes);
      }

      int flags = 0;
      int offset = 0;
      media_status_t mstat = AMEDIA_ERROR_UNKNOWN;

      if (cryptoInfo)
      {
        mstat = AMediaCodec_queueSecureInputBuffer(m_codec, index, offset, cryptoInfo, presentationTimeUs, flags);
        AMediaCodecCryptoInfo_delete(cryptoInfo);
      }
      else
        mstat = AMediaCodec_queueInputBuffer(m_codec, index, offset, iSize, presentationTimeUs, flags);
      if (mstat != AMEDIA_OK)
        CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::Decode error(%d)", mstat);

      // Free saved buffer it there was one
      m_demux_pkt.FreeData();
    }
    else
    {
      // We couldn't get an input buffer. Save the packet for next iteration, if it wasn't already
      if (!m_demux_pkt.pData)
        m_demux_pkt = packet;
    }
  }

  return rtn;
}

void CDVDAudioCodecAndroidMediaCodec::Reset()
{
  if (!m_opened)
    return;

  // dump any pending demux packets
  m_demux_pkt.FreeData();

  if (m_codec)
  {
    // now we can flush the actual MediaCodec object
    AMediaCodec_flush(m_codec);
  }
}

CAEChannelInfo CDVDAudioCodecAndroidMediaCodec::GetChannelMap()
{
  CAEChannelInfo chaninfo;

  for (int i=0; i<m_channels; ++i)
    chaninfo += KnownChannels[i];

  return chaninfo;
}

bool CDVDAudioCodecAndroidMediaCodec::ConfigureMediaCodec(void)
{
  // setup a MediaFormat to match the audio content,
  // used by codec during configure
  AMediaFormat* mediaformat = AMediaFormat_new();
  AMediaFormat_setString(mediaformat, AMEDIAFORMAT_KEY_MIME, m_mime.c_str());
  AMediaFormat_setInt32(mediaformat, AMEDIAFORMAT_KEY_SAMPLE_RATE, m_hints.samplerate);
  AMediaFormat_setInt32(mediaformat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, m_hints.channels);

  // handle codec extradata
  if (m_hints.extrasize)
  {
    size_t size = m_hints.extrasize;
    void  *src_ptr = m_hints.extradata;

    AMediaFormat_setBuffer(mediaformat, "csd-0", src_ptr, size);
  }
  else if (m_hints.codec == AV_CODEC_ID_AAC || m_hints.codec == AV_CODEC_ID_AAC_LATM)
  {
    AMediaFormat_setInt32(mediaformat, AMEDIAFORMAT_KEY_IS_ADTS, 1);
  }

  int flags = 0;
  media_status_t mstat;
  mstat = AMediaCodec_configure(m_codec, mediaformat, nullptr, m_crypto, flags);

  if (mstat != AMEDIA_OK)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec configure error: %d", mstat);
    return false;
  }

  mstat = AMediaCodec_start(m_codec);
  if (mstat != AMEDIA_OK)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec start error: %d", mstat);
    return false;
  }

  // There is no guarantee we'll get an INFO_OUTPUT_FORMAT_CHANGED (up to Android 4.3)
  // Configure the output with defaults
  ConfigureOutputFormat(mediaformat);

  return true;
}

int CDVDAudioCodecAndroidMediaCodec::GetData(uint8_t** dst)
{
  m_bufferUsed = 0;

  AMediaCodecBufferInfo bufferInfo;
  int64_t timeout_us = 10000;
  int index = AMediaCodec_dequeueOutputBuffer(m_codec, &bufferInfo, timeout_us);
  if (index >= 0)
  {
    int flags = bufferInfo.flags;
    if (flags & AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
      CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec:: AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED");

    if (flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
    {
      CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec:: AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM");
      AMediaCodec_releaseOutputBuffer(m_codec, index, false);
      return -1;
    }

    size_t out_size;
    uint8_t* buffer = AMediaCodec_getOutputBuffer(m_codec, index, &out_size);
    if (buffer && out_size)
    {
      if (out_size > m_bufferSize)
      {
        m_bufferSize = out_size;
        m_buffer = (uint8_t*)realloc(m_buffer, m_bufferSize);
      }

      memcpy(m_buffer, buffer, out_size);
      m_bufferUsed = out_size;
    }
    else
      return 0;

    media_status_t mstat = AMediaCodec_releaseOutputBuffer(m_codec, index, false);
    if (mstat != AMEDIA_OK)
      CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::GetOutputPicture error: releaseOutputBuffer(%d)", mstat);

#ifdef DEBUG_VERBOSE
    CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec::GetData "
      "index(%d), size(%d)", index, m_bufferUsed);
#endif
  }
  else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
  {
    AMediaFormat* mediaformat = AMediaCodec_getOutputFormat(m_codec);
    if (!mediaformat)
      CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::GetOutputPicture(INFO_OUTPUT_FORMAT_CHANGED) ExceptionCheck: getOutputBuffers");
    else
      ConfigureOutputFormat(mediaformat);
  }
  else if (index == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
  {
    // normal dequeueOutputBuffer timeout, ignore it.
    m_bufferUsed = 0;
  }
  else
  {
    // we should never get here
    CLog::Log(LOGERROR, "CDVDAudioCodecAndroidMediaCodec::GetData unknown index(%d)", index);
  }

  *dst     = m_buffer;
  return m_bufferUsed;
}

void CDVDAudioCodecAndroidMediaCodec::ConfigureOutputFormat(AMediaFormat* mediaformat)
{
  m_samplerate       = 0;
  m_channels         = 0;

  int tmpVal;
  if (AMediaFormat_getInt32(mediaformat, AMEDIAFORMAT_KEY_SAMPLE_RATE, &tmpVal))
    m_samplerate = tmpVal;
  if (AMediaFormat_getInt32(mediaformat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &tmpVal))
    m_channels = tmpVal;

  CLog::Log(LOGDEBUG, "CDVDAudioCodecAndroidMediaCodec:: "
    "sample_rate(%d), channel_count(%d)",
    m_samplerate, m_channels);
}

