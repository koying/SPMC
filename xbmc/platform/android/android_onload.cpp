/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "CompileInfo.h"

#include "platform/android/activity/JNIMainActivity.h"
#include "platform/android/activity/JNIXBMCMainView.h"
#include "platform/android/activity/JNIXBMCVideoView.h"
#include "platform/android/activity/JNIXBMCAudioManagerOnAudioFocusChangeListener.h"
#include "platform/android/activity/JNIXBMCSurfaceTextureOnFrameAvailableListener.h"
#include "platform/android/activity/JNIXBMCNsdManagerDiscoveryListener.h"
#include "platform/android/activity/JNIXBMCMediaSession.h"
#include "platform/android/activity/JNIXBMCNsdManagerRegistrationListener.h"
#include "platform/android/activity/JNIXBMCNsdManagerResolveListener.h"
#include "platform/android/activity/JNIXBMCJsonHandler.h"
#include "platform/android/activity/JNIXBMCFile.h"
#include "platform/android/activity/JNIXBMCInputDeviceListener.h"
#include "utils/StringUtils.h"
#include "activity/XBMCApp.h"
#include "service/XBMCService.h"

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
  jint version = JNI_VERSION_1_6;
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), version) != JNI_OK)
    return -1;

  std::string pkgRoot = CCompileInfo::GetClass();
  
  std::string keepAliveService = pkgRoot + "/KeepAliveService";
  std::string mainClass = pkgRoot + "/Main";
  std::string bcReceiver = pkgRoot + "/XBMCBroadcastReceiver";
  std::string settingsObserver = pkgRoot + "/XBMCSettingsContentObserver";

  CJNIXBMCAudioManagerOnAudioFocusChangeListener::RegisterNatives(env);
  CJNIXBMCSurfaceTextureOnFrameAvailableListener::RegisterNatives(env);
  CJNIXBMCMainView::RegisterNatives(env);
  CJNIXBMCVideoView::RegisterNatives(env);
  jni::CJNIXBMCNsdManagerDiscoveryListener::RegisterNatives(env);
  jni::CJNIXBMCNsdManagerRegistrationListener::RegisterNatives(env);
  jni::CJNIXBMCNsdManagerResolveListener::RegisterNatives(env);
  jni::CJNIXBMCMediaSession::RegisterNatives(env);
  jni::CJNIXBMCJsonHandler::RegisterNatives(env);
  jni::CJNIXBMCFile::RegisterNatives(env);
  jni::CJNIXBMCInputDeviceListener::RegisterNatives(env);

  jclass cKASvc = env->FindClass(keepAliveService.c_str());
  if(cKASvc)
  {
    JNINativeMethod methods[] =
    {
      {"_launchApplication", "()V", (void*)&CXBMCService::_launchApplication},
    };
    env->RegisterNatives(cKASvc, methods, sizeof(methods)/sizeof(methods[0]));
  }

  jclass cMain = env->FindClass(mainClass.c_str());
  if(cMain)
  {
    JNINativeMethod methods[] = 
    {
      {"_onNewIntent", "(Landroid/content/Intent;)V", (void*)&CJNIMainActivity::_onNewIntent},
      {"_onActivityResult", "(IILandroid/content/Intent;)V", (void*)&CJNIMainActivity::_onActivityResult},
      {"_doFrame", "(J)V", (void*)&CJNIMainActivity::_doFrame},
      {"_callNative", "(JJ)V", (void*)&CJNIMainActivity::_callNative},
      {"_onCaptureAvailable", "(Landroid/media/Image;)V", (void*)&CJNIMainActivity::_onCaptureAvailable},
      {"_onScreenshotAvailable", "(Landroid/media/Image;)V", (void*)&CJNIMainActivity::_onScreenshotAvailable},
      {"_onVisibleBehindCanceled", "()V", (void*)&CJNIMainActivity::_onVisibleBehindCanceled},
      {"_onMultiWindowModeChanged", "(Z)V", (void*)&CJNIMainActivity::_onMultiWindowModeChanged},
      {"_onPictureInPictureModeChanged", "(Z)V", (void*)&CJNIMainActivity::_onPictureInPictureModeChanged},
      {"_onAudioDeviceAdded", "([Landroid/media/AudioDeviceInfo;)V", (void*)&CJNIMainActivity::_onAudioDeviceAdded},
      {"_onAudioDeviceRemoved", "([Landroid/media/AudioDeviceInfo;)V", (void*)&CJNIMainActivity::_onAudioDeviceRemoved},
    };
    env->RegisterNatives(cMain, methods, sizeof(methods)/sizeof(methods[0]));
  }

  jclass cBroadcastReceiver = env->FindClass(bcReceiver.c_str());
  if(cBroadcastReceiver)
  {
    JNINativeMethod methods[] = 
    {
      {"_onReceive", "(Landroid/content/Intent;)V", (void*)&CJNIBroadcastReceiver::_onReceive},
    };
    env->RegisterNatives(cBroadcastReceiver, methods, sizeof(methods)/sizeof(methods[0]));
  }

  jclass cSettingsObserver = env->FindClass(settingsObserver.c_str());
  if(cSettingsObserver)
  {
    JNINativeMethod methods[] = 
    {
      {"_onVolumeChanged", "(I)V", (void*)&CJNIMainActivity::_onVolumeChanged},
    };
    env->RegisterNatives(cSettingsObserver, methods, sizeof(methods)/sizeof(methods[0]));
  }

  return version;
}
