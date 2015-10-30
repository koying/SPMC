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

#include "DVDInputStream.h"
#include "dash/DASHSession.h"

class DemuxPacket;
class CDemuxStream;

class CDVDInputStreamMPD
    : public CDVDInputStream
    , public CDVDInputStream::IDisplayTime
    , public CDVDInputStream::ISeekTime
{
public:
  CDVDInputStreamMPD(CFileItem& fileitem);
  virtual ~CDVDInputStreamMPD();

  /* CDVDInputStream */
  virtual bool Open();
  virtual void Close();
  virtual int Read(uint8_t* buf, int buf_size) { return -1; }
  virtual int64_t Seek(int64_t offset, int whence) { return -1; }
  virtual bool Pause(double dTime) { return false; }
  virtual bool IsEOF() { return false; }
  virtual int64_t GetLength() { return -1; }

  /* IDisplayTime */
  virtual int GetTotalTime();
  virtual int GetTime();

  /* ISeekTime */
  virtual bool SeekTime(int ms) { return false; }

  const std::shared_ptr<CDASHSession> GetDashSession() const;

protected:
  void FillStreams();

  std::shared_ptr<CDASHSession> m_AP4session;
};
