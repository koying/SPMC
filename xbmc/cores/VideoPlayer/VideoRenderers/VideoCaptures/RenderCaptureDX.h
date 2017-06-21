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

#pragma once

#include "../RenderCapture.h"
#include "guilib/D3DResource.h"

class CRenderCaptureDX : public CBaseRenderCapture, public ID3DResource
{
  public:
    CRenderCaptureDX();
    ~CRenderCaptureDX();

    int  GetCaptureFormat();

    void BeginRender();
    void EndRender();
    void ReadOut();
    
    virtual void OnDestroyDevice(bool fatal);
    virtual void OnLostDevice();
    virtual void OnCreateDevice() {};

  private:
    void SurfaceToBuffer();
    void CleanupDX();

    ID3D11Texture2D*        m_renderTexture;
    ID3D11RenderTargetView* m_renderSurface;
    ID3D11Texture2D*        m_copySurface;
    ID3D11Query*            m_query;
    unsigned int            m_surfaceWidth;
    unsigned int            m_surfaceHeight;
};

class CRenderCapture : public CRenderCaptureDX
{
  public:
    CRenderCapture() {};
};
