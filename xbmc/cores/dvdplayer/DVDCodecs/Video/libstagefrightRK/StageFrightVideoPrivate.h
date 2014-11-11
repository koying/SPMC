#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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
/***************************************************************************/

#include "threads/Thread.h"
#include "cores/VideoRenderers/RenderFormats.h"

#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>

#include "vpu_global.h"
#include "vpu_mem.h"

#include "EGL/egl.h"
#include "EGL/eglext.h"

#include <map>

#define INBUFCOUNT 16
#define OUTBUFCOUNT 2

using namespace android;

struct Frame
{
  status_t status;
  int32_t width, height;
  int64_t pts;
  MediaBuffer* medbuf;
};

class CStageFrightVideoPrivate : public MediaBufferObserver
{
public:
  CStageFrightVideoPrivate();

  virtual void signalBufferReturned(MediaBuffer *buffer);

  MediaBuffer* getBuffer(size_t size);
  bool inputBufferAvailable();

public: 
  MediaBuffer* inbuf[INBUFCOUNT];

  bool source_done;
  float aspect_ratio;

#if defined(DEBUG_VERBOSE)
  unsigned int cycle_time;
#endif
};
