/*
 *      Copyright (C) 2016 Christian Browet
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

#include "JNIXBMCSurfaceHolderCallback.h"
#include "android/jni/jutils/jutils-details.hpp"

#include "android/jni/ClassLoader.h"
#include "android/jni/SurfaceHolder.h"

#include "utils/StringUtils.h"

#include <algorithm>

using namespace jni;


CJNIXBMCSurfaceHolderCallback::CJNIXBMCSurfaceHolderCallback(CJNISurfaceHolderCallback* callback)
  : CJNIBase(CJNIContext::getPackageName() + ".XBMCSurfaceHolderCallback")
  , m_callback(callback)
{
  // Convert "the/class/name" to "the.class.name" as loadClass() expects it.
  std::string dotClassName = GetClassName();
  std::replace(dotClassName.begin(), dotClassName.end(), '/', '.');
  m_object = new_object(CJNIContext::getClassLoader().loadClass(dotClassName));
  m_object.setGlobal();
}

void CJNIXBMCSurfaceHolderCallback::_OnSurfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;
  
  CJNIXBMCSurfaceHolderCallback inst = CJNIXBMCSurfaceHolderCallback(jhobject(thiz));
  inst.OnSurfaceChanged(CJNISurfaceHolder(jhobject(holder)), format, width, height);
}

void CJNIXBMCSurfaceHolderCallback::_OnSurfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCSurfaceHolderCallback inst = CJNIXBMCSurfaceHolderCallback(jhobject(thiz));
  inst.OnSurfaceCreated(CJNISurfaceHolder(jhobject(holder)));
}

void CJNIXBMCSurfaceHolderCallback::_OnSurfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;
  
  CJNIXBMCSurfaceHolderCallback inst = CJNIXBMCSurfaceHolderCallback(jhobject(thiz));
  inst.OnSurfaceDestroyed(CJNISurfaceHolder(jhobject(holder)));
}

void CJNIXBMCSurfaceHolderCallback::OnSurfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  if (m_callback)
    m_callback->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCSurfaceHolderCallback::OnSurfaceCreated(CJNISurfaceHolder holder)
{
  if (m_callback)
    m_callback->surfaceCreated(holder);
}

void CJNIXBMCSurfaceHolderCallback::OnSurfaceDestroyed(CJNISurfaceHolder holder)
{
  if (m_callback)
    m_callback->surfaceDestroyed(holder);
}
