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

#include "DVDCodecInterface.h"

#include "Application.h"
#include "ApplicationMessenger.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"

CDVDCodecInterface::CDVDCodecInterface()
{
}

CApplication* CDVDCodecInterface::GetApplication() const
{
  return &g_application;
}

CApplicationMessenger* CDVDCodecInterface::GetApplicationMessenger() const
{
  return &CApplicationMessenger::Get();
}

CWinSystemEGL* CDVDCodecInterface::GetWindowSystem() const
{
  return &g_Windowing;
}

CAdvancedSettings* CDVDCodecInterface::GetAdvancedSettings() const
{
  return &g_advancedSettings;
}
