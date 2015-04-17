/*
 *      Copyright (C) 2013 Team XBMC
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
/***************************************************************************/

//#define DEBUG_VERBOSE 1

#include "system.h"
#include "system_gl.h"

#include "RkCodecVideo.h"

//#include "android/activity/XBMCApp.h"
#include "guilib/GraphicContext.h"
#include "DVDClock.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"
#include "utils/StringUtils.h"
#include "threads/Thread.h"
#include "threads/Event.h"
#include "settings/AdvancedSettings.h"
#include "DVDCodecs/DVDCodecInterface.h"
#include "DVDVideoCodecRKStageFright.h"

#include <linux/fb.h>
#include <new>

#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#define RKON2_ALIGN(value, x)   ((value + (x-1)) & (~(x-1)))

#define CLASSNAME "CRkCodecVideo"

#define XMEDIA_BITSTREAM_START_CODE      (0x42564b52)
#define RK_FBIOSET_YUV_ADDR	         0x5002
#define HAL_PIXEL_FORMAT_RGB_565         0x04
#define HAL_PIXEL_FORMAT_YCrCb_NV12      0x20
#define HAL_PIXEL_FORMAT_YCrCb_NV12_10   0x22
#define OMX_BUFFERFLAG_EXTRADATA         0x00000040

static int64_t pts_dtoi(double pts)
{
  return (int64_t)(pts);
}

CRkCodecVideo::CRkCodecVideo(CDVDCodecInterface* interface)
  : mVpuCtx(NULL), mPool(NULL)
  , mExtraData(NULL), mExtraDataSize(0)
  , mWidth(-1), mHeight(-1)
  , drop_state(false), resetting(false)
{
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::ctor: %d\n", CLASSNAME, sizeof(CRkStageFrightVideo));
#endif
  m_g_advancedSettings = interface->GetAdvancedSettings();
  m_g_renderManager = interface->GetRenderManager();
  m_g_graphicsContext = interface->GetGraphicsContext();

  mCurPacket.pData = NULL;
  mCurPacket.iSize = 0;

  memset(&mOn2DecPrivate, 0, sizeof(On2DecPrivate_t));
}

CRkCodecVideo::~CRkCodecVideo()
{
}

bool CRkCodecVideo::Open(CDVDStreamInfo &hints)
{
#define RK_FBIOSET_VSYNC_ENABLE     0x4629
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Open: ed:%d as:%f fa:%d\n", CLASSNAME, hints.extrasize, hints.aspect, hints.forced_aspect);
#endif

  // stagefright crashes with null size. Trap this...
  if (!hints.width || !hints.height)
  {
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"null size, cannot handle");
    return false;
  }

  m_fb1_fd = open("/dev/graphics/fb1", O_RDWR,0);
  if(m_fb1_fd < 0)
  {
    CLog::Log(LOGERROR, "GLES: Cannot open FB1");
    return false;
  }

  if (ioctl(m_fb1_fd, RK_FBIOSET_VSYNC_ENABLE, 1) == -1)
  {
    CLog::Log(LOGDEBUG, "%s(%d):  RK_FBIOSET_VSYNC_ENABLE[%d] Failed", __FUNCTION__, __LINE__, m_fb1_fd);
  }

  mWidth     = hints.width;
  mHeight    = hints.height;
  if (!hints.forced_aspect)
    mAspectRatio = hints.aspect;
  else
    mAspectRatio = 1.0;

  std::string use_codec;
  std::map<std::string, std::string> codec_config = m_g_advancedSettings->m_codecconfigs["rockchip"];

  int ret = vpu_open_context(&mVpuCtx);
  if (ret || mVpuCtx == NULL) {
    CLog::Log(LOGERROR, "%s::%s - Cannot instantiate vpu context: %p (%d)", CLASSNAME, __func__, mVpuCtx, ret);
    return false;
  }

  mVpuCtx->enableparsing = 0;
  mVpuCtx->width = mWidth;
  mVpuCtx->height = mHeight;
  mVpuCtx->codecType = CODEC_DECODER;
  mVpuCtx->no_thread = 0 /* 1 */;

  switch (hints.codec)
  {
    case AV_CODEC_ID_HEVC:
      use_codec = codec_config["useHEVCcodec"];
      if (use_codec == "0"
          || (use_codec == "sd" && hints.width > 800)
          || (use_codec == "hd" && hints.width <= 800)
          || (use_codec == "uhd" && hints.width <= 2000))
        return false;
      mCodecId = OMX_RK_VIDEO_CodingHEVC;
      mExtraDataSize = hints.extrasize;
      mExtraData = malloc(mExtraDataSize);
      memcpy(mExtraData, hints.extradata, mExtraDataSize);
      break;
    case AV_CODEC_ID_H264:
//  case AV_CODEC_ID_H264MVC:
      switch(hints.profile)
      {
        case FF_PROFILE_H264_HIGH_10:
        case FF_PROFILE_H264_HIGH_10_INTRA:
        case FF_PROFILE_H264_HIGH_422:
        case FF_PROFILE_H264_HIGH_422_INTRA:
        case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
        case FF_PROFILE_H264_HIGH_444_INTRA:
        case FF_PROFILE_H264_CAVLC_444:
          // Hi10P not supported
          return false;
      }

      use_codec = codec_config["useAVCcodec"];
      if (use_codec == "0"
          || (use_codec == "sd" && hints.width > 800)
          || (use_codec == "hd" && hints.width <= 800)
          || (use_codec == "uhd" && hints.width <= 2000))
        return false;
      mCodecId = OMX_ON2_VIDEO_CodingAVC;
      mExtraDataSize = hints.extrasize;
      mExtraData = malloc(mExtraDataSize);
      memcpy(mExtraData, hints.extradata, mExtraDataSize);
      break;
    case AV_CODEC_ID_MPEG4:
      use_codec = codec_config["useMP4codec"];
      if (use_codec == "0"
          || (use_codec == "sd" && hints.width > 800)
          || (use_codec == "hd" && hints.width <= 800)
          || (use_codec == "uhd" && hints.width <= 2000))
        return false;
      mCodecId = OMX_ON2_VIDEO_CodingMPEG4;
      mVpuCtx->enableparsing = 1;
      break;
    case AV_CODEC_ID_MPEG2VIDEO:
      use_codec = codec_config["useMPEG2codec"];
      if (use_codec == "0"
          || (use_codec == "sd" && hints.width > 800)
          || (use_codec == "hd" && hints.width <= 800)
          || (use_codec == "uhd" && hints.width <= 2000))
        return false;
      mCodecId = OMX_ON2_VIDEO_CodingMPEG2;
      break;
    case AV_CODEC_ID_VP8:
      use_codec = codec_config["useVPXcodec"];
      if (use_codec == "0"
          || (use_codec == "sd" && hints.width > 800)
          || (use_codec == "hd" && hints.width <= 800)
          || (use_codec == "uhd" && hints.width <= 2000))
        return false;
      mCodecId = OMX_ON2_VIDEO_CodingVP8;
      mVpuCtx->enableparsing = 1;
      break;
    case AV_CODEC_ID_WMV3:
      use_codec = codec_config["useVC1codec"];
      if (use_codec == "0"
          || (use_codec == "sd" && hints.width > 800)
          || (use_codec == "hd" && hints.width <= 800)
          || (use_codec == "uhd" && hints.width <= 2000))
        return false;
      mCodecId = OMX_ON2_VIDEO_CodingWMV;
      mVpuCtx->enableparsing = 1;
      mExtraDataSize = hints.extrasize;
      mExtraData = malloc(mExtraDataSize);
      memcpy(mExtraData, hints.extradata, mExtraDataSize);
      break;
    case AV_CODEC_ID_VC1:
      use_codec = codec_config["useVC1codec"];
      if (use_codec == "0"
          || (use_codec == "sd" && hints.width > 800)
          || (use_codec == "hd" && hints.width <= 800)
          || (use_codec == "uhd" && hints.width <= 2000))
        return false;
      mCodecId = OMX_ON2_VIDEO_CodingVC1;
      mVpuCtx->enableparsing = 1;
      mExtraDataSize = hints.extrasize;
      mExtraData = malloc(mExtraDataSize);
      memcpy(mExtraData, hints.extradata, mExtraDataSize);
    break;
  default:
    return false;
    break;
  }

  mVpuCtx->videoCoding = mCodecId;
  if(mVpuCtx->init(mVpuCtx, (RK_U8*)mExtraData, mExtraDataSize))
  {
    CLog::Log(LOGERROR, "%s::%s - Cannot initialize vpu", CLASSNAME, __func__);
    free(mExtraData);
    return false;
  }

  if ((mWidth != 0) && (mHeight != 0) && !mVpuCtx->no_thread)
  {
    int32_t align_w = mWidth;
    if (mCodecId == OMX_RK_VIDEO_CodingHEVC)
      align_w = ((mWidth+255)&(~255))| 256;
    if (create_vpu_memory_pool_allocator(&mPool, 8, (align_w*mHeight*2)))
    {
      CLog::Log(LOGERROR, "%s::%s - Cannot create vpu_mem_pool", CLASSNAME, __func__);
      free(mExtraData);
      return false;
    }
    if(mVpuCtx->control(mVpuCtx, VPU_API_SET_VPUMEM_CONTEXT, (void*)mPool) < 0)
    {
      if(mPool != NULL)
      {
        release_vpu_memory_pool_allocator(mPool);
        mPool = NULL;
      }
    }
  }

  m_g_renderManager->RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);
  m_g_renderManager->RegisterRenderFeaturesCallBack((const void*)this, RenderFeaturesCallBack);
  m_g_renderManager->RegisterRenderLockCallBack((const void*)this, RenderLockCallBack);
  m_g_renderManager->RegisterRenderReleaseCallBack((const void*)this, RenderReleaseCallBack);

  if (mExtraData && mCodecId == OMX_ON2_VIDEO_CodingVC1)
    Decode((uint8_t*)mExtraData, mExtraDataSize, 0, 0);

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, ">>> format col:%d, w:%d, h:%d, sw:%d, sh:%d, ctl:%d,%d; cbr:%d,%d\n", videoColorFormat, width, height, videoStride, videoSliceHeight, cropTop, cropLeft, cropBottom, cropRight);
#endif

  return true;
}

int32_t CRkCodecVideo::checkVideoInfoChange(void* aFrame)
{
  if (aFrame ==NULL)
    return 0;

  if (OMX_RK_VIDEO_CodingHEVC == mCodecId)
    return 0;

  VPU_FRAME* frame = (VPU_FRAME*)aFrame;
  On2DecPrivate_t* pOn2Privat = &mOn2DecPrivate;
  int32_t change = 0;

  int32_t w_old = RKON2_ALIGN(mWidth, 16);
  int32_t h_old = RKON2_ALIGN(mHeight, 16);
  int32_t w_new = RKON2_ALIGN(frame->DisplayWidth, 16);
  int32_t h_new = RKON2_ALIGN(frame->DisplayHeight, 16);

  if (!(pOn2Privat->flags & MBAFF_MODE_INFO_CHANGE) &&
      ((w_old != w_new) || (h_old != h_new)))
  {
    mWidth = frame->DisplayWidth;
    mHeight = frame->DisplayHeight;
    change =1;
    goto CHECK_INFO_OUT;
  }

  if (!(pOn2Privat->flags & MBAFF_MODE_INFO_CHANGE) &&
      (frame->FrameHeight > 0) &&
      (frame->FrameHeight != h_new))
  {
    mHeight = frame->FrameHeight;
    pOn2Privat->flags |= MBAFF_MODE_INFO_CHANGE;
    change =1;
    goto CHECK_INFO_OUT;
  }

CHECK_INFO_OUT:
  if (change)
    CLog::Log(LOGINFO, "video size change, from (%d x %d) to (%d x %d)",
              w_old, h_old, mWidth, mHeight);
  return change;
}

void CRkCodecVideo::releaseframe(void *aFrame)
{
  VPU_FRAME* frame = (VPU_FRAME*)aFrame;
  if (frame->vpumem.phy_addr)
  {
    VPUMemLink(&frame->vpumem);
    VPUFreeLinear(&frame->vpumem);
  }
}

/*** Decode ***/
int  CRkCodecVideo::Decode(uint8_t *pData, int iSize, double dts, double pts)
{
#if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s::Decode - d:%p; s:%d; dts:%f; pts:%f\n", CLASSNAME, pData, iSize, dts, pts);
#endif

  On2DecPrivate_t* pOn2Privat = &mOn2DecPrivate;
  int ret = 0;

  int64_t ipts = (pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : ((dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : 0);

  if (pData)
  {
    if (mCurPacket.pData == NULL)
    {
      if (mCodecId == OMX_ON2_VIDEO_CodingMPEG2)
      {
        mCurPacket.iSize = iSize + sizeof(VPU_BITSTREAM);
        mCurPacket.pData = (uint8_t*)malloc(iSize + sizeof(VPU_BITSTREAM));

        VPU_BITSTREAM m2vheader;
        m2vheader.StartCode = XMEDIA_BITSTREAM_START_CODE;
        m2vheader.SliceLength = iSize;
        m2vheader.SliceTime.TimeLow = (uint32_t)(ipts / 1000);
        m2vheader.SliceTime.TimeHigh = 0;
        m2vheader.SliceType = 0;
        m2vheader.SliceNum = 0;

        fast_memcpy(mCurPacket.pData, &m2vheader, sizeof(VPU_BITSTREAM));
        fast_memcpy(&mCurPacket.pData[sizeof(VPU_BITSTREAM)], pData, iSize);
      }
      else
      {
        mCurPacket.iSize = iSize;
        mCurPacket.pData = (uint8_t*)malloc(iSize);
        fast_memcpy(mCurPacket.pData, pData, iSize);
      }
    }
  }

  VideoPacket_t pkt;
  memset(&pkt, 0, sizeof(VideoPacket_t));

  pkt.data = mCurPacket.pData;
  pkt.size = mCurPacket.iSize;
  pkt.pts = pkt.dts = ipts;

  DecoderOut_t picture;
  memset(&picture, 0, sizeof(DecoderOut_t));
  uint8_t *frame = (uint8_t*)malloc(sizeof(VPU_FRAME));
  picture.data = frame;
  picture.size = 0;

  if (mVpuCtx->no_thread)
  {
    int vret = mVpuCtx->decode(mVpuCtx, &pkt, &picture);
    if(vret != 0)
    {
      if (picture.size)
        releaseframe(picture.data);
      free(picture.data);
      free(mCurPacket.pData); mCurPacket.pData= NULL;
      return VC_ERROR;
    }
  }
  else
  {
    int vret = mVpuCtx->decode_sendstream(mVpuCtx, &pkt);
    if(vret != 0)
    {
      free(mCurPacket.pData); mCurPacket.pData= NULL;
      return VC_ERROR;
    }

    vret = mVpuCtx->decode_getframe(mVpuCtx, &picture);
    if (vret != 0)
    {
      if (picture.size)
        releaseframe(picture.data);
      free(picture.data);
      free(mCurPacket.pData); mCurPacket.pData= NULL;
      return VC_ERROR;
    }
  }

  if (picture.size)
  {
    if (!(pOn2Privat->flags & FIRST_FRAME))
      pOn2Privat->flags |=FIRST_FRAME;

    if (checkVideoInfoChange(picture.data))
    {
      // format changed
      releaseframe(picture.data);
      free(picture.data);
    }
    else
    {
      mPicture = picture;
      ret |= VC_PICTURE;
    }
  }

  if (mCurPacket.pData && !(pkt.size))
  {
    free(mCurPacket.pData); mCurPacket.pData= NULL;
    ret |= VC_BUFFER;
  }

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Decode: pushed IN frame (%d); tm:%d\n", CLASSNAME,m_in_queue.size(), XbmcThreads::SystemClockMillis() - time);
#endif

  return ret;
}

bool CRkCodecVideo::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
 #if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif

  ReleaseVpuFrame((VPU_FRAME*)pDvdVideoPicture->render_ctx);

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::ClearPicture img:%p (%d)\n", CLASSNAME, pDvdVideoPicture, XbmcThreads::SystemClockMillis() - time);
#endif

  return true;
}

bool CRkCodecVideo::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  pDvdVideoPicture->format = RENDER_FMT_BYPASS;
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->pts = mPicture.timeUs;
  pDvdVideoPicture->iWidth  = mWidth;
  pDvdVideoPicture->iHeight = mHeight;
  if (!mAspectRatio || mAspectRatio == 1.0)
  {
    pDvdVideoPicture->iDisplayWidth = mWidth;
    pDvdVideoPicture->iDisplayHeight = mHeight;
  }
  else
  {
    pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;
    pDvdVideoPicture->iDisplayWidth  = ((int)RINT(pDvdVideoPicture->iHeight * mAspectRatio)) & -3;
    if (pDvdVideoPicture->iDisplayWidth > pDvdVideoPicture->iWidth)
    {
      pDvdVideoPicture->iDisplayWidth  = pDvdVideoPicture->iWidth;
      pDvdVideoPicture->iDisplayHeight = ((int)RINT(pDvdVideoPicture->iWidth / mAspectRatio)) & -3;
    }
  }

  pDvdVideoPicture->render_ctx = NULL;

  if (drop_state)
  {
    releaseframe(mPicture.data);
    free(mPicture.data);
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;
  }
  else
  {
    pDvdVideoPicture->iFlags  = DVP_FLAG_ALLOCATED;
    VPU_FRAME *vpuframe = (VPU_FRAME *)(mPicture.data);
    VPU_FRAME *vpucopy = NULL;
    if (vpuframe->vpumem.phy_addr)
    {
      vpucopy = (VPU_FRAME *)malloc(sizeof(VPU_FRAME));
      memcpy(vpucopy,vpuframe,sizeof(VPU_FRAME));
      VPUMemLink(&vpuframe->vpumem);
      VPUMemDuplicate(&vpucopy->vpumem,&vpuframe->vpumem);
      VPUFreeLinear(&vpuframe->vpumem);
      //      vpucopy = vpuframe;

      if (vpucopy->FrameWidth == 0)
        vpucopy->FrameWidth = vpucopy->DisplayWidth;
      if (vpucopy->FrameHeight == 0)
        vpucopy->FrameHeight = vpucopy->DisplayHeight;

      if (vpucopy)
      {
        LockVpuFrame(vpucopy);
        pDvdVideoPicture->render_ctx = (void*)vpucopy;
#if defined(DEBUG_VERBOSE)
        CLog::Log(LOGDEBUG, ">>>     va:%p,fa:%p,%p, w:%d, h:%d, dw:%d, dh:%d\n",
                  (void*)vpucopy->vpumem.vir_addr, (void*)vpucopy->FrameBusAddr[0], (void*)vpucopy->FrameBusAddr[1],
            vpucopy->FrameWidth, vpucopy->FrameHeight, vpucopy->DisplayWidth, vpucopy->DisplayHeight);

#endif
      }
      else
        pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;
    }
    free(mPicture.data);
  }

  return true;
}

void CRkCodecVideo::Dispose()
{
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Close\n", CLASSNAME);
#endif

  m_g_renderManager->RegisterRenderUpdateCallBack((const void*)NULL, NULL);
  m_g_renderManager->RegisterRenderFeaturesCallBack((const void*)NULL, NULL);
  m_g_renderManager->RegisterRenderLockCallBack((const void*)NULL, NULL);
  m_g_renderManager->RegisterRenderReleaseCallBack((const void*)NULL, NULL);

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning OUT\n");
#endif
  while(m_busy_vpu_queue.size())
  {
    VPU_FRAME* vpuframe = m_busy_vpu_queue.front();
    ReleaseVpuFrame(vpuframe);
  }

  if (mExtraData)
    free(mExtraData);

  // Workaround: resize fb1 and move offscreen; no solution for plain erasing
  struct fb_var_screeninfo info;

  if (ioctl(m_fb1_fd, FBIOGET_VSCREENINFO, &info) == -1)
  {
      CLog::Log(LOGDEBUG, "%s(%d):  FBIOGET_VSCREENINFO[%d] Failed", __FUNCTION__, __LINE__, m_fb1_fd);
      return;
  }

  info.activate = FB_ACTIVATE_NOW;
  info.activate |= FB_ACTIVATE_FORCE;
  info.nonstd &= 0xFFFFFF00;

  info.xoffset = 0;
  info.yoffset = 0;
  info.xres = 4096;
  info.yres = 4096;
  info.xres_virtual = 4096;
  info.yres_virtual = 4096;

  int nonstd = ((int)0xf00<<8) + ((int)0xf00<<20);
  int grayscale = ((int)16 << 8) + ((int)16 << 20);

  info.nonstd &= 0x00;
  info.nonstd |= HAL_PIXEL_FORMAT_YCrCb_NV12;
  info.nonstd |= nonstd;
  info.grayscale &= 0xff;
  info.grayscale |= grayscale;

  if (ioctl(m_fb1_fd, FBIOPUT_VSCREENINFO, &info) == -1)
  {
    CLog::Log(LOGDEBUG, "%s(%d):  FBIOPUT_VSCREENINFO[%d] Failed", __FUNCTION__, __LINE__, m_fb1_fd);
    return;
  }

  close(m_fb1_fd);

  if (mVpuCtx)
    vpu_close_context(&mVpuCtx);

  if(mPool)
    release_vpu_memory_pool_allocator(mPool);
}

void CRkCodecVideo::Reset(void)
{
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Reset\n", CLASSNAME);
#endif

  if (!mVpuCtx)
    return;

  mVpuCtx->flush(mVpuCtx);
  /*fix when sps pps has change in mp4 steam seek directly will cause decoder error. modify by csy 2014.8.4*/
  if(mExtraDataSize != 0 && mCodecId == OMX_ON2_VIDEO_CodingAVC)
  {
    VideoPacket_t pkt;
    memset(&pkt, 0, sizeof(VideoPacket_t));
    pkt.nFlags = OMX_BUFFERFLAG_EXTRADATA;
    pkt.data = (RK_U8*)mExtraData;
    pkt.size = mExtraDataSize;
    mVpuCtx->decode_sendstream(mVpuCtx, &pkt);
  }
}

void CRkCodecVideo::SetDropState(bool bDrop)
{
  if (bDrop == drop_state)
    return;

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::SetDropState (%d->%d)\n", CLASSNAME,drop_state,bDrop);
#endif

  drop_state = bDrop;
}

void CRkCodecVideo::SetSpeed(int iSpeed)
{
}

/***************/

void CRkCodecVideo::LockVpuFrame(VPU_FRAME *vpuframe)
{
  if (!vpuframe)
    return;

#if defined(DEBUG_VERBOSE)
//  CLog::Log(LOGDEBUG, "Locking %p\n", vpuframe);
#endif
  free_mutex.lock();
  m_busy_vpu_queue.push_back(vpuframe);
  free_mutex.unlock();
}

bool CRkCodecVideo::ReleaseVpuFrame(VPU_FRAME *vpuframe)
{
  if (!vpuframe)
    return true;

#if defined(DEBUG_VERBOSE)
//  CLog::Log(LOGDEBUG, "Unlocking %p\n", vpuframe);
#endif
  free_mutex.lock();
  int cnt = 0;
  std::list<VPU_FRAME*>::iterator it = m_busy_vpu_queue.begin();
  std::list<VPU_FRAME*>::iterator itfree;

  for(;it != m_busy_vpu_queue.end(); ++it)
  {
    if ((*it) == vpuframe)
    {
      cnt++;
      if (cnt>1)
        break;
      itfree = it;
    }
  }
  if (!cnt)
  {
    free_mutex.unlock();
    return true;
  }

  m_busy_vpu_queue.erase(itfree);
  if (cnt==1)
  {
#if defined(DEBUG_VERBOSE)
//    CLog::Log(LOGDEBUG, ">>> Deleting\n");
#endif
    releaseframe(vpuframe);
    free(vpuframe);
    free_mutex.unlock();
    return true;
  }
  free_mutex.unlock();
  return false;
}


void CRkCodecVideo::Render(const CRect &SrcRect, const CRect &DestRect, const VPU_FRAME *render_ctx)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "RenderUpdateCallBack buf:%p\n", render_ctx);
#endif

  if (!render_ctx)
    return;
  VPU_FRAME* vpubuf = (VPU_FRAME*)render_ctx;

  struct fb_var_screeninfo info;

  if (ioctl(m_fb1_fd, FBIOGET_VSCREENINFO, &info) == -1)
  {
      CLog::Log(LOGDEBUG, "%s(%d):  FBIOGET_VSCREENINFO[%d] Failed", __FUNCTION__, __LINE__, m_fb1_fd);
      return;
  }

  //ALOGE("nonstd %x grayscale %x", info.nonstd, info.grayscale);

  info.activate = FB_ACTIVATE_NOW;
  info.activate |= FB_ACTIVATE_FORCE;
  info.nonstd &= 0xFFFFFF00;

  info.xoffset = 0;
  info.yoffset = 0;
  info.xres = vpubuf->DisplayWidth;
  info.yres = vpubuf->DisplayHeight;
  info.xres_virtual = vpubuf->FrameWidth;
  info.yres_virtual = vpubuf->FrameHeight;

  CRect dst_rect = DestRect;
  RENDER_STEREO_MODE stereo_mode = m_g_graphicsContext->GetStereoMode();
  if (stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
  {
    dst_rect.x2 *= 2.0;
  }
  else if (stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
  {
    dst_rect.y2 *= 2.0;
  }

  if (dst_rect.x1 < 0)
  {
    dst_rect.x2 += dst_rect.x1;
    dst_rect.x1 = 0;
  }
  if (dst_rect.y1 < 0)
  {
    dst_rect.y2 += dst_rect.y1;
    dst_rect.y1 = 0;
  }

  int nonstd = (((int)dst_rect.x1 & 0xfff)<<8) + (((int)dst_rect.y1 & 0xfff)<<20);
  int grayscale = ((int)dst_rect.Width() << 8) + ((int)dst_rect.Height() << 20);

  info.nonstd &= 0x00;
  info.nonstd |= HAL_PIXEL_FORMAT_YCrCb_NV12;
  info.nonstd |= nonstd;
  info.grayscale &= 0xff;
  info.grayscale |= grayscale;

/* Check yuv format. */

  if (ioctl(m_fb1_fd, RK_FBIOSET_YUV_ADDR, (int *)vpubuf) == -1)
  {
    CLog::Log(LOGDEBUG, "%s(%d):  FBIOSET_YUV_ADDR[%d] Failed", __FUNCTION__, __LINE__, m_fb1_fd);
    return;
  }

  if (ioctl(m_fb1_fd, FBIOPUT_VSCREENINFO, &info) == -1)
  {
    CLog::Log(LOGDEBUG, "%s(%d):  FBIOPUT_VSCREENINFO[%d] Failed", __FUNCTION__, __LINE__, m_fb1_fd);
    return;
  }

  // There's double-buffering, visibly, so keep 2 buffers alive
  if (m_prev_stfbuf.size() > 1)
  {
    VPU_FRAME* prev_buf = m_prev_stfbuf.front();
    ReleaseVpuFrame(prev_buf);
    m_prev_stfbuf.pop();
  }
  LockVpuFrame(vpubuf);
  m_prev_stfbuf.push(vpubuf);

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, ">>>> tm:%d\n", XbmcThreads::SystemClockMillis() - time);
#endif
  return;
}

/**********************************/

void CRkCodecVideo::RenderFeaturesCallBack(const void *ctx, Features &renderFeatures)
{
  renderFeatures.push_back(RENDERFEATURE_ZOOM);
  renderFeatures.push_back(RENDERFEATURE_STRETCH);
  renderFeatures.push_back(RENDERFEATURE_PIXEL_RATIO);
}

void CRkCodecVideo::RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect, DWORD flags, const void* render_ctx)
{
  CRkCodecVideo *codec = (CRkCodecVideo*)ctx;
  if (codec)
    codec->Render(SrcRect, DestRect, (VPU_FRAME *)render_ctx);
}

void CRkCodecVideo::RenderLockCallBack(const void *ctx, const void* render_ctx)
{
  CRkCodecVideo *codec = (CRkCodecVideo*)ctx;
  if (codec)
    codec->LockVpuFrame((VPU_FRAME *)render_ctx);
}

void CRkCodecVideo::RenderReleaseCallBack(const void *ctx, const void* render_ctx)
{
  CRkCodecVideo *codec = (CRkCodecVideo*)ctx;
  if (codec)
    codec->ReleaseVpuFrame((VPU_FRAME *)render_ctx);
}
