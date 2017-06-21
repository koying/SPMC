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

#pragma once

#include "system.h" //HAS_DX, HAS_GL, HAS_GLES, opengl headers, direct3d headers

#include "threads/Event.h"

enum ECAPTURESTATE
{
  CAPTURESTATE_WORKING,
  CAPTURESTATE_NEEDSRENDER,
  CAPTURESTATE_NEEDSREADOUT,
  CAPTURESTATE_DONE,
  CAPTURESTATE_FAILED,
  CAPTURESTATE_NEEDSDELETE
};

class CBaseRenderCapture
{
  public:
    CBaseRenderCapture();
    ~CBaseRenderCapture();

    /* \brief Called by the rendermanager to set the state, should not be called by anything else */
    void SetState(ECAPTURESTATE state) { m_state = state; }

    /* \brief Called by the rendermanager to get the state, should not be called by anything else */
    ECAPTURESTATE GetState() { return m_state;}

    /* \brief Called by the rendermanager to set the userstate, should not be called by anything else */
    void SetUserState(ECAPTURESTATE state) { m_userState = state; }

    /* \brief Called by the code requesting the capture
       \return CAPTURESTATE_WORKING when the capture is in progress,
       CAPTURESTATE_DONE when the capture has succeeded,
       CAPTURESTATE_FAILED when the capture has failed
    */
    ECAPTURESTATE GetUserState() { return m_userState; }

    /* \brief The internal event will be set when the rendermanager has captured and read a videoframe, or when it has failed
       \return A reference to m_event
    */
    CEvent& GetEvent() { return m_event; }

    /* \brief Called by the rendermanager to set the flags, should not be called by anything else */
    void SetFlags(int flags) { m_flags = flags; }

    /* \brief Called by the rendermanager to get the flags, should not be called by anything else */
    int GetFlags() { return m_flags; }

    /* \brief Called by the rendermanager to set the width, should not be called by anything else */
    void  SetWidth(unsigned int width) { m_width = width; }

    /* \brief Called by the rendermanager to set the height, should not be called by anything else */
    void SetHeight(unsigned int height) { m_height = height; }

    /* \brief Called by the code requesting the capture to get the width */
    unsigned int GetWidth() { return m_width; }

    /* \brief Called by the code requesting the capture to get the height */
    unsigned int GetHeight() { return m_height; }

    /* \brief Called by the code requesting the capture to get the buffer where the videoframe is stored,
       the format is BGRA, this buffer is only valid when GetUserState returns CAPTURESTATE_DONE.
       The size of the buffer is GetWidth() * GetHeight() * 4.
    */
    uint8_t*  GetPixels() const { return m_pixels; }

    /* \brief Called by the rendermanager to know if the capture is readout async (using dma for example),
       should not be called by anything else.
    */
    bool  IsAsync() { return m_asyncSupported; }

  protected:
    bool UseOcclusionQuery();

    ECAPTURESTATE  m_state;     //state for the rendermanager
    ECAPTURESTATE  m_userState; //state for the thread that wants the capture
    int m_flags;
    CEvent m_event;

    uint8_t*  m_pixels;
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_bufferSize;

    //this is set after the first render
    bool m_asyncSupported;
    bool m_asyncChecked;
};

#if defined(TARGET_ANDROID)
#include "VideoCaptures/RenderCaptureAndroid.h"

#elif defined(HAS_IMXVPU)
#include "VideoCaptures/RenderCaptureIMX.h"

#elif defined(TARGET_RASPBERRY_PI)
#include "VideoCaptures/RenderCaptureRPI.h"

#elif defined(HAS_GL)
#include "VideoCaptures/RenderCaptureGL.h"

#elif defined(HAS_GLES)
#include "VideoCaptures/RenderCaptureGLES.h"

#elif HAS_DX /*HAS_GL*/
#include "VideoCaptures/RenderCaptureDX.h"

#endif
