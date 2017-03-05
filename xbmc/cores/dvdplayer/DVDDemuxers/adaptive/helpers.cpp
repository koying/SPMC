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

#ifndef BYTE
typedef unsigned char BYTE;
#endif

static const BYTE from_base64[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 62, 255, 63,
52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 0, 255, 255, 255,
255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 63,
255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255, 255, 255 };


bool b64_decode(const char *in, unsigned int in_len, uint8_t *out, unsigned int &out_len)
{
	// Make sure string length is a multiple of 4
	char *in_copy(0);
	if (in_len > 3 && strnicmp(in + (in_len - 3), "%3D", 3) == 0)
	{
		in_copy = (char *)malloc(in_len + 1);
		strcpy(in_copy, in);
		in = in_copy;
		if (in_len > 6 && strnicmp(in + (in_len - 6), "%3D", 3) == 0)
		{
			strcpy(in_copy + (in_len - 6), "==");
			in_len -= 4;
		}
		else {
			strcpy(in_copy + (in_len - 3), "=");
			in_len -= 2;
		}
	}

	if (in_len & 3)
	{
		free(in_copy);
        out_len = 0;
		return false;
	}

	unsigned int new_out_len = in_len / 4 * 3;
	if (in[in_len - 1] == '=') --new_out_len;
	if (in[in_len - 2] == '=') --new_out_len;
	if (new_out_len > out_len)
	{
		free(in_copy);
        out_len = 0;
		return false;
	}
	out_len = new_out_len;

	for (size_t i = 0; i < in_len; i += 4)
	{
		// Get values for each group of four base 64 characters
		BYTE b4[4];
		b4[0] = (in[i + 0] <= 'z') ? from_base64[in[i + 0]] : 0xff;
		b4[1] = (in[i + 1] <= 'z') ? from_base64[in[i + 1]] : 0xff;
		b4[2] = (in[i + 2] <= 'z') ? from_base64[in[i + 2]] : 0xff;
		b4[3] = (in[i + 3] <= 'z') ? from_base64[in[i + 3]] : 0xff;

		// Transform into a group of three bytes
		BYTE b3[3];
		b3[0] = ((b4[0] & 0x3f) << 2) + ((b4[1] & 0x30) >> 4);
		b3[1] = ((b4[1] & 0x0f) << 4) + ((b4[2] & 0x3c) >> 2);
		b3[2] = ((b4[2] & 0x03) << 6) + ((b4[3] & 0x3f) >> 0);

		// Add the byte to the return value if it isn't part of an '=' character (indicated by 0xff)
		if (b4[1] != 0xff) *out++ = b3[0];
		if (b4[2] != 0xff) *out++ = b3[1];
		if (b4[3] != 0xff) *out++ = b3[2];
	}
	free(in_copy);
	return true;
}

static const char *to_base64 =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ\
abcdefghijklmnopqrstuvwxyz\
0123456789+/";

std::string b64_encode(unsigned char const* in, unsigned int in_len, bool urlEncode)
{
	std::string ret;
	int i(3);
	unsigned char c_3[3];
	unsigned char c_4[4];

	while (in_len) {
		i = in_len > 2 ? 3 : in_len;
		in_len -= i;
		c_3[0] = *(in++);
		c_3[1] = i > 1 ? *(in++) : 0;
		c_3[2] = i > 2 ? *(in++) : 0;

		c_4[0] = (c_3[0] & 0xfc) >> 2;
		c_4[1] = ((c_3[0] & 0x03) << 4) + ((c_3[1] & 0xf0) >> 4);
		c_4[2] = ((c_3[1] & 0x0f) << 2) + ((c_3[2] & 0xc0) >> 6);
		c_4[3] = c_3[2] & 0x3f;

		for (int j = 0; (j < i + 1); ++j)
		{
			if (urlEncode && to_base64[c_4[j]] == '+')
				ret += "%2B";
			else if (urlEncode && to_base64[c_4[j]] == '/')
				ret += "%2F";
			else
				ret += to_base64[c_4[j]];
		}
	}
	while ((i++ < 3))
		ret += urlEncode ? "%3D" : "=";
	return ret;
}

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

bool create_ism_license(std::string key, std::string license_data, AP4_DataBuffer &init_data)
{
  if (key.size() != 16 || license_data.empty())
  {
    init_data.SetDataSize(0);
    return false;
  }

  uint8_t ld[1024];
  unsigned int ld_size(1024);
  b64_decode(license_data.c_str(), license_data.size(), ld, ld_size);

  const uint8_t *uuid((uint8_t*)strstr((const char*)ld, "{UUID}"));
  unsigned int license_size = uuid ? ld_size + 36 - 6 : ld_size;

  //Build up proto header
  init_data.Reserve(512);
  uint8_t *protoptr(init_data.UseData());
  *protoptr++ = 18; //id=16>>3=2, type=2(flexlen)
  *protoptr++ = 16; //length of key
  memcpy(protoptr, key.data(), 16);
  protoptr += 16;
  //-----------
  *protoptr++ = 34;//id=32>>3=4, type=2(flexlen)
  do {
    *protoptr++ = static_cast<uint8_t>(license_size & 127);
    license_size >>= 7;
    if (license_size)
      *(protoptr - 1) |= 128;
    else
      break;
  } while (1);
  if (uuid)
  {
    static const uint8_t hexmap[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
    memcpy(protoptr, ld, uuid - ld);
    protoptr += uuid - ld;

    for (unsigned int i(0); i < 16; ++i)
    {
      if(i == 4 || i == 6 || i == 8 || i == 10)
        *protoptr++ = '-';
      *protoptr++ = hexmap[(uint8_t)(key.data()[i]) >> 4];
      *protoptr++ = hexmap[(uint8_t)(key.data()[i]) & 15];
    }
    unsigned int sizeleft = ld_size - ((uuid - ld) + 6);
    memcpy(protoptr, uuid + 6, sizeleft);
    protoptr += sizeleft;
  }
  else
  {
    memcpy(protoptr, ld, ld_size);
    protoptr += ld_size;
  }
  init_data.SetDataSize(protoptr - init_data.UseData());

  return true;
}
