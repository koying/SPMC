/*
 *      Copyright (C) 2011-2014 Team XBMC
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

#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeAmlAndroid.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#if defined(TARGET_ANDROID)
  #include "android/activity/XBMCApp.h"
#endif
#include "utils/StringUtils.h"
#include "utils/AMLUtils.h"

bool CEGLNativeTypeAmlAndroid::CheckCompatibility()
{
#if defined(TARGET_ANDROID)
  return aml_present();
#endif
  return false;
}

bool CEGLNativeTypeAmlAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
  char mode[256] = {0};
  aml_get_sysfs_str("/sys/class/display/mode", mode, 255);
  if (aml_ModeToResolution(mode, res))
  {
    m_curResolution = mode;
    return true;
  }
  return false;
}

bool CEGLNativeTypeAmlAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
  switch((int)(res.fRefreshRate*10))
  {
    default:
    case 600:
      switch(res.iScreenWidth)
      {
        default:
        case 1280:
          SetDisplayResolution("720p");
          break;
        case 1920:
          if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
            SetDisplayResolution("1080i");
          else
            SetDisplayResolution("1080p");
          break;
      }
      break;
    case 500:
      switch(res.iScreenWidth)
      {
        default:
        case 1280:
          SetDisplayResolution("720p50hz");
          break;
        case 1920:
          if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
            SetDisplayResolution("1080i50hz");
          else
            SetDisplayResolution("1080p50hz");
          break;
      }
      break;
    case 599:
      switch(res.iScreenWidth)
      {
        case 3840:
          SetDisplayResolution("4k2k30hz");
          break;
        default:
          SetDisplayResolution("1080p30hz");
          break;
      }
      break;
    case 499:
      switch(res.iScreenWidth)
      {
        case 3840:
          SetDisplayResolution("4k2k25hz");
          break;
        default:
          SetDisplayResolution("1080p25hz");
          break;
      }
      break;
    case 479:
      switch(res.iScreenWidth)
      {
        case 3840:
          SetDisplayResolution("4k2k24hz");
          break;
        case 4096:
          SetDisplayResolution("4k2ksmpte");
          break;
        default:
          SetDisplayResolution("1080p24hz");
          break;
      }
      break;
  }

  return true;
}

bool CEGLNativeTypeAmlAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  char valstr[256] = {0};
  aml_get_sysfs_str("/sys/class/amhdmitx/amhdmitx0/disp_cap", valstr, 255);
  std::vector<CStdString> probe_str;
  StringUtils::SplitString(valstr, "\n", probe_str);

  resolutions.clear();
  RESOLUTION_INFO res;
  for (size_t i = 0; i < probe_str.size(); i++)
  {
    if(aml_ModeToResolution(probe_str[i].c_str(), &res))
      resolutions.push_back(res);
  }
  return resolutions.size() > 0;

}

bool CEGLNativeTypeAmlAndroid::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  // check display/mode, it gets defaulted at boot
  if (!GetNativeResolution(res))
  {
    // punt to 720p if we get nothing
    aml_ModeToResolution("720p", res);
  }

  return true;
}

bool CEGLNativeTypeAmlAndroid::SetDisplayResolution(const char *resolution)
{
  if (m_curResolution == resolution)
    return true;

  // switch display resolution
  aml_set_sysfs_str("/sys/class/display/mode", resolution);
  m_curResolution = resolution;

  return true;
}

