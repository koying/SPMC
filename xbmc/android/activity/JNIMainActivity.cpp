/*
 *      Copyright (C) 2015 Team XBMC
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

#include "JNIMainActivity.h"

#include "android/jni/jutils/jutils-details.hpp"

using namespace jni;

CJNIMainActivity* CJNIMainActivity::m_appInstance(NULL);

CJNIMainActivity::CJNIMainActivity(const ANativeActivity *nativeActivity)
  : CJNIActivity(nativeActivity)
{
  m_appInstance = this;
}

CJNIMainActivity::~CJNIMainActivity()
{
  m_appInstance = NULL;
}

void CJNIMainActivity::_onNewIntent(JNIEnv *env, jobject context, jobject intent)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onNewIntent(CJNIIntent(jhobject(intent)));
}

void CJNIMainActivity::_onActivityResult(JNIEnv *env, jobject context, jint requestCode, jint resultCode, jobject resultData)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onActivityResult(requestCode, resultCode, CJNIIntent(jhobject(resultData)));
}

void CJNIMainActivity::_callNative(JNIEnv *env, jobject context, jlong funcAddr, jlong variantAddr)
{
  (void)env;
  (void)context;
  ((void (*)(CVariant *))funcAddr)((CVariant *)variantAddr);
}

void CJNIMainActivity::_onAudioDeviceAdded(JNIEnv *env, jobject context, jobjectArray devices)
{
  (void)env;
  (void)context;
  if (m_appInstance)
  {
    m_appInstance->onAudioDeviceAdded(jcast<CJNIAudioDeviceInfos>(jhobjectArray(devices)));
  }
}

void CJNIMainActivity::_onAudioDeviceRemoved(JNIEnv *env, jobject context, jobjectArray devices)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onAudioDeviceRemoved(jcast<CJNIAudioDeviceInfos>(jhobjectArray(devices)));
}

void CJNIMainActivity::_onCaptureAvailable(JNIEnv *env, jobject context, jobject image)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onCaptureAvailable(CJNIImage(jhobject(image)));
}

void CJNIMainActivity::_onScreenshotAvailable(JNIEnv* env, jobject context, jobject image)
{
  (void)env;
  (void)context;
  if (m_appInstance)
    m_appInstance->onScreenshotAvailable(CJNIImage(jhobject(image)));
}

void CJNIMainActivity::runNativeOnUiThread(void (*callback)(CVariant *), CVariant* variant)
{
  call_method<void>(m_context,
                    "runNativeOnUiThread", "(JJ)V", (jlong)callback, (jlong)variant);
}

void CJNIMainActivity::startCrashHandler()
{
  call_method<void>(m_context,
                    "startCrashHandler", "()V");
}

void CJNIMainActivity::uploadLog()
{
  call_method<void>(m_context,
                    "uploadLog", "()V");
}

void CJNIMainActivity::_onVolumeChanged(JNIEnv *env, jobject context, jint volume)
{
  (void)env;
  (void)context;
  if(m_appInstance)
    m_appInstance->onVolumeChanged(volume);
}

void CJNIMainActivity::_onAudioFocusChange(JNIEnv *env, jobject context, jint focusChange)
{
  (void)env;
  (void)context;
  if(m_appInstance)
    m_appInstance->onAudioFocusChange(focusChange);
}

void CJNIMainActivity::_doFrame(JNIEnv *env, jobject context, jlong frameTimeNanos)
{
  (void)env;
  (void)context;
  if(m_appInstance)
    m_appInstance->doFrame(frameTimeNanos);
}

CJNIRect CJNIMainActivity::getDisplayRect()
{
  return call_method<jhobject>(m_context,
                               "getDisplayRect", "()Landroid/graphics/Rect;");
}

void CJNIMainActivity::registerMediaButtonEventReceiver()
{
  call_method<void>(m_context,
                    "registerMediaButtonEventReceiver", "()V");
}

void CJNIMainActivity::unregisterMediaButtonEventReceiver()
{
  call_method<void>(m_context,
                    "unregisterMediaButtonEventReceiver", "()V");
}

void CJNIMainActivity::screenOn()
{
  call_method<void>(m_context,
                    "screenOn", "()V");
}

void CJNIMainActivity::takeScreenshot()
{
  call_method<void>(m_context,
                    "takeScreenshot", "()V");
}

void CJNIMainActivity::startProjection()
{
  call_method<void>(m_context,
                    "startProjection", "()V");
}

void CJNIMainActivity::startCapture(int width, int height)
{
  call_method<void>(m_context,
                    "startCapture", "(II)V", width, height);
}

void CJNIMainActivity::stopCapture()
{
  call_method<void>(m_context,
                    "stopCapture", "()V");
}
