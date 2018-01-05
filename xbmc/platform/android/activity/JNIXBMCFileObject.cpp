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

#include "JNIXBMCFileObject.h"

#include <androidjni/jutils-details.hpp>

#include "CompileInfo.h"
#include "utils/log.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/model/File";

CJNIXBMCFileObject::CJNIXBMCFileObject()
  : CJNIBase(s_className)
{
  m_object = new_object(s_className);
  m_object.setGlobal();
}

void CJNIXBMCFileObject::setId(int64_t id)
{
  call_method<jhobject>(m_object,
    "setId", "(J)V", id);
}

void CJNIXBMCFileObject::setName(const std::string& str)
{
  call_method<jhstring>(m_object,
    "setName", "(Ljava/lang/String;)V", jcast<jhstring>(str));
}

void CJNIXBMCFileObject::setCategory(const std::string& str)
{
  call_method<jhstring>(m_object,
    "setCategory", "(Ljava/lang/String;)V", jcast<jhstring>(str));
}

void CJNIXBMCFileObject::setMediatype(const std::string& str)
{
  call_method<jhstring>(m_object,
    "setMediatype", "(Ljava/lang/String;)V", jcast<jhstring>(str));
}

void CJNIXBMCFileObject::setUri(const std::string& str)
{
  call_method<jhstring>(m_object,
    "setUri", "(Ljava/lang/String;)V", jcast<jhstring>(str));
}
