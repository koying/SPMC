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

#include <stdlib.h>

#include <android/log.h>

#include <androidjni/jutils.hpp>
#include <androidjni/File.h>
#include <androidjni/System.h>

#include "CompileInfo.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "platform/XbmcContext.h"
#include "platform/xbmc.h"

CCriticalSection CXBMCService::m_SvcMutex;
bool CXBMCService::m_SvcThreadCreated = false;
pthread_t CXBMCService::m_SvcThread;
CXBMCService* CXBMCService::m_xbmcserviceinstance = nullptr;

template<class T, void(T::*fn)()>
void* thread_run(void* obj)
{
  (static_cast<T*>(obj)->*fn)();
  return NULL;
}

using namespace jni;

CXBMCService::CXBMCService()
{
  m_xbmcserviceinstance = this;
}

int CXBMCService::android_printf(const char *format, ...)
{
  // For use before CLog is setup by XBMC_Run()
  va_list args;
  va_start(args, format);
  int result = __android_log_vprint(ANDROID_LOG_DEBUG, CCompileInfo::GetAppName(), format, args);
  va_end(args);
  return result;
}

void CXBMCService::SetupEnv()
{
  setenv("XBMC_ANDROID_SYSTEM_LIBS", CJNISystem::getProperty("java.library.path").c_str(), 0);
  setenv("XBMC_ANDROID_LIBS", m_jniservice.getApplicationInfo().nativeLibraryDir.c_str(), 0);
  setenv("XBMC_ANDROID_APK", m_jniservice.getPackageResourcePath().c_str(), 0);

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  std::string className = CCompileInfo::GetPackage();

  std::string xbmcHome = CJNISystem::getProperty("xbmc.home", "");
  if (xbmcHome.empty())
  {
    std::string cacheDir = m_jniservice.getCacheDir().getAbsolutePath();
    setenv("KODI_BIN_HOME", (cacheDir + "/apk/assets").c_str(), 0);
    setenv("KODI_HOME", (cacheDir + "/apk/assets").c_str(), 0);
  }
  else
  {
    setenv("KODI_BIN_HOME", (xbmcHome + "/assets").c_str(), 0);
    setenv("KODI_HOME", (xbmcHome + "/assets").c_str(), 0);
  }

  std::string externalDir = CJNISystem::getProperty("xbmc.data", "");
  if (externalDir.empty())
  {
    CJNIFile androidPath = m_jniservice.getExternalFilesDir("");
    if (!androidPath)
      androidPath = m_jniservice.getDir(className.c_str(), 1);

    if (androidPath)
      externalDir = androidPath.getAbsolutePath();
  }

  if (!externalDir.empty())
    setenv("HOME", externalDir.c_str(), 0);
  else
    setenv("HOME", getenv("KODI_TEMP"), 0);

  std::string apkPath = getenv("XBMC_ANDROID_APK");
  apkPath += "/assets/python2.7";
  setenv("PYTHONHOME", apkPath.c_str(), 1);
  setenv("PYTHONPATH", "", 1);
  setenv("PYTHONOPTIMIZE","", 1);
  setenv("PYTHONNOUSERSITE", "1", 1);
}

void CXBMCService::run()
{
  int status = 0;

  SetupEnv();
  XBMC::Context context;

  android_printf(" => running XBMC_Run...");
  try
  {
    CFileItemList dummyPL;
    status = XBMC_Run(false, dummyPL);
    android_printf(" => XBMC_Run finished with %d", status);
  }
  catch(...)
  {
    android_printf("ERROR: Exception caught on main loop. Exiting");
  }
}

void CXBMCService::LaunchApplication()
{
  CSingleLock lock(m_SvcMutex);

  if( !m_SvcThreadCreated)
  {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&m_SvcThread, &attr, thread_run<CXBMCService, &CXBMCService::run>, this);
    pthread_attr_destroy(&attr);

    m_SvcThreadCreated = true;
  }
}

void CXBMCService::_launchApplication(JNIEnv*, jobject thiz)
{
  CXBMCService::get()->m_jniservice = jhobject::fromJNI(thiz);
  CXBMCService::get()->LaunchApplication();
}
