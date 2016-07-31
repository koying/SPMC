/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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

#if defined(TARGET_ANDROID)
#include "utils/log.h"
#include "VideoSyncAndroid.h"
#include "video/VideoReferenceClock.h"
#include "utils/TimeUtils.h"
#include "platform/android/activity/XBMCApp.h"
#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"
#include "utils/MathUtils.h"
#include "linux/XTimeUtils.h"


bool CVideoSyncAndroid::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s setting up", __FUNCTION__);
  UpdateClock = func;
  m_abort = false;
  g_Windowing.Register(this);

  return true;
}

void CVideoSyncAndroid::Run(std::atomic<bool>& stop)
{
  /* This shouldn't be very busy and timing is important so increase priority */
  CThread::GetCurrentThread()->SetPriority(CThread::GetCurrentThread()->GetPriority()+1);

  int64_t lastSync = 0;

  while (!stop && !m_abort)
  {
    if (!CXBMCApp::WaitVSync(1000))
    {
      CLog::Log(LOGERROR, "CVideoSyncAndroid: timeout waiting for sync");
      return;
    }
    uint64_t vsynctime = CXBMCApp::GetVsyncTime();

    //calculate how many vblanks happened
    int64_t FT = (vsynctime - lastSync);
    double VBlankTime = FT / (double)g_VideoReferenceClock.GetFrequency();
    double NrVBlanks = VBlankTime * m_fps;

//    CLog::Log(LOGDEBUG, "CVideoSyncAndroid heartbeat: %lld(%f fps), %f", FT, 1.0/((double)FT/1000000000), NrVBlanks);

    int iNrVBlanks = MathUtils::round_int(NrVBlanks);
    if (iNrVBlanks >= 1)
    {
      //save the timestamp of this vblank so we can calculate how many happened next time
      lastSync = vsynctime;

      UpdateClock(iNrVBlanks, vsynctime);
    }
  }
}

void CVideoSyncAndroid::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s cleaning up", __FUNCTION__);
  g_Windowing.Unregister(this);
}

float CVideoSyncAndroid::GetFps()
{
  m_fps = g_graphicsContext.GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncAndroid::%s Detected refreshrate: %f hertz", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncAndroid::OnResetDisplay()
{
  m_abort = true;
}

void CVideoSyncAndroid::FrameCallback(int64_t frameTimeNanos)
{
  int           NrVBlanks;
  double        VBlankTime;
  int64_t       nowtime = CurrentHostCounter();
  
  //calculate how many vblanks happened
  int64_t FT = (frameTimeNanos - m_LastVBlankTime);
  VBlankTime = FT / (double)g_VideoReferenceClock.GetFrequency();
  NrVBlanks = MathUtils::round_int(VBlankTime * m_fps);
  if (NrVBlanks > 1)
    CLog::Log(LOGDEBUG, "CVideoSyncAndroid::FrameCallback late: %lld(%f fps), %d", FT, 1.0/((double)FT/1000000000), NrVBlanks);

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = frameTimeNanos;
  
  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  UpdateClock(NrVBlanks, nowtime, m_refClock);
}

#endif //TARGET_ANDROID
