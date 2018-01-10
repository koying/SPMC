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

#include "JNIXBMCBroadcastReceiver.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Intent.h>

#include "CompileInfo.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCBroadcastReceiver";

CJNIXBMCBroadcastReceiver::CJNIXBMCBroadcastReceiver()
  : CJNIBase()
{
  add_instance(m_object, this);
}

CJNIXBMCBroadcastReceiver::CJNIXBMCBroadcastReceiver(const CJNIXBMCBroadcastReceiver& other)
  : CJNIBase(other)
{
  add_instance(m_object, this);
}

CJNIXBMCBroadcastReceiver::~CJNIXBMCBroadcastReceiver()
{
  remove_instance(this);
}

void CJNIXBMCBroadcastReceiver::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onReceive", "(Landroid/content/Intent;)V", (void*)&CJNIXBMCBroadcastReceiver::_onReceive},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCBroadcastReceiver::_onReceive(JNIEnv *env, jobject thiz, jobject intent)
{
  (void)env;

  CJNIXBMCBroadcastReceiver *inst = find_instance(thiz);
  if (inst)
    inst->onReceive(CJNIIntent(jhobject::fromJNI(intent)));
}
