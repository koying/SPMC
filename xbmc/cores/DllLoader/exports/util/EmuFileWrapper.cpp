/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "EmuFileWrapper.h"
#include "filesystem/File.h"
#include "threads/SingleLock.h"

CEmuFileWrapper g_emuFileWrapper;

namespace
{

#if defined(TARGET_WINDOWS) && (_MSC_VER >= 1900)
constexpr kodi_iobuf* FileDescriptor(FILE& f)
{
  return static_cast<kodi_iobuf*>(f._Placeholder);
}

constexpr bool isValidFilePtr(FILE* f)
{
  return (f != nullptr && f->_Placeholder != nullptr);
}

#else
constexpr FILE* FileDescriptor(FILE& f)
{
  return &f;
}

constexpr bool isValidFilePtr(FILE* f)
{
  return (f != nullptr);
}
#endif
}
CEmuFileWrapper::CEmuFileWrapper()
{
  // since we always use dlls we might just initialize it directly
  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    memset(&m_files[i], 0, sizeof(EmuFileObject));
    m_files[i].used = false;
#if defined(__ANDROID_API__)
    FileDescriptor(m_files[i].file_emu)->__private[0] = -1;
#else
#if defined(TARGET_WINDOWS) && (_MSC_VER >= 1900)
    m_files[i].file_emu._Placeholder = new kodi_iobuf();
#endif
    FileDescriptor(m_files[i].file_emu)->_file = -1;
#endif
  }
}

CEmuFileWrapper::~CEmuFileWrapper()
{
  CleanUp();
}

void CEmuFileWrapper::CleanUp()
{
  CSingleLock lock(m_criticalSection);
  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    if (m_files[i].used)
    {
      m_files[i].file_xbmc->Close();
      delete m_files[i].file_xbmc;

      if (m_files[i].file_lock)
      {
        delete m_files[i].file_lock;
        m_files[i].file_lock = nullptr;
      }
#if !defined(TARGET_WINDOWS)
      //Don't memset on Windows as it overwrites our pointer
      memset(&m_files[i], 0, sizeof(EmuFileObject));
#endif
      m_files[i].used = false;
#if defined(__ANDROID_API__)
      FileDescriptor(m_files[i].file_emu)->__private[0] = -1;
#else
      FileDescriptor(m_files[i].file_emu)->_file = -1;
#endif
    }
#if defined(TARGET_WINDOWS) && (_MSC_VER >= 1900)
    delete static_cast<kodi_iobuf*>(m_files[i].file_emu._Placeholder);
    m_files[i].file_emu._Placeholder = nullptr;
#endif
  }
}

EmuFileObject* CEmuFileWrapper::RegisterFileObject(XFILE::CFile* pFile)
{
  EmuFileObject* object = nullptr;

  CSingleLock lock(m_criticalSection);

  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    if (!m_files[i].used)
    {
      // found a free location
      object = &m_files[i];
      object->used = true;
      object->file_xbmc = pFile;
#if defined(__ANDROID_API__)
      FileDescriptor(m_files[i].file_emu)->__private[0] = (i + FILE_WRAPPER_OFFSET);
#else
      FileDescriptor(object->file_emu)->_file = (i + FILE_WRAPPER_OFFSET);
#endif
      object->file_lock = new CCriticalSection();
      break;
    }
  }

  return object;
}

void CEmuFileWrapper::UnRegisterFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (! (i >= 0 && i < MAX_EMULATED_FILES))
    return;

  if (!m_files[i].used)
    return;

  CSingleLock lock(m_criticalSection);

  // we assume the emulated function alreay deleted the CFile object
  if (m_files[i].file_lock)
  {
    delete m_files[i].file_lock;
    m_files[i].file_lock = nullptr;
  }
#if !defined(TARGET_WINDOWS)
  //Don't memset on Windows as it overwrites our pointer
  memset(&m_files[i], 0, sizeof(EmuFileObject));
#endif
  m_files[i].used = false;
#if defined(__ANDROID_API__)
  FileDescriptor(m_files[i].file_emu)->__private[0] = -1;
#else
  FileDescriptor(m_files[i].file_emu)->_file = -1;
#endif
}

void CEmuFileWrapper::UnRegisterFileObjectByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
#if defined(__ANDROID_API__)
    return UnRegisterFileObjectByDescriptor(FileDescriptor(*stream)->__private[0]);
#else
    return UnRegisterFileObjectByDescriptor(FileDescriptor(*stream)->_file);
#endif
  }
}

void CEmuFileWrapper::LockFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].used)
    {
      m_files[i].file_lock->lock();
    }
  }
}

bool CEmuFileWrapper::TryLockFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].used)
    {
      return m_files[i].file_lock->try_lock();
    }
  }
  return false;
}

void CEmuFileWrapper::UnlockFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].used)
    {
      m_files[i].file_lock->unlock();
    }
  }
}

EmuFileObject* CEmuFileWrapper::GetFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].used)
    {
      return &m_files[i];
    }
  }
  return nullptr;
}

EmuFileObject* CEmuFileWrapper::GetFileObjectByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
#if defined(__ANDROID_API__)
    return GetFileObjectByDescriptor(FileDescriptor(*stream)->__private[0]);
#else
    return GetFileObjectByDescriptor(FileDescriptor(*stream)->_file);
#endif
  }

  return nullptr;
}

XFILE::CFile* CEmuFileWrapper::GetFileXbmcByDescriptor(int fd)
{
  auto object = GetFileObjectByDescriptor(fd);
  if (object != nullptr && object->used)
  {
    return object->file_xbmc;
  }
  return nullptr;
}

XFILE::CFile* CEmuFileWrapper::GetFileXbmcByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
#if defined(__ANDROID_API__)
    auto object = GetFileObjectByDescriptor(FileDescriptor(*stream)->__private[0]);
#else
    auto object = GetFileObjectByDescriptor(FileDescriptor(*stream)->_file);
#endif
    if (object != nullptr && object->used)
    {
      return object->file_xbmc;
    }
  }
  return nullptr;
}

int CEmuFileWrapper::GetDescriptorByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
#if defined(__ANDROID_API__)
    int i = FileDescriptor(*stream)->__private[0] - FILE_WRAPPER_OFFSET;
#else
    int i = FileDescriptor(*stream)->_file - FILE_WRAPPER_OFFSET;
#endif
    if (i >= 0 && i < MAX_EMULATED_FILES)
    {
      return i + FILE_WRAPPER_OFFSET;
    }
  }
  return -1;
}

FILE* CEmuFileWrapper::GetStreamByDescriptor(int fd)
{
  auto object = GetFileObjectByDescriptor(fd);
  if (object != nullptr && object->used)
  {
    return &object->file_emu;
  }
  return nullptr;
}

bool CEmuFileWrapper::StreamIsEmulatedFile(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
#if defined(__ANDROID_API__)
    return DescriptorIsEmulatedFile(FileDescriptor(*stream)->__private[0]);
#else
    return DescriptorIsEmulatedFile(FileDescriptor(*stream)->_file);
#endif
  }
  return false;
}
