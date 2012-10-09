/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "VideoDatabaseFile.h"
#include "video/VideoDatabase.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <sys/stat.h>

using namespace XFILE;

CVideoDatabaseFile::CVideoDatabaseFile(void)
{
}

CVideoDatabaseFile::~CVideoDatabaseFile(void)
{
  Close();
}

std::string CVideoDatabaseFile::TranslateUrl(const CURL& url)
{
  std::string strFileName=URIUtils::GetFileName(url.Get());
  if (strFileName.empty())
	  return "";

  std::string strPath = URIUtils::GetDirectory(url.Get());
  if (strPath.empty())
	  return "";

  std::string strExtension = URIUtils::GetExtension(strFileName);
  URIUtils::RemoveExtension(strFileName);

  if (!StringUtils::IsNaturalNumber(strFileName))
    return "";
  long idDb=atol(strFileName.c_str());

  std::vector<std::string> pathElem = StringUtils::Split(strPath, "/");
  if (pathElem.size() == 0)
    return "";

  std::string itemType = pathElem.at(2);
  VIDEODB_CONTENT_TYPE type;
  if (itemType == "movies" || itemType == "recentlyaddedmovies")
    type = VIDEODB_CONTENT_MOVIES;
  else if (itemType == "episodes" || itemType == "recentlyaddedepisodes")
    type = VIDEODB_CONTENT_EPISODES;
  else if (itemType == "musicvideos" || itemType == "recentlyaddedmusicvideos")
    type = VIDEODB_CONTENT_MUSICVIDEOS;
  else
    return "";

  CVideoDatabase videoDatabase;
  if (!videoDatabase.Open())
    return "";

  std::string realFilename;
  videoDatabase.GetFilePathById(idDb, realFilename, type);

  return realFilename;
}

bool CVideoDatabaseFile::Open(const CURL& url)
{
  return m_file.Open(TranslateUrl(url));
}

bool CVideoDatabaseFile::Exists(const CURL& url)
{
  return !TranslateUrl(url).empty();
}

int CVideoDatabaseFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return m_file.Stat(TranslateUrl(url), buffer);
}

ssize_t CVideoDatabaseFile::Read(void* lpBuf, size_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}

int64_t CVideoDatabaseFile::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CVideoDatabaseFile::Close()
{
  m_file.Close();
}

int64_t CVideoDatabaseFile::GetPosition()
{
  return m_file.GetPosition();
}

int64_t CVideoDatabaseFile::GetLength()
{
  return m_file.GetLength();
}

