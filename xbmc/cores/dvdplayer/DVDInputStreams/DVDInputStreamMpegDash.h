#pragma once
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

#include "DVDInputStreamFile.h"
#include "filesystem/File.h"

//! \brief Wraps a multi-segment stream from MPEG DASH.
class CDVDInputStreamMpegDashComponent : public CDVDInputStreamFile
{
  public:
    //! \brief Constructor.
    //! \param[in] segments List of segment URLs
    //! \details The segment URLS are relative to the base path which is passed in Open().
    //!           First entry should be the initialization file.
    CDVDInputStreamMpegDashComponent(const std::vector<std::string>& segments);

    //! \brief Destructor.
    virtual ~CDVDInputStreamMpegDashComponent();

    //! \brief Open a segmented stream.
    //! \param[in] strFile Base path of segment stream.
    virtual bool    Open(const char* strFile, const std::string &content, bool contentLookup);

    //! \brief Close input stream
    virtual void    Close();

    //! \brief Read data from stream
    virtual int     Read(uint8_t* buf, int buf_size);

    //! \brief Seeek in stream
    virtual int64_t Seek(int64_t offset, int whence);

    //! \brief Seek to a time position in stream
    bool            SeekTime(int iTimeInMsec);

    //! \brief
    bool            CanSeek()  { return false; }
    bool            CanPause() { return false; }
    //! \brief Pause stream
    virtual bool    Pause(double dTime);
    //! \brief Return true if we have reached EOF
    virtual bool    IsEOF();

    //! \brief Get length of input data
    virtual int64_t GetLength();
  protected:
    size_t      m_cSegId; //!< Current segment ID
    std::string m_base;
    std::vector<std::string> m_segments; //!< Vector of segment URLS (relative to the basepath)
};

//! \brief Input stream class for MPEG DASH.
class CDVDInputStreamMpegDash
  : public CDVDInputStream
  , public CDVDInputStream::ISeekTime
  , public CDVDInputStream::ISeekable
{
public:
  //! \brief Default constructor
  CDVDInputStreamMpegDash();

  //! \brief Destructor.
  virtual ~CDVDInputStreamMpegDash();

  //! \brief Open a MPD file
  virtual bool    Open(const char* strFile, const std::string &content, bool contentLookup);

  //! \brief Close input stream
  virtual void    Close();

  //! \brief Read data from stream
  virtual int     Read(uint8_t* buf, int buf_size);

  //! \brief Seeek in stream
  virtual int64_t Seek(int64_t offset, int whence);

  //! \brief Seek to a time position in stream
  bool            SeekTime(int iTimeInMsec);

  //! \brief
  bool            CanSeek()  { return false; }
  bool            CanPause() { return false; }
  //! \brief Pause stream
  virtual bool    Pause(double dTime);
  //! \brief Return true if we have reached EOF
  virtual bool    IsEOF();

  //! \brief Get length of input data
  virtual int64_t GetLength();
protected:
  //! \brief In-memory representation of an MPD file
  struct MPD
  {
    //! \brief Structure describing an adaptation set
    struct Representation
    {
      size_t bandwidth; //!< Bandwidth required by stream
      size_t duration; //!< Duration between segments
      std::vector<std::string> segments; //!< Segment urls
    };

    std::string base; //!< Base path for segments
    std::map<size_t, std::vector<Representation>> sets; //!< Adaptation sets available. Maps from stream ID
  };

  MPD        m_mpd; //!< In-memory representation of opened MPD file
  size_t     m_crepId; //!< Current representation ID used.
  std::vector<std::shared_ptr<CDVDInputStreamMpegDashComponent>> m_streams; //!< Input streams for current representation.
};
