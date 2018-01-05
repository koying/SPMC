/*
 *      Copyright (C) 2018 Christian Browet
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

#include "JNIXBMCFileContent.h"

#include "JNIXBMCFileObject.h"
#include "Util.h"
#include "utils/FileUtils.h"
#include "URL.h"
#include "filesystem/Directory.h"

#include "CompileInfo.h"
#include "utils/log.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/ArrayList.h>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/content/XBMCFileContentProvider";

static std::list<std::pair<jni::jhobject, CJNIXBMCFileContent*>> s_object_map;

static void add_instance(const jni::jhobject& o, CJNIXBMCFileContent* inst)
{
  s_object_map.push_back(std::pair<jni::jhobject, CJNIXBMCFileContent*>(o, inst));
}

static CJNIXBMCFileContent* find_instance(const jobject& o)
{
  for( auto it = s_object_map.begin(); it != s_object_map.end(); ++it )
  {
    if (it->first == o)
      return it->second;
  }
  return nullptr;
}

static void remove_instance(CJNIXBMCFileContent* inst)
{
  for( auto it = s_object_map.begin(); it != s_object_map.end(); ++it )
  {
    if (it->second == inst)
    {
      s_object_map.erase(it);
      break;
    }
  }
}


CJNIXBMCFileContent::CJNIXBMCFileContent()
  : CJNIBase(s_className)
{
}

CJNIXBMCFileContent::~CJNIXBMCFileContent()
{
}

void CJNIXBMCFileContent::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_createNativeInstance", "()V", (void*)&CJNIXBMCFileContent::_createNativeInstance},
      {"_releaseNativeInstance", "()V", (void*)&CJNIXBMCFileContent::_releaseNativeInstance},
      {"_getDirectoryContent", "(Ljava/lang/String;)Ljava/util/ArrayList;", (void*)&CJNIXBMCFileContent::_getDirectoryContent},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCFileContent::_createNativeInstance(JNIEnv* env, jobject thiz)
{
  (void)env;

  add_instance(jhobject::fromJNI(thiz), new CJNIXBMCFileContent());
}

void CJNIXBMCFileContent::_releaseNativeInstance(JNIEnv* env, jobject thiz)
{
  (void)env;

  CJNIXBMCFileContent *inst = find_instance(thiz);
  if (inst)
  {
    remove_instance(inst);
    delete inst;
  }
}

jobject CJNIXBMCFileContent::_getDirectoryContent(JNIEnv* env, jobject thiz, jstring url)
{
  (void)env;

  CJNIArrayList<CJNIXBMCFileObject> al;

  std::string strPath = jcast<std::string>(jhstring(url));
  if (!CFileUtils::RemoteAccessAllowed(strPath))
    return al.get_raw();

  CFileItemList items;

  std::vector<std::string> regexps;
  if (XFILE::CDirectory::GetDirectory(strPath, items))
  {
    for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    {
      if (CUtil::ExcludeFileOrFolder(items[i]->GetPath(), regexps))
        continue;

      if (items[i]->IsSmb())
      {
        CURL url(items[i]->GetPath());
        items[i]->SetPath(url.GetWithoutUserDetails());
      }

    }

  }

  return al.get_raw();
}
