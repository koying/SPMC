/*
 *      Copyright (C) 2016 Christian Browet
 *      Copyright (C) 2016-2016 peak3d
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

#include "DVDInputStreamMPD.h"

#include "guilib/GraphicContext.h"
#include "settings/DisplaySettings.h"

#ifdef TARGET_ANDROID
#include "utils/StringUtils.h"
#include "android/jni/SystemProperties.h"
#endif

#include "utils/log.h"

/*******************************************************
 Implementation
********************************************************/


CDVDInputStreamMPD::CDVDInputStreamMPD(CFileItem& fileitem)
  : CDVDInputStream(DVDSTREAM_TYPE_MPD, fileitem)
  , m_AP4session(nullptr)
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamMPD::%s", __FUNCTION__);
}

CDVDInputStreamMPD::~CDVDInputStreamMPD()
{
  CLog::Log(LOGDEBUG, "CDVDDemuxMPD::%s", __FUNCTION__);
  Close();
}

bool CDVDInputStreamMPD::Open()
{
  // Find larger possible resolution
  RESOLUTION_INFO res_info = CDisplaySettings::GetInstance().GetResolutionInfo(g_graphicsContext.GetVideoResolution());
  for (unsigned int i=0; i<CDisplaySettings::GetInstance().ResolutionInfoSize(); ++i)
  {
    RESOLUTION_INFO res = CDisplaySettings::GetInstance().GetResolutionInfo(i);
    if (res.iWidth > res_info.iWidth || res.iHeight > res_info.iHeight)
      res_info = res;
  }
#ifdef TARGET_ANDROID
  #include "android/jni/SystemProperties.h"

  // Android might go even higher via surface
  std::string displaySize = CJNISystemProperties::get("sys.display-size", "");
  if (!displaySize.empty())
  {
    std::vector<std::string> aSize = StringUtils::Split(displaySize, "x");
    if (aSize.size() == 2)
    {
      res_info.iWidth = StringUtils::IsInteger(aSize[0]) ? atoi(aSize[0].c_str()) : 0;
      res_info.iHeight = StringUtils::IsInteger(aSize[1]) ? atoi(aSize[1].c_str()) : 0;
    }
  }
#endif
  CLog::Log(LOGINFO, "CDVDInputStreamMPD - matching against %d x %d", res_info.iWidth, res_info.iHeight);
  m_AP4session.reset(new CDASHSession(m_item.GetPath(), res_info.iWidth, res_info.iHeight, "", "", "special://profile/"));

  if (!m_AP4session->initialize())
  {
    m_AP4session = nullptr;
    return false;
  }
  return true;
}

void CDVDInputStreamMPD::Close()
{
}

int CDVDInputStreamMPD::GetTotalTime()
{
  if (!m_AP4session)
    return 0;

  return static_cast<int>(m_AP4session->GetTotalTime()*1000);
}

int CDVDInputStreamMPD::GetTime()
{
  if (!m_AP4session)
    return 0;

  return static_cast<int>(m_AP4session->GetPTS() * 1000);
}

const std::shared_ptr<CDASHSession> CDVDInputStreamMPD::GetDashSession() const
{
  return m_AP4session;
}

