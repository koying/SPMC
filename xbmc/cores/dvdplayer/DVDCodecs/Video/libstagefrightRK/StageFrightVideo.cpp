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

#include "DVDVideoCodecStageFright.h"
#include "StageFrightVideo.h"
#include "StageFrightVideoPrivate.h"

#include "android/activity/XBMCApp.h"
#include "guilib/GraphicContext.h"
#include "DVDClock.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"
#include "utils/StringUtils.h"
#include "threads/Thread.h"
#include "threads/Event.h"
#include "settings/AdvancedSettings.h"
#include "DVDCodecs/DVDCodecInterface.h"

#include "xbmc/guilib/FrameBufferObject.h"

#include "windowing/egl/EGLWrapper.h"
#include "windowing/WindowingFactory.h"

#include <new>

#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))

#define OMX_QCOM_COLOR_FormatYVU420SemiPlanar 0x7FA30C00
#define OMX_TI_COLOR_FormatYUV420PackedSemiPlanar 0x7F000100

#define CLASSNAME "CStageFrightVideo"

#define EGL_NATIVE_BUFFER_ANDROID 0x3140
#define EGL_IMAGE_PRESERVED_KHR   0x30D2

const char *MEDIA_MIMETYPE_VIDEO_WMV  = "video/x-ms-wmv";
const char *MEDIA_MIMETYPE_VIDEO_VC1  = "video/vc1";
const char *MEDIA_MIMETYPE_VIDEO_VP6  = "video/vp6";

const int kKeyVC1ExtraSize      = 'vc1e';  // vc1 extra data size
const int kKeyVC1               = 'vc1c';  // vc1 codec config info
const int kKeyHVCC              = 'hvcc';

#define XMEDIA_BITSTREAM_START_CODE         (0x42564b52)

using namespace android;

static int64_t pts_dtoi(double pts)
{
  return (int64_t)(pts);
}

static double pts_itod(int64_t pts)
{
  return (double)pts;
}

/***********************************************************/

class CStageFrightMediaSource : public MediaSource
{
public:
  CStageFrightMediaSource(CStageFrightVideoPrivate *priv, sp<MetaData> meta)
  {
    p = priv;
    source_meta = meta;
  }

  virtual sp<MetaData> getFormat()
  {
    return source_meta;
  }

  virtual status_t start(MetaData *params)
  {
    return OK;
  }

  virtual status_t stop()
  {
    return OK;
  }

  virtual status_t read(MediaBuffer **buffer,
                        const MediaSource::ReadOptions *options)
  {
    Frame *frame;
    status_t ret;
    *buffer = NULL;
    int64_t time_us = -1;
    MediaSource::ReadOptions::SeekMode mode;
    
    if (options && options->getSeekTo(&time_us, &mode))
    {
#if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, "%s: reading source(%d): seek:%llu\n", CLASSNAME,p->in_queue.size(), time_us);
#endif
    }
    else
    {
#if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, "%s: reading source(%d)\n", CLASSNAME,p->in_queue.size());
#endif
    }

    p->in_mutex.lock();
    while (p->in_queue.empty() && p->decode_thread)
      p->in_condition.wait(p->in_mutex);

    if (p->in_queue.empty())
    {
      p->in_mutex.unlock();
      return VC_ERROR;
    }
    
    std::map<int64_t,Frame*>::iterator it = p->in_queue.begin();
    frame = it->second;
    ret = frame->status;
    *buffer = frame->medbuf;

    p->in_queue.erase(it);
    p->in_mutex.unlock();

#if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> exiting reading source(%d); pts:%llu\n", p->in_queue.size(),frame->pts);
#endif

    free(frame);

    return ret;
  }

private:
  sp<MetaData> source_meta;
  CStageFrightVideoPrivate *p;
};

/********************************************/

class CStageFrightDecodeThread : public CThread
{
protected:
  CStageFrightVideoPrivate *p;

public:
  CStageFrightDecodeThread(CStageFrightVideoPrivate *priv)
  : CThread("CStageFrightDecodeThread")
  , p(priv)
  {}
  
  void OnStartup()
  {
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: entering decode thread\n", CLASSNAME);
  #endif
  }
  
  void OnExit()
  {
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: exited decode thread\n", CLASSNAME);
  #endif
  }
  
  void Process()
  {
    Frame* frame;
    int32_t w, h, dw, dh;
    int decode_done = 0;
    int32_t keyframe = 0;
    int32_t unreadable = 0;
    MediaSource::ReadOptions readopt;
    // GLuint texid;

    //SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
    do
    {
      #if defined(DEBUG_VERBOSE)
      unsigned int time = XbmcThreads::SystemClockMillis();
      CLog::Log(LOGDEBUG, "%s: >>> Handling frame\n", CLASSNAME);
      #endif
      p->cur_frame = NULL;
      frame = (Frame*)malloc(sizeof(Frame));
      if (!frame) 
      {
        decode_done   = 1;
        continue;
      }

      frame->eglimg = EGL_NO_IMAGE_KHR;
      frame->medbuf = NULL;
      if (p->resetting)
      {
        CDVDClock *playerclock = CDVDClock::GetMasterClock();
        if (playerclock)
          readopt.setSeekTo(pts_dtoi(playerclock->GetClock()));
        else
          readopt.setSeekTo(0);
        p->resetting = false;
      }
      frame->status = p->decoder->read(&frame->medbuf, &readopt);
      readopt.clearSeekTo();
      
      if (frame->status == OK)
      {
        if (!frame->medbuf->graphicBuffer().get())  // hw buffers
        {
          if (frame->medbuf->range_length() == 0)
          {
            // Empty frame? ignore
            frame->medbuf->release();
            frame->medbuf = NULL;
            free(frame);
            continue;
          }
          else if (frame->medbuf->range_length() == sizeof(VPU_FRAME))
            frame->format = RENDER_FMT_STFBUF;
          else
            frame->format = RENDER_FMT_YUV420P;
        }
        else
          frame->format = RENDER_FMT_EGLIMG;
      }

      if (frame->status == OK)
      {
        frame->width = p->width;
        frame->height = p->height;
        frame->pts = 0;

        sp<MetaData> outFormat = p->decoder->getFormat();
        outFormat->findInt32(kKeyWidth , &w);
        outFormat->findInt32(kKeyHeight, &h);

        if (!outFormat->findInt32(kKeyDisplayWidth , &dw))
          dw = w;
        if (!outFormat->findInt32(kKeyDisplayHeight, &dh))
          dh = h;

        if (!outFormat->findInt32(kKeyIsSyncFrame, &keyframe))
          keyframe = 0;
        if (!outFormat->findInt32(kKeyIsUnreadable, &unreadable))
          unreadable = 0;

        frame->pts = 0;

        w = (w + 15)&(~15);
        h = (h + 15)&(~15);

        // The OMX.SEC decoder doesn't signal the modified width/height
        if (p->decoder_component && (w & 15 || h & 15) && !strncmp(p->decoder_component, "OMX.SEC", 7))
        {
          if (((w + 15)&~15) * ((h + 15)&~15) * 3/2 == frame->medbuf->range_length())
          {
            w = (w + 15)&~15;
            h = (h + 15)&~15;
            frame->width = w;
            frame->height = h;
          }
        }
        frame->medbuf->meta_data()->findInt64(kKeyTime, &(frame->pts));
      }
      else if (frame->status == INFO_FORMAT_CHANGED)
      {
        int32_t cropLeft, cropTop, cropRight, cropBottom;
        sp<MetaData> outFormat = p->decoder->getFormat();

        outFormat->findInt32(kKeyWidth , &p->width);
        outFormat->findInt32(kKeyHeight, &p->height);

        cropLeft = cropTop = cropRight = cropBottom = 0;
        if (!outFormat->findRect(kKeyCropRect, &cropLeft, &cropTop, &cropRight, &cropBottom))
        {
          p->x = 0;
          p->y = 0;
        }
        else
        {
          p->x = cropLeft;
          p->y = cropTop;
          p->width = cropRight - cropLeft + 1;
          p->height = cropBottom - cropTop + 1;
        }
        outFormat->findInt32(kKeyColorFormat, &p->videoColorFormat);
        if (!outFormat->findInt32(kKeyStride, &p->videoStride))
          p->videoStride = p->width;
        if (!outFormat->findInt32(kKeySliceHeight, &p->videoSliceHeight))
          p->videoSliceHeight = p->height;

#if defined(DEBUG_VERBOSE)
        CLog::Log(LOGDEBUG, ">>> new format col:%d, w:%d, h:%d, sw:%d, sh:%d, ctl:%d,%d; cbr:%d,%d\n", p->videoColorFormat, p->width, p->height, p->videoStride, p->videoSliceHeight, cropTop, cropLeft, cropBottom, cropRight);
#endif

        if (frame->medbuf)
          frame->medbuf->release();
        frame->medbuf = NULL;
        free(frame);
        continue;
      }
      else
      {
        CLog::Log(LOGERROR, "%s - decoding error (%d)\n", CLASSNAME,frame->status);
        if (frame->medbuf)
          frame->medbuf->release();
        frame->medbuf = NULL;
        free(frame);
        continue;
      }

/*
      if (frame->format == RENDER_FMT_EGLIMG)
      {
        if (!p->eglInitialized)
        {
          p->InitializeEGL(frame->width, frame->height);
        } 
        else if (p->texwidth != frame->width || p->texheight != frame->height)
        {
          p->UninitializeEGL();
          p->InitializeEGL(frame->width, frame->height);
        }

        if (p->free_queue.empty())
        {
          CLog::Log(LOGERROR, "%s::%s - Error: No free output buffers\n", CLASSNAME, __func__);
          if (frame->medbuf) 
            frame->medbuf->release();
          free(frame);
          continue;
        }  

        ANativeWindowBuffer* graphicBuffer = frame->medbuf->graphicBuffer()->getNativeBuffer();
        native_window_set_buffers_timestamp(p->natwin.get(), frame->pts * 1000);
        int err = p->natwin.get()->queueBuffer(p->natwin.get(), graphicBuffer);
        if (err == 0)
          frame->medbuf->meta_data()->setInt32(kKeyRendered, 1);
        frame->medbuf->release();
        frame->medbuf = NULL;
        p->m_g_xbmcapp->UpdateStagefrightTexture();
        // p->m_g_xbmcapp->GetSurfaceTexture()->updateTexImage();

        if (!p->drop_state)
        {
          // static const EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
          // EGLImageKHR img = eglCreateImageKHR(p->eglDisplay, EGL_NO_CONTEXT,
                                    // EGL_NATIVE_BUFFER_ANDROID,
                                    // (EGLClientBuffer)graphicBuffer->getNativeBuffer(),
                                    // eglImgAttrs);

          p->free_mutex.lock();
          while (!p->free_queue.size())
            usleep(10000);
          std::list<std::pair<EGLImageKHR, int> >::iterator itfree = p->free_queue.begin();
          int cur_slot = itfree->second;
          p->fbo.BindToTexture(GL_TEXTURE_2D, p->slots[cur_slot].texid);
          p->fbo.BeginRender();

          glDisable(GL_DEPTH_TEST);
          //glClear(GL_COLOR_BUFFER_BIT);

          const GLfloat triangleVertices[] = {
          -1.0f, 1.0f,
          -1.0f, -1.0f,
          1.0f, -1.0f,
          1.0f, 1.0f,
          };

          glVertexAttribPointer(p->mPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, triangleVertices);
          glEnableVertexAttribArray(p->mPositionHandle);

          glUseProgram(p->mPgm);
          glUniform1i(p->mTexSamplerHandle, 0);

          // glGenTextures(1, &texid);
          // glBindTexture(GL_TEXTURE_EXTERNAL_OES, texid);
          // glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, img);

          glBindTexture(GL_TEXTURE_EXTERNAL_OES, p->m_g_xbmcapp->GetAndroidTexture());
          
          GLfloat texMatrix[16];
          // const GLfloat texMatrix[] = {
            // 1, 0, 0, 0,
            // 0, -1, 0, 0,
            // 0, 0, 1, 0,
            // 0, 1, 0, 1
          // };
          p->m_g_xbmcapp->GetStagefrightTransformMatrix(texMatrix);
          // p->m_g_xbmcapp->GetSurfaceTexture()->getTransformMatrix(texMatrix);
          glUniformMatrix4fv(p->mTexMatrixHandle, 1, GL_FALSE, texMatrix);

          glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

          glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
          // glDeleteTextures(1, &texid);
          // eglDestroyImageKHR(p->eglDisplay, img);
          p->fbo.EndRender();

          glBindTexture(GL_TEXTURE_2D, 0);

          frame->eglimg = p->slots[cur_slot].eglimg;
          p->busy_queue.push_back(std::pair<EGLImageKHR, int>(*itfree));
          p->free_queue.erase(itfree);
          p->free_mutex.unlock();
        }
      } */
      if (frame->format == RENDER_FMT_STFBUF)
      {
        p->out_mutex.lock();
        p->outbuf_queue.push_back(frame);

#if defined(DEBUG_VERBOSE)
        CLog::Log(LOGDEBUG, "%s: >>> pushed OUT frame; w:%d, h:%d, dw:%d, dh:%d, kf:%d, ur:%d, buf:%p, tm:%d\n", CLASSNAME, frame->width, frame->height, dw, dh, keyframe, unreadable, frame->medbuf, XbmcThreads::SystemClockMillis() - time);
#endif

        while (p->outbuf_queue.size() >= OUTBUFCOUNT)
          p->out_condition.wait(p->out_mutex);
        p->out_mutex.unlock();
        continue;
      }
    #if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, "%s: >>> pushed OUT frame; w:%d, h:%d, dw:%d, dh:%d, kf:%d, ur:%d, img:%p, tm:%d\n", CLASSNAME, frame->width, frame->height, dw, dh, keyframe, unreadable, frame->eglimg, XbmcThreads::SystemClockMillis() - time);
    #endif

      p->out_mutex.lock();
      p->cur_frame = frame;
      while (p->cur_frame)
        p->out_condition.wait(p->out_mutex);
      p->out_mutex.unlock();
    }
    while (!decode_done && !m_bStop);
    
    if (p->eglInitialized)
      p->UninitializeEGL();
    
  }
};

/***********************************************************/

CStageFrightVideo::CStageFrightVideo(CDVDCodecInterface* interface)
{
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::ctor: %d\n", CLASSNAME, sizeof(CStageFrightVideo));
#endif
  p = new CStageFrightVideoPrivate;
  p->m_g_application = interface->GetApplication();
  p->m_g_applicationMessenger = interface->GetApplicationMessenger();
  p->m_g_Windowing = interface->GetWindowSystem();
  p->m_g_advancedSettings = interface->GetAdvancedSettings();
}

CStageFrightVideo::~CStageFrightVideo()
{
  SAFE_DELETE(p);
}

bool CStageFrightVideo::Open(CDVDStreamInfo &hints)
{
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Open: ed:%d as:%f fa:%d\n", CLASSNAME, hints.extrasize, hints.aspect, hints.forced_aspect);
#endif

  CSingleLock lock(g_graphicsContext);

  // stagefright crashes with null size. Trap this...
  if (!hints.width || !hints.height)
  {
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"null size, cannot handle");
    return false;
  }
  p->codec     = hints.codec;
  p->width     = hints.width;
  p->height    = hints.height;
  if (!hints.forced_aspect)
    p->aspect_ratio = hints.aspect;
  else
    p->aspect_ratio = 1.0;

//  if (p->m_g_advancedSettings->m_stagefrightConfig.useSwRenderer)   // Force VPU's pseudo soft
    p->quirks |= QuirkSWRender;
    
  sp<MetaData> outFormat;
  int32_t cropLeft, cropTop, cropRight, cropBottom;
  //Vector<String8> matchingCodecs;

  p->meta = new MetaData;
  if (p->meta == NULL)
  {
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"cannot allocate MetaData");
    return false;
  }

  const char* mimetype;
  switch (hints.codec)
  {
    case AV_CODEC_ID_HEVC:
      if (p->m_g_advancedSettings->m_stagefrightConfig.useHEVCcodec == 0)
        return false;
      mimetype = "video/hevc";
      p->meta->setData(kKeyHVCC, kTypeAVCC, hints.extradata, hints.extrasize);
      break;
  case AV_CODEC_ID_H264:
      if (p->m_g_advancedSettings->m_stagefrightConfig.useAVCcodec == "0"
          || (p->m_g_advancedSettings->m_stagefrightConfig.useAVCcodec == "sd" && hints.width > 800)
          || (p->m_g_advancedSettings->m_stagefrightConfig.useAVCcodec == "hd" && hints.width <= 800))
        return false;
      mimetype = "video/avc";
      p->meta->setData(kKeyAVCC, kTypeAVCC, hints.extradata, hints.extrasize);
    break;
  case AV_CODEC_ID_MPEG4:
      if (p->m_g_advancedSettings->m_stagefrightConfig.useMP4codec == "0"
          || (p->m_g_advancedSettings->m_stagefrightConfig.useMP4codec == "sd" && hints.width > 800)
          || (p->m_g_advancedSettings->m_stagefrightConfig.useMP4codec == "hd" && hints.width <= 800))
        return false;
    mimetype = "video/mp4v-es";
    break;
  case AV_CODEC_ID_MPEG2VIDEO:
      if (p->m_g_advancedSettings->m_stagefrightConfig.useMPEG2codec == "0"
          || (p->m_g_advancedSettings->m_stagefrightConfig.useMPEG2codec == "sd" && hints.width > 800)
          || (p->m_g_advancedSettings->m_stagefrightConfig.useMPEG2codec == "hd" && hints.width <= 800))
        return false;
    mimetype = "video/mpeg2";
    break;
  case AV_CODEC_ID_VP3:
  case AV_CODEC_ID_VP6:
  case AV_CODEC_ID_VP6F:
      if (p->m_g_advancedSettings->m_stagefrightConfig.useVPXcodec == "0"
          || (p->m_g_advancedSettings->m_stagefrightConfig.useVPXcodec == "sd" && hints.width > 800)
          || (p->m_g_advancedSettings->m_stagefrightConfig.useVPXcodec == "hd" && hints.width <= 800))
        return false;
    mimetype = "video/vp6";
    break;
    case AV_CODEC_ID_VP8:
      if (p->m_g_advancedSettings->m_stagefrightConfig.useVPXcodec == "0"
          || (p->m_g_advancedSettings->m_stagefrightConfig.useVPXcodec == "sd" && hints.width > 800)
          || (p->m_g_advancedSettings->m_stagefrightConfig.useVPXcodec == "hd" && hints.width <= 800))
        return false;
      mimetype = "video/x-vnd.on2.vp8";
      break;
  case AV_CODEC_ID_VC1:
  //case AV_CODEC_ID_WMV3:
      if (p->m_g_advancedSettings->m_stagefrightConfig.useVC1codec == "0"
          || (p->m_g_advancedSettings->m_stagefrightConfig.useVC1codec == "sd" && hints.width > 800)
          || (p->m_g_advancedSettings->m_stagefrightConfig.useVC1codec == "hd" && hints.width <= 800))
        return false;
    mimetype = "video/vc1";
    p->extrasize = hints.extrasize;
    p->extradata = hints.extradata;
    p->meta->setInt32(kKeyVC1ExtraSize, hints.extrasize);
    break;
  default:
    return false;
    break;
  }

  p->meta->setCString(kKeyMIMEType, mimetype);
  p->meta->setInt32(kKeyWidth, p->width);
  p->meta->setInt32(kKeyHeight, p->height);

  android::ProcessState::self()->startThreadPool();

  p->source    = new CStageFrightMediaSource(p, p->meta);
  p->client    = new OMXClient;

  if (p->source == NULL || p->client == NULL)
  {
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Cannot obtain source / client");
    return false;
  }

  if (p->client->connect() !=  OK)
  {
    delete p->client;
    p->client = NULL;
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Cannot connect OMX client");
    return false;
  }

/*
  if ((p->quirks & QuirkSWRender) == 0)
  {
    p->m_g_xbmcapp->InitStagefrightSurface();
    p->natwin = p->m_g_xbmcapp->GetAndroidVideoWindow();
    native_window_api_connect(p->natwin.get(), NATIVE_WINDOW_API_MEDIA);
  }
  */
  p->decoder  = OMXCodec::Create(p->client->interface(), p->meta,
                                         false, p->source, NULL,
                                         (p->quirks & QuirkSWRender ? OMXCodec::kSoftwareCodecsOnly | OMXCodec::kClientNeedsFramebuffer : OMXCodec::kHardwareCodecsOnly),
                                         p->natwin
                                         );

  if (!(p->decoder != NULL && p->decoder->start() ==  OK))
  {
    p->decoder = NULL;
    return false;
  }

  outFormat = p->decoder->getFormat();

  if (!outFormat->findInt32(kKeyWidth, &p->width) || !outFormat->findInt32(kKeyHeight, &p->height)
        || !outFormat->findInt32(kKeyColorFormat, &p->videoColorFormat))
    return false;

  const char *component;
  if (outFormat->findCString(kKeyDecoderComponent, &component))
  {
    CLog::Log(LOGDEBUG, "%s::%s - component: %s\n", CLASSNAME, __func__, component);
    
    //Blacklist
    if (!strncmp(component, "OMX.google", 10))
    {
      // On some platforms, software decoders are returned anyway
      CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Blacklisted component (software)");
      goto fail;
    }
    else if (!strncmp(component, "OMX.Nvidia.mp4.decode", 21) && p->m_g_advancedSettings->m_stagefrightConfig.useMP4codec != "1")
    {
      // Has issues with some XVID encoded MP4. Only fails after actual decoding starts...
      CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Blacklisted component (MP4)");
      goto fail;
    }
  }

  cropLeft = cropTop = cropRight = cropBottom = 0;
  if (!outFormat->findRect(kKeyCropRect, &cropLeft, &cropTop, &cropRight, &cropBottom))
  {
    p->x = 0;
    p->y = 0;
  }
  else
  {
    p->x = cropLeft;
    p->y = cropTop;
    p->width = cropRight - cropLeft + 1;
    p->height = cropBottom - cropTop + 1;
  }

  if (!outFormat->findInt32(kKeyStride, &p->videoStride))
    p->videoStride = p->width;
  if (!outFormat->findInt32(kKeySliceHeight, &p->videoSliceHeight))
    p->videoSliceHeight = p->height;
  
  for (int i=0; i<INBUFCOUNT; ++i)
  {
    p->inbuf[i] = new MediaBuffer(300000);
    p->inbuf[i]->setObserver(p);
  }
  
  p->decode_thread = new CStageFrightDecodeThread(p);
  p->decode_thread->Create(true /*autodelete*/);

  if (p->extrasize)
    Decode((uint8_t*)p->extradata, p->extrasize, 0, 0);

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, ">>> format col:%d, w:%d, h:%d, sw:%d, sh:%d, ctl:%d,%d; cbr:%d,%d\n", p->videoColorFormat, p->width, p->height, p->videoStride, p->videoSliceHeight, cropTop, cropLeft, cropBottom, cropRight);
#endif

  return true;

fail:
  if (p->decoder != 0)
    p->decoder->stop();
  if (p->client)
  {
    p->client->disconnect();
    delete p->client;
  }
  if (p->decoder_component)
    free(&p->decoder_component);
/*
  if ((p->quirks & QuirkSWRender) == 0)
    p->m_g_xbmcapp->UninitStagefrightSurface();
*/
  return false;
}

/*** Decode ***/
int  CStageFrightVideo::Decode(uint8_t *pData, int iSize, double dts, double pts)
{
#if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s::Decode - d:%p; s:%d; dts:%f; pts:%f\n", CLASSNAME, pData, iSize, dts, pts);
#endif

  Frame *frame;
  int demuxer_bytes = iSize;
  uint8_t *demuxer_content = pData;
  int ret = 0;

  if (demuxer_content)
  {
    frame = (Frame*)malloc(sizeof(Frame));
    if (!frame)
      return VC_ERROR;

    frame->status  = OK;
    if (p->m_g_advancedSettings->m_stagefrightConfig.useInputDTS)
      frame->pts = (dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : ((pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : 0);
    else
      frame->pts = (pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : ((dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : 0);

    // No valid pts? libstagefright asserts on this.
    if (frame->pts < 0)
    {
      free(frame);
      return ret;
    }

    if (p->codec == CODEC_ID_MPEG2VIDEO)
    {
      frame->medbuf = p->getBuffer(demuxer_bytes + sizeof(VPU_BITSTREAM));
      frame->medbuf->set_range(0, demuxer_bytes  + sizeof(VPU_BITSTREAM));
    }
    else
    {
      frame->medbuf = p->getBuffer(demuxer_bytes);
      frame->medbuf->set_range(0, demuxer_bytes);
    }

    if (!frame->medbuf)
    {
      free(frame);
      return VC_ERROR;
    }

    if (p->codec == CODEC_ID_MPEG2VIDEO)
    {
      VPU_BITSTREAM m2vheader;
      m2vheader.StartCode = XMEDIA_BITSTREAM_START_CODE;
      m2vheader.SliceLength = demuxer_bytes;
      m2vheader.SliceTime.TimeLow = (uint32_t)(frame->pts / 1000);
      m2vheader.SliceTime.TimeHigh = 0;
      m2vheader.SliceType = 0;
      m2vheader.SliceNum = 0;

      fast_memcpy(frame->medbuf->data(), &m2vheader, sizeof(VPU_BITSTREAM));
      fast_memcpy((void*)((long)frame->medbuf->data() + sizeof(VPU_BITSTREAM)), demuxer_content, demuxer_bytes);
    }
    else
      fast_memcpy(frame->medbuf->data(), demuxer_content, demuxer_bytes);
    frame->medbuf->meta_data()->clear();
    frame->medbuf->meta_data()->setInt64(kKeyTime, frame->pts);
    
    p->in_mutex.lock();
    p->framecount++;
    p->in_queue.insert(std::pair<int64_t, Frame*>(p->framecount, frame));
    p->in_condition.notify();
    p->in_mutex.unlock();
  }

  if (p->inputBufferAvailable() && p->in_queue.size() < INBUFCOUNT)
    ret |= VC_BUFFER;
  else
    usleep(1000);
  if (p->cur_frame != NULL || p->outbuf_queue.size())
    ret |= VC_PICTURE;
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Decode: pushed IN frame (%d); tm:%d\n", CLASSNAME,p->in_queue.size(), XbmcThreads::SystemClockMillis() - time);
#endif

  return ret;
}

bool CStageFrightVideo::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
 #if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  if (pDvdVideoPicture->format == RENDER_FMT_EGLIMG || pDvdVideoPicture->format == RENDER_FMT_STFBUF)
  {
    ReleaseBuffer(pDvdVideoPicture->stfbuf);
#if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s::ClearPicture buf:%p (%d)\n", CLASSNAME, pDvdVideoPicture->stfbuf, XbmcThreads::SystemClockMillis() - time);
#endif
    return true;
  }

  if (p->prev_frame) {
    if (p->prev_frame->medbuf)
      p->prev_frame->medbuf->release();
    free(p->prev_frame);
    p->prev_frame = NULL;
  }
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::ClearPicture img:%p (%d)\n", CLASSNAME, pDvdVideoPicture, XbmcThreads::SystemClockMillis() - time);
#endif

  return true;
}

bool CStageFrightVideo::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
#if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s::GetPicture\n", CLASSNAME);
  if (p->cycle_time != 0)
    CLog::Log(LOGDEBUG, ">>> cycle dur:%d\n", XbmcThreads::SystemClockMillis() - p->cycle_time);
  p->cycle_time = time;
#endif

  status_t status;

  p->out_mutex.lock();
  Frame *frame = NULL;
  if (p->cur_frame)
    frame = p->cur_frame;
  else if (p->outbuf_queue.size())
  {
    frame = p->outbuf_queue.front();
    p->outbuf_queue.pop_front();
  }
  else
  {
    CLog::Log(LOGERROR, "%s::%s - Error getting frame\n", CLASSNAME, __func__);
    p->out_condition.notify();
    p->out_mutex.unlock();
    return false;
  }

  status  = frame->status;

  pDvdVideoPicture->format = frame->format;
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->pts = frame->pts;
  pDvdVideoPicture->iWidth  = p->width;
  pDvdVideoPicture->iHeight = p->height;
  if (!p->aspect_ratio || p->aspect_ratio == 1.0)
  {
    pDvdVideoPicture->iDisplayWidth = p->width;
    pDvdVideoPicture->iDisplayHeight = p->height;
  }
  else
  {
    pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;
    pDvdVideoPicture->iDisplayWidth  = ((int)RINT(pDvdVideoPicture->iHeight * p->aspect_ratio)) & -3;
    if (pDvdVideoPicture->iDisplayWidth > pDvdVideoPicture->iWidth)
    {
      pDvdVideoPicture->iDisplayWidth  = pDvdVideoPicture->iWidth;
      pDvdVideoPicture->iDisplayHeight = ((int)RINT(pDvdVideoPicture->iWidth / p->aspect_ratio)) & -3;
    }
  }

  pDvdVideoPicture->iFlags  = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->stfbuf = NULL;

  if (status != OK)
  {
    CLog::Log(LOGERROR, "%s::%s - Error getting picture from frame(%d)\n", CLASSNAME, __func__,status);
    if (frame->medbuf) {
      frame->medbuf->release();
    }
    free(frame);
    p->cur_frame = NULL;
    p->out_condition.notify();
    p->out_mutex.unlock();
    return false;
  }

  if (pDvdVideoPicture->format == RENDER_FMT_EGLIMG)
  {
    CDVDVideoCodecStageFrightBuffer* stfbuf = new CDVDVideoCodecStageFrightBuffer;
    stfbuf->format = RENDER_FMT_EGLIMG;
    stfbuf->subformat = 0;
    stfbuf->buffer = (void*)frame->eglimg;
    pDvdVideoPicture->stfbuf = stfbuf;
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> pic dts:%f, pts:%llu, img:%p, tm:%d\n", pDvdVideoPicture->dts, frame->pts, pDvdVideoPicture->stfbuf, XbmcThreads::SystemClockMillis() - time);
  #endif
  } 
  else if (pDvdVideoPicture->format == RENDER_FMT_STFBUF)
  {
    VPU_FRAME *vpuframe = (VPU_FRAME *)((long)frame->medbuf->data() + frame->medbuf->range_offset());
    VPU_FRAME *vpucopy = NULL;
    if (vpuframe->vpumem.phy_addr)
    {
      vpucopy = (VPU_FRAME *)malloc(sizeof(VPU_FRAME));
      memcpy(vpucopy,vpuframe,sizeof(VPU_FRAME));
      VPUMemLink(&vpuframe->vpumem);
      VPUMemDuplicate(&vpucopy->vpumem,&vpuframe->vpumem);
      VPUFreeLinear(&vpuframe->vpumem);
//      vpucopy = vpuframe;

      VPUMemLink(&vpucopy->vpumem);
      if (vpucopy->FrameWidth == 0)
        vpucopy->FrameWidth = vpucopy->DisplayWidth;
      if (vpucopy->FrameHeight == 0)
        vpucopy->FrameHeight = vpucopy->DisplayHeight;
    }
    frame->medbuf->release();
    SAFE_DELETE(frame);

    if (vpucopy)
    {
      CDVDVideoCodecStageFrightBuffer* stfbuf = new CDVDVideoCodecStageFrightBuffer;
      stfbuf->format = RENDER_FMT_STFBUF;
      stfbuf->subformat = 'rkvp';
      stfbuf->frameWidth = vpucopy->FrameWidth;
      stfbuf->frameHeight = vpucopy->FrameHeight;
      stfbuf->buffer = (void*)vpucopy->vpumem.vir_addr;
      stfbuf->context = (void*)vpucopy;
      LockBuffer(stfbuf);

      pDvdVideoPicture->stfbuf = stfbuf;
//#if defined(DEBUG_VERBOSE)
      //CLog::Log(LOGDEBUG, ">>> pic dts:%f, pts:%llu, buf:%p, tm:%d\n", pDvdVideoPicture->dts, pDvdVideoPicture->pts, pDvdVideoPicture->stfbuf, XbmcThreads::SystemClockMillis() - time);
      CLog::Log(LOGDEBUG, ">>>     va:%p,fa:%p,%p, w:%d, h:%d, dw:%d, dh:%d\n",
                vpucopy->vpumem.vir_addr, vpucopy->FrameBusAddr[0], vpucopy->FrameBusAddr[1],
          vpucopy->FrameWidth, vpucopy->FrameHeight, vpucopy->DisplayWidth, vpucopy->DisplayHeight);

//#endif
    }
    else
      pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;
  }
  else if (pDvdVideoPicture->format == RENDER_FMT_YUV420P)
  {    
    pDvdVideoPicture->color_range  = 0;
    pDvdVideoPicture->color_matrix = 4;

    unsigned int luma_pixels = frame->width  * frame->height;
    unsigned int chroma_pixels = luma_pixels/4;
    uint8_t* data = NULL;
    if (frame->medbuf)
    {
      data = (uint8_t*)((long)frame->medbuf->data() + frame->medbuf->range_offset());
    }
    switch (p->videoColorFormat)
    {
      case OMX_COLOR_FormatYUV420Planar:
        pDvdVideoPicture->iLineSize[0] = frame->width;
        pDvdVideoPicture->iLineSize[1] = frame->width / 2;
        pDvdVideoPicture->iLineSize[2] = frame->width / 2;
        pDvdVideoPicture->iLineSize[3] = 0;
        pDvdVideoPicture->data[0] = data;
        pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] + luma_pixels;
        pDvdVideoPicture->data[2] = pDvdVideoPicture->data[1] + chroma_pixels;
        pDvdVideoPicture->data[3] = 0;
        break;
      case OMX_COLOR_FormatYUV420SemiPlanar:
      case OMX_QCOM_COLOR_FormatYVU420SemiPlanar:
      case OMX_TI_COLOR_FormatYUV420PackedSemiPlanar:
        pDvdVideoPicture->iLineSize[0] = frame->width;
        pDvdVideoPicture->iLineSize[1] = frame->width;
        pDvdVideoPicture->iLineSize[2] = 0;
        pDvdVideoPicture->iLineSize[3] = 0;
        pDvdVideoPicture->data[0] = data;
        pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] + luma_pixels;
        pDvdVideoPicture->data[2] = 0;
        pDvdVideoPicture->data[3] = 0;
        pDvdVideoPicture->format = RENDER_FMT_NV12;
        break;
      default:
        CLog::Log(LOGERROR, "%s::%s - Unsupported color format(%d)\n", CLASSNAME, __func__,p->videoColorFormat);
    }
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> pic pts:%f, data:%p, col:%d, w:%d, h:%d, tm:%d\n", pDvdVideoPicture->pts, data, p->videoColorFormat, frame->width, frame->height, XbmcThreads::SystemClockMillis() - time);
  #endif
  }

  if (p->drop_state)
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;

  p->prev_frame = frame;
  p->cur_frame = NULL;
  p->out_condition.notify();
  p->out_mutex.unlock();

  return true;
}

void CStageFrightVideo::Dispose()
{
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Close\n", CLASSNAME);
#endif

  Frame *frame;

  if (p->decode_thread && p->decode_thread->IsRunning())
    p->decode_thread->StopThread(false);
  p->decode_thread = NULL;
  p->in_condition.notify();

  // Give decoder_thread time to process EOS, if stuck on reading
  usleep(50000);

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning OUT\n");
#endif
  p->out_mutex.lock();
  if (p->cur_frame)
  {
    if (p->cur_frame->medbuf)
      p->cur_frame->medbuf->release();
    free(p->cur_frame);
    p->cur_frame = NULL;
  }
  while (p->outbuf_queue.size())
  {
    Frame* frame = p->outbuf_queue.front();
    p->outbuf_queue.pop_front();
    if (frame->medbuf)
    {
      if (frame->medbuf->range_length() == sizeof(VPU_FRAME))
      {
        VPU_FRAME *vpuframe = (VPU_FRAME *)((long)frame->medbuf->data() + frame->medbuf->range_offset());
        if(vpuframe->vpumem.phy_addr)
        {
          VPUMemLink(&vpuframe->vpumem);
          VPUFreeLinear(&vpuframe->vpumem);
        }
      }
      frame->medbuf->release();
    }
    free(frame);
  }
  while(p->busy_vpu_queue.size())
  {
    VPU_FRAME* vpuframe = p->busy_vpu_queue.front();
    ReleaseVpuFrame(vpuframe);
  }

  p->out_condition.notify();
  p->out_mutex.unlock();

  if (p->prev_frame)
  {
    if (p->prev_frame->medbuf)
    {
      if (p->prev_frame->medbuf->range_length() == sizeof(VPU_FRAME))
      {
        VPU_FRAME *vpuframe = (VPU_FRAME *)((long)p->prev_frame->medbuf->data() + p->prev_frame->medbuf->range_offset());
        if(vpuframe->vpumem.phy_addr)
        {
          VPUMemLink(&vpuframe->vpumem);
          VPUFreeLinear(&vpuframe->vpumem);
        }
      }
      p->prev_frame->medbuf->release();
    }
    free(p->prev_frame);
    p->prev_frame = NULL;
  }

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Stopping omxcodec\n");
#endif
  if (p->decoder != NULL)
    p->decoder->stop();
  if (p->client)
    p->client->disconnect();

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning IN(%d)\n", p->in_queue.size());
#endif
  std::map<int64_t,Frame*>::iterator it;
  while (!p->in_queue.empty())
  {
    it = p->in_queue.begin();
    frame = it->second;
    p->in_queue.erase(it);
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
  }
  
  if (p->decoder_component)
    free(&p->decoder_component);

  delete p->client;

/*
  if ((p->quirks & QuirkSWRender) == 0)
    p->m_g_xbmcapp->UninitStagefrightSurface();
*/

  for (int i=0; i<INBUFCOUNT; ++i)
  {
    if (p->inbuf[i])
    {
      p->inbuf[i]->setObserver(NULL);
      p->inbuf[i]->release();
      p->inbuf[i] = NULL;
    }
  }
}

void CStageFrightVideo::Reset(void)
{
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Reset\n", CLASSNAME);
#endif
  Frame* frame;
  p->in_mutex.lock();
  std::map<int64_t,Frame*>::iterator it;
  while (!p->in_queue.empty())
  {
    it = p->in_queue.begin();
    frame = it->second;
    p->in_queue.erase(it);
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
  }
  p->resetting = true;
  p->framecount = 0;

  p->in_mutex.unlock();
}

void CStageFrightVideo::SetDropState(bool bDrop)
{
  if (bDrop == p->drop_state)
    return;

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::SetDropState (%d->%d)\n", CLASSNAME,p->drop_state,bDrop);
#endif

  p->drop_state = bDrop;
}

void CStageFrightVideo::SetSpeed(int iSpeed)
{
}

/***************/

void CStageFrightVideo::LockEGLBuffer(EGLImageKHR eglimg)
{
  if (eglimg == EGL_NO_IMAGE_KHR)
    return;

 #if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  p->free_mutex.lock();
  std::list<std::pair<EGLImageKHR, int> >::iterator it = p->free_queue.begin();
  for(;it != p->free_queue.end(); ++it)
  {
    if ((*it).first == eglimg)
      break;
  }
  if (it == p->free_queue.end())
  {
    p->busy_queue.push_back(std::pair<EGLImageKHR, int>(*it));
    p->free_mutex.unlock();
    return;
  }
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Locking %p: tm:%d\n", eglimg, XbmcThreads::SystemClockMillis() - time);
#endif

  p->busy_queue.push_back(std::pair<EGLImageKHR, int>(*it));
  p->free_queue.erase(it);
  p->free_mutex.unlock();
}

bool CStageFrightVideo::ReleaseEGLBuffer(EGLImageKHR eglimg)
{
  if (eglimg == EGL_NO_IMAGE_KHR)
    return true;

#if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  p->free_mutex.lock();
  int cnt = 0;
  std::list<std::pair<EGLImageKHR, int> >::iterator it = p->busy_queue.begin();
  std::list<std::pair<EGLImageKHR, int> >::iterator itfree;
  for(;it != p->busy_queue.end(); ++it)
  {
    if ((*it).first == eglimg)
    {
      cnt++;
      if (cnt==1)
        itfree = it;
      else
        break;
    }
  }
  if (it == p->busy_queue.end() && !cnt)
  {
    p->free_mutex.unlock();
    return true;
  }
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Unlocking %p: cnt:%d tm:%d\n", eglimg, cnt, XbmcThreads::SystemClockMillis() - time);
#endif

  if (cnt==1)
  {
    p->free_queue.push_back(std::pair<EGLImageKHR, int>(*itfree));
    p->busy_queue.erase(itfree);
    p->free_mutex.unlock();
    return true;
  }
  p->free_mutex.unlock();
  return false;
}

void CStageFrightVideo::LockVpuFrame(VPU_FRAME *vpuframe)
{
  if (!vpuframe)
    return;

#if defined(DEBUG_VERBOSE)
//  CLog::Log(LOGDEBUG, "Locking %p\n", vpuframe);
#endif
  p->free_mutex.lock();
  p->busy_vpu_queue.push_back(vpuframe);
  p->free_mutex.unlock();
}

bool CStageFrightVideo::ReleaseVpuFrame(VPU_FRAME *vpuframe)
{
  if (!vpuframe)
    return true;

#if defined(DEBUG_VERBOSE)
//  CLog::Log(LOGDEBUG, "Unlocking %p\n", vpuframe);
#endif
  p->free_mutex.lock();
  int cnt = 0;
  std::list<VPU_FRAME*>::iterator it = p->busy_vpu_queue.begin();
  std::list<VPU_FRAME*>::iterator itfree;

  for(;it != p->busy_vpu_queue.end(); ++it)
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
    p->free_mutex.unlock();
    return true;
  }

  p->busy_vpu_queue.erase(itfree);
  if (cnt==1)
  {
#if defined(DEBUG_VERBOSE)
//    CLog::Log(LOGDEBUG, ">>> Deleting\n");
#endif
    if (vpuframe->vpumem.vir_addr)
      VPUFreeLinear(&vpuframe->vpumem);
    free(vpuframe);
    p->free_mutex.unlock();
    return true;
  }
  p->free_mutex.unlock();
  return false;
}


void CStageFrightVideo::LockBuffer(CDVDVideoCodecStageFrightBuffer *buf)
{
  if (buf)
  {
    if (buf->format == RENDER_FMT_EGLIMG)
      LockEGLBuffer((EGLImageKHR) buf->buffer);
    else if (buf->format == RENDER_FMT_STFBUF && buf->subformat == 'rkvp')
      LockVpuFrame((VPU_FRAME*) buf->context);
  }
}

void CStageFrightVideo::ReleaseBuffer(CDVDVideoCodecStageFrightBuffer *buf)
{
  if (buf)
  {
    bool ret = false;
    if (buf->format == RENDER_FMT_EGLIMG)
      ret = ReleaseEGLBuffer((EGLImageKHR) buf->buffer);
    else if (buf->format == RENDER_FMT_STFBUF && buf->subformat == 'rkvp')
      ret = ReleaseVpuFrame((VPU_FRAME*) buf->context);
    if (ret)
      delete buf;
  }
}
