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

#include <stdint.h>

enum DVDDecyptSchemes
{
  DVDDECRYPTSCHEME_WIDEVINE,  // UUID(0xEDEF8BA979D64ACEL, 0xA3C827DCD51D21EDL);
  DVDDECRYPTSCHEME_PLAYREADY, // UUID(0x9A04F07998404286L, 0xAB92E65BE0885F95L);
};

struct DVDDecyptPacket
{
  uint32_t size;
  void* buffer;
};

class IDVDDecrypt
{
  void Initialize(DVDDecyptSchemes scheme) = 0;
  void SetProviderResponse(DVDDecyptPacket* response) = 0;
  void SetKeyResponse(DVDDecyptPacket* response) = 0;
  void InitializeDecrypt(const std::string& cipherAlgorithm, const std::string& macAlgorithm) = 0;
  DVDDecyptPacket* DecryptPacket(DVDDecyptPacket* data) = 0;
};
