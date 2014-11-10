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

#include "vpu_global.h"
#include "vpu_mem.h"

#include "cores/dvdplayer/DVDStreamInfo.h"
#include "DVDVideoCodec.h"

#include "Application.h"
#include "ApplicationMessenger.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "guilib/GraphicContext.h"

#include <utils/RefBase.h>
#include <binder/ProcessState.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <gui/SurfaceTexture.h>
#include <media/stagefright/MediaSource.h>

#include "threads/Thread.h"
#include "libavcodec/avcodec.h"

#include <list>
#include <map>

class CDVDCodecInterface;
class CStageFrightVideoPrivate;
class CStageFrightDecodeThread;
struct Frame;

namespace android { class MediaBuffer; }

using namespace android;

class CRkStageFrightVideo
{
  friend class CStageFrightMediaSource;
  friend class CStageFrightDecodeThread;

public:
  CRkStageFrightVideo(CDVDCodecInterface* interface);
  virtual ~CRkStageFrightVideo();

  bool Open(CDVDStreamInfo &hints);
  void Dispose(void);
  int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  void Reset(void);
  bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  void SetDropState(bool bDrop);
  virtual void SetSpeed(int iSpeed);

  void LockVpuFrame(VPU_FRAME *vpuframe);
  bool ReleaseVpuFrame(VPU_FRAME *vpuframe);
  void Render(const CRect &SrcRect, const CRect &DestRect, const VPU_FRAME* buf);

protected:
  CStageFrightVideoPrivate* p;
  CStageFrightDecodeThread* decode_thread;

  CAdvancedSettings* m_g_advancedSettings;
  CXBMCRenderManager* m_g_renderManager;
  CGraphicContext* m_g_graphicsContext;

  OMXClient *m_omxclient;
  sp<MediaSource> m_mediasource;
  sp<MediaSource> m_stfdecoder;
  sp<MetaData> m_metadata;

  std::queue<Frame*> m_in_queue;
  std::map<int64_t, Frame*> m_out_queue;
  CCriticalSection in_mutex;
  CCriticalSection out_mutex;
  CCriticalSection free_mutex;
  XbmcThreads::ConditionVariable in_condition;
  XbmcThreads::ConditionVariable out_condition;
  std::list <Frame*> outbuf_queue;
  std::list< VPU_FRAME* > m_busy_vpu_queue;

  AVCodecID codec;
  int videoColorFormat;
  int videoStride;
  int videoSliceHeight;
  int x, y;
  int width, height;

  bool drop_state;
  bool resetting;

  int m_fb1_fd;
  std::queue<VPU_FRAME*> m_prev_stfbuf;

private:
  static void RenderFeaturesCallBack(const void *ctx, Features &renderFeatures);
  static void RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect, const void* render_ctx);
  static void RenderLockCallBack(const void *ctx, const void *render_ctx);
  static void RenderReleaseCallBack(const void *ctx, const void *render_ctx);
};

