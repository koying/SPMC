/*
 *      Copyright (C) 2005-2016 Team XBMC
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

#include "OverlayRenderer.h"
#include <string>

#define OVERLAY_LINES 6

class CDVDOverlayText;

class CDebugRenderer
{
public:
  CDebugRenderer();
  virtual ~CDebugRenderer();
  void SetInfo(std::string &info1, std::string &info2, std::string &info3, std::string &info4, std::string &info5, std::string &info6);
  void Render(CRect &src, CRect &dst, CRect &view);
  void Flush();

protected:

  class CRenderer : public OVERLAY::CRenderer
  {
  public:
    CRenderer();
    void Render(int idx) override;
  };

  std::string m_strDebug[OVERLAY_LINES];
  CDVDOverlayText *m_overlay[OVERLAY_LINES];
  CRenderer m_overlayRenderer;
};
