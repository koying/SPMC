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

#include "JNIXBMCVideoGLView.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Context.h>

#include "Application.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <list>
#include <algorithm>
#include <cassert>

#include "CompileInfo.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCVideoGLView";
CJNIXBMCVideoGLView* CJNIXBMCVideoGLView::m_lastInstance = nullptr;
std::list<std::pair<jni::jhobject, CJNIXBMCVideoGLView*>> CJNIXBMCVideoGLView::m_object_map;

void CJNIXBMCVideoGLView::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] = 
    {
      {"_surfaceCreated", "()V", (void*)&CJNIXBMCVideoGLView::_surfaceCreated},
      {"_surfaceChanged", "(II)V", (void*)&CJNIXBMCVideoGLView::_surfaceChanged},
      {"_onDrawFrame", "()V", (void*)&CJNIXBMCVideoGLView::_onDrawFrame},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

CJNIXBMCVideoGLView::CJNIXBMCVideoGLView()
{
}

CJNIXBMCVideoGLView::CJNIXBMCVideoGLView(const jni::jhobject &object)
  : CJNIBase(object)
{
}

CJNIXBMCVideoGLView::~CJNIXBMCVideoGLView()
{
  if (m_lastInstance = this)
    m_lastInstance = nullptr;
  if (!m_object)
    return;
  release();
}

CJNIXBMCVideoGLView* CJNIXBMCVideoGLView::createVideoGLView()
{
  std::string signature = "()L" + s_className + ";";

  CJNIXBMCVideoGLView* pvw = new CJNIXBMCVideoGLView(call_static_method<jhobject>(xbmc_jnienv(), CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)),
                                                                              "createVideoGLView", signature.c_str()));
  if (!*pvw)
  {
    CLog::Log(LOGERROR, "Cannot instantiate VideoGLView!!");
    delete pvw;
    return nullptr;
  }

  m_lastInstance = pvw;
  m_object_map.push_back(std::pair<jni::jhobject, CJNIXBMCVideoGLView*>(pvw->get_raw(), pvw));
  if (pvw->isCreated())
    pvw->m_surfaceCreated.Set();
  pvw->add();

  return pvw;
}

CJNIXBMCVideoGLView* CJNIXBMCVideoGLView::find_instance(const jni::jhobject& o)
{
  for( auto it = m_object_map.begin(); it != m_object_map.end(); ++it )
  {
    if (it->first == o)
      return it->second;
  }
  return nullptr;
}

void CJNIXBMCVideoGLView::_onDrawFrame(JNIEnv *env, jobject thiz)
{
  (void)env;

  CJNIXBMCVideoGLView *inst = find_instance(jhobject(thiz));
  if (inst)
    inst->onDrawFrame();
}

void CJNIXBMCVideoGLView::_surfaceChanged(JNIEnv*, jobject thiz, jint width, jint height )
{
  CJNIXBMCVideoGLView *inst = find_instance(jhobject(thiz));
  if (inst)
    inst->surfaceChanged(width, height);
}

void CJNIXBMCVideoGLView::_surfaceCreated(JNIEnv*, jobject thiz)
{
  CJNIXBMCVideoGLView *inst = find_instance(jhobject(thiz));
  if (inst)
    inst->surfaceCreated();
}

void CJNIXBMCVideoGLView::surfaceChanged(int width, int height)
{
}

void CJNIXBMCVideoGLView::surfaceCreated()
{
  m_surfaceCreated.Set();
}

void CJNIXBMCVideoGLView::add()
{
  call_method<void>(m_object,
                    "add", "()V");
}

void CJNIXBMCVideoGLView::release()
{
  for( auto it = m_object_map.begin(); it != m_object_map.end(); ++it )
  {
    if (it->second == this)
    {
      m_object_map.erase(it);
      break;
    }
  }

  call_method<void>(m_object,
                    "release", "()V");
}

bool CJNIXBMCVideoGLView::isCreated() const
{
  return get_field<jboolean>(m_object, "mIsCreated");
}

bool CJNIXBMCVideoGLView::waitForSurface(unsigned int millis)
{
  return m_surfaceCreated.WaitMSec(millis);
}

void CJNIXBMCVideoGLView::onDrawFrame()
{
  g_application.m_pPlayer->DoRender(m_lastClear, m_lastAlpha);
}

void CJNIXBMCVideoGLView::requestRender(bool clear, uint8_t alpha)
{
  m_lastClear = clear;
  m_lastAlpha = alpha;

  call_method<void>(m_object,
                    "requestRender", "()V");
}
