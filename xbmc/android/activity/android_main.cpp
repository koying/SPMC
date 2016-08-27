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

#include <stdlib.h>
#include <errno.h>
#include <android_native_app_glue.h>
#include "EventLoop.h"
#include "XBMCApp.h"
#include "android/jni/SurfaceTexture.h"
#include "utils/StringUtils.h"
#include "CompileInfo.h"

#if defined(HAVE_BREAKPAD)
#include "client/linux/handler/minidump_descriptor.h"
#include "client/linux/handler/exception_handler.h"
#endif

#include "android/activity/JNIMainActivity.h"

#if defined(HAVE_BREAKPAD)
static void *startCrashHandler(void* arg)
{
  CJNIMainActivity::startCrashHandler();
  return NULL;
}

static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                          void* context, bool succeeded)
{
  // Issue with breakpad to use JNI from callback. Have to use a thread
  // https://code.google.com/p/android/issues/detail?id=162663
  pthread_t t;
  pthread_create(&t, NULL, startCrashHandler, NULL);
  pthread_join(t, NULL);
  return succeeded;
}
#endif

// copied from new android_native_app_glue.c
static void process_input(struct android_app* app, struct android_poll_source* source) {
    AInputEvent* event = NULL;
    int processed = 0;
    while (AInputQueue_getEvent(app->inputQueue, &event) >= 0) {
        if (AInputQueue_preDispatchEvent(app->inputQueue, event)) {
            continue;
        }
        int32_t handled = 0;
        if (app->onInputEvent != NULL) handled = app->onInputEvent(app, event);
        AInputQueue_finishEvent(app->inputQueue, event, handled);
        processed = 1;
    }
    if (processed == 0) {
        CXBMCApp::android_printf("process_input: Failure reading next input event: %s", strerror(errno));
    }
}

extern void android_main(struct android_app* state)
{
  {
    // make sure that the linker doesn't strip out our glue
    app_dummy();

    // revector inputPollSource.process so we can shut up
    // its useless verbose logging on new events (see ouya)
    // and fix the error in handling multiple input events.
    // see https://code.google.com/p/android/issues/detail?id=41755
    state->inputPollSource.process = process_input;

    CEventLoop eventLoop(state);
    CXBMCApp xbmcApp(state->activity);
    if (xbmcApp.isValid())
    {
#if defined(HAVE_BREAKPAD)
      google_breakpad::MinidumpDescriptor descriptor(google_breakpad::MinidumpDescriptor::kMicrodumpOnConsole);
      google_breakpad::ExceptionHandler eh(descriptor,
                                        NULL,
                                        dumpCallback,
                                        NULL,
                                        true,
                                        -1);
#endif

      IInputHandler inputHandler;
      eventLoop.run(xbmcApp, inputHandler);
    }
    else
      CXBMCApp::android_printf("android_main: setup failed");

    CXBMCApp::android_printf("android_main: Exiting");
    // We need to call exit() so that all loaded libraries are properly unloaded
    // otherwise on the next start of the Activity android will simple re-use
    // those loaded libs in the state they were in when we quit XBMC last time
    // which will lead to crashes because of global/static classes that haven't
    // been properly uninitialized
  }
  exit(0);
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
  jint version = JNI_VERSION_1_6;
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), version) != JNI_OK)
    return -1;

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  std::string pkgRoot = CCompileInfo::GetPackage();
  StringUtils::ToLower(pkgRoot);
  StringUtils::Replace(pkgRoot, '.', '/');
  std::string mainClass = pkgRoot + "/Main";
  std::string bcReceiver = pkgRoot + "/XBMCBroadcastReceiver";
  std::string frameListener = pkgRoot + "/XBMCOnFrameAvailableListener";
  std::string settingsObserver = pkgRoot + "/XBMCSettingsContentObserver";
  std::string audioFocusChangeListener = pkgRoot + "/XBMCOnAudioFocusChangeListener";

  jclass cMain = env->FindClass(mainClass.c_str());
  if(cMain)
  {
    JNINativeMethod mOnNewIntent = {
      "_onNewIntent",
      "(Landroid/content/Intent;)V",
      (void*)&CJNIMainActivity::_onNewIntent
    };
    env->RegisterNatives(cMain, &mOnNewIntent, 1);

    JNINativeMethod mOnActivityResult = {
      "_onActivityResult",
      "(IILandroid/content/Intent;)V",
      (void*)&CJNIMainActivity::_onActivityResult
    };
    env->RegisterNatives(cMain, &mOnActivityResult, 1);

    JNINativeMethod mDoFrame = {
      "_doFrame",
      "(J)V",
      (void*)&CJNIMainActivity::_doFrame
    };
    env->RegisterNatives(cMain, &mDoFrame, 1);

    JNINativeMethod mCallNative = {
      "_callNative",
      "(JJ)V",
      (void*)&CJNIMainActivity::_callNative
    };
    env->RegisterNatives(cMain, &mCallNative, 1);

    JNINativeMethod mAudioDeviceAdded = {
      "_onAudioDeviceAdded",
      "([Landroid/media/AudioDeviceInfo;)V",
      (void*)&CJNIMainActivity::_onAudioDeviceAdded
    };
    env->RegisterNatives(cMain, &mAudioDeviceAdded, 1);

    JNINativeMethod mAudioDeviceRemoved = {
      "_onAudioDeviceRemoved",
      "([Landroid/media/AudioDeviceInfo;)V",
      (void*)&CJNIMainActivity::_onAudioDeviceRemoved
    };
    env->RegisterNatives(cMain, &mAudioDeviceRemoved, 1);
  }

  jclass cBroadcastReceiver = env->FindClass(bcReceiver.c_str());
  if(cBroadcastReceiver)
  {
    JNINativeMethod mOnReceive =  {
      "_onReceive",
      "(Landroid/content/Intent;)V",
      (void*)&CJNIBroadcastReceiver::_onReceive
    };
    env->RegisterNatives(cBroadcastReceiver, &mOnReceive, 1);
  }

  jclass cFrameAvailableListener = env->FindClass(frameListener.c_str());
  if(cFrameAvailableListener)
  {
    JNINativeMethod mOnFrameAvailable = {
      "_onFrameAvailable",
      "(Landroid/graphics/SurfaceTexture;)V",
      (void*)&CJNISurfaceTextureOnFrameAvailableListener::_onFrameAvailable
    };
    env->RegisterNatives(cFrameAvailableListener, &mOnFrameAvailable, 1);
  }

  jclass cSettingsObserver = env->FindClass(settingsObserver.c_str());
  if(cSettingsObserver)
  {
    JNINativeMethod mOnVolumeChanged = {
      "_onVolumeChanged",
      "(I)V",
      (void*)&CJNIMainActivity::_onVolumeChanged
    };
    env->RegisterNatives(cSettingsObserver, &mOnVolumeChanged, 1);
  }

  jclass cAudioFocusChangeListener = env->FindClass(audioFocusChangeListener.c_str());
  if(cAudioFocusChangeListener)
  {
    JNINativeMethod mOnAudioFocusChange = {
      "_onAudioFocusChange",
      "(I)V",
      (void*)&CJNIMainActivity::_onAudioFocusChange
    };
    env->RegisterNatives(cAudioFocusChangeListener, &mOnAudioFocusChange, 1);
  }

  return version;
}
