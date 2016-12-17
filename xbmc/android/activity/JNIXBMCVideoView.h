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

#include "android/jni/JNIBase.h"

#include "android/jni/Context.h"
#include "android/jni/SurfaceHolder.h"
#include "android/jni/Surface.h"
#include "android/jni/Rect.h"

#include "guilib/Geometry.h"

#include "threads/Event.h"

class CJNIXBMCVideoView : public CJNIBase
{
public:
  CJNIXBMCVideoView(const jni::jhobject &object);
  ~CJNIXBMCVideoView();

  static CJNIXBMCVideoView* createVideoView(CJNISurfaceHolderCallback* callback);

  static void _OnSurfaceChanged(JNIEnv* env, jobject thiz, jobject holder, jint format, jint width, jint height);
  static void _OnSurfaceCreated(JNIEnv* env, jobject thiz, jobject holder);
  static void _OnSurfaceDestroyed(JNIEnv* env, jobject thiz, jobject holder);

  bool waitForSurface(unsigned int millis);
  bool isActive() { return m_surfaceCreated->Signaled(); }
  CJNISurface getSurface();
  const CRect& getSurfaceRect();
  void setSurfaceRect(const CRect& rect);
  void add();
  void release();
  int ID() const;
  bool isCreated() const;

public:
  // Out-of-main-java-loop class search is hiccupy.
  // Have it initialized at JNI_OnLoad time
  static jclass m_jclass;

protected:
  CJNISurfaceHolderCallback* m_callback;
  CEvent* m_surfaceCreated;

  void OnSurfaceChanged(CJNISurfaceHolder holder, int format, int width, int height);
  void OnSurfaceCreated(CJNISurfaceHolder holder);
  void OnSurfaceDestroyed(CJNISurfaceHolder holder);

  CRect m_surfaceRect;

private:
  CJNIXBMCVideoView();
};
