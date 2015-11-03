/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDInputStreamMpegDash.h"
#include "utils/XBMCTinyXML.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"

CDVDInputStreamMpegDashComponent::CDVDInputStreamMpegDashComponent(const std::vector<std::string>& segments)
  : CDVDInputStreamFile(), m_segments(segments)
{
}

CDVDInputStreamMpegDashComponent::~CDVDInputStreamMpegDashComponent()
{
  Close();
}

bool CDVDInputStreamMpegDashComponent::Open(const char* strFile, const std::string& content, bool contentLookup)
{
  m_cSegId = 0;
  m_base = strFile;
  if (m_segments.empty())
    return false;
  return CDVDInputStreamFile::Open(URIUtils::AddFileToFolder(m_base,m_segments.front()).c_str(),content,contentLookup);
}

void CDVDInputStreamMpegDashComponent::Close()
{
  CDVDInputStreamFile::Close();
}

int CDVDInputStreamMpegDashComponent::Read(uint8_t* buf, int buf_size)
{
  if (buf_size == 0)
    return 0;


  ssize_t read = CDVDInputStreamFile::IsEOF()?0:CDVDInputStreamFile::Read(buf, buf_size);

  if (read < 0)
    return read;

  if (read == 0) // assume eof - open next segment
  {
    ++m_cSegId;
    if (m_cSegId >= m_segments.size())
      return 0;
    CDVDInputStreamFile::Close();
    if (!CDVDInputStreamFile::Open(URIUtils::AddFileToFolder(m_base,m_segments[m_cSegId]).c_str(),"",false))
      return -1;

    read = CDVDInputStreamFile::Read(buf, buf_size);
  }

  return read;
}

int64_t CDVDInputStreamMpegDashComponent::Seek(int64_t offset, int whence)
{
  if (whence == SEEK_POSSIBLE)
    return 0;
  else
    return -1;
}

bool CDVDInputStreamMpegDashComponent::SeekTime(int iTimeInMsec)
{
  return false;
}

bool CDVDInputStreamMpegDashComponent::IsEOF()
{
  return m_cSegId == m_segments.size()-1 && CDVDInputStreamFile::IsEOF();
}

int64_t CDVDInputStreamMpegDashComponent::GetLength()
{
  return -1;
}

bool CDVDInputStreamMpegDashComponent::Pause(double dTime)
{
  return true;
}

CDVDInputStreamMpegDash::CDVDInputStreamMpegDash()
  : CDVDInputStream(DVDSTREAM_TYPE_DASH)
{
}

CDVDInputStreamMpegDash::~CDVDInputStreamMpegDash()
{
  Close();
}

bool CDVDInputStreamMpegDash::IsEOF()
{
  return false;
}

bool CDVDInputStreamMpegDash::Open(const char* strFile, const std::string& content, bool contentLookup)
{
  if (!CDVDInputStream::Open(strFile, content, contentLookup))
    return false;

  CXBMCTinyXML doc;
  doc.LoadFile(strFile);

  if (!doc.RootElement() || strcmp(doc.RootElement()->Value(), "MPD"))
    return false;

  const TiXmlElement* child = doc.RootElement()->FirstChildElement("BaseURL");
  if (child && child->FirstChild())
    m_mpd.base = child->FirstChild()->Value();
  else
  {
    CURL url(m_url);
    std::string filename = URIUtils::GetDirectory(m_url.GetFileName());
    url.SetFileName(filename);
    m_mpd.base =  url.Get();
  }

  child = doc.RootElement()->FirstChildElement("Period");
  if (!child)
    return false;

  const TiXmlElement* aset = child->FirstChildElement("AdaptationSet");
  while (aset)
  {
    int id = -1;
    const TiXmlElement* child2 = aset->FirstChildElement("ContentComponent");
    if (child2 && child2->Attribute("id"))
      id = strtol(child2->Attribute("id"), nullptr, 10);

    child = aset->FirstChildElement("Representation");
    while (child)
    {
      int id2(id);
      if (id2 == -1)
        id2 = strtol(child->Attribute("id"), nullptr, 10);

      m_mpd.sets[id2].resize(m_mpd.sets[id].size()+1);
      if (child->Attribute("bandwidth"))
        m_mpd.sets[id2].back().bandwidth = strtol(child->Attribute("bandwidth"), nullptr, 10);
      const TiXmlElement* slist = child->FirstChildElement("SegmentList");
      if (slist)
      {
        if (slist->Attribute("duration"))
          m_mpd.sets[id2].back().duration = strtol(slist->Attribute("duration"), nullptr, 10);
        const TiXmlElement* init = slist->FirstChildElement("Initialization");
        if (init && init->Attribute("sourceURL"))
          m_mpd.sets[id2].back().segments.push_back(init->Attribute("sourceURL"));
        const TiXmlElement* seg = slist->FirstChildElement("SegmentURL");
        while (seg)
        {
          if (seg->Attribute("media"))
            m_mpd.sets[id2].back().segments.push_back(seg->Attribute("media"));
          seg = seg->NextSiblingElement();
        }
      }
      child = child->NextSiblingElement();
    }
    aset = aset->NextSiblingElement();
  }

  if (m_mpd.sets.empty())
    return false;

  std::vector<std::pair<size_t, size_t>> bw;
  bw.reserve(m_mpd.sets.size());
  for (auto& it : m_mpd.sets)
  {
    size_t s = 0;
    for (auto& it2 : it.second)
      s += it2.bandwidth;

    bw.push_back(std::make_pair(s, it.first));
  }
  std::sort(bw.begin(), bw.end(),
            [](const std::pair<size_t, size_t>& a, const std::pair<size_t, size_t>& b)
            {
              return a.first < b.first;
            });

  std::cout << "Found MPD with base path: " << m_mpd.base << std::endl;
  for (auto& it : bw)
    std::cout << "\t Representation with bw " << it.first << std::endl;
//               << ", segment duration " << m_mpd.sets[it.second].first().duration
//               << ", " << m_mpd.sets[it.second].first().segments.size() << " segments" << std::endl;

  int maxbw = CSettings::GetInstance().GetInt("network.bandwidth")*1000/8;
  if (maxbw == 0)
    maxbw = bw.back().first+1;

  m_crepId = 0;
  std::cout << "maxbw is " << maxbw << std::endl;
  while (bw[m_crepId].first < maxbw && m_crepId+1 < bw.size()) {
    std::cout << "skipping since bw is " << bw[m_crepId].first << std::endl;
    ++m_crepId;
  }

  std::cout << "Using representation " << m_crepId << ", bandwidth "
            << bw[m_crepId].first << std::endl;

  std::cout << "we got " << m_mpd.sets.size() << std::endl;
  for (size_t i=1; i < m_mpd.sets[bw[m_crepId].second].size();++i)
  {
    std::shared_ptr<CDVDInputStreamMpegDashComponent> 
      comp(new CDVDInputStreamMpegDashComponent(m_mpd.sets[bw[m_crepId].second][i].segments));

    if (!comp->Open(m_mpd.base.c_str(),content,contentLookup))
    {
      m_streams.clear();
      return false;
    }
    m_streams.push_back(comp);
  }

  return true;
}

// close file and reset everything
void CDVDInputStreamMpegDash::Close()
{
  for (auto& it : m_streams)
    it->Close();
  m_mpd.base.clear();
  m_mpd.sets.clear();
}

int CDVDInputStreamMpegDash::Read(uint8_t* buf, int buf_size)
{
  if (m_streams.empty())
    return -1;
  return m_streams.front()->Read(buf, buf_size); // TODO
}

int64_t CDVDInputStreamMpegDash::Seek(int64_t offset, int whence)
{
  if (m_streams.empty())
    return -1;
  return m_streams.front()->Seek(offset, whence);
}

bool CDVDInputStreamMpegDash::SeekTime(int iTimeInMsec)
{
  if (m_streams.empty())
    return false;
  return m_streams.front()->SeekTime(iTimeInMsec);
}

int64_t CDVDInputStreamMpegDash::GetLength()
{
  return -1;
}

bool CDVDInputStreamMpegDash::Pause(double dTime)
{
  return true;
}
