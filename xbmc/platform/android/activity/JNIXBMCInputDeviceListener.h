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

#include <androidjni/JNIBase.h>

#include <androidjni/InputManager.h>

namespace jni
{

class CJNIXBMCInputDeviceListener : public CJNIInputManagerInputDeviceListener, public CJNIInterfaceImplem<CJNIXBMCInputDeviceListener>
{
public:
  CJNIXBMCInputDeviceListener();
  CJNIXBMCInputDeviceListener(const CJNIXBMCInputDeviceListener& other);
  CJNIXBMCInputDeviceListener(const jni::jhobject &object) : CJNIBase(object) {}
  virtual ~CJNIXBMCInputDeviceListener();

  static void RegisterNatives(JNIEnv* env);

  // CJNINsdManagerDiscoveryListener interface
public:
  void onInputDeviceAdded(int deviceId) = 0;
  void onInputDeviceChanged(int deviceId) = 0;
  void onInputDeviceRemoved(int deviceId) = 0;

protected:
  static void _onInputDeviceAdded(JNIEnv *env, jobject thiz, jint deviceId);
  static void _onInputDeviceChanged(JNIEnv *env, jobject thiz, jint deviceId);
  static void _onInputDeviceRemoved(JNIEnv *env, jobject thiz, jint deviceId);};
}
