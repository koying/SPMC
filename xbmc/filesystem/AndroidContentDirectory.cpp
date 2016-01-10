/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "system.h"

#if defined(TARGET_ANDROID)
#include "AndroidContentDirectory.h"
#include "xbmc/android/activity/XBMCApp.h"
#include "FileItem.h"
#include "File.h"
#include "utils/URIUtils.h"
#include <vector>
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "CompileInfo.h"

#include "android/jni/Intent.h"

using namespace XFILE;

CAndroidContentDirectory::CAndroidContentDirectory(void)
{
}

CAndroidContentDirectory::~CAndroidContentDirectory(void)
{
}

bool CAndroidContentDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string dirname = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(dirname);
  CLog::Log(LOGDEBUG, "CAndroidContentDirectory::GetDirectory: %s",dirname.c_str());

  if (dirname.empty())
  {
    CJNIIntent intent = CJNIIntent(CJNIIntent::ACTION_OPEN_DOCUMENT_TREE);
    CXBMCApp::startActivityForResult(intent, 421);
  }

  CLog::Log(LOGERROR, "CAndroidContentDirectory::GetDirectory Failed to open %s", url.Get().c_str());
  return false;
}

#endif
