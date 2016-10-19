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

#include "AndroidContentDirectory.h"
#include "platform/android/activity/XBMCApp.h"
#include "FileItem.h"
#include "File.h"
#include "utils/URIUtils.h"
#include <vector>
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "CompileInfo.h"

#include "platform/android/jni/Intent.h"
#include "platform/android/jni/URI.h"
#include "platform/android/jni/DocumentsContract.h"
#include "platform/android/jni/Document.h"
#include "platform/android/jni/ContentResolver.h"
#include "platform/android/jni/Cursor.h"

#define ACTION_OPEN_DOCUMENT_TREE_REQID 421

using namespace XFILE;

CAndroidContentDirectory::CAndroidContentDirectory(void)
{
}

CAndroidContentDirectory::~CAndroidContentDirectory(void)
{
}

bool CAndroidContentDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  CLog::Log(LOGDEBUG, "CAndroidContentDirectory::GetDirectory: %s", url.Get().c_str());

  CJNIURI childrenUri = CJNIURI::parse(url.Get());
  if (childrenUri.getPath().empty())
  {
    CJNIIntent intent = CJNIIntent(CJNIIntent::ACTION_OPEN_DOCUMENT_TREE);
    CJNIIntent result;
    if (CXBMCApp::WaitForActivityResult(intent, ACTION_OPEN_DOCUMENT_TREE_REQID, result))
    {
      CJNIURI rooturi = result.getData();
      childrenUri = CJNIDocumentsContract::buildChildDocumentsUriUsingTree(rooturi, CJNIDocumentsContract::getTreeDocumentId(rooturi));
    }
    else
    {
      CLog::Log(LOGERROR, "CAndroidContentDirectory::GetDirectory Failed to open %s", url.Get().c_str());
      return false;
    }
  }
  else
  {
    childrenUri = CJNIDocumentsContract::buildChildDocumentsUriUsingTree(childrenUri, CJNIDocumentsContract::getDocumentId(childrenUri));
  }
  CLog::Log(LOGDEBUG, "CAndroidContentDirectory::GetDirectory opened succesfully: %s", childrenUri.toString().c_str());

  std::vector<std::string> columns;
  columns.push_back(CJNIDocument::COLUMN_DISPLAY_NAME);
  columns.push_back(CJNIDocument::COLUMN_MIME_TYPE);
  columns.push_back(CJNIDocument::COLUMN_DOCUMENT_ID);
  columns.push_back(CJNIDocument::COLUMN_SIZE);
  CJNICursor docCursor = CXBMCApp::getContentResolver().query(childrenUri, columns, std::string(), std::vector<std::string>(), std::string());
  while (docCursor.moveToNext())
  {
    int columnIndex = docCursor.getColumnIndex(columns[0]);
    std::string disp_name = docCursor.getString(columnIndex);
    columnIndex = docCursor.getColumnIndex(columns[1]);
    std::string mime_type = docCursor.getString(columnIndex);
    columnIndex = docCursor.getColumnIndex(columns[2]);
    std::string doc_id = docCursor.getString(columnIndex);
    columnIndex = docCursor.getColumnIndex(columns[3]);
    int64_t size = docCursor.getLong(columnIndex);

    CFileItemPtr pItem(new CFileItem(disp_name));
    pItem->m_dwSize = size;
    if ((pItem->m_bIsFolder = (mime_type == CJNIDocument::MIME_TYPE_DIR)))
      pItem->m_dwSize = -1;
    CJNIURI uri = CJNIDocumentsContract::buildDocumentUriUsingTree(childrenUri, doc_id);

    pItem->SetPath(uri.toString());
    pItem->SetLabel(disp_name);
    //        pItem->SetArt("thumb", path+".png");
    items.Add(pItem);

    CLog::Log(LOGDEBUG, "CAndroidContentDirectory::GetDirectory added: %s(%s | %s)", disp_name.c_str(), doc_id.c_str(), uri.toString().c_str());
  }
  docCursor.close();

  return true;
}
