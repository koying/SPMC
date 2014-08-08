#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://www.xbmc.org
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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "vpu_global.h"
#include "vpu_mem.h"

#include "cores/dvdplayer/DVDStreamInfo.h"
#include "DVDVideoCodec.h"

class CDVDCodecInterface;
class CStageFrightVideoPrivate;

namespace android { class MediaBuffer; }

class CStageFrightVideo
{
public:
  CStageFrightVideo(CDVDCodecInterface* interface);
  virtual ~CStageFrightVideo();

  bool Open(CDVDStreamInfo &hints);
  void Dispose(void);
  int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  void Reset(void);
  bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  void SetDropState(bool bDrop);
  virtual void SetSpeed(int iSpeed);

  void LockBuffer(CDVDVideoCodecStageFrightBuffer* buf);
  void ReleaseBuffer(CDVDVideoCodecStageFrightBuffer* buf);

private:
  CStageFrightVideoPrivate* p;

  void LockEGLBuffer(EGLImageKHR eglimg);
  bool ReleaseEGLBuffer(EGLImageKHR eglimg);

  void LockVpuFrame(VPU_FRAME* vpuframe);
  bool ReleaseVpuFrame(VPU_FRAME* vpuframe);

};

