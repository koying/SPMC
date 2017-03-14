/*
*      Copyright (C) 2016-2016 peak3d
*      http://www.peak3d.de
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
*  <http://www.gnu.org/licenses/>.
*
*/

#include "helpers.h"
#include <cstring>
#include "oscompat.h"
#include <stdlib.h>
#include "ap4/Ap4DataBuffer.h"

#include "utils/Base64.h"

#ifndef BYTE
typedef unsigned char BYTE;
#endif

std::vector<std::string> split(const std::string& s, char seperator)
{
  std::vector<std::string> output;
  std::string::size_type prev_pos = 0, pos = 0;

  if (s.size() == 0)
    return output;

  while ((pos = s.find(seperator, pos)) != std::string::npos)
  {
    output.push_back(s.substr(prev_pos, pos - prev_pos));
    prev_pos = ++pos;
  }
  output.push_back(s.substr(prev_pos));
  return output;
}

std::string &trim(std::string &src)
{
  src.erase(0, src.find_first_not_of(" "));
  src.erase(src.find_last_not_of(" ") + 1);
  return src;
}

static char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

std::string url_decode(std::string text) {
  char h;
  std::string escaped;

  for (auto i = text.begin(), n = text.end(); i != n; ++i) {
    std::string::value_type c = (*i);
    if (c == '%') {
      if (i[1] && i[2]) {
        h = from_hex(i[1]) << 4 | from_hex(i[2]);
        escaped += h;
        i += 2;
      }
    }
    else if (c == '+')
      escaped += ' ';
    else {
      escaped += c;
    }
  }
  return escaped;
}

static unsigned char HexNibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  else if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return 0;
}

std::string annexb_to_avc(const char *b16_data)
{
  unsigned int sz = strlen(b16_data) >> 1, szRun(sz);
  std::string result;

  if (sz > 1024)
    return result;

  uint8_t buffer[1024], *data(buffer);
  while (szRun--)
  {
    *data = (HexNibble(*b16_data) << 4) + HexNibble(*(b16_data+1));
    b16_data += 2;
    ++data;
  }

  if (sz <= 6 || buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 0 || buffer[3] != 1)
  {
    result = std::string((const char*)buffer, sz);
    return result;
  }

  uint8_t *sps = 0, *pps = 0, *end = buffer + sz;

  sps = pps = buffer + 4;

  while (pps + 4 <= end && (pps[0] != 0 || pps[1] != 0 || pps[2] != 0 || pps[3] != 1))
    ++pps;

  //Make sure we have found pps start
  if (pps + 4 >= end)
    return result;

  pps += 4;

  result.resize(sz + 3); //need 3 byte more for new header
  unsigned int pos(0);

  result[pos++] = 1;
  result[pos++] = static_cast<char>(sps[1]);
  result[pos++] = static_cast<char>(sps[2]);
  result[pos++] = static_cast<char>(sps[3]);
  result[pos++] = static_cast<char>(0xFF); //6 bits reserved(111111) + 2 bits nal size length - 1 (11)
  result[pos++] = static_cast<char>(0xe1); //3 bits reserved (111) + 5 bits number of sps (00001)

  sz = pps - sps - 4;
  result[pos++] = static_cast<const char>(sz >> 8);
  result[pos++] = static_cast<const char>(sz & 0xFF);
  result.replace(pos, sz, (const char*)sps, sz); pos += sz;

  result[pos++] = 1;
  sz = end - pps;
  result[pos++] = static_cast<const char>(sz >> 8);
  result[pos++] = static_cast<const char>(sz & 0xFF);
  result.replace(pos, sz, (const char*)pps, sz); pos += sz;

  return result;
}

void prkid2wvkid(const char *input, char *output)
{
  static const uint8_t remap[16] = { 3,2,1,0,5,4,7,6,8,9,10,11,12,13,14,15 };
  for (unsigned int i(0); i < 16; ++i)
    output[i] = input[remap[i]];
}

