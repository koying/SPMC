/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include "EGLNativeTypeAndroid.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#if defined(TARGET_ANDROID)
  #include "android/activity/XBMCApp.h"
  #if defined(HAS_AMLPLAYER) || defined(HAS_LIBAMCODEC)
    #include "utils/AMLUtils.h"
  #endif
#endif
#include "utils/StringUtils.h"
#include "utils/RegExp.h"

struct vic_item
{
  int id;
  int width;
  int height;
  bool interlaced;
  int rfrate;
};

static const struct vic_item vic_table[] =
{
  {1, 640, 480, false, 60},
  {2, 720, 480, false, 60},
  {3, 720, 480, false, 60},
  {4, 1280, 720, false, 60},
  {5, 1920, 1080, true, 60},
  {6, 720, 480, true, 60},
  {7, 720, 480, true, 60},
  {14, 1440, 480, false, 60},
  {15, 1440, 480, false, 60},
  {16, 1920, 1080, false, 60},
  {17, 720, 576, false, 50},
  {18, 720, 576, false, 50},
  {19, 1280, 720, false, 50},
  {20, 1920, 1080, true, 50},
  {21, 720, 576, true, 50},
  {22, 720, 576, true, 50},
  {29, 1440, 576, false, 50},
  {30, 1440, 576, false, 50},
  {31, 1920, 1080, false, 50},
  {32, 1920, 1080, false, 24},
  {33, 1920, 1080, false, 25},
  {34, 1920, 1080, false, 30},
  {39, 1920, 1080, true, 50},
  {41, 1280, 720, false, 100},
  {42, 720, 576, false, 100},
  {43, 720, 576, false, 100},
  {44, 720, 576, true, 100},
  {45, 720, 576, true, 100},
  {47, 1280, 720, false, 120},
  {48, 720, 480, false, 120},
  {49, 720, 480, false, 120},
  {-1, 0, 0, false, 0}
};

CEGLNativeTypeAndroid::CEGLNativeTypeAndroid()
{
}

CEGLNativeTypeAndroid::~CEGLNativeTypeAndroid()
{
} 

bool CEGLNativeTypeAndroid::SysModeToResolution(std::string mode, RESOLUTION_INFO *res) const
{
  if (!res)
    return false;

  res->iWidth = 0;
  res->iHeight= 0;

  if(mode.empty())
    return false;

  std::string fromMode = mode;
  if (!isdigit(mode[0]))
    fromMode = StringUtils::Mid(mode, 2);
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

bool CEGLNativeTypeAndroid::CheckCompatibility()
{
#if defined(TARGET_ANDROID)
  return true;
#endif
  return false;
}

void CEGLNativeTypeAndroid::Initialize()
{
#if defined(TARGET_ANDROID) && (defined(HAS_AMLPLAYER) || defined(HAS_LIBAMCODEC))
  aml_permissions();
  aml_cpufreq_min(true);
  aml_cpufreq_max(true);
#endif

  return;
}
void CEGLNativeTypeAndroid::Destroy()
{
#if defined(TARGET_ANDROID) && (defined(HAS_AMLPLAYER) || defined(HAS_LIBAMCODEC))
  aml_cpufreq_min(false);
  aml_cpufreq_max(false);
#endif

  return;
}

bool CEGLNativeTypeAndroid::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeAndroid::CreateNativeWindow()
{
#if defined(TARGET_ANDROID)
  // Android hands us a window, we don't have to create it
  return true;
#else
  return false;
#endif
}  

bool CEGLNativeTypeAndroid::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeAndroid::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
#if defined(TARGET_ANDROID)
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) CXBMCApp::GetNativeWindow(30000);
  return (*nativeWindow != NULL);
#else
  return false;
#endif
}

bool CEGLNativeTypeAndroid::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeAndroid::DestroyNativeWindow()
{
  return true;
}

bool CEGLNativeTypeAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(TARGET_ANDROID)
  EGLNativeWindowType *nativeWindow = (EGLNativeWindowType*)CXBMCApp::GetNativeWindow(30000);
  if (!nativeWindow)
    return false;

  bool gotfromsystem = false;

  char valstr[256] = {0};
  int ret = aml_get_sysfs_str("/sys/class/amhdmitx/amhdmitx0/disp_mode", valstr, 255);  // amlogic
  if (ret >= 0 && StringUtils::StartsWith(valstr, "VIC:"))
  {
    int vicid = atoi(&valstr[4]);
    for (int i=0; vic_table[i].id != -1; ++i)
    {
      if (vic_table[i].id == vicid)
      {
        res->iWidth = vic_table[i].width;
        res->iHeight = vic_table[i].height;
        res->fRefreshRate = vic_table[i].rfrate;
        res->dwFlags = (vic_table[i].interlaced ? D3DPRESENTFLAG_INTERLACED : D3DPRESENTFLAG_PROGRESSIVE);
        gotfromsystem = true;
        break;
      }
    }
  }

  if (!gotfromsystem)
  {
    ret = aml_get_sysfs_str("/sys/class/display/display0.HDMI/mode", valstr, 255);  // Rockchip
    if (ret >= 0)
    {
      RESOLUTION_INFO sysres;
      if (SysModeToResolution(valstr, &sysres))
      {
        res->iWidth = sysres.iWidth;
        res->iHeight = sysres.iHeight;
        res->fRefreshRate = sysres.fRefreshRate;
        res->dwFlags = sysres.dwFlags;
        gotfromsystem = true;
      }
    }
  }

  if (!gotfromsystem)
  {
    ANativeWindow_acquire(*nativeWindow);
    res->iWidth = ANativeWindow_getWidth(*nativeWindow);
    res->iHeight= ANativeWindow_getHeight(*nativeWindow);
    ANativeWindow_release(*nativeWindow);

    res->fRefreshRate = 60;
    res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  }
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode       = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
  res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  CLog::Log(LOGNOTICE,"Current resolution: %s\n",res->strMode.c_str());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
  return false;
}

bool CEGLNativeTypeAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  RESOLUTION_INFO res;
  bool ret = false;
  ret = GetNativeResolution(&res);
  if (ret && res.iWidth > 1 && res.iHeight > 1)
  {
    resolutions.push_back(res);
    return true;
  }
  return false;
}

bool CEGLNativeTypeAndroid::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeAndroid::ShowWindow(bool show)
{
  return false;
}
