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

#include "DASHCodecHandler.h"

#include "utils/log.h"

CAVCDASHCodecHandler::CAVCDASHCodecHandler(AP4_SampleDescription* sd)
  : CDASHCodecHandler(sd)
  , countPictureSetIds(0)
  , needSliceInfo(false)
{
  unsigned int width(0), height(0);
  if (AP4_VideoSampleDescription *video_sample_description = AP4_DYNAMIC_CAST(AP4_VideoSampleDescription, sample_description))
  {
    width = video_sample_description->GetWidth();
    height = video_sample_description->GetHeight();
    CLog::Log(LOGDEBUG, "CAVCDASHCodecHandler wxh (%dx%d)", width, height);
  }
  if (AP4_AvcSampleDescription *avc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sample_description))
  {
    extra_data_size = avc->GetRawBytes().GetDataSize();
    extra_data = avc->GetRawBytes().GetData();
    countPictureSetIds = avc->GetPictureParameters().ItemCount();
    naluLengthSize = avc->GetNaluLengthSize();
    needSliceInfo = (countPictureSetIds > 1 || !width || !height);
  }
}

void CAVCDASHCodecHandler::UpdatePPSId(const AP4_DataBuffer& buffer)
{
  if (!needSliceInfo)
     return;

  //Search the Slice header NALU
  const AP4_UI08 *data(buffer.GetData());
  unsigned int data_size(buffer.GetDataSize());
  for (; data_size;)
  {
    // sanity check
    if (data_size < naluLengthSize)
      break;

    // get the next NAL unit
    AP4_UI32 nalu_size;
    switch (naluLengthSize) {
    case 1:nalu_size = *data++; data_size--; break;
    case 2:nalu_size = AP4_BytesToInt16BE(data); data += 2; data_size -= 2; break;
    case 4:nalu_size = AP4_BytesToInt32BE(data); data += 4; data_size -= 4; break;
    default: data_size = 0; nalu_size = 1; break;
    }
    if (nalu_size > data_size)
      break;

    // Stop further NALU processing
    if (countPictureSetIds < 2)
      needSliceInfo = false;

    unsigned int nal_unit_type = *data & 0x1F;

    if (
      //nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_NON_IDR_PICTURE ||
      nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE //||
      //nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A ||
      //nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B ||
      //nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C
    ) {

      AP4_DataBuffer unescaped(data, data_size);
      AP4_NalParser::Unescape(unescaped);
      AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());

      bits.SkipBits(8); // NAL Unit Type

      auto ReadGolomb = [](AP4_BitReader& bits) -> unsigned int
      {
        unsigned int leading_zeros = 0;
        while (bits.ReadBit() == 0)
        {
          leading_zeros++;
          if (leading_zeros > 32)
            return 0; // safeguard
        }
        if (leading_zeros)
          return (unsigned int)((1 << leading_zeros) - 1 + bits.ReadBits(leading_zeros));

        return 0;
      };

      ReadGolomb(bits); // first_mb_in_slice
      ReadGolomb(bits); // slice_type
      pictureId = ReadGolomb(bits); //picture_set_id
    }
    // move to the next NAL unit
    data += nalu_size;
    data_size -= nalu_size;
  }
}

bool CAVCDASHCodecHandler::GetVideoInformation(int& width, int& height)
{
  if (pictureId == pictureIdPrev)
    return false;
  pictureIdPrev = pictureId;

  if (AP4_VideoSampleDescription *avc = AP4_DYNAMIC_CAST(AP4_VideoSampleDescription, sample_description))
  {
    if (avc->GetWidth() != width || avc->GetHeight() != height)
    {
      width = avc->GetWidth();
      height = avc->GetHeight();
      return true;
    }
  }
  return false;
}


CHEVCDASHCodecHandler::CHEVCDASHCodecHandler(AP4_SampleDescription* sd)
  :CDASHCodecHandler(sd)
{
  if (AP4_HevcSampleDescription *hevc = AP4_DYNAMIC_CAST(AP4_HevcSampleDescription, sample_description))
  {
    extra_data_size = hevc->GetRawBytes().GetDataSize();
    extra_data = hevc->GetRawBytes().GetData();
    naluLengthSize = hevc->GetNaluLengthSize();
  }
}

CMPEGDASHCodecHandler::CMPEGDASHCodecHandler(AP4_SampleDescription* sd)
  :CDASHCodecHandler(sd)
{
  if (AP4_MpegSampleDescription *aac = AP4_DYNAMIC_CAST(AP4_MpegSampleDescription, sample_description))
  {
    extra_data_size = aac->GetDecoderInfo().GetDataSize();
    extra_data = aac->GetDecoderInfo().GetData();
  }
}

bool CMPEGDASHCodecHandler::GetAudioInformation(int& channels)
{
  AP4_AudioSampleDescription *mpeg = AP4_DYNAMIC_CAST(AP4_AudioSampleDescription, sample_description);
  if (mpeg != nullptr && mpeg->GetChannelCount() != channels)
  {
    channels = mpeg->GetChannelCount();
    return true;
  }
  return false;
}
