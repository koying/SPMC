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
#include "utils/log.h"

CEmuFileWrapper g_emuFileWrapper;

EmuFileObject CEmuFileWrapper::m_files[MAX_EMULATED_FILES];

constexpr bool isValidFilePtr(FILE* f)
{
  return (f != nullptr);
}

CEmuFileWrapper::CEmuFileWrapper()
{
  // since we always use dlls we might just initialize it directly
  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    memset(&m_files[i], 0, sizeof(EmuFileObject));
    m_files[i].used = false;
    m_files[i].fd = -1;
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
      m_files[i].used = false;
      m_files[i].fd = -1;
    }
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
      object->fd = (i + FILE_WRAPPER_OFFSET);
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
  m_files[i].used = false;
  m_files[i].fd = -1;
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
  CLog::Log(LOGDEBUG, "%s - enter", __PRETTY_FUNCTION__);
  if (isValidFilePtr(stream))
  {
    for (int i = 0; i < MAX_EMULATED_FILES; i++)
    {
      if (&(m_files[i].file_emu) == stream)
      {
        auto object = GetFileObjectByDescriptor(m_files[i].fd);
        if (object != nullptr && object->used)
        {
          return object->file_xbmc;
        }
      }
    }
  }
  CLog::Log(LOGDEBUG, "%s - exit", __PRETTY_FUNCTION__);
  return nullptr;
}

int CEmuFileWrapper::GetDescriptorByStream(FILE* stream)
{
  CLog::Log(LOGDEBUG, "%s - enter", __PRETTY_FUNCTION__);
  if (isValidFilePtr(stream))
  {
    for (int i = 0; i < MAX_EMULATED_FILES; i++)
    {
      if (&m_files[i].file_emu == stream)
      {
        int f = m_files[i].fd - FILE_WRAPPER_OFFSET;
        if (f >= 0 && f < MAX_EMULATED_FILES)
        {
          return f + FILE_WRAPPER_OFFSET;
        }
      }
    }
  }
  CLog::Log(LOGDEBUG, "%s - exit", __PRETTY_FUNCTION__);
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
    for (int i = 0; i < MAX_EMULATED_FILES; i++)
    {
      if (&m_files[i].file_emu == stream)
        return DescriptorIsEmulatedFile(m_files[i].fd);
    }
  }
  return false;
}
