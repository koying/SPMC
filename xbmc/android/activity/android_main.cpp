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

#include <unwind.h>
#include <dlfcn.h>
#include <cxxabi.h>

#include "android/activity/JNIMainActivity.h"

static struct sigaction old_sa[NSIG];

struct android_backtrace_state
{
  void **current;
  void **end;
};

_Unwind_Reason_Code android_unwind_callback(struct _Unwind_Context* context, void* arg)
{
  android_backtrace_state* state = (android_backtrace_state *)arg;
  uintptr_t pc = _Unwind_GetIP(context);
  if (pc)
  {
    if (state->current == state->end)
    {
      return _URC_END_OF_STACK;
    }
    else
    {
      *state->current++ = reinterpret_cast<void*>(pc);
    }
  }
  return _URC_NO_REASON;
}

void dump_stack(void)
{
  CXBMCApp::android_printf("------------------");
  CXBMCApp::android_printf("android stack dump");

  const int max = 100;
  void* buffer[max];

  android_backtrace_state state;
  state.current = buffer;
  state.end = buffer + max;

  _Unwind_Backtrace(android_unwind_callback, &state);

  int count = (int)(state.current - buffer);

  for (int idx = 0; idx < count; idx++)
  {
    const void* addr = buffer[idx];
    const char* symbol = "";

    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname)
    {
      symbol = info.dli_sname;
    }
    int status = 0;
    char *demangled = __cxxabiv1::__cxa_demangle(symbol, 0, 0, &status);

    CXBMCApp::android_printf("%03d: 0x%p %s",
            idx,
            addr,
            (NULL != demangled && 0 == status) ?
              demangled : symbol);

    if (NULL != demangled)
      free(demangled);
  }

  CXBMCApp::android_printf("android stack dump done");
}

void android_sigaction(int signal, siginfo_t *info, void *reserved)
{
#if defined(__arm__)
  const ucontext_t* uc = reinterpret_cast<const ucontext_t*>(reserved);
  void **bp = reinterpret_cast<void**>(uc->uc_mcontext.arm_sp);
  void *ip = reinterpret_cast<void*>(uc->uc_mcontext.arm_ip);
#endif

  CXBMCApp::android_printf("Segmentation Fault!");
  CXBMCApp::android_printf("info.si_signo = %d", signal);
  CXBMCApp::android_printf("info.si_errno = %d", info->si_errno);
  CXBMCApp::android_printf("info.si_addr  = %p", info->si_addr);

  CXBMCApp::android_printf("------------");
  CXBMCApp::android_printf("Stack trace:");

  Dl_info dlinfo;
  int f=0;
  while(bp && ip) {
    if(!dladdr(ip, &dlinfo))
      break;

    const char *symname = dlinfo.dli_sname;

    int status;
    char * tmp = __cxxabiv1::__cxa_demangle(symname, NULL, 0, &status);

    if (status == 0 && tmp)
      symname = tmp;

    CXBMCApp::android_printf("Stack trace: % 2d: %p <%s+%lu> (%s)",
               ++f,
               ip,
               symname,
               (unsigned long)ip - (unsigned long)dlinfo.dli_saddr,
               dlinfo.dli_fname);

      if (tmp)
          free(tmp);

      if(dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
          break;

      ip = bp[1];
      bp = (void**)bp[0];
  }

  CJNIMainActivity::startCrashHandler();
  old_sa[signal].sa_handler(signal);
}

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

#if !defined(HAVE_BREAKPAD)
  // Try to catch crashes...
  struct sigaction handler;
  memset(&handler, 0, sizeof(struct sigaction));
  handler.sa_sigaction = android_sigaction;
  handler.sa_flags = SA_RESETHAND | SA_SIGINFO;
#define CATCHSIG(X) sigaction(X, &handler, &old_sa[X])
  CATCHSIG(SIGILL);
  CATCHSIG(SIGABRT);
  CATCHSIG(SIGBUS);
  CATCHSIG(SIGFPE);
  CATCHSIG(SIGSEGV);
  CATCHSIG(SIGSTKFLT);
  CATCHSIG(SIGPIPE);
#endif

  return version;
}
