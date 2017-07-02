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

#include "DASHSession.h"

#include "DVDDemuxers/DVDDemux.h"

#include "parsers/DASHTree.h"
#include "parsers/SmoothTree.h"

#include "system.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "filesystem/File.h"

CDASHSession::CDASHSession(const CDASHSession::MANIFEST_TYPE manifest_type, const std::string& strURL, int width, int height, const char *strLicType, const char* strLicKey, const char* profile_path)
  :single_sample_decryptor_(0)
  , manifest_type_(manifest_type)
  , fileURL_(strURL)
  , license_type_(strLicType)
  , license_key_(strLicKey)
  , profile_path_(profile_path)
  , adaptiveTree_(nullptr)
  , width_(width)
  , height_(height)
  , last_pts_(0)
  , decrypterModule_(0)
//  , decrypter_(0)
  , changed_(false)
  , manual_streams_(false)
{
  switch (manifest_type)
  {
    case MANIFEST_TYPE_MPD:
      adaptiveTree_ = new adaptive::DASHTree;
      break;
    case MANIFEST_TYPE_ISM:
      adaptiveTree_ = new adaptive::SmoothTree;
      break;
    default:;
  };
  
  XFILE::CFile f;

  std::string fn = URIUtils::AddFileToFolder(profile_path_, "bandwidth.bin");
  if (f.Open(fn, READ_NO_CACHE))
  {
    double val;
    f.Read((void*)&val, sizeof(double));
    adaptiveTree_->bandwidth_ = static_cast<uint32_t>(val);
    f.Close();
  }
  else
    adaptiveTree_->bandwidth_ = 4000000;
  adaptiveTree_->set_download_speed(adaptiveTree_->bandwidth_);
  CLog::Log(LOGDEBUG, "CDASHSession - Initial bandwidth: %u ", adaptiveTree_->bandwidth_);

  manual_streams_ = false;
}

CDASHSession::~CDASHSession()
{
  for (std::vector<STREAM*>::iterator b(streams_.begin()), e(streams_.end()); b != e; ++b)
    SAFE_DELETE(*b);
  streams_.clear();

//  if (decrypterModule_)
//  {
//    dlclose(decrypterModule_);
//    decrypterModule_ = 0;
//    decrypter_ = 0;
//  }

  XFILE::CFile f;

  std::string fn = URIUtils::AddFileToFolder(profile_path_, "bandwidth.bin");
  if (f.OpenForWrite(fn, READ_NO_CACHE))
  {
    double val(adaptiveTree_->get_average_download_speed());
    f.Write((const void*)&val, sizeof(double));
    f.Close();
  }
  else
    CLog::Log(LOGERROR, "CDASHSession - Cannot write bandwidth.bin");
}

CDASHSession::STREAM::STREAM(adaptive::AdaptiveTree& t, adaptive::AdaptiveTree::StreamType s)
  : stream_(t, s)
  , enabled(false)
  , current_segment_(0)
  , input_(0)
  , dmuxstrm(nullptr)
  , reader_(0)
  , input_file_(0)
{
}

CDASHSession::STREAM::~STREAM()
{
  disable();
  if (dmuxstrm)
    SAFE_DELETE(dmuxstrm);
}

void CDASHSession::STREAM::disable()
{
  if (enabled)
  {
    stream_.stop();
    SAFE_DELETE(reader_);
    SAFE_DELETE(input_file_);
    SAFE_DELETE(input_);
    enabled = false;
  }
}

//void CDASHSession::GetSupportedDecrypterURN(std::pair<std::string, std::string> &urn)
//{
//  typedef SSD_DECRYPTER *(*CreateDecryptorInstanceFunc)(SSD_HOST *host, uint32_t version);

//  char specialpath[1024];
//  if (!xbmc->GetSetting("DECRYPTERPATH", specialpath))
//  {
//    CLog::Log(LOGDEBUG, "DECRYPTERPATH not specified in settings.xml");
//    return;
//  }
//  addonstring path(xbmc->TranslateSpecialProtocol(specialpath));

//  kodihost.SetLibraryPath(path.c_str());

//  VFSDirEntry *items(0);
//  unsigned int num_items(0);

//  CLog::Log(LOGDEBUG, "Searching for decrypters in: %s", path.c_str());

//  if (!xbmc->GetDirectory(path.c_str(), "", &items, &num_items))
//    return;

//  for (unsigned int i(0); i < num_items; ++i)
//  {
//    if (strncmp(items[i].label, "ssd_", 4) && strncmp(items[i].label, "libssd_", 7))
//      continue;

//    void * mod(dlopen(items[i].path, RTLD_LAZY));
//    if (mod)
//    {
//      CreateDecryptorInstanceFunc startup;
//      if ((startup = (CreateDecryptorInstanceFunc)dlsym(mod, "CreateDecryptorInstance")))
//      {
//        SSD_DECRYPTER *decrypter = startup(&kodihost, SSD_HOST::version);
//        const char *suppUrn(0);

//        if (decrypter && (suppUrn = decrypter->Supported(license_type_.c_str(), license_key_.c_str())))
//        {
//          CLog::Log(LOGDEBUG, "Found decrypter: %s", items[i].path);
//          decrypterModule_ = mod;
//          decrypter_ = decrypter;
//          urn.first = suppUrn;
//          break;
//        }
//      }
//      dlclose(mod);
//    }
//  }
//  xbmc->FreeDirectory(items, num_items);
//}

//AP4_CencSingleSampleDecrypter *CDASHSession::CreateSingleSampleDecrypter(AP4_DataBuffer &streamCodec)
//{
//  if (decrypter_)
//    return decrypter_->CreateSingleSampleDecrypter(streamCodec);
//  return 0;
//};

/*----------------------------------------------------------------------
|   initialize
+---------------------------------------------------------------------*/

bool CDASHSession::initialize()
{
  // Get URN's wich are supported by this addon
//  if (!license_type_.empty())
//  {
//    GetSupportedDecrypterURN(dashtree_->adp_pssh_);
//    CLog::Log(LOGDEBUG, "Supported URN: %s", dashtree_->adp_pssh_.first.c_str());
//  }

  // Open mpd file
  size_t paramPos = fileURL_.find('?');
  adaptiveTree_->base_url_ = (paramPos == std::string::npos) ? fileURL_ : fileURL_.substr(0, paramPos);
  
  paramPos = adaptiveTree_->base_url_.find_last_of('/', adaptiveTree_->base_url_.length());
  if (paramPos == std::string::npos)
  {
    CLog::Log(LOGERROR, "Invalid adaptive URL: / expected (%s)", fileURL_.c_str());
    return false;
  }
  adaptiveTree_->base_url_.resize(paramPos + 1);

  if (!adaptiveTree_->open(fileURL_.c_str()) || adaptiveTree_->empty())
  {
    CLog::Log(LOGERROR, "Could not open / parse adaptive URL (%s)", fileURL_.c_str());
    return false;
  }
  CLog::Log(LOGINFO, "Successfully parsed adaptive manifest. #Streams: %d Download speed: %0.4f Bytes/s", adaptiveTree_->periods_[0]->adaptationSets_.size(), adaptiveTree_->download_speed_);

  if (adaptiveTree_->encryptionState_ == adaptive::AdaptiveTree::ENCRYTIONSTATE_ENCRYPTED)
  {
    CLog::Log(LOGERROR, "Unable to handle decryption. Unsupported!");
    return false;
  }

  uint32_t min_bandwidth(0), max_bandwidth(0);
  /*
  {
    int buf;
    xbmc->GetSetting("MINBANDWIDTH", (char*)&buf); min_bandwidth = buf;
    xbmc->GetSetting("MAXBANDWIDTH", (char*)&buf); max_bandwidth = buf;
  }
  */

  // create CDASHSession::STREAM objects. One for each AdaptationSet
  const adaptive::AdaptiveTree::AdaptationSet *adp;

  for (std::vector<STREAM*>::iterator b(streams_.begin()), e(streams_.end()); b != e; ++b)
    SAFE_DELETE(*b);
  streams_.clear();

  for (unsigned int i=0; (adp = adaptiveTree_->GetAdaptationSet(i)); ++i)
  {
    size_t repId = manual_streams_ ? adp->repesentations_.size() : 0;

    do {
      streams_.push_back(new STREAM(*adaptiveTree_, adp->type_));
      STREAM &stream(*streams_.back());
      stream.stream_.prepare_stream(adp, width_, height_, min_bandwidth, max_bandwidth, repId);

      switch (adp->type_)
      {
        case adaptive::AdaptiveTree::VIDEO:
          stream.dmuxstrm = new CDemuxStreamVideo();
          break;
        case adaptive::AdaptiveTree::AUDIO:
          stream.dmuxstrm = new CDemuxStreamAudio();
          break;
        case adaptive::AdaptiveTree::TEXT:
          stream.dmuxstrm = new CDemuxStreamTeletext();
          break;
        default:
          break;
      }
      stream.dmuxstrm->iId = i;
      stream.dmuxstrm->iPhysicalId = i | (repId << 16);
      strcpy(stream.dmuxstrm->language, adp->language_.c_str());
      stream.dmuxstrm->ExtraData = nullptr;
      stream.dmuxstrm->ExtraSize = 0;

      UpdateStream(stream);

    } while (repId--);
  }

//  // Try to initialize an SingleSampleDecryptor
//  if (dashtree_->encryptionState_)
//  {
//    AP4_DataBuffer init_data;

//    if (dashtree_->adp_pssh_.second == "FILE")
//    {
//      std::string strkey(dashtree_->adp_pssh_.first.substr(9));
//      size_t pos;
//      while ((pos = strkey.find('-')) != std::string::npos)
//        strkey.erase(pos, 1);
//      if (strkey.size() != 32)
//      {
//        CLog::Log(LOGERROR, "Key system mismatch (%s)!", dashtree_->adp_pssh_.first.c_str());
//        return false;
//      }

//      unsigned char key_system[16];
//      AP4_ParseHex(strkey.c_str(), key_system, 16);

//      CDASHSession::STREAM *stream(streams_[0]);

//      stream->enabled = true;
//      stream->stream_.start_stream(0, width_, height_);
//      stream->stream_.select_stream(true,false, stream->info_.m_pID>>16);

//      stream->input_ = new AP4_DASHStream(&stream->stream_);
//      stream->input_file_ = new AP4_File(*stream->input_, AP4_DefaultAtomFactory::Instance_, true);
//      AP4_Movie* movie = stream->input_file_->GetMovie();
//      if (movie == NULL)
//      {
//        CLog::Log(LOGERROR, "No MOOV in stream!");
//        stream->disable();
//        return false;
//      }
//      AP4_Array<AP4_PsshAtom*>& pssh = movie->GetPsshAtoms();

//      for (unsigned int i = 0; !init_data.GetDataSize() && i < pssh.ItemCount(); i++)
//      {
//        if (memcmp(pssh[i]->GetSystemId(), key_system, 16) == 0)
//          init_data.AppendData(pssh[i]->GetData().GetData(), pssh[i]->GetData().GetDataSize());
//      }

//      if (!init_data.GetDataSize())
//      {
//        CLog::Log(LOGERROR, "Could not extract license from video stream (PSSH not found)");
//        stream->disable();
//        return false;
//      }
//      stream->disable();
//    }
//    else
//    {
//      if (manifest_type_ == MANIFEST_TYPE_ISM)
//      {
//        create_ism_license(adaptiveTree_->defaultKID_, license_data_, init_data);
//      }
//      else
//      {
//        init_data.SetBufferSize(1024);
//        unsigned int init_data_size(1024);
//        b64_decode(dashtree_->pssh_.second.data(), dashtree_->pssh_.second.size(), init_data.UseData(), init_data_size);
//        init_data.SetDataSize(init_data_size);
//      }
//    }
//    return (single_sample_decryptor_ = CreateSingleSampleDecrypter(init_data))!=0;
//  }
  return true;
}

void CDASHSession::UpdateStream(STREAM &stream)
{
  const adaptive::AdaptiveTree::Representation *rep(stream.stream_.getRepresentation());

  // we currently use only the first track!
  std::string::size_type pos = rep->codecs_.find(",");
  if (pos == std::string::npos)
    pos = rep->codecs_.size();

  stream.codecInternalName = rep->codecs_.substr(0, pos);

  if (rep->codecs_.find("mp4a") == 0 || rep->codecs_.find("aacl") == 0)
    stream.codecName = "aac";
  else if (rep->codecs_.find("ec-3") == 0 || rep->codecs_.find("ac-3") == 0)
    stream.codecName = "eac3";
  else if (rep->codecs_.find("avc") == 0 || rep->codecs_.find("h264") == 0)
    stream.codecName = "h264";
  else if (rep->codecs_.find("hevc") == 0)
    stream.codecName = "hevc";
  else if (rep->codecs_.find("vp9") == 0)
    stream.codecName = "vp9";
  else if (rep->codecs_.find("opus") == 0)
    stream.codecName = "opus";
  else if (rep->codecs_.find("vorbis") == 0)
    stream.codecName = "vorbis";
  else if (rep->codecs_.find("wvc1") == 0)
    stream.codecName = "vc1";
  else if (rep->codecs_.find("wmap") == 0)
    stream.codecName = "wmapro";

  AVCodec *codec = avcodec_find_decoder_by_name(stream.codecName.c_str());
  if (codec)
    stream.dmuxstrm->codec = codec->id;
  else
    CLog::Log(LOGERROR, "UpdateStream: cannot find codec %s (%s)", stream.codecName.c_str(), rep->codecs_.c_str());

  stream.bandwidth = rep->bandwidth_;

  if (stream.dmuxstrm->type == STREAM_VIDEO)
  {
    CDemuxStreamVideo* vstrm = static_cast<CDemuxStreamVideo*>(stream.dmuxstrm);
    vstrm->iWidth = rep->width_;
    vstrm->iHeight = rep->height_;
    vstrm->fAspect = rep->aspect_;
    vstrm->iFpsRate = rep->fpsRate_;
    vstrm->iFpsScale = rep->fpsScale_;
    vstrm->sStreamInfo = StringUtils::Format("ADP Video: %s / %d x %d / %d kbps", stream.codecName.c_str(), rep->width_, rep->height_, rep->bandwidth_ / 1024);

    if (!vstrm->ExtraSize && rep->codec_private_data_.size())
    {
      vstrm->ExtraSize = rep->codec_private_data_.size();
      vstrm->ExtraData = (uint8_t*)malloc(vstrm->ExtraSize);
      memcpy((void*)vstrm->ExtraData, rep->codec_private_data_.data(), vstrm->ExtraSize);
    }
  }
  else if (stream.dmuxstrm->type == STREAM_AUDIO)
  {
    CDemuxStreamAudio* astrm = static_cast<CDemuxStreamAudio*>(stream.dmuxstrm);
    astrm->iSampleRate = rep->samplingRate_;
    astrm->iChannels = rep->channelCount_;
    astrm->sStreamInfo = StringUtils::Format("ADP Audio: %s / %d ch / %d Hz / %d kbps", stream.codecName.c_str(), rep->channelCount_, rep->samplingRate_, rep->bandwidth_ / 1024);
  }
}

bool CDASHSession::CheckChange(bool bSet)
{
  bool ret = changed_;
  changed_ = bSet;
  return ret;
}

void CDASHSession::SetVideoResolution(unsigned int w, unsigned int h)
{
  width_ = w < maxwidth_ ? w : maxwidth_;
  height_ = h < maxheight_ ? h : maxheight_;
}

CDASHFragmentedSampleReader *CDASHSession::GetNextSample()
{
  STREAM *res(0);
  for (std::vector<STREAM*>::const_iterator b(streams_.begin()), e(streams_.end()); b != e; ++b)
    if ((*b)->enabled && !(*b)->reader_->EOS()
    && (!res || (*b)->reader_->DTS() < res->reader_->DTS()))
        res = *b;

  if (res && AP4_SUCCEEDED(res->reader_->Start()))
  {
    CDemuxStreamVideo* vstrm = dynamic_cast<CDemuxStreamVideo*>(res->dmuxstrm);
    if (vstrm)
    {
      if (res->reader_->GetVideoInformation(vstrm->iWidth, vstrm->iHeight))
        changed_ = true;
    } else {
      CDemuxStreamAudio* astrm = dynamic_cast<CDemuxStreamAudio*>(res->dmuxstrm);
      if (res->reader_->GetAudioInformation(astrm->iChannels))
        changed_ = true;
    }
    last_pts_ = res->reader_->PTS();
    return res->reader_;
  }
  return 0;
}

bool CDASHSession::SeekTime(double seekTime, unsigned int streamId, bool preceeding)
{
  bool ret(false);

  for (std::vector<STREAM*>::const_iterator b(streams_.begin()), e(streams_.end()); b != e; ++b)
    if ((*b)->enabled && (streamId == 0 || (*b)->dmuxstrm->iId == streamId))
    {
      bool bReset;
      if ((*b)->stream_.seek_time(seekTime + GetPresentationTimeOffset(), last_pts_, bReset))
      {
        if (bReset)
          (*b)->reader_->Reset(false);
        if (!(*b)->reader_->TimeSeek(seekTime, preceeding))
          (*b)->reader_->Reset(true);
        else
        {
          CLog::Log(LOGINFO, "seekTime(%0.4f) for Stream:%d continues at %0.4f", seekTime, (*b)->dmuxstrm->iId, (*b)->reader_->PTS());
          ret = true;
        }
      }
      else
        (*b)->reader_->Reset(true);
    }
  return ret;
}

const AP4_UI08*CDASHSession::GetDefaultKeyId() const
{
  static const AP4_UI08 default_key[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
  if (adaptiveTree_->defaultKID_.size() == 16)
    return reinterpret_cast<const AP4_UI08 *>(adaptiveTree_->defaultKID_.data());
  return default_key;
}

void CDASHSession::BeginFragment(AP4_UI32 streamId)
{
  STREAM *s(streams_[streamId]);
  s->reader_->SetPTSOffset(s->stream_.GetPTSOffset());
}

void CDASHSession::EndFragment(AP4_UI32 streamId)
{
  STREAM *s(streams_[streamId]);
  adaptiveTree_->SetFragmentDuration(
        s->stream_.getAdaptationSet(), 
        s->stream_.getRepresentation(), 
        s->stream_.getSegmentPos(), 
        s->reader_->GetFragmentDuration(),
        s->reader_->GetTimeScale()
        );
}


