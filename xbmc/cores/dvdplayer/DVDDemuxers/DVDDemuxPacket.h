#pragma once

/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include <memory>

#define DMX_SPECIALID_STREAMINFO    -10
#define DMX_SPECIALID_STREAMCHANGE  -11
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and int64_t

struct DemuxCryptoInfo;

typedef struct DemuxPacket
{
  DemuxPacket() 
    : pData(nullptr)
    , iSize(0)
    , pts(DVD_NOPTS_VALUE)
    , dts(DVD_NOPTS_VALUE)
    , dataOwned(false)
  {}
  
  DemuxPacket(unsigned char *pData, int const iSize, double const pts, double const dts)
    : pData(pData)
    , iSize(iSize)
    , pts(pts)
    , dts(dts)
    , dataOwned(false)
  {}
  
  DemuxPacket(const DemuxPacket& other)
    : iSize(other.iSize)
    , iStreamId(other.iStreamId)
    , iGroupId(other.iGroupId)
    , pts(other.pts)
    , dts(other.dts)
    , duration(other.duration)
    , cryptoInfo(other.cryptoInfo)
    , dataOwned(true)
  {
    pData = reinterpret_cast<unsigned char *>(malloc(iSize));
    memcpy(pData, other.pData, iSize);
  }

  DemuxPacket& operator=(const DemuxPacket& other)
  {
    iSize = other.iSize;
    iStreamId = other.iStreamId;
    iGroupId = other.iGroupId;
    pts = other.pts;
    dts = other.dts;
    duration = other.duration;
    cryptoInfo = other.cryptoInfo;
    dataOwned = true;

    pData = reinterpret_cast<unsigned char *>(malloc(iSize));
    memcpy(pData, other.pData, iSize);
    
    return *this;
  }
  
  ~DemuxPacket()
  {
    FreeData();
  }

  void FreeData()
  {
    if (pData && dataOwned)
    {
      free(pData);
      pData = nullptr;
    }
  }

  unsigned char* pData;   // data
  int iSize;     // data size
  int iStreamId; // integer representing the stream index
  int iGroupId;  // the group this data belongs to, used to group data from different streams together

  double pts; // pts in DVD_TIME_BASE
  double dts; // dts in DVD_TIME_BASE
  double duration; // duration in DVD_TIME_BASE if available
  
  bool dataOwned;
  
  std::shared_ptr<DemuxCryptoInfo> cryptoInfo;
  
} DemuxPacket;
