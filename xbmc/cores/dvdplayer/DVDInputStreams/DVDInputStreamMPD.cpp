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

#include "utils/log.h"

/*******************************************************
 Implementation
********************************************************/


CDVDInputStreamMPD::CDVDInputStreamMPD()
  : CDVDInputStream(DVDSTREAM_TYPE_MPD)
  , m_AP4session(nullptr)
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamMPD::%s", __FUNCTION__);
}

CDVDInputStreamMPD::~CDVDInputStreamMPD()
{
  Close();
}

bool CDVDInputStreamMPD::Open(const char* strFile, const std::string& content, bool contentLookup)
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamMPD::%s", __FUNCTION__);

  RESOLUTION_INFO& res_info = CDisplaySettings::GetInstance().GetResolutionInfo(g_graphicsContext.GetVideoResolution());
  m_AP4session.reset(new CDASHSession(strFile, res_info.iWidth, res_info.iHeight, "", "", "special://profile/"));

  if (!m_AP4session->initialize())
  {
    m_AP4session = nullptr;
    return false;
  }
  return true;
}

void CDVDInputStreamMPD::Close()
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamMPD::%s", __FUNCTION__);
}

int CDVDInputStreamMPD::GetTotalTime()
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamMPD::%s", __FUNCTION__);
  if (!m_AP4session)
    return 0;

  return static_cast<int>(m_AP4session->GetTotalTime()*1000);
}

int CDVDInputStreamMPD::GetTime()
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamMPD::%s", __FUNCTION__);
  if (!m_AP4session)
    return 0;

  return static_cast<int>(m_AP4session->GetPTS() * 1000);
}

const std::shared_ptr<CDASHSession> CDVDInputStreamMPD::GetDashSession() const
{
  return m_AP4session;
}

