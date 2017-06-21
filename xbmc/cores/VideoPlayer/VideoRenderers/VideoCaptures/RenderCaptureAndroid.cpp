/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "RenderCaptureAndroid.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "cores/IPlayer.h"
#include "platform/android/activity/XBMCApp.h"

extern "C" {
#include "libavutil/mem.h"
#include "libswscale/swscale.h"
}

CRenderCaptureAndroid::CRenderCaptureAndroid()
{
}

CRenderCaptureAndroid::~CRenderCaptureAndroid()
{
  if (g_advancedSettings.m_videoUseDroidProjectionCapture)
    CXBMCApp::StopCapture();
  delete[] m_pixels;
}

int CRenderCaptureAndroid::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureAndroid::BeginRender()
{
  if (m_bufferSize != m_width * m_height * 4)
  {
    delete[] m_pixels;
    m_bufferSize = m_width * m_height * 4;
    m_pixels = new uint8_t[m_bufferSize];
  }
  if (g_advancedSettings.m_videoUseDroidProjectionCapture)
  {
    m_asyncSupported = true;
    CXBMCApp::startCapture(m_width, m_height);
  }
}

void CRenderCaptureAndroid::EndRender()
{
  if (g_advancedSettings.m_videoUseDroidProjectionCapture)
  {
    if (m_flags & CAPTUREFLAG_IMMEDIATELY)
      ReadOut();
    else
      SetState(CAPTURESTATE_NEEDSREADOUT);
  }
  else
    SetState(CAPTURESTATE_DONE);
}

void* CRenderCaptureAndroid::GetRenderBuffer()
{
    return m_pixels;
}

void CRenderCaptureAndroid::ReadOut()
{
  if (g_advancedSettings.m_videoUseDroidProjectionCapture)
  {
    jni::CJNIImage image;
    if (CXBMCApp::GetCapture(image))
    {
      int iWidth = image.getWidth();
      int iHeight = image.getHeight();

      std::vector<jni::CJNIImagePlane> planes = image.getPlanes();
      CJNIByteBuffer bytebuffer = planes[0].getBuffer();

      struct SwsContext *context = sws_getContext(iWidth, iHeight, AV_PIX_FMT_RGBA,
                                                  m_width, m_height, AV_PIX_FMT_BGRA,
                                                  SWS_FAST_BILINEAR, NULL, NULL, NULL);

      void *buf_ptr = xbmc_jnienv()->GetDirectBufferAddress(bytebuffer.get_raw());

      uint8_t *src[] = { (uint8_t*)buf_ptr, 0, 0, 0 };
      int     srcStride[] = { planes[0].getRowStride(), 0, 0, 0 };

      uint8_t *dst[] = { m_pixels, 0, 0, 0 };
      int     dstStride[] = { (int)m_width * 4, 0, 0, 0 };

      if (context)
      {
        sws_scale(context, src, srcStride, 0, iHeight, dst, dstStride);
        sws_freeContext(context);
      }

      image.close();
      SetState(CAPTURESTATE_DONE);
    }
    else
      SetState(CAPTURESTATE_FAILED);
  }
}
