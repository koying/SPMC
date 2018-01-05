#pragma once
/*
 *      Copyright (C) 2018 Christian Browet
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

#include <androidjni/JNIBase.h>

namespace jni
{

class CJNIXBMCFileContent : public CJNIBase
{
public:
  CJNIXBMCFileContent();
  ~CJNIXBMCFileContent();

  static void RegisterNatives(JNIEnv* env);

protected:
  static void _createNativeInstance(JNIEnv* env, jobject thiz);
  static void _releaseNativeInstance(JNIEnv* env, jobject thiz);
  static jobject _getDirectoryContent(JNIEnv* env, jobject thiz, jstring url);

};

}
