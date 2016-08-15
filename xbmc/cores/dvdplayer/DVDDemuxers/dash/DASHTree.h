/*
* DASHTree.h
*****************************************************************************
* Copyright (C) 2015, liberty_developer
*
* Email: liberty.developer@xmail.net
*
* This source code and its use and distribution, is subject to the terms
* and conditions of the applicable license agreement.
*****************************************************************************/

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <inttypes.h>
#include "expat.h"

namespace dash
{
  template <typename T>
  struct SPINCACHE
  {
    SPINCACHE() :basePos(0) {};

    size_t basePos;

    const T *operator[](uint32_t pos) const
    {
      if (!~pos)
        return 0;
      size_t realPos = basePos + pos;
      if (realPos >= data.size())
      {
        realPos -= data.size();
        if (realPos == basePos)
          return 0;
      }
      return &data[realPos];
    };

    uint32_t pos(const T* elem) const
    {
      size_t realPos = elem - &data[0];
      if (realPos < basePos)
        realPos += data.size() - basePos;
      else
        realPos -= basePos;
      return static_cast<std::uint32_t>(realPos);
    };

    void insert(const T &elem)
    {
      data[basePos] = elem;
      ++basePos;
      if (basePos == data.size())
        basePos = 0;
    }

    std::vector<T> data;
  };

  class DASHTree
  {
  public:
    enum StreamType
    {
      NOTYPE,
      VIDEO,
      AUDIO,
      TEXT,
      STREAM_TYPE_COUNT
    };

    // Node definition
    struct Segment
    {
      void SetRange(const char *range);
      uint64_t range_begin_;
      uint64_t range_end_;
      std::string media_;
      uint64_t startPTS_;
    };

    struct SegmentTemplate
    {
      SegmentTemplate() :duration(0), startNumber(1), timescale(0), presentationTimeOffset(0){};
      std::string initialization;
      std::string media;
      unsigned int startNumber;
      unsigned int timescale, duration;
      double presentationTimeOffset;
    };

    struct Representation
    {
      Representation() :timescale_(0), duration_(0), bandwidth_(0), samplingRate_(0), width_(0), height_(0),
        aspect_(1.0f), fpsRate_(0), fpsScale_(1), channelCount_(0), flags_(0), indexRangeMin_(0), indexRangeMax_(0){};
      std::string url_;
      std::string id;
      std::string codecs_;
      std::string codec_private_data_;
      uint32_t bandwidth_;
      uint32_t samplingRate_;
      uint16_t width_, height_;
      uint32_t fpsRate_, fpsScale_;
      float aspect_;
      //Flags
      static const unsigned int BYTERANGE = 0;
      static const unsigned int INDEXRANGEEXACT = 1;
      static const unsigned int TEMPLATE = 2;
      static const unsigned int TIMELINE = 4;
      static const unsigned int INITIALIZATION = 8;
      static const unsigned int TIMETEMPLATE = 16;
      static const unsigned int SEGMENTBASE = 32;
      static const unsigned int SEGMENTMEDIA = 64;
      uint32_t flags_;

      uint32_t indexRangeMin_, indexRangeMax_;
      uint8_t channelCount_;
      SegmentTemplate segtpl_;
      //SegmentList
      uint32_t duration_, timescale_;
      Segment initialization_;
      SPINCACHE<Segment> segments_;
      const Segment *get_initialization()const { return (flags_ & INITIALIZATION) ? &initialization_ : 0; };
      const Segment *get_next_segment(const Segment *seg)const
      {
        if (!seg || seg == &initialization_)
          return segments_[0];
        else
          return segments_[segments_.pos(seg) + 1];
      };

      const Segment *get_segment(uint32_t pos)const
      {
        return segments_[pos];
      };

      const uint32_t get_segment_pos(const Segment *segment)const
      {
        return segments_.data.empty() ? 0: segments_.pos(segment);
      }
    }*current_representation_;

    struct AdaptationSet
    {
      AdaptationSet() :type_(NOTYPE), timescale_(0), startPTS_(0){ language_ = "unk"; };
      ~AdaptationSet(){ for (std::vector<Representation* >::const_iterator b(repesentations_.begin()), e(repesentations_.end()); b != e; ++b) delete *b; };
      StreamType type_;
      uint32_t timescale_;
      uint64_t startPTS_;
      std::string language_;
      std::string mimeType_;
      std::string base_url_;
      std::string codecs_;
      std::vector<Representation*> repesentations_;
      SPINCACHE<uint32_t> segment_durations_;
      const uint32_t get_segment_duration(uint32_t pos)const
      {
        return *segment_durations_[pos];
      };
      SegmentTemplate segtpl_;
    }*current_adaptationset_;

    struct Period
    {
      Period(){};
      ~Period(){ for (std::vector<AdaptationSet* >::const_iterator b(adaptationSets_.begin()), e(adaptationSets_.end()); b != e; ++b) delete *b; };
      std::vector<AdaptationSet*> adaptationSets_;
      std::string base_url_;
    }*current_period_;

    std::vector<Period*> periods_;
    std::string base_url_;

    /* XML Parsing*/
    XML_Parser parser_;
    uint32_t currentNode_;
    uint32_t segcount_;
    double overallSeconds_;
    uint64_t stream_start_, live_start_, publish_time_, base_time_;
    double minPresentationOffset;

    uint32_t bandwidth_;

    double download_speed_, average_download_speed_;

    std::pair<std::string, std::string> pssh_, adp_pssh_;

    enum
    {
      ENCRYTIONSTATE_UNENCRYPTED = 0,
      ENCRYTIONSTATE_ENCRYPTED = 1,
      ENCRYTIONSTATE_SUPPORTED = 2
    };
    unsigned int  encryptionState_;
    uint8_t adpChannelCount_;

    enum
    {
      MPDNODE_MPD = 1 << 0,
      MPDNODE_PERIOD = 1 << 1,
      MPDNODE_ADAPTIONSET = 1 << 2,
      MPDNODE_CONTENTPROTECTION = 1 << 3,
      MPDNODE_REPRESENTATION = 1 << 4,
      MPDNODE_BASEURL = 1 << 5,
      MPDNODE_SEGMENTLIST = 1 << 6,
      MPDNODE_INITIALIZATION = 1 << 7,
      MPDNODE_SEGMENTURL = 1 << 8,
      MPDNODE_SEGMENTDURATIONS = 1 << 9,
      MPDNODE_S = 1 << 11,
      MPDNODE_PSSH = 1 << 12,
      MPDNODE_SEGMENTTEMPLATE = 1 << 13,
      MPDNODE_SEGMENTTIMELINE = 1 << 14
    };
    std::string strXMLText_;

    DASHTree();
    ~DASHTree();
    bool open(const char *url);
    bool has_type(StreamType t);
    uint32_t estimate_segcount(uint32_t duration, uint32_t timescale);
    double get_download_speed() const { return download_speed_; };
    double get_average_download_speed() const { return average_download_speed_; };
    void set_download_speed(double speed);
    void SetFragmentDuration(const AdaptationSet* adp, const Representation* rep, size_t pos, uint32_t fragmentDuration);

    bool empty();
    const AdaptationSet *GetAdaptationSet(unsigned int pos) const { return current_period_ && pos < current_period_->adaptationSets_.size() ? current_period_->adaptationSets_[pos] : 0; };
protected:
  virtual bool download(const char* url);
  bool write_data(void *buffer, size_t buffer_size);
};

}
