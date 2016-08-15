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

#include "DASHByteStream.h"


AP4_Result CDASHByteStream::ReadPartial(void* buffer, AP4_Size bytesToRead, AP4_Size& bytesRead)
{
  bytesRead = dash_stream_->read(buffer, bytesToRead);
  return bytesRead > 0 ? AP4_SUCCESS : AP4_ERROR_READ_FAILED;
}

AP4_Result CDASHByteStream::Seek(AP4_Position position)
{
  return dash_stream_->seek(position) ? AP4_SUCCESS : AP4_ERROR_NOT_SUPPORTED;
}

AP4_Result CDASHByteStream::Tell(AP4_Position& position)
{
  position = dash_stream_->tell();
  return AP4_SUCCESS;
}
