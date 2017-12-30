/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "HTTPLogHandler.h"
#include "URL.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/File.h"
#include "network/WebServer.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"

CHTTPLogHandler::CHTTPLogHandler(const HTTPRequest &request)
  : CHTTPFileHandler(request)
{
  std::string file;
  int responseStatus = MHD_HTTP_BAD_REQUEST;

  if (m_request.pathUrl.size() > 5 && StringUtils::EndsWith(m_request.pathUrl, ".log"))
  {
    std::string path = m_request.pathUrl.substr(5);
    file = URIUtils::GetFileName(path);

    if (file == path)
    {
      if (XFILE::CFile::Exists("special://logpath/" + file))
      {
        file = CSpecialProtocol::TranslatePath("special://logpath/" + file);
        responseStatus = MHD_HTTP_OK;
      }
      else
        responseStatus = MHD_HTTP_NOT_FOUND;
    }
    else
      responseStatus = MHD_HTTP_UNAUTHORIZED;
  }
  else
    responseStatus = MHD_HTTP_NOT_FOUND;


  // set the file and the HTTP response status
  SetFile(file, responseStatus);
}

bool CHTTPLogHandler::CanHandleRequest(const HTTPRequest &request)
{
  return request.pathUrl.find("/log") == 0;
}
