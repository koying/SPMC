#pragma once
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

#include <pthread.h>

#include <androidjni/Service.h>

#include "threads/Event.h"
#include "threads/SharedSection.h"

class CXBMCService
{
  friend class XBMCApp;

public:
  CXBMCService();

  static CXBMCService* get() { return m_xbmcserviceinstance; }
  static void _launchApplication(JNIEnv*, jobject thiz);
  int android_printf(const char* format...);

protected:
  void run();
  void SetupEnv();

  CEvent m_appReady;

private:
  static CCriticalSection m_SvcMutex;
  static bool m_SvcThreadCreated;
  static pthread_t m_SvcThread;
  static CXBMCService* m_xbmcserviceinstance;
  CJNIService m_jniservice;

  void LaunchApplication();
};
