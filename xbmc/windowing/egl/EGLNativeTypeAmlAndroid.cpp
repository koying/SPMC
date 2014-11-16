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
  CEGLNativeTypeAndroid::GetNativeResolution(&m_fb_res);

  char mode[256] = {0};
  RESOLUTION_INFO hdmi_res;
  if (aml_get_sysfs_str("/sys/class/display/mode", mode, 255) == 0 && aml_mode_to_resolution(mode, &hdmi_res))
  {
    m_curHdmiResolution = mode;
    *res = hdmi_res;
    res->iWidth = m_fb_res.iWidth;
    res->iHeight = m_fb_res.iHeight;
    res->iSubtitles = (int)(0.965 * res->iHeight);
  }
  else
    *res = m_fb_res;

  return true;
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
          return SetDisplayResolution("720p");
          break;
        case 1920:
          if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
            return SetDisplayResolution("1080i");
          else
            return SetDisplayResolution("1080p");
          break;
      }
      break;
    case 500:
      switch(res.iScreenWidth)
      {
        default:
        case 1280:
          return SetDisplayResolution("720p50hz");
          break;
        case 1920:
          if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
            return SetDisplayResolution("1080i50hz");
          else
            return SetDisplayResolution("1080p50hz");
          break;
      }
      break;
    case 599:
      switch(res.iScreenWidth)
      {
        case 3840:
          return SetDisplayResolution("4k2k30hz");
          break;
        default:
          return SetDisplayResolution("1080p30hz");
          break;
      }
      break;
    case 499:
      switch(res.iScreenWidth)
      {
        case 3840:
          return SetDisplayResolution("4k2k25hz");
          break;
        default:
          return SetDisplayResolution("1080p25hz");
          break;
      }
      break;
    case 479:
      switch(res.iScreenWidth)
      {
        case 3840:
          return SetDisplayResolution("4k2k24hz");
          break;
        case 4096:
          return SetDisplayResolution("4k2ksmpte");
          break;
        default:
          return SetDisplayResolution("1080p24hz");
          break;
      }
      break;
  }

  return false;
}

bool CEGLNativeTypeAmlAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  CEGLNativeTypeAndroid::GetNativeResolution(&m_fb_res);

  char valstr[256] = {0};
  if (aml_get_sysfs_str("/sys/class/amhdmitx/amhdmitx0/disp_cap", valstr, 255) < 0)
    return false;
  std::vector<std::string> probe_str = StringUtils::Split(valstr, "\n");

  resolutions.clear();
  RESOLUTION_INFO res;
  for (size_t i = 0; i < probe_str.size(); i++)
  {
    if(aml_mode_to_resolution(probe_str[i].c_str(), &res))
    {
      res.iWidth = m_fb_res.iWidth;
      res.iHeight = m_fb_res.iHeight;
      res.iSubtitles = (int)(0.965 * res.iHeight);

      // Android is locked at 60fps. Fools XBMC into closest match
      if (res.fRefreshRate < 50)
        res.fRefreshRate = res.fRefreshRate*2 - 0.1;
      resolutions.push_back(res);
    }
  }
  return resolutions.size() > 0;

}

bool CEGLNativeTypeAmlAndroid::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return GetNativeResolution(res);
}

bool CEGLNativeTypeAmlAndroid::SetDisplayResolution(const char *resolution)
{
  if (m_curHdmiResolution == resolution)
    return true;

  // switch display resolution
  if (aml_set_sysfs_str("/sys/class/display/mode", resolution) < 0)
    return false;

  m_curHdmiResolution = resolution;

  return true;
}

