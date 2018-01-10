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

#include <androidjni/BroadcastReceiver.h>

namespace jni
{

class CJNIXBMCBroadcastReceiver : public CJNIBroadcastReceiver, public CJNIInterfaceImplem<CJNIXBMCBroadcastReceiver>
{
public:
  CJNIXBMCBroadcastReceiver(CJNIBroadcastReceiver* receiver);
  CJNIXBMCBroadcastReceiver(const CJNIXBMCBroadcastReceiver& other);
  CJNIXBMCBroadcastReceiver(const jni::jhobject &object) : CJNIBase(object) {}
  virtual ~CJNIXBMCBroadcastReceiver();

  static void RegisterNatives(JNIEnv* env);

public:
  void onReceive(CJNIIntent intent);

protected:
  CJNIBroadcastReceiver* m_receiver;
  static void _onReceive(JNIEnv* env, jobject thiz, jobject intent);
};

}
