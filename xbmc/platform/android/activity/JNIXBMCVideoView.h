#pragma once
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

#include <androidjni/JNIBase.h>

#include <androidjni/Context.h>
#include <androidjni/Rect.h>
#include <androidjni/Surface.h>
#include <androidjni/SurfaceHolder.h>
#include <androidjni/GL10.h>
#include <androidjni/EGLConfig.h>

#include "guilib/Geometry.h"
#include "threads/Event.h"

class CJNIXBMCVideoView : virtual public CJNIBase, public CJNISurfaceHolderCallback, public CJNIInterfaceImplem<CJNIXBMCVideoView>
{
public:
  CJNIXBMCVideoView(const jni::jhobject &object);
  ~CJNIXBMCVideoView();

  static void RegisterNatives(JNIEnv* env);
  
  static CJNIXBMCVideoView* createVideoView(CJNISurfaceHolderCallback* holderCallback);

  // CJNISurfaceHolderCallback interface
  void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height);
  void surfaceCreated(CJNISurfaceHolder holder);
  void surfaceDestroyed(CJNISurfaceHolder holder);

  static void _surfaceChanged(JNIEnv* env, jobject thiz, jobject holder, jint format, jint width, jint height);
  static void _surfaceCreated(JNIEnv* env, jobject thiz, jobject holder);
  static void _surfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder);

  // CJNIGLSurfaceViewRender interface
  virtual void onDrawFrame(CJNIGL10 gl);
  virtual void onSurfaceCreated(CJNIGL10 gl, CJNIEGLConfig config);
  virtual void onSurfaceChanged(CJNIGL10 gl, int width, int height);

  static void _onDrawFrame(JNIEnv* env, jobject thiz, jobject gl);
  static void _onSurfaceCreated(JNIEnv* env, jobject thiz, jobject gl, jobject config);
  static void _onSurfaceChanged(JNIEnv* env, jobject thiz, jobject gl, int width, int height) ;

  bool waitForSurface(unsigned int millis);
  bool isActive() { return m_surfaceCreated.Signaled(); }
  CJNISurface getSurface();
  const CRect& getSurfaceRect();
  void setSurfaceRect(const CRect& rect);
  void add();
  void release();
  int ID() const;
  bool isCreated() const;

protected:
  CJNISurfaceHolderCallback* m_holderCallback;
  CEvent m_surfaceCreated;
  CRect m_surfaceRect;

private:
  CJNIXBMCVideoView();
  };
