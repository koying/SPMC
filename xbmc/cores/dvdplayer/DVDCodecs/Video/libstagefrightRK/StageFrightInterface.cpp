/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistfribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distfributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "StageFrightInterface.h"
#include "RkStagefrightVideo.h"

void* create_stf(CDVDCodecInterface* interface)
{
  return (void*)new CRkStageFrightVideo(interface);
}

void destroy_stf(void* stf)
{
  delete (CRkStageFrightVideo*)stf;
}

bool stf_Open(void* stf, CDVDStreamInfo &hints)
{
  return ((CRkStageFrightVideo*)stf)->Open(hints);
}

void stf_Dispose(void* stf)
{
  ((CRkStageFrightVideo*)stf)->Dispose();
}

int  stf_Decode(void* stf, uint8_t *pData, int iSize, double dts, double pts)
{
  return ((CRkStageFrightVideo*)stf)->Decode(pData, iSize, dts, pts);
}

void stf_Reset(void* stf)
{
  ((CRkStageFrightVideo*)stf)->Reset();
}

bool stf_GetPicture(void* stf, DVDVideoPicture *pDvdVideoPicture)
{
  return ((CRkStageFrightVideo*)stf)->GetPicture(pDvdVideoPicture);
}

bool stf_ClearPicture(void* stf, DVDVideoPicture* pDvdVideoPicture)
{
  return ((CRkStageFrightVideo*)stf)->ClearPicture(pDvdVideoPicture);
}

void stf_SetDropState(void* stf, bool bDrop)
{
  ((CRkStageFrightVideo*)stf)->SetDropState(bDrop);
}

void stf_SetSpeed(void* stf, int iSpeed)
{
  ((CRkStageFrightVideo*)stf)->SetSpeed(iSpeed);
}
