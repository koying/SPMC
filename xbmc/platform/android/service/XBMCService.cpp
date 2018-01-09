/*
 *      Copyright (C) 2018 Christian Browet
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

#include "XBMCService.h"

CCriticalSection CXBMCService::m_SvcMutex;
bool CXBMCService::m_SvcThreadCreated = false;
pthread_t CXBMCService::m_SvcThread;

CXBMCService::CXBMCService()
{

}

void CXBMCService::LaunchApplication()
{
  /*
  CSingleLock lock(m_SvcMutex);

  if( !m_SvcThreadCreated)
  {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&m_SvcThread, &attr, thread_run<CXBMCApp, &CXBMCApp::run>, this);
    pthread_attr_destroy(&attr);

    m_SvcThreadCreated = true;
  }
  */
}

void CXBMCService::_launchApplication(JNIEnv*, jobject)
{
  /*
  m_xbmcserviceinstance = new CXBMCService();
  m_xbmcserviceinstance->m_jniservice = jhobject::fromJNI(thiz);
  m_xbmcserviceinstance->LaunchApplication();
  */
}
