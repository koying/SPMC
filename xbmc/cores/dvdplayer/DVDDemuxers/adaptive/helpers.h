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

#pragma once

#include <string>
#include <stdint.h>
#include <vector>


class AP4_DataBuffer;

bool b64_decode(const char *in, unsigned int in_len, uint8_t *out, unsigned int &out_len);

std::string b64_encode(unsigned char const* in, unsigned int in_len, bool urlEncode);

std::vector<std::string> split(const std::string& s, char seperator);

std::string &trim(std::string &src);

std::string url_decode(std::string text);

std::string annexb_to_avc(const char *b16_data);

void prkid2wvkid(const char *input, char *output);
bool create_ism_license(std::string key, std::string license_data, AP4_DataBuffer &init_data);
