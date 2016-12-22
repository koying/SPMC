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

#include "JNIXBMCVideoView.h"
#include "android/jni/jutils/jutils-details.hpp"

#include "android/jni/ClassLoader.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>

using namespace jni;

CJNIXBMCVideoView::CJNIXBMCVideoView()
  : m_callback(nullptr)
  , m_surfaceCreated(nullptr)
{
}

CJNIXBMCVideoView::~CJNIXBMCVideoView()
{
  release();
  delete m_surfaceCreated;
}

CJNIXBMCVideoView* CJNIXBMCVideoView::createVideoView(CJNISurfaceHolderCallback* callback)
{
  std::string slashClassName = CJNIContext::getPackageName() + "/XBMCVideoView";
  std::replace(slashClassName.begin(), slashClassName.end(), '.', '/');
  std::string signature = "()L" + slashClassName + ";";
  
  JNIEnv *jenv = xbmc_jnienv();

  jhclass jcView = find_class(slashClassName.c_str());
  if (!jcView) 
  {
    CLog::Log(LOGERROR, "%s: Error getting class XBMCVideoView", __PRETTY_FUNCTION__);
    return nullptr;
  }

  jhobject o = call_static_method<jhobject>(jenv, jcView,
    "createVideoView", signature.c_str());
  
  CJNIXBMCVideoView*pvw = new CJNIXBMCVideoView(o);
  pvw->m_callback = callback;
  pvw->m_surfaceCreated = new CEvent;
  pvw->add();
  
  jenv->DeleteLocalRef(jcView);

  return pvw;
}

void CJNIXBMCVideoView::_OnSurfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;
  
  CJNIXBMCVideoView *inst = new CJNIXBMCVideoView(jhobject(thiz));
  inst->OnSurfaceChanged(CJNISurfaceHolder(jhobject(holder)), format, width, height);
  delete inst;
}

void CJNIXBMCVideoView::_OnSurfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = new CJNIXBMCVideoView(jhobject(thiz));
  inst->OnSurfaceCreated(CJNISurfaceHolder(jhobject(holder)));
  delete inst;
}

void CJNIXBMCVideoView::_OnSurfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;
  
  CJNIXBMCVideoView *inst = new CJNIXBMCVideoView(jhobject(thiz));
  inst->OnSurfaceDestroyed(CJNISurfaceHolder(jhobject(holder)));
  delete inst;
}

void CJNIXBMCVideoView::OnSurfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  if (m_callback)
    m_callback->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCVideoView::OnSurfaceCreated(CJNISurfaceHolder holder)
{
  m_surfaceCreated->Set();
  if (m_callback)
    m_callback->surfaceCreated(holder);
}

void CJNIXBMCVideoView::OnSurfaceDestroyed(CJNISurfaceHolder holder)
{
  m_surfaceCreated->Reset();
  if (m_callback)
    m_callback->surfaceDestroyed(holder);
}

bool CJNIXBMCVideoView::waitForSurface(unsigned int millis)
{
  return m_surfaceCreated->WaitMSec(millis);
}

void CJNIXBMCVideoView::add()
{
  call_method<void>(m_object,
                    "add", "()V");
}

void CJNIXBMCVideoView::release()
{
  call_method<void>(m_object,
                    "release", "()V");
}

void CJNIXBMCVideoView::clearSurface()
{
  call_method<void>(m_object,
                    "clearSurface", "()V");
}

CJNISurface CJNIXBMCVideoView::getSurface()
{
  return call_method<jhobject>(m_object,
                               "getSurface", "()Landroid/view/Surface;");
}

CJNIRect CJNIXBMCVideoView::getSurfaceRect()
{
  return call_method<jhobject>(m_object,
                               "getSurfaceRect", "()Landroid/graphics/Rect;");
}

void CJNIXBMCVideoView::setSurfaceRect(int l, int t, int r, int b)
{
  call_method<void>(m_object,
                    "setSurfaceRect", "(IIII)V", l, t, r, b);
}


