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

#include "ap4/Ap4.h"

class CDASHCodecHandler
{
public:
  CDASHCodecHandler(AP4_SampleDescription *sd)
    : sample_description(sd)
    , extra_data(0)
    , extra_data_size(0)
    , naluLengthSize(0)
    , pictureId(0)
    , pictureIdPrev(0)
  {}
  virtual ~CDASHCodecHandler() {}
  
  virtual void UpdatePPSId(AP4_DataBuffer const&) {}
  virtual bool GetVideoInformation(int &width, int &height) { return false; }
  virtual bool GetAudioInformation(int &channels) { return false; }

  AP4_SampleDescription *sample_description;
  const AP4_UI08 *extra_data;
  AP4_Size extra_data_size;
  AP4_UI08 naluLengthSize;
  AP4_UI08 pictureId, pictureIdPrev;
};

/***********************   AVC   ************************/

class CAVCDASHCodecHandler : public CDASHCodecHandler
{
public:
  CAVCDASHCodecHandler(AP4_SampleDescription *sd);
  virtual void UpdatePPSId(AP4_DataBuffer const &buffer) override;
  virtual bool GetVideoInformation(int &width, int &height) override;

private:
  unsigned int countPictureSetIds;
  bool needSliceInfo;
};

/***********************   HEVC   ************************/

class CHEVCDASHCodecHandler : public CDASHCodecHandler
{
public:
  CHEVCDASHCodecHandler(AP4_SampleDescription *sd);
};

/***********************   MPEG   ************************/

class CMPEGDASHCodecHandler : public CDASHCodecHandler
{
public:
  CMPEGDASHCodecHandler(AP4_SampleDescription *sd);

  virtual bool GetAudioInformation(int &channels) override;
};

