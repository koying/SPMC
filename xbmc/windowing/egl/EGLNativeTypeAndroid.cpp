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
#include <stdlib.h>

#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeAndroid.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "guilib/gui3d.h"
#include "android/activity/XBMCApp.h"
#include "utils/StringUtils.h"
#include "android/jni/SystemProperties.h"
#include "android/jni/View.h"
#include "android/jni/Window.h"
#include "android/jni/WindowManager.h"
#include "android/jni/Build.h"
#include "android/jni/System.h"

#include "utils/SysfsUtils.h"

CEGLNativeTypeAndroid::CEGLNativeTypeAndroid()
  : m_width(0), m_height(0)
{
}

CEGLNativeTypeAndroid::~CEGLNativeTypeAndroid()
{
}

bool CEGLNativeTypeAndroid::CheckCompatibility()
{
  return true;
}

void CEGLNativeTypeAndroid::Initialize()
{
  std::string displaySize;
  m_width = m_height = 0;

  CJNIWindow window = CXBMCApp::getWindow();
  if (window)
  {
    CJNIView view(window.getDecorView());
    if (view)
      m_display = view.getDisplay();
  }

  if (m_display)
  {
    std::vector<CJNIDisplayMode> modes = m_display.getSupportedModes();
    for (auto m : modes)
    {
      CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: available mode: %dx%d@%f", m.getPhysicalWidth(), m.getPhysicalHeight(), m.getRefreshRate());
      if (m.getPhysicalWidth() > m_width || m.getPhysicalHeight() > m_height)
      {
        m_width = m.getPhysicalWidth();
        m_height = m.getPhysicalHeight();
      }
    }

    if (modes.size())
    {
      CJNIDisplayMode mode = m_display.getMode();
      CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: current mode: %dx%d@%f", mode.getPhysicalWidth(), mode.getPhysicalHeight(), mode.getRefreshRate());
    }
  }

  if (!m_width || !m_height)
  {
    // Property available on some devices
    displaySize = CJNISystemProperties::get("sys.display-size", "");
    if (!displaySize.empty())
    {
      std::vector<std::string> aSize = StringUtils::Split(displaySize, "x");
      if (aSize.size() == 2)
      {
        m_width = StringUtils::IsInteger(aSize[0]) ? atoi(aSize[0].c_str()) : 0;
        m_height = StringUtils::IsInteger(aSize[1]) ? atoi(aSize[1].c_str()) : 0;
      }
      CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: display-size: %s(%dx%d)", displaySize.c_str(), m_width, m_height);
    }
  }

  CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: maximum/current resolution: %dx%d", m_width, m_height);
  int limit = CSettings::GetInstance().GetInt("videoscreen.limitgui");
  switch (limit)
  {
    case 0: // auto
      m_width = 0;
      m_height = 0;
      break;

    case 9999:  // unlimited
      break;

    case 720:
      if (m_height > 720)
      {
        m_width = 1280;
        m_height = 720;
      }
      break;

    case 1080:
      if (m_height > 1080)
      {
        m_width = 1920;
        m_height = 1080;
      }
      break;
  }
  CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: selected resolution: %dx%d", m_width, m_height);
}
void CEGLNativeTypeAndroid::Destroy()
{
  return;
}

bool CEGLNativeTypeAndroid::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeAndroid::CreateNativeWindow()
{
  // Android hands us a window, we don't have to create it
  return true;
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
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) CXBMCApp::GetNativeWindow(2000);
  return (*nativeWindow != NULL && **nativeWindow != NULL);
}

bool CEGLNativeTypeAndroid::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeAndroid::DestroyNativeWindow()
{
  return true;
}

static float overrideRefreshRate()
{
  static float overridedRR = -1;

  if (overridedRR == -1)  // Do it once
  {
    int CEAmode;
    overridedRR = 0.0;

    // AFTV stick
    // Always return 60, whatever the actual HDMI mode
    std::string sCEAmode = CJNISystemProperties::get("hw.brcm.tv.hdmi.mode", "");
    if (!sCEAmode.empty() && StringUtils::IsInteger(sCEAmode))
    {
      CEAmode = atoi(sCEAmode.c_str());
      RESOLUTION_INFO res = SysfsUtils::CEAtoRES(CEAmode);
      CLog::Log(LOGINFO, "CEGLNativeTypeAndroid: AFTVS refresh rate: %f", res.fRefreshRate);
      overridedRR = res.fRefreshRate;
    }

    // AFTV1
    // Always return 60, whatever the actual HDMI mode
    if (SysfsUtils::GetInt("/sys/class/graphics/fb0/video_mode", CEAmode) == 0)
    {
      RESOLUTION_INFO res = SysfsUtils::CEAtoRES(CEAmode);
      CLog::Log(LOGINFO, "CEGLNativeTypeAndroid: AFTV1 refresh rate: %f", res.fRefreshRate);
      overridedRR = res.fRefreshRate;
    }
  }
  return overridedRR;
}

static float currentRefreshRate()
{
  float overridedrate = overrideRefreshRate();
  if (overridedrate)
    return overridedrate;

  CJNIWindow window = CXBMCApp::getWindow();
  if (window)
  {
    float preferredRate = window.getAttributes().getpreferredRefreshRate();
    if (preferredRate > 20.0 && preferredRate < 70.0)
    {
      CLog::Log(LOGINFO, "CEGLNativeTypeAndroid: Preferred refresh rate: %f", preferredRate);
      return preferredRate;
    }
    CJNIView view(window.getDecorView());
    if (view) {
      CJNIDisplay display(view.getDisplay());
      if (display)
      {
        float reportedRate = display.getRefreshRate();
        if (reportedRate > 20.0 && reportedRate < 70.0)
        {
          CLog::Log(LOGINFO, "CEGLNativeTypeAndroid: Current display refresh rate: %f", reportedRate);
          return reportedRate;
        }
      }
    }
  }
  CLog::Log(LOGDEBUG, "found no refresh rate");
  return 60.0;
}

bool CEGLNativeTypeAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
  EGLNativeWindowType *nativeWindow = (EGLNativeWindowType*)CXBMCApp::GetNativeWindow(30000);
  if (!nativeWindow)
    return false;

  if (!*nativeWindow)
    return false;

  if (!m_width || !m_height)
  {
    ANativeWindow_acquire(*nativeWindow);
    res->iWidth = ANativeWindow_getWidth(*nativeWindow);
    res->iHeight= ANativeWindow_getHeight(*nativeWindow);
    ANativeWindow_release(*nativeWindow);
  }
  else
  {
    res->iWidth = m_width;
    res->iHeight = m_height;
  }

  res->fRefreshRate = currentRefreshRate();
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode       = StringUtils::Format("%dx%d @ %.6f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
                                           res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  CLog::Log(LOGNOTICE,"CEGLNativeTypeAndroid: Current resolution: %s\n",res->strMode.c_str());
  return true;
}

bool CEGLNativeTypeAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
  CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: SetNativeResolution: %dx%d@%f", m_width, m_height, res.fRefreshRate);

  if (m_width && m_height)
    CXBMCApp::SetBuffersGeometry(m_width, m_height, 0);

  if (abs(currentRefreshRate() - res.fRefreshRate) > 0.0001)
    CXBMCApp::SetRefreshRate(res.fRefreshRate);

  return true;
}

bool CEGLNativeTypeAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  RESOLUTION_INFO res;
  bool ret = GetNativeResolution(&res);

  if (ret && res.iWidth > 1 && res.iHeight > 1)
  {
    if (overrideRefreshRate() == 0.0)  // If override => assume not standard
    {
      std::vector<float> refreshRates;
      CJNIWindow window = CXBMCApp::getWindow();
      if (window)
      {
        CJNIView view = window.getDecorView();
        if (view)
        {
          CJNIDisplay display = view.getDisplay();
          if (display)
          {
            refreshRates = display.getSupportedRefreshRates();
          }
        }
      }

      if (!refreshRates.empty())
      {
        for (unsigned int i = 0; i < refreshRates.size(); i++)
        {
          if (refreshRates[i] < 20.0 || refreshRates[i] > 70.0)
            continue;
          res.fRefreshRate = refreshRates[i];
          res.strMode      = StringUtils::Format("%dx%d @ %.6f%s - Full Screen", res.iScreenWidth, res.iScreenHeight, res.fRefreshRate,
                                                 res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
          resolutions.push_back(res);
        }
      }
    }
    if (resolutions.empty())
    {
      /* No valid refresh rates available, just provide the current one */
      resolutions.push_back(res);
    }
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

bool CEGLNativeTypeAndroid::BringToFront()
{
  CXBMCApp::BringToFront();
}
