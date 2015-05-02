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

#include "StageFrightVideoPrivate.h"


CStageFrightVideoPrivate::CStageFrightVideoPrivate()
{
  for (int i=0; i<INBUFCOUNT; ++i)
    inbuf[i] = NULL;
}

void CStageFrightVideoPrivate::signalBufferReturned(MediaBuffer *buffer)
{
}

MediaBuffer* CStageFrightVideoPrivate::getBuffer(size_t size)
{
  int i=0;
  for (; i<INBUFCOUNT; ++i)
    if (inbuf[i]->refcount() == 0 && inbuf[i]->size() >= size)
      break;
  if (i == INBUFCOUNT)
  {
    i = 0;
    for (; i<INBUFCOUNT; ++i)
      if (inbuf[i]->refcount() == 0)
        break;
    if (i == INBUFCOUNT)
      return NULL;
    inbuf[i]->setObserver(NULL);
    inbuf[i]->release();
    inbuf[i] = new MediaBuffer(size);
    inbuf[i]->setObserver(this);
  }

  inbuf[i]->add_ref();
  inbuf[i]->set_range(0, size);
  return inbuf[i];
}

bool CStageFrightVideoPrivate::inputBufferAvailable()
{
  for (int i=0; i<INBUFCOUNT; ++i)
    if (inbuf[i]->refcount() == 0)
      return true;
      
  return false;
}

