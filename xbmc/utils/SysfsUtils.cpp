/*
 *      Copyright (C) 2011-2014 Team XBMC
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

#include "SysfsUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

int SysfsUtils::SetString(const std::string& path, const std::string& valstr)
{
  int fd = open(path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd >= 0)
  {
    write(fd, valstr.c_str(), valstr.size());
    close(fd);
    return 0;
  }
  CLog::Log(LOGERROR, "%s: error writing %s",__FUNCTION__, path.c_str());
  return -1;
}

int SysfsUtils::GetString(const std::string& path, std::string& valstr)
{
  int len;
  char buf[256] = {0};

  int fd = open(path.c_str(), O_RDONLY);
  if (fd >= 0)
  {
    valstr.clear();
    while ((len = read(fd, buf, 256)) > 0)
      valstr.append(buf, len);
    close(fd);
 
    StringUtils::Trim(valstr);
    
    return 0;
  }

  CLog::Log(LOGERROR, "%s: error reading %s",__FUNCTION__, path.c_str());
  valstr = "fail";
  return -1;
}

int SysfsUtils::SetInt(const std::string& path, const int val)
{
  int fd = open(path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd >= 0)
  {
    char bcmd[16];
    sprintf(bcmd, "%d", val);
    write(fd, bcmd, strlen(bcmd));
    close(fd);
    return 0;
  }

  CLog::Log(LOGERROR, "%s: error writing %s",__FUNCTION__, path.c_str());
  return -1;
}

int SysfsUtils::GetInt(const std::string& path, int& val)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd >= 0)
  {
    char bcmd[16] = {0};
    ssize_t sz = read(fd, bcmd, sizeof(bcmd));
    if (sz > 0)
    {
      val = strtol(bcmd, NULL, 16);
      close(fd);
      return 0;
    }
    close(fd);
  }

  CLog::Log(LOGERROR, "%s: error reading %s",__FUNCTION__, path.c_str());
  return -1;
}

bool SysfsUtils::Has(const std::string &path)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd >= 0)
  {
    close(fd);
    return true;
  }
  return false;
}

bool SysfsUtils::HasRW(const std::string &path)
{
  int fd = open(path.c_str(), O_RDWR);
  if (fd >= 0)
  {
    close(fd);
    return true;
  }
  else
    CLog::Log(LOGERROR, "%s: error opening %s(%d)",__FUNCTION__, path.c_str(), errno);

  return false;
}

bool SysfsUtils::Su(const std::string &cmd)
{
  std::string su = StringUtils::Format("su -c '%s'", cmd.c_str());
  system(su.c_str());
  return true;
}

bool SysfsUtils::Chmod(const std::string &path)
{
  std::string cmd = StringUtils::Format("chmod 666 %s", path.c_str());
  SysfsUtils::Su(cmd);
  return true;
}
