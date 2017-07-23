/*
 *      Copyright (C) 2017 Christian Browet
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

#include "JNIXBMCMainView.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Context.h>

#include "utils/StringUtils.h"
#include "utils/log.h"
#include "XBMCApp.h"

#include <list>
#include <algorithm>
#include <cassert>

#include "CompileInfo.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCMainView";
CEvent CJNIXBMCMainView::m_surfaceCreated;
CJNIXBMCMainView* CJNIXBMCMainView::m_instance = nullptr;

void CJNIXBMCMainView::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_attach", "()V", (void*)&CJNIXBMCMainView::_attach},
      {"_surfaceChanged", "(Landroid/view/SurfaceHolder;III)V", (void*)&CJNIXBMCMainView::_surfaceChanged},
      {"_surfaceCreated", "(Landroid/view/SurfaceHolder;)V", (void*)&CJNIXBMCMainView::_surfaceCreated},
      {"_surfaceDestroyed", "(Landroid/view/SurfaceHolder;)V", (void*)&CJNIXBMCMainView::_surfaceDestroyed},
      {"_onLayoutChange", "(IIII)V", (void*)&CJNIXBMCMainView::_onLayoutChange},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

CJNIXBMCMainView::CJNIXBMCMainView(CXBMCApp* xbmcapp)
  : m_xbmcapp(xbmcapp)
{
  m_instance = this;
}

CJNIXBMCMainView::~CJNIXBMCMainView()
{
}

void CJNIXBMCMainView::_attach(JNIEnv* env, jobject thiz)
{
  (void)env;

  if (m_instance)
    m_instance->attach(thiz);
}

void CJNIXBMCMainView::_surfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;

  if (m_instance)
    m_instance->surfaceChanged(CJNISurfaceHolder(jhobject::fromJNI(holder)), format, width, height);
}

void CJNIXBMCMainView::_surfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  if (m_instance)
    m_instance->surfaceCreated(CJNISurfaceHolder(jhobject::fromJNI(holder)));
}

void CJNIXBMCMainView::_surfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  if (m_instance)
    m_instance->surfaceDestroyed(CJNISurfaceHolder(jhobject::fromJNI(holder)));
}

void CJNIXBMCMainView::_onLayoutChange(JNIEnv* env, jobject thiz, jint left, jint top, jint width, jint height)
{
  (void)env;

  if (m_instance)
    m_instance->onLayoutChange(left, top, width, height);
}

void CJNIXBMCMainView::attach(const jobject& thiz)
{
  if (!m_object)
  {
    m_object.reset(thiz);
    m_object.setGlobal();
  }
}

void CJNIXBMCMainView::surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  if (m_xbmcapp)
    m_xbmcapp->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCMainView::surfaceCreated(CJNISurfaceHolder holder)
{
  if (m_xbmcapp)
    m_xbmcapp->surfaceCreated(holder);
  m_surfaceCreated.Set();
}

void CJNIXBMCMainView::surfaceDestroyed(CJNISurfaceHolder holder)
{
  m_surfaceCreated.Reset();
  if (m_xbmcapp)
    m_xbmcapp->surfaceDestroyed(holder);
}

void CJNIXBMCMainView::onLayoutChange(int left, int top, int width, int height)
{
  if (m_xbmcapp)
    m_xbmcapp->onLayoutChange(left, top, width, height);
}

bool CJNIXBMCMainView::waitForSurface(unsigned int millis)
{
  return m_surfaceCreated.WaitMSec(millis);
}

bool CJNIXBMCMainView::isCreated() const
{
  if (!m_object)
    return false;
  return get_field<jboolean>(m_object, "mIsCreated");

}

