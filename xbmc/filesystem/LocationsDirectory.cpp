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

#include "LocationsDirectory.h"

#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/PVRDirectory.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "guilib/TextureManager.h"
#if defined(TARGET_ANDROID)
#include "android/activity/XBMCApp.h"
#endif

using namespace XFILE;

CLocationsDirectory::CLocationsDirectory(void)
{
}


CLocationsDirectory::~CLocationsDirectory(void)
{
}

bool CLocationsDirectory::GetDirectory(const CStdString& strPath, CFileItemList &pItems)
{
  CURL url(strPath);
  m_type = url.GetUserName();

  if (url.GetHostName().empty())
  {
    VECSOURCES loclocations;
    g_mediaManager.GetLocalDrives(loclocations);
    for (int i=0; i<loclocations.size(); ++i)
    {
      url.SetHostName(loclocations[i].strPath);
      loclocations[i].strPath = url.Get();
      CFileItemPtr pItem(new CFileItem(loclocations[i]));
      pItem->SetIconImage("DefaultHardDisk.png");
      pItem->m_bIsShareOrDrive = true;
      pItems.Add(pItem);
    }

    VECSOURCES removablelocations;
    g_mediaManager.GetRemovableDrives(removablelocations);
    for (int i=0; i<removablelocations.size(); ++i)
    {
      url.SetHostName(removablelocations[i].strPath);
      removablelocations[i].strPath = url.Get();
      CFileItemPtr pItem(new CFileItem(removablelocations[i]));
      pItem->SetIconImage("DefaultRemovableDisk.png");
      pItem->m_bIsShareOrDrive = true;
      pItems.Add(pItem);
    }

    VECSOURCES netlocations;
    g_mediaManager.GetNetworkLocations(netlocations);
    for (int i=0; i<netlocations.size(); ++i)
    {
      url.SetHostName(netlocations[i].strPath);
      netlocations[i].strPath = url.Get();
      CFileItemPtr pItem(new CFileItem(netlocations[i]));
      pItem->SetIconImage("DefaultNetwork.png");
      pItem->m_bIsShareOrDrive = netlocations[i].m_ignore;
      pItems.Add(pItem);
    }

    VECSOURCES autosources;
    g_mediaManager.GetAutoSources(autosources);
    for (int i=0; i<autosources.size(); ++i)
    {
      url.SetHostName(autosources[i].strPath);
      autosources[i].strPath = url.Get();
      CFileItemPtr pItem(new CFileItem(autosources[i]));

      CStdString strIcon;
      // We have the real DVD-ROM, set icon on disktype
      if (autosources[i].m_iDriveType == CMediaSource::SOURCE_TYPE_DVD && autosources[i].m_strThumbnailImage.empty())
      {
        CUtil::GetDVDDriveIcon( autosources[i].strPath, strIcon );
        // CDetectDVDMedia::SetNewDVDShareUrl() caches disc thumb as special://temp/dvdicon.tbn
        CStdString strThumb = "special://temp/dvdicon.tbn";
        if (XFILE::CFile::Exists(strThumb))
          pItem->SetArt("thumb", strThumb);
      }
      else if (pItem->IsISO9660())
        strIcon = "DefaultDVDRom.png";
      else if (pItem->IsDVD())
        strIcon = "DefaultDVDRom.png";
      else if (pItem->IsCDDA())
        strIcon = "DefaultCDDA.png";
      else
        strIcon = "DefaultHardDisk.png";
      pItem->SetIconImage(strIcon);
      pItem->m_bIsShareOrDrive = autosources[i].m_ignore;
      pItems.Add(pItem);
    }

    if (m_type == "music")
    {
  #if defined(TARGET_ANDROID)
      // add the default android music directory
      std::string path;
      if (CXBMCApp::GetExternalStorage(path, "music") && !path.empty() && CFile::Exists(path))
      {
        url.SetHostName(path);
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(20240));
        pItem->SetIconImage("DefaultAudio.png");
        pItems.Add(pItem);
      }
  #endif

      {
        // add the music playlist location
        url.SetHostName("special://musicplaylists/");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(20011));
        pItem->SetIconImage("DefaultPlaylist.png");
        pItems.Add(pItem);
      }

      {
        url.SetHostName("sap://");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        //TODO localize
        pItem->SetLabel("SAP Streams");
        pItem->SetIconImage("DefaultNetwork.png");
        pItems.Add(pItem);
      }

      if (CSettings::Get().GetString("audiocds.recordingpath") != "")
      {
        url.SetHostName("special://recordings/");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(21883));
        pItem->SetIconImage("DefaultFolder.png");
        pItems.Add(pItem);
      }
    }
    else if (m_type == "video")
    {
  #if defined(TARGET_ANDROID)
      // add the default android video directory
      std::string path;
      if (CXBMCApp::GetExternalStorage(path, "videos") && !path.empty() && CFile::Exists(path))
      {
        url.SetHostName(path);
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(20241));
        pItem->SetIconImage("DefaultFolder.png");
        pItems.Add(pItem);
      }
  #endif

      {
        // add the video playlist location
        url.SetHostName("special://videoplaylists/");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(20012));
        pItem->SetIconImage("DefaultPlaylist.png");
        pItems.Add(pItem);
      }

      {
        url.SetHostName("rtv://*/");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        //TODO localize
        pItem->SetLabel("ReplayTV Devices");
        pItem->SetIconImage("DefaultNetwork.png");
        pItems.Add(pItem);
      }

      {
        url.SetHostName("hdhomerun://");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        //TODO localize
        pItem->SetLabel("HDHomerun Devices");
        pItem->SetIconImage("DefaultNetwork.png");
        pItems.Add(pItem);
      }

      {
        url.SetHostName("sap://");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        //TODO localize
        pItem->SetLabel("SAP Streams");
        pItem->SetIconImage("DefaultNetwork.png");
        pItems.Add(pItem);
      }

      // add the recordings dir as needed
      if (CPVRDirectory::HasRecordings())
      {
        url.SetHostName("pvr://recordings/");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(19017));
        pItem->SetIconImage("DefaultVideo.png");
        pItems.Add(pItem);
      }
    }
    else if (m_type == "pictures")
    {
  #if defined(TARGET_ANDROID)
      // add the default android picture directories
      std::string path;
      if (CXBMCApp::GetExternalStorage(path, "pictures") && !path.empty() && CFile::Exists(path))
      {
        url.SetHostName(path);
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(20242));
        pItem->SetIconImage("DefaultPicture.png");
        pItems.Add(pItem);
      }

      path.clear();
      if (CXBMCApp::GetExternalStorage(path, "photos") && !path.empty() &&  CFile::Exists(path))
      {
        url.SetHostName(path);
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(20243));
        pItem->SetIconImage("DefaultPicture.png");
        pItems.Add(pItem);
      }
  #endif

      if (CSettings::Get().GetString("debug.screenshotpath") != "")
      {
        url.SetHostName("special://screenshots/");
        CFileItemPtr pItem(new CFileItem(url.Get(), true));
        pItem->SetLabel(g_localizeStrings.Get(20008));
        pItem->SetIconImage("DefaultPicture.png");
        pItems.Add(pItem);
      }
    }
    else if (m_type == "programs")
    {
    }
  }
  else
    return CDirectory::GetDirectory(url.GetHostName(), pItems);
  return true;;
}

bool CLocationsDirectory::Exists(const char* strPath)
{
  return true;
}
