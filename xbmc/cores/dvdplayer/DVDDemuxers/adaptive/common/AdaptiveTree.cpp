/*
*      Copyright (C) 2016-2016 peak3d
*      http://www.peak3d.de
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
*  <http://www.gnu.org/licenses/>.
*
*/

#include "AdaptiveTree.h"
#include <string.h>

#include "URL.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "filesystem/CurlFile.h"

#include "utils/log.h"

namespace adaptive
{
  void AdaptiveTree::Segment::SetRange(const char *range)
  {
    const char *delim(strchr(range, '-'));
    if (delim)
    {
      range_begin_ = strtoull(range, 0, 10);
      range_end_ = strtoull(delim + 1, 0, 10);
    }
    else
      range_begin_ = range_end_ = 0;
  }

  AdaptiveTree::AdaptiveTree()
    : current_period_(0)
    , parser_(0)
    , currentNode_(0)
    , segcount_(0)
    , overallSeconds_(0.0)
    , stream_start_(0)
    , available_time_(0)
    , publish_time_(0)
    , base_time_(0)
    , minPresentationOffset(0.0)
    , has_timeshift_buffer_(false)
    , download_speed_(0.0)
    , average_download_speed_(0.0f)
    , encryptionState_(ENCRYTIONSTATE_UNENCRYPTED)
  {
  }

  bool AdaptiveTree::has_type(StreamType t)
  {
    if (periods_.empty())
      return false;

    for (std::vector<AdaptationSet*>::const_iterator b(periods_[0]->adaptationSets_.begin()), e(periods_[0]->adaptationSets_.end()); b != e; ++b)
      if ((*b)->type_ == t)
        return true;
    return false;
  }

  uint32_t AdaptiveTree::estimate_segcount(uint32_t duration, uint32_t timescale)
  {
    double tmp(duration);
    tmp /= timescale;
    return static_cast<uint32_t>((overallSeconds_ / tmp)*1.01);
  }

  void AdaptiveTree::set_download_speed(double speed)
  {
    download_speed_ = speed;
    if (!average_download_speed_)
      average_download_speed_ = download_speed_;
    else
      average_download_speed_ = average_download_speed_*0.9 + download_speed_*0.1;
  };

  void AdaptiveTree::SetFragmentDuration(const AdaptationSet* adp, const Representation* rep, size_t pos, uint32_t fragmentDuration, uint32_t movie_timescale)
  {
    if (!has_timeshift_buffer_)
      return;

    //Get a modifiable adaptationset
    AdaptationSet *adpm(static_cast<AdaptationSet *>((void*)adp));

    // Check if its the last frame we watch
    if (adp->segment_durations_.data.size())
    {
      if (pos == adp->segment_durations_.data.size() - 1)
      {
        adpm->segment_durations_.insert(static_cast<std::uint64_t>(fragmentDuration)*adp->timescale_ / movie_timescale);
      }
      else
        return;
    }
    else if (pos != rep->segments_.data.size() - 1)
      return;

    fragmentDuration = static_cast<std::uint32_t>(static_cast<std::uint64_t>(fragmentDuration)*rep->timescale_ / movie_timescale);

    Segment seg(*(rep->segments_[pos]));
    if(~seg.range_begin_)
      seg.range_begin_ += fragmentDuration;
    seg.range_end_ += (rep->flags_ & (Representation::STARTTIMETPL | Representation::TIMETEMPLATE)) ? fragmentDuration : 1;
    seg.startPTS_ += fragmentDuration;

    for (std::vector<Representation*>::iterator b(adpm->repesentations_.begin()), e(adpm->repesentations_.end()); b != e; ++b)
      (*b)->segments_.insert(seg);
  }
  
  bool AdaptiveTree::download_manifest(const char* url)
  {
    // open the file
    CURL uUrl(url);
    if (URIUtils::IsInternetStream(uUrl))
    {
      uUrl.SetProtocolOption("seekable", "0");
      uUrl.SetProtocolOption("acceptencoding", "gzip");

      std::string data;
      XFILE::CCurlFile* file = new XFILE::CCurlFile();
      if (!file->Get(uUrl, data))
        return false;

      m_manifestUrl = file->GetLastEffectiveUrl();
      size_t paramPos = m_manifestUrl.find('?');
      base_url_ = (paramPos == std::string::npos) ? m_manifestUrl : m_manifestUrl.substr(0, paramPos);

      paramPos = base_url_.find_last_of('/', base_url_.length());
      if (paramPos == std::string::npos)
      {
        CLog::Log(LOGERROR, "Invalid adaptive URL: / expected (%s)", m_manifestUrl.c_str());
        return false;
      }
      base_url_.resize(paramPos + 1);

      bool ret =  write_manifest_data(data.c_str(), data.size());
      return ret;
    }
    else
    {
      size_t nbRead = 0;
      XFILE::CFile file;
      if (!file.Open(uUrl, READ_CHUNKED | READ_NO_CACHE))
        return false;

      // read the file
      static const unsigned int CHUNKSIZE = 16384;
      char buf[CHUNKSIZE];
      while ((nbRead = file.Read(buf, CHUNKSIZE)) > 0 && ~nbRead && write_manifest_data(buf, nbRead));

      //download_speed_ = xbmc->GetFileDownloadSpeed(file);

      file.Close();
      return nbRead == 0;
    }
    return false;
  }
}
