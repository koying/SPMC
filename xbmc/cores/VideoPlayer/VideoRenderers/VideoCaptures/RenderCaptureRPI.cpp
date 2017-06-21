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

#include "RenderCaptureRPI.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "cores/IPlayer.h"
extern "C" {
#include "libavutil/mem.h"
}

CRenderCaptureDispmanX::CRenderCaptureDispmanX()
{
  m_pixels = nullptr;
}

CRenderCaptureDispmanX::~CRenderCaptureDispmanX()
{
  delete[] m_pixels;
}

int CRenderCaptureDispmanX::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureDispmanX::BeginRender()
{
}

void CRenderCaptureDispmanX::EndRender()
{
  delete[] m_pixels;
  m_pixels = g_RBP.CaptureDisplay(m_width, m_height, NULL, true);

  SetState(CAPTURESTATE_DONE);
}

void* CRenderCaptureDispmanX::GetRenderBuffer()
{
  return m_pixels;
}

void CRenderCaptureDispmanX::ReadOut()
{
}
