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


#include "DASHFragmentedSampleReader.h"

#include "utils/log.h"

CDASHFragmentedSampleReader::CDASHFragmentedSampleReader(AP4_ByteStream* input, AP4_Movie* movie, AP4_Track* track, AP4_UI32 streamId, AP4_CencSingleSampleDecrypter* ssd, const double pto)
  : AP4_LinearReader(*movie, input)
  , m_Track(track)
  , m_dts(0.0)
  , m_pts(0.0)
  , m_fail_count_(0)
  , m_eos(false)
  , m_started(false)
  , m_StreamId(streamId)
  , m_SampleDescIndex(1)
  , m_SingleSampleDecryptor(ssd)
  , m_Decrypter(0)
  , m_Protected_desc(0)
  , m_codecHandler(0)
  , m_Observer(0)
  , m_DefaultKey(0)
  , m_presentationTimeOffset(pto)
  , m_bSampleDescChanged(false)
{
  EnableTrack(m_Track->GetId());

  AP4_SampleDescription *desc(m_Track->GetSampleDescription(0));
  if (desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED)
  {
    m_Protected_desc = static_cast<AP4_ProtectedSampleDescription*>(desc);
    desc = m_Protected_desc->GetOriginalSampleDescription();
  }
  UpdateSampleDescription();
}

CDASHFragmentedSampleReader::~CDASHFragmentedSampleReader()
{
  delete m_Decrypter;
  delete m_codecHandler;
}

AP4_Result CDASHFragmentedSampleReader::Start()
{
  if (m_started)
    return AP4_SUCCESS;
  m_started = true;
  return ReadSample();
}

AP4_Result CDASHFragmentedSampleReader::SeekSample(AP4_UI32 track_id, AP4_UI64 ts, AP4_Ordinal& sample_index, bool preceedingSync)
{
  // we only support fragmented sources for now
  if (!m_HasFragments)
    return AP4_ERROR_NOT_SUPPORTED;

  if (m_Trackers.ItemCount() == 0) {
    return AP4_ERROR_NO_SUCH_ITEM;
  }

  // look for a sample from a specific track
  Tracker* tracker = FindTracker(track_id);
  if (tracker == NULL)
    return AP4_ERROR_INVALID_PARAMETERS;

  // don't continue if we've reached the end of that tracker
  if (tracker->m_Eos)
    return AP4_ERROR_EOS;

  AP4_Result result;

  if (!tracker->m_SampleTable && AP4_FAILED(result = Advance()))
    return result;

  while (AP4_FAILED(result = tracker->m_SampleTable->GetSampleIndexForTimeStamp(ts, sample_index)))
  {
    if (result == AP4_ERROR_NOT_ENOUGH_DATA)
    {
      tracker->m_NextSampleIndex = tracker->m_SampleTable->GetSampleCount();
      if (AP4_FAILED(result = Advance()))
        return result;
      continue;
    }
    return result;
  }

  sample_index = tracker->m_SampleTable->GetNearestSyncSampleIndex(sample_index, preceedingSync);
  //we have reached the end -> go for the first sample of the next segment
  if (sample_index == tracker->m_SampleTable->GetSampleCount())
  {
    tracker->m_NextSampleIndex = tracker->m_SampleTable->GetSampleCount();
    if (AP4_FAILED(result = Advance()))
      return result;
    sample_index = 0;
  }
  return SetSampleIndex(tracker->m_Track->GetId(), sample_index);
}

AP4_Result CDASHFragmentedSampleReader::ReadSample()
{
  AP4_Result result;
  if (AP4_FAILED(result = ReadNextSample(m_Track->GetId(), m_sample_, m_Protected_desc ? m_encrypted : m_sample_data_)))
  {
    if (result == AP4_ERROR_EOS)
      m_eos = true;
    return result;
  }

  if (m_Protected_desc)
  {
    if (m_Decrypter)
    {
      // Make sure that the decrypter is NOT allocating memory!
      // If decrypter and addon are compiled with different DEBUG / RELEASE
      // options freeing HEAP memory will fail.
      m_sample_data_.Reserve(m_encrypted.GetDataSize() + 4096);
      m_SingleSampleDecryptor->SetFrameInfo(m_DefaultKey?16:0, m_DefaultKey, m_codecHandler->naluLengthSize);

      if (AP4_FAILED(result = m_Decrypter->DecryptSampleData(m_encrypted, m_sample_data_, NULL)))
      {
        CLog::Log(LOGERROR, "Decrypt Sample returns failure!");
        if (++m_fail_count_ > 50)
        {
          Reset(true);
          return result;
        }
        else
          m_sample_data_.SetDataSize(0);
      }
      else
        m_fail_count_ = 0;
    }
    else
      result = m_sample_data_.SetData(m_encrypted.GetData(), m_encrypted.GetDataSize());
  }

  m_dts = (double)m_sample_.GetDts() / (double)m_Track->GetMediaTimeScale() - m_presentationTimeOffset;
  m_pts = (double)m_sample_.GetCts() / (double)m_Track->GetMediaTimeScale() - m_presentationTimeOffset;

  m_codecHandler->UpdatePPSId(m_sample_data_);

  return AP4_SUCCESS;
}

void CDASHFragmentedSampleReader::Reset()
{
  // flush any queued samples
  FlushQueues();

  // reset tracker states
  for (unsigned int i = 0; i<m_Trackers.ItemCount(); i++) {
    if (m_Trackers[i]->m_SampleTableIsOwned) {
      delete m_Trackers[i]->m_SampleTable;
    }
    delete m_Trackers[i]->m_NextSample;
    m_Trackers[i]->m_SampleTable = NULL;
    m_Trackers[i]->m_NextSample = NULL;
    m_Trackers[i]->m_NextSampleIndex = 0;
    m_Trackers[i]->m_Eos = false;
  }
  m_NextFragmentPosition = 0;
}

void CDASHFragmentedSampleReader::Reset(bool bEOS)
{
  Reset();
  m_eos = bEOS;
}

bool CDASHFragmentedSampleReader::TimeSeek(double pts, bool preceeding)
{
  AP4_Ordinal sampleIndex;
  if (AP4_SUCCEEDED(SeekSample(m_Track->GetId(), static_cast<AP4_UI64>((pts+ m_presentationTimeOffset)*(double)m_Track->GetMediaTimeScale()), sampleIndex, preceeding)))
  {
    if (m_Decrypter)
      m_Decrypter->SetSampleIndex(sampleIndex);
    m_started = true;
    return AP4_SUCCEEDED(ReadSample());
  }
  return false;
}

uint64_t CDASHFragmentedSampleReader::GetFragmentDuration()
{
  Tracker* trk = FindTracker(m_Track->GetId());
  if (!trk->m_SampleTable)
    return 0;
  
  return dynamic_cast<AP4_FragmentSampleTable*>(trk->m_SampleTable)->GetDuration(); 
}

AP4_Result CDASHFragmentedSampleReader::ProcessMoof(AP4_ContainerAtom* moof, AP4_Position moof_offset, AP4_Position mdat_payload_offset)
{
  AP4_Result result = AP4_SUCCESS;

  if (m_Observer)
    m_Observer->BeginFragment(m_StreamId);

  // create a new fragment
  delete m_Fragment;
  m_Fragment = new AP4_MovieFragment(moof);

  // update the trackers
  AP4_Array<AP4_UI32> ids;
  m_Fragment->GetTrackIds(ids);
  for (unsigned int i=0; i<m_Trackers.ItemCount() && AP4_SUCCEEDED(result); i++)
  {
    Tracker* tracker = m_Trackers[i];
    if (tracker->m_SampleTableIsOwned) {
      delete tracker->m_SampleTable;
    }
    tracker->m_SampleTable = NULL;
    tracker->m_NextSampleIndex = 0;
    for (unsigned int j=0; j<ids.ItemCount() && AP4_SUCCEEDED(result); j++) {
      if (ids.ItemCount()==1 || ids[j] == tracker->m_Track->GetId()) {
        AP4_FragmentSampleTable* sample_table = NULL;
        result = m_Fragment->CreateSampleTable(&m_Movie,
                                               ids[j],
                                               m_FragmentStream,
                                               moof_offset,
                                               mdat_payload_offset,
                                               tracker->m_NextDts,
                                               sample_table);
        if (AP4_FAILED(result)) break;
        tracker->m_SampleTable = sample_table;
        tracker->m_SampleTableIsOwned = true;
        tracker->m_Eos = false;
        break;
      }
    }
  }

  if (AP4_SUCCEEDED(result))
  {
    //Check if the sample table description has changed
    AP4_ContainerAtom *traf = AP4_DYNAMIC_CAST(AP4_ContainerAtom, moof->GetChild(AP4_ATOM_TYPE_TRAF, 0));
    if (!traf)
      return AP4_ERROR_INVALID_FORMAT;

    AP4_TfhdAtom *tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD, 0));
    if (tfhd && tfhd->GetSampleDescriptionIndex() != m_SampleDescIndex)
    {
      m_SampleDescIndex = tfhd->GetSampleDescriptionIndex();
      UpdateSampleDescription();
    }
    else if (m_SampleDescIndex != 1)
    {
      m_SampleDescIndex = 1;
      UpdateSampleDescription();
    }

    if (m_Protected_desc)
    {
      //Setup the decryption
      AP4_CencSampleInfoTable *sample_table;
      AP4_UI32 algorithm_id = 0;

      delete m_Decrypter;
      m_Decrypter = 0;

      if (AP4_FAILED(result = AP4_CencSampleInfoTable::Create(m_Protected_desc, traf, algorithm_id, *m_FragmentStream, moof_offset, sample_table)))
      {
        // we assume unencrypted fragment here
        CLog::Log(LOGDEBUG, "AP4_CencSampleInfoTable::Create failed !!");
        return AP4_SUCCESS;
      }

      AP4_ContainerAtom *schi;
      m_DefaultKey = 0;
      if (m_Protected_desc->GetSchemeInfo() && (schi = m_Protected_desc->GetSchemeInfo()->GetSchiAtom()))
      {
        AP4_TencAtom* tenc(AP4_DYNAMIC_CAST(AP4_TencAtom, schi->GetChild(AP4_ATOM_TYPE_TENC, 0)));
        if (tenc)
          m_DefaultKey = tenc->GetDefaultKid();
      }

      m_Decrypter = new AP4_CencSampleDecrypter(m_SingleSampleDecryptor, sample_table, false);
      if (!m_Decrypter)
        return AP4_ERROR_INVALID_PARAMETERS;
    }
  }

  if (m_Observer)
    m_Observer->EndFragment(m_StreamId);

  return result;
}

void CDASHFragmentedSampleReader::UpdateSampleDescription()
{
  if (m_codecHandler)
    delete m_codecHandler;
  m_codecHandler = 0;
  m_bSampleDescChanged = true;

  AP4_SampleDescription *desc(m_Track->GetSampleDescription(m_SampleDescIndex - 1));
  if (!desc)
    return;
  
  if (desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED)
  {
    m_Protected_desc = static_cast<AP4_ProtectedSampleDescription*>(desc);
    desc = m_Protected_desc->GetOriginalSampleDescription();
  }
  switch (desc->GetFormat())
  {
  case AP4_SAMPLE_FORMAT_AVC1:
  case AP4_SAMPLE_FORMAT_AVC2:
  case AP4_SAMPLE_FORMAT_AVC3:
  case AP4_SAMPLE_FORMAT_AVC4:
    m_codecHandler = new CAVCDASHCodecHandler(desc);
    break;
  case AP4_SAMPLE_FORMAT_HEV1:
  case AP4_SAMPLE_FORMAT_HVC1:
    m_codecHandler = new CHEVCDASHCodecHandler(desc);
    break;
  case AP4_SAMPLE_FORMAT_MP4A:
    m_codecHandler = new CMPEGDASHCodecHandler(desc);
    break;
  default:
    m_codecHandler = new CDASHCodecHandler(desc);
    break;
  }
}

