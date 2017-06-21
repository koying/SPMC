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

#include "RenderCaptureGL.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "cores/IPlayer.h"
extern "C" {
#include "libavutil/mem.h"
}

CRenderCaptureGL::CRenderCaptureGL()
{
  m_pbo   = 0;
  m_query = 0;
  m_occlusionQuerySupported = false;
}

CRenderCaptureGL::~CRenderCaptureGL()
{
  if (m_asyncSupported)
  {
    if (m_pbo)
    {
      glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
      glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
      glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
      glDeleteBuffersARB(1, &m_pbo);
    }

    if (m_query)
      glDeleteQueriesARB(1, &m_query);
  }

  delete[] m_pixels;
}

int CRenderCaptureGL::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureGL::BeginRender()
{
  if (!m_asyncChecked)
  {
    m_asyncSupported = g_Windowing.IsExtSupported("GL_ARB_pixel_buffer_object");
    m_occlusionQuerySupported = g_Windowing.IsExtSupported("GL_ARB_occlusion_query");

    if (m_flags & CAPTUREFLAG_CONTINUOUS)
    {
      if (!m_occlusionQuerySupported)
        CLog::Log(LOGWARNING, "CRenderCaptureGL: GL_ARB_occlusion_query not supported, performance might suffer");
      if (!g_Windowing.IsExtSupported("GL_ARB_pixel_buffer_object"))
        CLog::Log(LOGWARNING, "CRenderCaptureGL: GL_ARB_pixel_buffer_object not supported, performance might suffer");
      if (UseOcclusionQuery())
        CLog::Log(LOGWARNING, "CRenderCaptureGL: GL_ARB_occlusion_query disabled, performance might suffer");
    }
    m_asyncChecked = true;
  }

  if (m_asyncSupported)
  {
    if (!m_pbo)
      glGenBuffersARB(1, &m_pbo);

    if (UseOcclusionQuery() && m_occlusionQuerySupported)
    {
      //generate an occlusion query if we don't have one
      if (!m_query)
        glGenQueriesARB(1, &m_query);
    }
    else
    {
      //don't use an occlusion query, clean up any old one
      if (m_query)
      {
        glDeleteQueriesARB(1, &m_query);
        m_query = 0;
      }
    }

    //start the occlusion query
    if (m_query)
      glBeginQueryARB(GL_SAMPLES_PASSED_ARB, m_query);

    //allocate data on the pbo and pixel buffer
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
    if (m_bufferSize != m_width * m_height * 4)
    {
      m_bufferSize = m_width * m_height * 4;
      glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, m_bufferSize, 0, GL_STREAM_READ_ARB);
      delete[] m_pixels;
      m_pixels = new uint8_t[m_bufferSize];
    }
  }
  else
  {
    if (m_bufferSize != m_width * m_height * 4)
    {
      delete[] m_pixels;
      m_bufferSize = m_width * m_height * 4;
      m_pixels = new uint8_t[m_bufferSize];
    }
  }
}

void CRenderCaptureGL::EndRender()
{
  if (m_asyncSupported)
  {
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);

    if (m_query)
      glEndQueryARB(GL_SAMPLES_PASSED_ARB);

    if (m_flags & CAPTUREFLAG_IMMEDIATELY)
      PboToBuffer();
    else
      SetState(CAPTURESTATE_NEEDSREADOUT);
  }
  else
  {
    SetState(CAPTURESTATE_DONE);
  }
}

void* CRenderCaptureGL::GetRenderBuffer()
{
  if (m_asyncSupported)
  {
    return NULL; //offset into the pbo
  }
  else
  {
    return m_pixels;
  }
}

void CRenderCaptureGL::ReadOut()
{
  if (m_asyncSupported)
  {
    //we don't care about the occlusion query, we just want to know if the result is available
    //when it is, the write into the pbo is probably done as well,
    //so it can be mapped and read without a busy wait

    GLuint readout = 1;
    if (m_query)
      glGetQueryObjectuivARB(m_query, GL_QUERY_RESULT_AVAILABLE_ARB, &readout);

    if (readout)
      PboToBuffer();
  }
}

void CRenderCaptureGL::PboToBuffer()
{
  glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
  GLvoid* pboPtr = glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);

  if (pboPtr)
  {
    memcpy(m_pixels, pboPtr, m_bufferSize);
    SetState(CAPTURESTATE_DONE);
  }
  else
  {
    CLog::Log(LOGERROR, "CRenderCaptureGL::PboToBuffer: glMapBufferARB failed");
    SetState(CAPTURESTATE_FAILED);
  }

  glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
  glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
}

