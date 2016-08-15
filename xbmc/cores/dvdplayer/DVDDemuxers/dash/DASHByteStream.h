#pragma once

/*
 *      Copyright (C) 2016 Christian Browet
 *      Copyright (C) 2016-2016 peak3d
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

#include "ap4/Ap4.h"

#include "DASHStream.h"

class CDASHByteStream : public AP4_ByteStream
{
public:
  // Constructor
  CDASHByteStream(dash::DASHStream *dashStream) :dash_stream_(dashStream) {}

  // AP4_ByteStream methods
  AP4_Result ReadPartial(void* buffer, AP4_Size  bytesToRead, AP4_Size& bytesRead) override;
  AP4_Result Seek(AP4_Position position) override;
  AP4_Result Tell(AP4_Position& position) override;

  AP4_Result WritePartial(const void* buffer, AP4_Size bytesToWrite, AP4_Size& bytesWritten) override { return AP4_ERROR_NOT_SUPPORTED; }
  AP4_Result GetSize(AP4_LargeSize& size) override { return AP4_ERROR_NOT_SUPPORTED; }

  // AP4_Referenceable methods
  void AddReference() override {}
  void Release() override {}

protected:
  // members
  dash::DASHStream *dash_stream_;
};
