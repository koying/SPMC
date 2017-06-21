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

#include "RenderCaptureIMX.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "cores/IPlayer.h"
extern "C" {
#include "libavutil/mem.h"
}

CRenderCaptureIMX::CRenderCaptureIMX()
{
}

CRenderCaptureIMX::~CRenderCaptureIMX()
{
}

int CRenderCaptureIMX::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureIMX::BeginRender()
{
}

void CRenderCaptureIMX::EndRender()
{
  if (g_IMXContext.CaptureDisplay(m_pixels, m_width, m_height))
    SetState(CAPTURESTATE_DONE);
  else
    SetState(CAPTURESTATE_FAILED);
}

void* CRenderCaptureIMX::GetRenderBuffer()
{
  return m_pixels;
}

void CRenderCaptureIMX::ReadOut()
{
}
