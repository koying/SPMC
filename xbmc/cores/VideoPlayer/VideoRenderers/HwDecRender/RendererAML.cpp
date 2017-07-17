/*
 *      Copyright (C) 2007-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RendererAML.h"

#if defined(HAS_LIBAMCODEC)
#include "cores/IPlayer.h"
#include "windowing/egl/EGLWrapper.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecAmlogic.h"
#include "cores/VideoPlayer/DVDCodecs/Video/AMLCodec.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderCapture.h"

static int set_pts_pcrscr(int64_t value)
{
  int fd = open("/sys/class/tsync/pts_pcrscr", O_WRONLY);
  if (fd >= 0)
  {
    char pts_str[64];
    unsigned long pts = (unsigned long)value;
    sprintf(pts_str, "0x%lx", pts);
    write(fd, pts_str, strlen(pts_str));
    close(fd);
    return 0;
  }

  CLog::Log(LOGERROR, "set_pts_pcrscr: open pts_pcrscr error");
  return -1;
}

static void SetVideoPtsSeconds(const double pts)
{
  //CLog::Log(LOGDEBUG, "CAMLCodec::SetVideoPtsSeconds: pts(%f)", pts);
  if (pts >= 0.0)
  {
    int64_t pts_video = (int64_t)(pts * AML_PTS_FREQ);
    set_pts_pcrscr(pts_video);
  }
}


CRendererAML::CRendererAML()
{

}

CRendererAML::~CRendererAML()
{

}

bool CRendererAML::RenderCapture(CRenderCapture* capture)
{
  capture->BeginRender();
  capture->EndRender();
  return true;
}

void CRendererAML::AddVideoPictureHW(DVDVideoPicture &picture, int index, double currentClock)
{
  YUVBUFFER &buf = m_buffers[index];
  if (picture.amlcodec)
  {
    CDVDAmlogicInfo *amli = static_cast<CDVDAmlogicInfo *>(buf.hwDec);
    amli->m_pts_video = picture.pts;
    amli->m_clock = currentClock;
    buf.hwDec = picture.amlcodec->Retain();
  }
}

void CRendererAML::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    CDVDAmlogicInfo *amli = static_cast<CDVDAmlogicInfo *>(buf.hwDec);
    SAFE_RELEASE(amli);
    buf.hwDec = NULL;
  }
}

int CRendererAML::GetImageHook(YV12Image *image, int source, bool readonly)
{
  return source;
}

bool CRendererAML::IsGuiLayer()
{
  return false;
}

bool CRendererAML::Supports(EINTERLACEMETHOD method)
{
  return false;
}

bool CRendererAML::Supports(ESCALINGMETHOD method)
{
  return false;
}

bool CRendererAML::Supports(ERENDERFEATURE feature)
{
  if (feature == RENDERFEATURE_ZOOM ||
      feature == RENDERFEATURE_CONTRAST ||
      feature == RENDERFEATURE_BRIGHTNESS ||
      feature == RENDERFEATURE_STRETCH ||
      feature == RENDERFEATURE_PIXEL_RATIO ||
      feature == RENDERFEATURE_ROTATION)
    return true;

  return false;
}

EINTERLACEMETHOD CRendererAML::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

bool CRendererAML::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using AML render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_FMT_AML;
  return false;
}

bool CRendererAML::RenderHook(int index)
{
  return true;// nothing to be done for aml
}

bool CRendererAML::RenderUpdateVideoHook(bool clear, DWORD flags, DWORD alpha)
{
  ManageRenderArea();

  CDVDAmlogicInfo *amli = static_cast<CDVDAmlogicInfo *>(m_buffers[m_iYV12RenderBuffer].hwDec);
  if (amli)
  {
    CAMLCodec *amlcodec = amli->getAmlCodec();
    if (amlcodec)
      amlcodec->SetVideoRect(m_sourceRect, m_destRect);

    double app_pts = amli->m_clock;
    // add in audio delay/display latency contribution
    // FIXME: Replace Video latency?
    double offset  = 0 - CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay;
    // correct video pts by user set delay and rendering delay
    app_pts += offset;

    //CLog::Log(LOGDEBUG, "CAMLCodec::Process: app_pts(%f), pts_video/PTS_FREQ(%f)",
    //  app_pts, (double)pts_video/PTS_FREQ);

    double error = app_pts - (double)amli->m_pts_video/AML_PTS_FREQ;
    double abs_error = fabs(error);
    if (abs_error > 0.125)
    {
      //CLog::Log(LOGDEBUG, "CAMLCodec::Process pts diff = %f", error);
      if (abs_error > 0.150)
      {
        // big error so try to reset pts_pcrscr
        SetVideoPtsSeconds(app_pts);
      }
      else
      {
        // small error so try to avoid a frame jump
        SetVideoPtsSeconds((double)amli->m_pts_video/AML_PTS_FREQ + error/4);
      }
    }
  }
  usleep(10000);

  return true;
}

#endif

