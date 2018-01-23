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

#include <androidjni/jutils-details.hpp>

#include "cores/IPlayer.h"
#include "cores/VideoPlayer/VideoPlayerMessenger.h"
#include "platform/android/service/XBMCService.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <list>
#include <algorithm>
#include <cassert>

#include "CompileInfo.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCVideoView";
static std::string s_renderclassName = std::string(CCompileInfo::GetClass()) + "/XBMCVideoView$XBMCVideoRenderer";

void CJNIXBMCVideoRenderer::RegisterNatives(JNIEnv *env)
{
  jclass cClass = env->FindClass(s_renderclassName.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onDrawFrame", "(Ljavax/microedition/khronos/opengles/GL10;)V", (void*)&CJNIXBMCVideoRenderer::_onDrawFrame},
      {"_onSurfaceCreated", "(Ljavax/microedition/khronos/opengles/GL10;Ljavax/microedition/khronos/egl/EGLConfig;)V", (void*)&CJNIXBMCVideoRenderer::_onSurfaceCreated},
      {"_onSurfaceChanged", "(Ljavax/microedition/khronos/opengles/GL10;II)V", (void*)&CJNIXBMCVideoRenderer::_onSurfaceChanged},
      {"_setThreadId", "()V", (void*)&CJNIXBMCVideoRenderer::_setThreadId}
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

CJNIXBMCVideoRenderer::CJNIXBMCVideoRenderer(jhobject jRenderer, IPlayer* player)
  : CJNIBase ()
  , m_player(player)
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  m_object.reset(jRenderer);
  m_object.setGlobal();
  add_instance(jRenderer, this);
}


void CJNIXBMCVideoRenderer::_setThreadId(JNIEnv* env, jobject thiz)
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  (void)env;

  KODI::VIDEOPLAYER::CVideoPlayerMessenger::GetInstance().setMainThreadId(pthread_self());
}

void CJNIXBMCVideoRenderer::_onDrawFrame(JNIEnv* env, jobject thiz, jobject gl)
{
  (void)env;

  CJNIXBMCVideoRenderer *inst = find_instance(thiz);
  if (inst)
    inst->onDrawFrame(CJNIGL10(jhobject::fromJNI(gl)));
}

void CJNIXBMCVideoRenderer::_onSurfaceCreated(JNIEnv* env, jobject thiz, jobject gl, jobject config)
{
  (void)env;

  CJNIXBMCVideoRenderer *inst = find_instance(thiz);
  if (inst)
    inst->onSurfaceCreated(CJNIGL10(jhobject::fromJNI(gl)), CJNIEGLConfig(jhobject::fromJNI(config)));
}

void CJNIXBMCVideoRenderer::_onSurfaceChanged(JNIEnv* env, jobject thiz, jobject gl, int width, int height)
{
  (void)env;

  CJNIXBMCVideoRenderer *inst = find_instance(thiz);
  if (inst)
    inst->onSurfaceChanged(CJNIGL10(jhobject::fromJNI(gl)), width, height);
}

void CJNIXBMCVideoRenderer::onDrawFrame(CJNIGL10 /*gl*/)
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  KODI::VIDEOPLAYER::CVideoPlayerMessenger::GetInstance().ProcessMessages();
  if (m_player)
  {
    m_player->FrameMove();
    m_player->Render(false, 255, false);
  }
}

void CJNIXBMCVideoRenderer::onSurfaceCreated(CJNIGL10 /*gl*/, CJNIEGLConfig /*config*/)
{

}

void CJNIXBMCVideoRenderer::onSurfaceChanged(CJNIGL10 /*gl*/, int /*width*/, int /*height*/)
{

}

/*****************************/

void CJNIXBMCVideoView::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] = 
    {
      {"_surfaceChanged", "(Landroid/view/SurfaceHolder;III)V", (void*)&CJNIXBMCVideoView::_surfaceChanged},
      {"_surfaceCreated", "(Landroid/view/SurfaceHolder;)V", (void*)&CJNIXBMCVideoView::_surfaceCreated},
      {"_surfaceDestroyed", "(Landroid/view/SurfaceHolder;)V", (void*)&CJNIXBMCVideoView::_surfaceDestroyed},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }

  CJNIXBMCVideoRenderer::RegisterNatives(env);
}

CJNIXBMCVideoView::CJNIXBMCVideoView()
  : m_holderCallback(nullptr)
{
}

CJNIXBMCVideoView::CJNIXBMCVideoView(const jni::jhobject &object)
  : CJNIBase(object)
  , m_holderCallback(nullptr)
{
}

CJNIXBMCVideoView::~CJNIXBMCVideoView()
{
}

CJNIXBMCVideoView* CJNIXBMCVideoView::createVideoView(IPlayer* player)
{
  std::string signature = "()L" + s_className + ";";

  CJNIXBMCVideoView* pvw = new CJNIXBMCVideoView(call_static_method<jhobject>(CXBMCService::get()->getClassLoader().loadClass(GetDotClassName(s_className)),
                                                                              "createVideoView", signature.c_str()));
  if (!*pvw)
  {
    CLog::Log(LOGERROR, "Cannot instantiate VideoView!!");
    delete pvw;
    return nullptr;
  }

  add_instance(pvw->get_raw(), pvw);
  if (pvw->isCreated())
    pvw->m_surfaceCreated.Set();
  pvw->add();

  std::string signature_renderer = "L" + s_renderclassName + ";";

  jhobject jRenderer = get_field<jhobject>(pvw->m_object, "mRenderer", signature_renderer.c_str());
  pvw->m_renderer.reset(new CJNIXBMCVideoRenderer(jRenderer, player));

  return pvw;
}

void CJNIXBMCVideoView::setSurfaceholderCallback(CJNISurfaceHolderCallback* holderCallback)
{
  m_holderCallback = holderCallback;
}

void CJNIXBMCVideoView::_surfaceChanged(JNIEnv *env, jobject thiz, jobject holder, jint format, jint width, jint height )
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(thiz);
  if (inst)
    inst->surfaceChanged(CJNISurfaceHolder(jhobject::fromJNI(holder)), format, width, height);
}

void CJNIXBMCVideoView::_surfaceCreated(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(thiz);
  if (inst)
    inst->surfaceCreated(CJNISurfaceHolder(jhobject::fromJNI(holder)));
}

void CJNIXBMCVideoView::_surfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder)
{
  (void)env;

  CJNIXBMCVideoView *inst = find_instance(thiz);
  if (inst)
    inst->surfaceDestroyed(CJNISurfaceHolder(jhobject::fromJNI(holder)));
}

void CJNIXBMCVideoView::surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  // Reset Surface Rect
  m_surfaceRect = CRect();

  if (m_holderCallback)
    m_holderCallback->surfaceChanged(holder, format, width, height);
}

void CJNIXBMCVideoView::surfaceCreated(CJNISurfaceHolder holder)
{
  if (m_holderCallback)
    m_holderCallback->surfaceCreated(holder);
  m_surfaceCreated.Set();
}

void CJNIXBMCVideoView::surfaceDestroyed(CJNISurfaceHolder holder)
{
  m_surfaceCreated.Reset();
  if (m_holderCallback)
    m_holderCallback->surfaceDestroyed(holder);
}

bool CJNIXBMCVideoView::waitForSurface(unsigned int millis)
{
  return m_surfaceCreated.WaitMSec(millis);
}

void CJNIXBMCVideoView::add()
{
  call_method<void>(m_object,
                    "add", "()V");
}

void CJNIXBMCVideoView::release()
{
  remove_instance(this);
  call_method<void>(m_object,
                    "release", "()V");
}

CJNISurface CJNIXBMCVideoView::getSurface()
{
  return call_method<jhobject>(m_object,
                               "getSurface", "()Landroid/view/Surface;");
}

const CRect& CJNIXBMCVideoView::getSurfaceRect()
{
  return m_surfaceRect;
}

void CJNIXBMCVideoView::setSurfaceRect(const CRect& rect)
{
  call_method<void>(m_object,
                    "setSurfaceRect", "(IIII)V", int(rect.x1), int(rect.y1), int(rect.x2), int(rect.y2));
  m_surfaceRect = rect;
}

bool CJNIXBMCVideoView::isCreated() const
{
  return get_field<jboolean>(m_object, "mIsCreated");
}


