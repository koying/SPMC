/*
*      Copyright (C) 2016-2016 peak3d
*      http://www.peak3d.de
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
*  <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include "../common/AdaptiveTree.h"

#include "threads/Thread.h"

namespace adaptive
{

  class SmoothTree : public AdaptiveTree, public IRunnable
  {
  public:
    SmoothTree(bool main=true);
    virtual ~SmoothTree();
    virtual bool open_manifest(const char *url) override;
    virtual bool write_manifest_data(const char *buffer, size_t buffer_size) override;

    void parse_protection();

    enum
    {
      SSMNODE_SSM = 1 << 0,
      SSMNODE_PROTECTION = 1 << 1,
      SSMNODE_STREAMINDEX = 1 << 2,
      SSMNODE_PROTECTIONHEADER = 1 << 3,
      SSMNODE_PROTECTIONTEXT = 1 << 4
    };

    uint64_t pts_helper_;
    
    // IRunnable interface
  public:
    virtual void Run() override;

  protected:
    CThread *m_refreshThread;
    bool m_stopRefreshing;
    bool m_main;
  };

}
