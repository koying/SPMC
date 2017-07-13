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

#include "RendererMediaCodecSurface.h"

#if defined(TARGET_ANDROID)
#include "../RenderCapture.h"

#include "platform/android/activity/XBMCApp.h"
#include "DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "utils/log.h"
#include "settings/MediaSettings.h"

CRendererMediaCodecSurface::CRendererMediaCodecSurface()
{
}

CRendererMediaCodecSurface::~CRendererMediaCodecSurface()
{
}

bool CRendererMediaCodecSurface::RenderCapture(CRenderCapture* capture)
{
  capture->BeginRender();
  capture->EndRender();
  return true;
}

bool CRendererMediaCodecSurface::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation)
{
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_renderOrientation = orientation;

  // Save the flags.
  m_iFlags = flags;
  m_format = format;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(d_width, d_height);
  SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);
  ManageRenderArea();

  m_bConfigured = true;
  m_bImageReady = false;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  m_bValidated = true;

  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
    m_buffers[i].image.flags = 0;

  m_iLastRenderBuffer = -1;

  return true;
}

void CRendererMediaCodecSurface::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
  int mindex = -1;
#endif

  YUVBUFFER &buf = m_buffers[index];
  if (picture.mediacodec)
  {
    buf.hwDec = picture.mediacodec->Retain();
#ifdef DEBUG_VERBOSE
    mindex = ((CDVDMediaCodecInfo *)buf.hwDec)->GetIndex();
#endif
  }

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "AddProcessor %d: img:%d tm:%d", index, mindex, XbmcThreads::SystemClockMillis() - time);
#endif
}

void CRendererMediaCodecSurface::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured)
    return;

  if (!m_bImageReady) return;

  int index = m_iYV12RenderBuffer;
  YUVBUFFER& buf =  m_buffers[index];

  if (buf.image.flags==0)
    return;

  ManageRenderArea();

  m_iLastRenderBuffer = index;
}

void CRendererMediaCodecSurface::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    CDVDMediaCodecInfo *mci = static_cast<CDVDMediaCodecInfo *>(buf.hwDec);
    SAFE_RELEASE(mci);
    buf.hwDec = NULL;
  }
}

int CRendererMediaCodecSurface::GetImageHook(YV12Image *image, int source, bool readonly)
{
  return source;
}

bool CRendererMediaCodecSurface::Supports(EINTERLACEMETHOD method)
{
  return false;
}

EINTERLACEMETHOD CRendererMediaCodecSurface::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

CRenderInfo CRendererMediaCodecSurface::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = 4;
  info.optimal_buffer_size = 3;
  return info;
}

bool CRendererMediaCodecSurface::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using MediaCodec (Surface) render method");
  m_renderMethod = RENDER_MEDIACODECSURFACE;
  m_textureTarget = GL_TEXTURE_2D;
  return true;
}

bool CRendererMediaCodecSurface::RenderHook(int index)
{
  CDVDMediaCodecInfo *mci = static_cast<CDVDMediaCodecInfo *>(m_buffers[index].hwDec);
  if (mci && !mci->IsReleased())
  {
    // this hack is needed to get the 2D mode of a 3D movie going
    RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();
    if (stereo_mode)
      g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_LEFT);

    ManageRenderArea();

    if (stereo_mode)
      g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_OFF);

    CRect dstRect(m_destRect);
    CRect srcRect(m_sourceRect);
    switch (stereo_mode)
    {
      case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
        dstRect.y2 *= 2.0;
        srcRect.y2 *= 2.0;
      break;

      case RENDER_STEREO_MODE_SPLIT_VERTICAL:
        dstRect.x2 *= 2.0;
        srcRect.x2 *= 2.0;
      break;

      case RENDER_STEREO_MODE_MONO:
        dstRect.y2 = dstRect.y2 * (dstRect.y2 / m_sourceRect.y2);
        dstRect.x2 = dstRect.x2 * (dstRect.x2 / m_sourceRect.x2);
      break;

      default:
      break;
    }


    // Handle orientation
    switch (m_renderOrientation)
    {
      case 90:
      case 270:
      {
        int diffX = 0;
        int diffY = 0;
        int centerX = 0;
        int centerY = 0;

        int newWidth = dstRect.Height(); // new width is old height
        int newHeight = dstRect.Width(); // new height is old width
        int diffWidth = newWidth - dstRect.Width(); // difference between old and new width
        int diffHeight = newHeight - dstRect.Height(); // difference between old and new height

        // if the new width is bigger then the old or
        // the new height is bigger then the old - we need to scale down
        if (diffWidth > 0 || diffHeight > 0 )
        {
          float aspectRatio = GetAspectRatio();
          // scale to fit screen width because
          // the difference in width is bigger then the
          // difference in height
          if (diffWidth > diffHeight)
          {
            newWidth = dstRect.Width(); // clamp to the width of the old dest rect
            newHeight *= aspectRatio;
          }
          else // scale to fit screen height
          {
            newHeight = dstRect.Height(); // clamp to the height of the old dest rect
            newWidth /= aspectRatio;
          }
        }

        // calculate the center point of the view
        centerX = m_viewRect.x1 + m_viewRect.Width() / 2;
        centerY = m_viewRect.y1 + m_viewRect.Height() / 2;

        // calculate the number of pixels we need to go in each
        // x direction from the center point
        diffX = newWidth / 2;
        // calculate the number of pixels we need to go in each
        // y direction from the center point
        diffY = newHeight / 2;

        dstRect = CRect(centerX - diffX, centerY - diffY, centerX + diffX, centerY + diffY);

        break;
      }

      default:
        break;
    }

    mci->RenderUpdate(srcRect, dstRect);
  }
  return true;
}

bool CRendererMediaCodecSurface::CreateTexture(int index)
{
  return true; // nothing todo
}

void CRendererMediaCodecSurface::DeleteTexture(int index)
{
  return; // nothing todo
}

bool CRendererMediaCodecSurface::UploadTexture(int index)
{
  return true; // nothing todo
}

void CRendererMediaCodecSurface::UnInit()
{
  CLog::Log(LOGDEBUG, "RendererMediaCodecSurface: Cleaning up resources");
  m_bValidated = false;
  m_bImageReady = false;
  m_bConfigured = false;
}


#endif
