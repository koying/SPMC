/*
 *      Copyright (C) 2016 Christian Browet
 *      Copyright (C) 2016-2016 peak3d
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

#include "DVDDemuxAdaptive.h"


#include "DVDDemuxPacket.h"
#include "DemuxCrypto.h"
#include "DVDDemuxUtils.h"
#include "DVDInputStreams/DVDInputStream.h"

#include "adaptive/DASHByteStream.h"

#ifdef TARGET_ANDROID
#include "androidjni/SystemProperties.h"
#endif
#ifdef TARGET_WINDOWS
#pragma comment(lib, "libexpat.lib")
#pragma comment(lib, "ap4.lib")
#endif

#include "utils/StringUtils.h"
#include "utils/log.h"

CDVDDemuxAdaptive::CDVDDemuxAdaptive()
  : CDVDDemux()
{
  CLog::Log(LOGDEBUG, "CDVDDemuxAdaptive::%s", __FUNCTION__);
}

CDVDDemuxAdaptive::~CDVDDemuxAdaptive()
{
  CLog::Log(LOGDEBUG, "CDVDDemuxAdaptive::%s", __FUNCTION__);
}

bool CDVDDemuxAdaptive::Open(CDVDInputStream* pInput, uint32_t maxWidth, uint32_t maxHeight)
{
  CLog::Log(LOGINFO, "CDVDDemuxAdaptive - matching against %d x %d", maxWidth, maxHeight);
  
  CFileItem item = pInput->GetFileItem();
  CDASHSession::MANIFEST_TYPE type = CDASHSession::MANIFEST_TYPE_UNKNOWN;
  
  if (item.GetMimeType() == "video/vnd.mpeg.dash.mpd"
	  || item.IsType(".mpd")
	  || item.GetProperty("inputstream.adaptive.manifest_type").asString() == "mpd"
	  )
	type = CDASHSession::MANIFEST_TYPE_MPD;
  else if (item.GetMimeType() == "application/vnd.ms-sstr+xml"
           || item.IsType(".ismc")
           || item.IsType(".ism")
           || item.IsType(".isml")
           || item.GetProperty("inputstream.adaptive.manifest_type").asString() == "ism"
           )
    type = CDASHSession::MANIFEST_TYPE_ISM;
 
  if (type == CDASHSession::MANIFEST_TYPE_UNKNOWN)
    return false;
  
  std::string sLicType, sLicKey, sLicData, sServCert;
  if (item.HasProperty("inputstream.adaptive.license_type"))
    sLicType = item.GetProperty("inputstream.adaptive.license_type").asString();
  if (item.HasProperty("inputstream.adaptive.license_key"))
    sLicKey = item.GetProperty("inputstream.adaptive.license_key").asString();
  if (item.HasProperty("inputstream.adaptive.license_data"))
    sLicData = item.GetProperty("inputstream.adaptive.license_data").asString();
  if (item.HasProperty("inputstream.adaptive.server_certificate"))
    sServCert = item.GetProperty("inputstream.adaptive.server_certificate").asString();
  m_session.reset(new CDASHSession(type, pInput->GetFileName(), maxWidth, maxHeight, sLicType, sLicKey, sLicData, sServCert, "special://profile/"));

  if (!m_session->initialize())
  {
    m_session = nullptr;
    return false;
  }
  return true;
}

void CDVDDemuxAdaptive::Dispose()
{
}

void CDVDDemuxAdaptive::Reset()
{
}

void CDVDDemuxAdaptive::Abort()
{
}

void CDVDDemuxAdaptive::Flush()
{
}

DemuxPacket*CDVDDemuxAdaptive::Read()
{
  if (!m_session)
    return NULL;

  CDASHFragmentedSampleReader *sr(m_session->GetNextSample());

  if (m_session->CheckChange())
  {
    DemuxPacket *p = CDVDDemuxUtils::AllocateDemuxPacket(0);
    p->iStreamId = DMX_SPECIALID_STREAMCHANGE;
    CLog::Log(LOGDEBUG, "DMX_SPECIALID_STREAMCHANGE");
    return p;
  }

  if (sr)
  {
    AP4_Size iSize = sr->GetSampleDataSize();
    const AP4_UI08 *pData = sr->GetSampleData();
    DemuxPacket *p;

#ifdef TARGET_ANDROID
    if (iSize && pData && sr->IsEncrypted())
    {
      unsigned int numSubSamples = *((unsigned int*)pData); 
      pData += sizeof(numSubSamples);
      p = CDVDDemuxUtils::AllocateEncryptedDemuxPacket(iSize, numSubSamples);
      memcpy(p->cryptoInfo->clearBytes, pData, numSubSamples * sizeof(uint16_t));
      pData += (numSubSamples * sizeof(uint16_t));
      memcpy(p->cryptoInfo->cipherBytes, pData, numSubSamples * sizeof(uint32_t));
      pData += (numSubSamples * sizeof(uint32_t));
      memcpy(p->cryptoInfo->iv, pData, 16);
      pData += 16;
      memcpy(p->cryptoInfo->kid, pData, 16);
      pData += 16;
      iSize -= (pData - sr->GetSampleData());
      p->cryptoInfo->flags = 0;
    }
    else
#endif
      p = CDVDDemuxUtils::AllocateDemuxPacket(sr->GetSampleDataSize());

    p->dts = sr->DTS() * 1000000;
    p->pts = sr->PTS() * 1000000;
    p->duration = sr->GetDuration() * 1000000;
    p->iStreamId = sr->GetStreamId();
    p->iGroupId = 0;
    p->iSize = iSize;
    memcpy(p->pData, pData, iSize);

    CLog::Log(LOGDEBUG, "CDVDDemuxAdaptive::Read - DTS: %0.4f, PTS:%0.4f, ID: %u SZ: %d CRYPT: %s", p->dts, p->pts, p->iStreamId, p->iSize, sr->IsEncrypted() ? "true" : "false");

    sr->ReadSample();
    return p;
  }
  CLog::Log(LOGDEBUG, "CDVDDemuxAdaptive::Read - No sample");
  return NULL;
}

bool CDVDDemuxAdaptive::SeekTime(int time, bool backwards, double* startpts)
{
  if (!m_session)
    return false;

  return m_session->SeekTime(static_cast<double>(time)*0.001f, 0, !backwards);
}

void CDVDDemuxAdaptive::SetSpeed(int speed)
{
}

int CDVDDemuxAdaptive::GetNrOfStreams()
{
  int n = 0;
  if (m_session)
    n = m_session->GetStreamCount();

  return n;
}

CDemuxStream* CDVDDemuxAdaptive::GetStream(int streamid)
{
  CDASHSession::STREAM *stream(m_session->GetStream(streamid));
  if (!stream)
  {
    CLog::Log(LOGERROR, "CDVDDemuxAdaptive::GetStream(%d): error getting stream", streamid);
    return nullptr;
  }
  return stream->dmuxstrm;
}

void CDVDDemuxAdaptive::EnableStream(int streamid, bool enable)
{
  CLog::Log(LOGDEBUG, "EnableStream(%d: %s)", streamid, enable?"true":"false");

  if (!m_session)
    return;

  CDASHSession::STREAM *stream(m_session->GetStream(streamid));
  if (!stream)
    return;

  if (enable)
  {
    if (stream->enabled)
      return;

    stream->enabled = true;

    stream->stream_.start_stream(~0, m_session->GetWidth(), m_session->GetHeight());
    const adaptive::AdaptiveTree::Representation *rep(stream->stream_.getRepresentation());
    CLog::Log(LOGDEBUG, "Selecting stream with conditions: w: %u, h: %u, bw: %u",
      stream->stream_.getWidth(), stream->stream_.getHeight(), stream->stream_.getBandwidth());

    if (!stream->stream_.select_stream(true, false, stream->dmuxstrm->iPhysicalId >> 16))
    {
      CLog::Log(LOGERROR, "Unable to select stream!");
      return stream->disable();
    }

    if(rep != stream->stream_.getRepresentation())
    {
      m_session->UpdateStream(*stream);
      m_session->CheckChange(true);
    }

    stream->input_ = new CDASHByteStream(&stream->stream_);
    static const AP4_Track::Type TIDC[adaptive::AdaptiveTree::STREAM_TYPE_COUNT] = { 
      AP4_Track::TYPE_UNKNOWN,
      AP4_Track::TYPE_VIDEO,
      AP4_Track::TYPE_AUDIO,
      AP4_Track::TYPE_TEXT };

    AP4_Movie* movie = nullptr;
    if (m_session->GetManifestType() == CDASHSession::MANIFEST_TYPE_ISM && stream->stream_.getRepresentation()->get_initialization() == nullptr)
    {
      //We'll create a Movie out of the things we got from manifest file
      //note: movie will be deleted in destructor of stream->input_file_
      movie = new AP4_Movie();
      
      AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();
      AP4_SampleDescription *sample_descryption = nullptr;
      AP4_UI32 format = 0;
      if (rep->codecs_ == "avc1" || rep->codecs_ == "avcb" || rep->codecs_ == "h264")
      {
        AP4_MemoryByteStream* data = new AP4_MemoryByteStream(reinterpret_cast<const AP4_UI08*>(rep->codec_private_data_.data()), rep->codec_private_data_.size());
        CLog::Log(LOGDEBUG, "extradata: %d / %d", rep->codec_private_data_.size(), data->GetDataSize());
        AP4_AvccAtom* codecAtom = AP4_AvccAtom::Create(data->GetDataSize() + AP4_ATOM_HEADER_SIZE, *data);
        assert(codecAtom != NULL);
        data->Release();
        format = AP4_SAMPLE_FORMAT_AVC1;
        sample_descryption = new AP4_AvcSampleDescription(format, rep->width_, rep->height_, 0, "", codecAtom);
      }
      else
        sample_descryption = new AP4_SampleDescription(AP4_SampleDescription::TYPE_UNKNOWN, format, 0);
      if (stream->stream_.getAdaptationSet()->encrypted)
      {
        AP4_ContainerAtom schi(AP4_ATOM_TYPE_SCHI);
        schi.AddChild(new AP4_TencAtom(AP4_CENC_ALGORITHM_ID_CTR, 8, m_session->GetDefaultKeyId()));
        sample_descryption = new AP4_ProtectedSampleDescription(format, sample_descryption, format, AP4_PROTECTION_SCHEME_TYPE_PIFF, 0, "", &schi);
      }
      sample_table->AddSampleDescription(sample_descryption);
      
      movie->AddTrack(new AP4_Track(TIDC[stream->stream_.get_type()], sample_table, ~0, stream->stream_.getRepresentation()->timescale_, 0, stream->stream_.getRepresentation()->timescale_, 0, "", 0, 0));
      //Create a dumy MOOV Atom to tell Bento4 its a fragmented stream
      AP4_MoovAtom *moov = new AP4_MoovAtom();
      moov->AddChild(new AP4_ContainerAtom(AP4_ATOM_TYPE_MVEX));
      movie->SetMoovAtom(moov);    
    }

    stream->input_file_ = new AP4_File(*stream->input_, AP4_DefaultAtomFactory::Instance_, true, movie);
    movie = stream->input_file_->GetMovie();
    if (movie == NULL)
    {
      CLog::Log(LOGERROR, "No MOOV in stream!");
      return stream->disable();
    }

    AP4_Track *track = movie->GetTrack(TIDC[stream->stream_.get_type()]);
    if (!track)
    {
      CLog::Log(LOGERROR, "No suitable track found in stream");
      return stream->disable();
    }

    stream->reader_ = new CDASHFragmentedSampleReader(stream->input_, movie, track, streamid, m_session->GetSingleSampleDecryptor(), m_session->GetPresentationTimeOffset());
    stream->reader_->SetObserver(dynamic_cast<IDASHFragmentObserver*>(m_session.get()));

    if (!stream->dmuxstrm->ExtraSize && stream->reader_->GetExtraDataSize())
    {
      // ExtraData is now available......
      stream->dmuxstrm->ExtraSize = stream->reader_->GetExtraDataSize();      
      stream->dmuxstrm->ExtraData = (uint8_t*)malloc(stream->dmuxstrm->ExtraSize);
      memcpy((void*)stream->dmuxstrm->ExtraData, stream->reader_->GetExtraData(), stream->dmuxstrm->ExtraSize);
      // Set the session Changed to force new GetStreamInfo call from kodi -> addon
      m_session->CheckChange(true);
    }
    return;
  }
  CLog::Log(LOGDEBUG, ">>>> ERROR");
  return stream->disable();
}

int CDVDDemuxAdaptive::GetStreamLength()
{
  if (!m_session)
    return 0;

  return static_cast<int>(m_session->GetTotalTime()*1000);
}

std::string CDVDDemuxAdaptive::GetFileName()
{
  if (!m_session)
    return "";

  return m_session->GetUrl();
}

void CDVDDemuxAdaptive::GetStreamCodecName(int iStreamId, std::string& strName)
{
  strName = "";

  CDASHSession::STREAM *stream(m_session->GetStream(iStreamId));
  if (stream)
    strName = stream->codecName;
}
