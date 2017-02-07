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

#include <inttypes.h>

#include "ap4/Ap4.h"
#include "DASHFragmentObserver.h"
#include "DASHCodecHandler.h"

class CDASHFragmentedSampleReader : public AP4_LinearReader
{
public:

  CDASHFragmentedSampleReader(AP4_ByteStream *input, AP4_Movie *movie, AP4_Track *track,
    AP4_UI32 streamId, AP4_CencSingleSampleDecrypter *ssd, const double pto);
  ~CDASHFragmentedSampleReader();

  AP4_Result Start();
  AP4_Result SeekSample(AP4_UI32 track_id, AP4_UI64 ts, AP4_Ordinal &sample_index, bool preceedingSync);
  AP4_Result ReadSample();
  void Reset();
  void Reset(bool bEOS);
  bool TimeSeek(double pts, bool preceeding);

  bool EOS() const { return m_eos; }
  double DTS() const { return m_dts; }
  double PTS() const { return m_pts; }
  const AP4_Sample &Sample() const { return m_sample_; }
  AP4_UI32 GetStreamId() const { return m_StreamId; }
  AP4_Size GetSampleDataSize() const { return m_sample_data_.GetDataSize(); }
  const AP4_Byte *GetSampleData() const { return m_sample_data_.GetData(); }
  double GetDuration() const { return (double)m_sample_.GetDuration() / (double)m_Track->GetMediaTimeScale(); }
  const AP4_UI08 *GetExtraData() { return m_codecHandler->extra_data; }
  AP4_Size GetExtraDataSize() { return m_codecHandler->extra_data_size; }
  bool GetVideoInformation(int &width, int &height) { return  m_codecHandler->GetVideoInformation(width, height); }
  bool GetAudioInformation(int &channelCount) { return  m_codecHandler->GetAudioInformation(channelCount); }
  void SetObserver(IDASHFragmentObserver *observer) { m_Observer = observer; }
  void SetPTSOffset(uint64_t offset) { FindTracker(m_Track->GetId())->m_NextDts = offset; }
  uint64_t GetFragmentDuration() { return dynamic_cast<AP4_FragmentSampleTable*>(FindTracker(m_Track->GetId())->m_SampleTable)->GetDuration(); }
  uint32_t GetTimeScale() { return m_Track->GetMediaTimeScale(); }
                            
protected:
  virtual AP4_Result ProcessMoof(AP4_ContainerAtom* moof,
    AP4_Position       moof_offset,
    AP4_Position       mdat_payload_offset);

private:
  AP4_Track *m_Track;
  AP4_UI32 m_StreamId;
  bool m_eos, m_started;
  double m_dts, m_pts;
  double m_presentationTimeOffset;

  AP4_Sample     m_sample_;
  AP4_DataBuffer m_encrypted, m_sample_data_;

  CDASHCodecHandler *m_codecHandler;
  const AP4_UI08 *m_DefaultKey;

  AP4_ProtectedSampleDescription *m_Protected_desc;
  AP4_CencSingleSampleDecrypter *m_SingleSampleDecryptor;
  AP4_CencSampleDecrypter *m_Decrypter;
  IDASHFragmentObserver *m_Observer;
};

