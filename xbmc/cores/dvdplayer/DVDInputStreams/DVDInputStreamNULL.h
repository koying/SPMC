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

#include "DVDInputStream.h"

class CDVDInputStreamNULL
    : public CDVDInputStream
{
public:
  CDVDInputStreamNULL(CFileItem& fileitem)
    : CDVDInputStream(DVDSTREAM_TYPE_NULL, fileitem)
  {}
  virtual ~CDVDInputStreamNULL() {}

  virtual int Read(uint8_t* buf, int buf_size) { return -1; }
  virtual int64_t Seek(int64_t offset, int whence) { return -1; }
  virtual bool Pause(double dTime) { return false; }
  virtual bool IsEOF() { return false; }
  virtual int64_t GetLength() { return -1; }
};
