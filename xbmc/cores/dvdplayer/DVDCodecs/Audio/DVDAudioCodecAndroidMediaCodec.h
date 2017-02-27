#pragma once
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

#include <queue>
#include <vector>
#include <memory>

#include "DVDAudioCodec.h"
#include "DVDStreamInfo.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaCrypto.h>

class CDVDAudioCodecAndroidMediaCodec : public CDVDAudioCodec
{
public:
  CDVDAudioCodecAndroidMediaCodec();
  virtual ~CDVDAudioCodecAndroidMediaCodec();

  // required overrides
public:
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int  Decode(const DemuxPacket &packet);
  virtual int  GetData(uint8_t** dst);
  virtual void Reset();
  virtual int  GetChannels               () { return m_channels; }
  virtual int  GetEncodedChannels        () { return m_channels; }
  virtual CAEChannelInfo GetChannelMap       ();
  virtual int  GetSampleRate             () { return m_samplerate; }
  virtual int  GetEncodedSampleRate      () { return m_samplerate; }
  virtual enum AEDataFormat GetDataFormat() { return AE_FMT_S16NE; }
  virtual bool NeedPassthrough           () { return false;        }
  virtual const char* GetName            () { return "mediacodec"; }

protected:
  bool            ConfigureMediaCodec(void);
  int             GetOutputPicture(void);
  void            ConfigureOutputFormat(AMediaFormat* mediaformat);

  // surface handling functions
  static void     CallbackInitSurfaceTexture(void*);
  void            InitSurfaceTexture(void);
  void            ReleaseSurfaceTexture(void);

  CDVDStreamInfo  m_hints;
  std::string     m_mime;
  std::string     m_codecname;
  std::string     m_formatname;
  bool            m_opened;
  int             m_samplerate;
  int             m_channels;
  uint8_t*        m_buffer;
  size_t          m_bufferSize;
  int             m_bufferUsed;

  AMediaCodec*    m_codec;
  AMediaCrypto*   m_crypto;
  DemuxPacket     m_demux_pkt;
};
