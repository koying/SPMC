#pragma once

/*
 *      Copyright (C) 2014 Team Kodi
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

#include "utils/GlobalsHandling.h"

class CApplication;
class CApplicationMessenger;
class CWinSystemEGL;
class CAdvancedSettings;
class CXBMCRenderManager;
class CGraphicContext;

class CDVDCodecInterface
{
public:
  CDVDCodecInterface();

public:
  CApplication *GetApplication() const;
  CApplicationMessenger *GetApplicationMessenger() const;
  CWinSystemEGL *GetWindowSystem() const;
  CAdvancedSettings *GetAdvancedSettings() const;
  CXBMCRenderManager *GetRenderManager() const;
  CGraphicContext* GetGraphicsContext() const;
};

XBMC_GLOBAL_REF(CDVDCodecInterface,g_dvdcodecinterface);
#define g_dvdcodecinterface XBMC_GLOBAL_USE(CDVDCodecInterface)
