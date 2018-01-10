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

#include <algorithm>

#include "JNIXBMCInputDeviceListener.h"
#include <androidjni/jutils-details.hpp>

#include "CompileInfo.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCInputDeviceListener";

CJNIXBMCInputDeviceListener::CJNIXBMCInputDeviceListener()
  : CJNIBase()
{
  add_instance(m_object, this);
}

CJNIXBMCInputDeviceListener::CJNIXBMCInputDeviceListener(const CJNIXBMCInputDeviceListener& other)
  : CJNIBase(other)
{
  add_instance(m_object, this);
}

CJNIXBMCInputDeviceListener::~CJNIXBMCInputDeviceListener()
{
  remove_instance(this);
}

void CJNIXBMCInputDeviceListener::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      { "_onInputDeviceAdded", "(I)V", (void*)&CJNIXBMCInputDeviceListener::_onInputDeviceAdded },
      { "_onInputDeviceChanged", "(I)V", (void*)&CJNIXBMCInputDeviceListener::_onInputDeviceChanged },
      { "_onInputDeviceRemoved", "(I)V", (void*)&CJNIXBMCInputDeviceListener::_onInputDeviceRemoved }
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCInputDeviceListener::_onInputDeviceAdded(JNIEnv *env, jobject thiz, jint deviceId)
{
  (void)env;

  CJNIXBMCInputDeviceListener *inst = find_instance(thiz);
  if (inst)
    inst->onInputDeviceAdded(deviceId);
}

void CJNIXBMCInputDeviceListener::_onInputDeviceChanged(JNIEnv *env, jobject thiz, jint deviceId)
{
  (void)env;

  CJNIXBMCInputDeviceListener *inst = find_instance(thiz);
  if (inst)
    inst->onInputDeviceChanged(deviceId);
}

void CJNIXBMCInputDeviceListener::_onInputDeviceRemoved(JNIEnv *env, jobject thiz, jint deviceId)
{
  (void)env;

  CJNIXBMCInputDeviceListener *inst = find_instance(thiz);
  if (inst)
    inst->onInputDeviceRemoved(deviceId);
}
