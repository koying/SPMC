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

#include <stdlib.h>

#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeTegraAndroid.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#include "android/activity/XBMCApp.h"
#include "android/jni/Build.h"
#include "utils/StringUtils.h"
#include "utils/SysfsUtils.h"
#include "utils/RegExp.h"

bool CEGLNativeTypeTegraAndroid::CheckCompatibility()
{
  if (!SysfsUtils::Has("/sys/class/graphics/fb0/device/nvdps"))
    return false;

  return SysfsUtils::Has("/system/xbin/su");
}

bool CEGLNativeTypeTegraAndroid::SysModeToResolution(std::string mode, RESOLUTION_INFO *res) const
{
  if (!res)
    return false;

  res->iWidth = 0;
  res->iHeight= 0;

  if(mode.empty())
    return false;

  std::string fromMode = StringUtils::Mid(mode, 2);
  StringUtils::Trim(fromMode);

  CRegExp split(true);
  split.RegComp("([0-9]+)x([0-9]+)([pi])-([0-9]+)");
  if (split.RegFind(fromMode) < 0)
    return false;

  int w = atoi(split.GetMatch(1).c_str());
  int h = atoi(split.GetMatch(2).c_str());
  std::string p = split.GetMatch(3);
  int r = atoi(split.GetMatch(4).c_str());

  res->iWidth = w;
  res->iHeight= h;
  res->iScreenWidth = w;
  res->iScreenHeight= h;
  res->fRefreshRate = r;
  res->dwFlags = p[0] == 'p' ? D3DPRESENTFLAG_PROGRESSIVE : D3DPRESENTFLAG_INTERLACED;

  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);

  res->fPixelRatio   = 1.0f;
  res->strMode       = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
                                           res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  res->strId         = mode;

  return res->iWidth > 0 && res->iHeight> 0;
}

bool CEGLNativeTypeTegraAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
  CEGLNativeTypeAndroid::GetNativeResolution(&m_fb_res);

  std::string mode;
  RESOLUTION_INFO hdmi_res;
  if (SysfsUtils::GetString("/sys/class/graphics/fb0/mode", mode) == 0 && SysModeToResolution(mode, &hdmi_res))
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

bool CEGLNativeTypeTegraAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
  std::string mode = StringUtils::Format("%dx%d%c-%d",
                                        res.iScreenWidth,
                                        res.iScreenHeight,
                                        res.dwFlags & D3DPRESENTFLAG_INTERLACED ? 'i' : 'p',
                                        (int)res.fRefreshRate);
  if (m_curHdmiResolution == mode)
    return true;

  SysfsUtils::Su("echo 0 > /sys/class/graphics/fb0/device/enable");
  std::string cmd = StringUtils::Format("echo %d > /sys/class/graphics/fb0/device/nvdps", int(res.fRefreshRate));
  SysfsUtils::Su(cmd);
  SysfsUtils::Su("echo 1 > /sys/class/graphics/fb0/device/enable");

  m_curHdmiResolution = mode;

  return true;
}

bool CEGLNativeTypeTegraAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  CEGLNativeTypeAndroid::GetNativeResolution(&m_fb_res);

  std::string valstr;
  if (SysfsUtils::GetString("/sys/class/graphics/fb0/modes", valstr) < 0)
    return false;
  std::vector<std::string> probe_str = StringUtils::Split(valstr, "\n");

  resolutions.clear();
  RESOLUTION_INFO res;
  for (size_t i = 0; i < probe_str.size(); i++)
  {
    if(SysModeToResolution(probe_str[i].c_str(), &res))
    {
      res.iWidth = m_fb_res.iWidth;
      res.iHeight = m_fb_res.iHeight;
      res.iSubtitles    = (int)(0.965 * res.iHeight);
      resolutions.push_back(res);
    }
  }
  return resolutions.size() > 0;

}

bool CEGLNativeTypeTegraAndroid::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return GetNativeResolution(res);
}
