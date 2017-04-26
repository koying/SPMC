#pragma once
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

#include <androidjni/Activity.h>
#include <androidjni/InputManager.h>
#include <androidjni/Rect.h>
#include <androidjni/Image.h>

class CJNIMainActivity : public CJNIActivity, public CJNIInputManagerInputDeviceListener
{
public:
  CJNIMainActivity(const ANativeActivity *nativeActivity);
  ~CJNIMainActivity();

  static CJNIMainActivity* GetAppInstance() { return m_appInstance; }

  static void _onNewIntent(JNIEnv *env, jobject context, jobject intent);
  static void _onActivityResult(JNIEnv *env, jobject context, jint requestCode, jint resultCode, jobject resultData);
  static void _onVolumeChanged(JNIEnv *env, jobject context, jint volume);
  static void _doFrame(JNIEnv *env, jobject context, jlong frameTimeNanos);
  static void _onInputDeviceAdded(JNIEnv *env, jobject context, jint deviceId);
  static void _onInputDeviceChanged(JNIEnv *env, jobject context, jint deviceId);
  static void _onInputDeviceRemoved(JNIEnv *env, jobject context, jint deviceId);
  static void _onCaptureAvailable(JNIEnv *env, jobject context, jobject image);
  static void _onScreenshotAvailable(JNIEnv *env, jobject context, jobject image);
  static void _onVisibleBehindCanceled(JNIEnv *env, jobject context);
  static void _onMultiWindowModeChanged(JNIEnv *env, jobject context, jboolean isInMultiWindowMode);
  static void _onPictureInPictureModeChanged(JNIEnv *env, jobject context, jboolean isInPictureInPictureMode);

  static void _callNative(JNIEnv *env, jobject context, jlong funcAddr, jlong variantAddr);
  static void runNativeOnUiThread(void (*callback)(CVariant *), CVariant *variant);
  static void registerMediaButtonEventReceiver();
  static void unregisterMediaButtonEventReceiver();
  static void startCrashHandler();
  static void uploadLog();

  CJNIRect getDisplayRect();
  
  static void takeScreenshot();
  static void startProjection();
  static void startCapture(int width, int height);
  static void stopCapture();

private:
  static CJNIMainActivity *m_appInstance;

protected:
  virtual void onNewIntent(CJNIIntent intent)=0;
  virtual void onActivityResult(int requestCode, int resultCode, CJNIIntent resultData)=0;
  virtual void onCaptureAvailable(jni::CJNIImage image)=0;
  virtual void onScreenshotAvailable(jni::CJNIImage image)=0;
  virtual void onVolumeChanged(int volume)=0;
  virtual void doFrame(int64_t frameTimeNanos)=0;
  virtual void onVisibleBehindCanceled() = 0;
  virtual void onMultiWindowModeChanged(bool isInMultiWindowMode) = 0;
  virtual void onPictureInPictureModeChanged(bool isInPictureInPictureMode) = 0;
};
