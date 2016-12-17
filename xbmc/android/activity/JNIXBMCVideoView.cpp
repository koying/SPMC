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

#include <map>
#include <algorithm>

using namespace jni;

jclass CJNIXBMCVideoView::m_jclass;
std::map<int, CJNIXBMCVideoView*> s_videoview_map;

CJNIXBMCVideoView::CJNIXBMCVideoView()
  : m_callback(nullptr)
  , m_surfaceCreated(nullptr)
{
}

CJNIXBMCVideoView::CJNIXBMCVideoView(const jni::jhobject &object)
  : CJNIBase(object)
  , m_callback(nullptr)
  , m_surfaceCreated(nullptr)
{
}

CJNIXBMCVideoView::~CJNIXBMCVideoView()
{
  delete m_surfaceCreated;
  for( auto it = s_videoview_map.begin(); it != s_videoview_map.end(); ++it )
  {
    if (it->second == this)
    {
      s_videoview_map.erase(it);
      break;
    }
  }
}

CJNIXBMCVideoView* CJNIXBMCVideoView::createVideoView(CJNISurfaceHolderCallback* callback)
{
  std::string slashClassName = CJNIContext::getPackageName() + "/XBMCVideoView";
  std::replace(slashClassName.begin(), slashClassName.end(), '.', '/');
  std::string signature = "()L" + slashClassName + ";";

  jhobject o = call_static_method<jhobject>(xbmc_jnienv(), jhclass(m_jclass),
    "createVideoView", signature.c_str());
  if (!o)
  {
    CLog::Log(LOGERROR, "Cannot instantiate VideoView!!");
    return nullptr;
  }

  CJNIXBMCVideoView*pvw = new CJNIXBMCVideoView(o);
  s_videoview_map.insert(std::pair<int, CJNIXBMCVideoView*>(pvw->ID(), pvw));
  pvw->m_callback = callback;
  pvw->m_surfaceCreated = new CEvent;
  if (pvw->isCreated())
    pvw->m_surfaceCreated->Set();
  pvw->add();

  return pvw;
}

void CJNIXBMCVideoView::_OnSurfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;

  CJNIXBMCVideoView*pvw = new CJNIXBMCVideoView(jhobject(thiz));
  CJNIXBMCVideoView *inst = s_videoview_map[pvw->ID()];
  inst->OnSurfaceChanged(CJNISurfaceHolder(jhobject(holder)), format, width, height);
  delete pvw;
}

void CJNIXBMCVideoView::_OnSurfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView*pvw = new CJNIXBMCVideoView(jhobject(thiz));
  CJNIXBMCVideoView *inst = s_videoview_map[pvw->ID()];
  inst->OnSurfaceCreated(CJNISurfaceHolder(jhobject(holder)));
  delete pvw;
}

void CJNIXBMCVideoView::_OnSurfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView*pvw = new CJNIXBMCVideoView(jhobject(thiz));
  CJNIXBMCVideoView *inst = s_videoview_map[pvw->ID()];
  inst->OnSurfaceDestroyed(CJNISurfaceHolder(jhobject(holder)));
  delete pvw;
}

void CJNIXBMCVideoView::OnSurfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  if (m_callback)
    m_callback->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCVideoView::OnSurfaceCreated(CJNISurfaceHolder holder)
{
  if (m_surfaceCreated)
    m_surfaceCreated->Set();
  if (m_callback)
    m_callback->surfaceCreated(holder);
}

void CJNIXBMCVideoView::OnSurfaceDestroyed(CJNISurfaceHolder holder)
{
  if (m_surfaceCreated)
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

int CJNIXBMCVideoView::ID() const
{
  return get_field<jint>(m_object, "mID");
}

bool CJNIXBMCVideoView::isCreated() const
{
  return get_field<jboolean>(m_object, "mIsCreated");
}

