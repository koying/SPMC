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
CJNIXBMCVideoGLView* CJNIXBMCVideoGLView::m_instance = nullptr;

void CJNIXBMCVideoGLView::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] = 
    {
      {"_attach", "()V", (void*)&CJNIXBMCVideoGLView::_attach},
      {"_onDrawFrame", "()V", (void*)&CJNIXBMCVideoGLView::_onDrawFrame},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

CJNIXBMCVideoGLView::CJNIXBMCVideoGLView()
{
  m_instance = this;
}

CJNIXBMCVideoGLView::CJNIXBMCVideoGLView(const jni::jhobject &object)
  : CJNIBase(object)
{
  m_instance = this;
}

CJNIXBMCVideoGLView::~CJNIXBMCVideoGLView()
{
}

void CJNIXBMCVideoGLView::_attach(JNIEnv* env, jobject thiz)
{
  (void)env;

  if (m_instance)
    m_instance->attach(thiz);
}

void CJNIXBMCVideoGLView::attach(const jobject& thiz)
{
  if (!m_object)
  {
    m_object = jhobject(thiz);
    m_object.setGlobal();
  }
}

void CJNIXBMCVideoGLView::_onDrawFrame(JNIEnv *env, jobject thiz)
{
  (void)env;

  if (m_instance)
    m_instance->onDrawFrame();
}

void CJNIXBMCVideoGLView::onDrawFrame()
{
  g_application.m_pPlayer->RenderInternal(m_lastClear, m_lastAlpha, m_lastGui);
}

void CJNIXBMCVideoGLView::requestRender(bool clear, uint32_t alpha, bool gui)
{
  m_lastClear = clear;
  m_lastAlpha = alpha;
  m_lastGui = gui;

  call_method<void>(m_object,
                    "requestRender", "()V");
}
