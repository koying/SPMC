#pragma once

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

#include <cstdint>
#include <float.h>

#include "SSD_dll.h"
#include "DASHFragmentObserver.h"
#include "DASHFragmentedSampleReader.h"
#include "DASHStream.h"
#include "common/AdaptiveTree.h"

#include "DVDDemuxers/DVDDemux.h"

class CDASHSession: public IDASHFragmentObserver
{
public:
  class STREAM
  {
  public:
    STREAM(adaptive::AdaptiveTree &t, adaptive::AdaptiveTree::StreamType s);
    ~STREAM();
    void disable();

    bool enabled;
    uint32_t current_segment_;
    dash::DASHStream stream_;
    AP4_ByteStream *input_;
    AP4_File *input_file_;
    CDemuxStream* dmuxstrm;
    uint32_t bandwidth;
    std::string codecName;
    std::string codecInternalName;
    CDASHFragmentedSampleReader *reader_;
  };

  enum MANIFEST_TYPE
  {
    MANIFEST_TYPE_UNKNOWN,
    MANIFEST_TYPE_MPD,
    MANIFEST_TYPE_ISM
  };
  
  CDASHSession(const CDASHSession::MANIFEST_TYPE manifest_type, const std::string& strURL, int width, int height, const std::string& strLicType, const char* strLicKey, const char* profile_path);
  virtual ~CDASHSession();
  bool initialize();
  CDASHFragmentedSampleReader *GetNextSample();

  void UpdateStream(STREAM &stream);

  std::string GetUrl() { return fileURL_; }
  STREAM *GetStream(unsigned int sid) const { return (sid < streams_.size() ? streams_[sid] : 0); }
  unsigned int GetStreamCount() const { return streams_.size(); }
  const AP4_DataBuffer &GetCryptoData() { return m_cryptoData; }
  std::uint16_t GetWidth() const { return width_; }
  std::uint16_t GetHeight() const { return height_; }
  AP4_CencSingleSampleDecrypter * GetSingleSampleDecryptor() const { return single_sample_decryptor_; }
  double GetPresentationTimeOffset() { return adaptiveTree_->minPresentationOffset < DBL_MAX? adaptiveTree_->minPresentationOffset:0; }
  double GetTotalTime() const { return adaptiveTree_->overallSeconds_; }
  double GetPTS() const { return last_pts_; }
  bool CheckChange(bool bSet = false);
  void SetVideoResolution(unsigned int w, unsigned int h);
  bool SeekTime(double seekTime, unsigned int streamId = 0, bool preceeding=true);
  bool IsLive() const { return adaptiveTree_->has_timeshift_buffer_; }
  MANIFEST_TYPE GetManifestType() const { return manifest_type_; }
  const AP4_UI08 *GetDefaultKeyId() const;

  //Observer Section
  void BeginFragment(AP4_UI32 streamId) override;
  void EndFragment(AP4_UI32 streamId) override;

protected:
  bool GetSupportedDecrypterURN(std::pair<std::string, std::string> &urn);
  AP4_CencSingleSampleDecrypter *CreateSingleSampleDecrypter(AP4_DataBuffer &streamCodec);

private:
  MANIFEST_TYPE manifest_type_;
  std::string fileURL_;
  std::string license_key_, license_type_, license_data_;
  AP4_DataBuffer server_certificate_;
  std::string profile_path_;
  SSD::SSD_DECRYPTER *decrypter_;
  AP4_DataBuffer m_cryptoData;
  
  adaptive::AdaptiveTree* adaptiveTree_;

  std::vector<STREAM*> streams_;

  uint16_t width_, height_;
  uint16_t maxwidth_, maxheight_;
  uint32_t fixed_bandwidth_;
  bool changed_;
  bool manual_streams_;
  double last_pts_;

  AP4_CencSingleSampleDecrypter *single_sample_decryptor_;
};

