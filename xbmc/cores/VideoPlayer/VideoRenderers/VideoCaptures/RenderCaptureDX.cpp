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

#include "RenderCaptureDX.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "cores/IPlayer.h"
extern "C" {
#include "libavutil/mem.h"
}

CRenderCaptureDX::CRenderCaptureDX()
{
  m_renderTexture = nullptr;
  m_renderSurface = nullptr;
  m_copySurface   = nullptr;
  m_query         = nullptr;
  m_surfaceWidth  = 0;
  m_surfaceHeight = 0;

  g_Windowing.Register(this);
}

CRenderCaptureDX::~CRenderCaptureDX()
{
  CleanupDX();
  av_freep(&m_pixels);

  g_Windowing.Unregister(this);
}

int CRenderCaptureDX::GetCaptureFormat()
{
  return CAPTUREFORMAT_BGRA;
}

void CRenderCaptureDX::BeginRender()
{
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();
  ID3D11Device* pDevice = g_Windowing.Get3D11Device();
  CD3D11_QUERY_DESC queryDesc(D3D11_QUERY_EVENT);

  if (!m_asyncChecked)
  {
    m_asyncSupported = SUCCEEDED(pDevice->CreateQuery(&queryDesc, nullptr));
    if (m_flags & CAPTUREFLAG_CONTINUOUS)
    {
      if (!m_asyncSupported)
        CLog::Log(LOGWARNING, "CRenderCaptureDX: D3D11_QUERY_OCCLUSION not supported, performance might suffer");
      if (!UseOcclusionQuery())
        CLog::Log(LOGWARNING, "CRenderCaptureDX: D3D11_QUERY_OCCLUSION disabled, performance might suffer");
    }
    m_asyncChecked = true;
  }

  HRESULT result;

  if (m_surfaceWidth != m_width || m_surfaceHeight != m_height)
  {
    SAFE_RELEASE(m_renderSurface);
    SAFE_RELEASE(m_copySurface);

    CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_B8G8R8A8_UNORM, m_width, m_height, 1, 1, D3D11_BIND_RENDER_TARGET);
    result = pDevice->CreateTexture2D(&texDesc, nullptr, &m_renderTexture);
    if (FAILED(result))
    {
      CLog::Log(LOGERROR, "CRenderCaptureDX::BeginRender: CreateTexture2D (RENDER_TARGET) failed %s",
                g_Windowing.GetErrorDescription(result).c_str());
      SetState(CAPTURESTATE_FAILED);
      return;
    }

    CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(D3D11_RTV_DIMENSION_TEXTURE2D);
    result = pDevice->CreateRenderTargetView(m_renderTexture, &rtDesc, &m_renderSurface);
    if (FAILED(result))
    {
      CLog::Log(LOGERROR, "CRenderCaptureDX::BeginRender: CreateRenderTargetView failed %s",
        g_Windowing.GetErrorDescription(result).c_str());
      SetState(CAPTURESTATE_FAILED);
      return;
    }

    texDesc.BindFlags = 0;
    texDesc.Usage = D3D11_USAGE_STAGING;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    result = pDevice->CreateTexture2D(&texDesc, nullptr, &m_copySurface);
    if (FAILED(result))
    {
      CLog::Log(LOGERROR, "CRenderCaptureDX::BeginRender: CreateTexture2D (USAGE_STAGING) failed %s",
                g_Windowing.GetErrorDescription(result).c_str());
      SetState(CAPTURESTATE_FAILED);
      return;
    }

    m_surfaceWidth = m_width;
    m_surfaceHeight = m_height;
  }

  if (m_bufferSize != m_width * m_height * 4)
  {
    m_bufferSize = m_width * m_height * 4;
    av_freep(&m_pixels);
    m_pixels = (uint8_t*)av_malloc(m_bufferSize);
  }

  pContext->OMSetRenderTargets(1, &m_renderSurface, nullptr);

  if (m_asyncSupported && UseOcclusionQuery())
  {
    //generate an occlusion query if we don't have one
    if (!m_query)
    {
      result = pDevice->CreateQuery(&queryDesc, &m_query);
      if (FAILED(result))
      {
        CLog::Log(LOGERROR, "CRenderCaptureDX::BeginRender: CreateQuery failed %s",
                  g_Windowing.GetErrorDescription(result).c_str());
        m_asyncSupported = false;
        SAFE_RELEASE(m_query);
      }
    }
  }
  else
  {
    //don't use an occlusion query, clean up any old one
    SAFE_RELEASE(m_query);
  }
}

void CRenderCaptureDX::EndRender()
{
  // send commands to the GPU queue
  g_Windowing.FinishCommandList();
  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();

  pContext->CopyResource(m_copySurface, m_renderTexture);

  if (m_query)
  {
    pContext->End(m_query);
  }

  if (m_flags & CAPTUREFLAG_IMMEDIATELY)
    SurfaceToBuffer();
  else
    SetState(CAPTURESTATE_NEEDSREADOUT);
}

void CRenderCaptureDX::ReadOut()
{
  if (m_query)
  {
    //if the result of the occlusion query is available, the data is probably also written into m_copySurface
    HRESULT result = g_Windowing.GetImmediateContext()->GetData(m_query, nullptr, 0, 0);
    if (SUCCEEDED(result))
    {
      if (S_OK == result)
        SurfaceToBuffer();
    }
    else
    {
      CLog::Log(LOGERROR, "CRenderCaptureDX::ReadOut: GetData failed");
      SurfaceToBuffer();
    }
  }
  else
  {
    SurfaceToBuffer();
  }
}

void CRenderCaptureDX::SurfaceToBuffer()
{
  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();

  D3D11_MAPPED_SUBRESOURCE lockedRect;
  if (pContext->Map(m_copySurface, 0, D3D11_MAP_READ, 0, &lockedRect) == S_OK)
  {
    //if pitch is same, do a direct copy, otherwise copy one line at a time
    if (lockedRect.RowPitch == m_width * 4)
    {
      memcpy(m_pixels, lockedRect.pData, m_width * m_height * 4);
    }
    else
    {
      for (unsigned int y = 0; y < m_height; y++)
        memcpy(m_pixels + y * m_width * 4, (uint8_t*)lockedRect.pData + y * lockedRect.RowPitch, m_width * 4);
    }
    pContext->Unmap(m_copySurface, 0);
    SetState(CAPTURESTATE_DONE);
  }
  else
  {
    CLog::Log(LOGERROR, "CRenderCaptureDX::SurfaceToBuffer: locking m_copySurface failed");
    SetState(CAPTURESTATE_FAILED);
  }
}

void CRenderCaptureDX::OnLostDevice()
{
  CleanupDX();
  SetState(CAPTURESTATE_FAILED);
}

void CRenderCaptureDX::OnDestroyDevice(bool fatal)
{
  CleanupDX();
  SetState(CAPTURESTATE_FAILED);
}

void CRenderCaptureDX::CleanupDX()
{
  SAFE_RELEASE(m_renderSurface);
  SAFE_RELEASE(m_renderTexture);
  SAFE_RELEASE(m_copySurface);
  SAFE_RELEASE(m_query);

  m_surfaceWidth = 0;
  m_surfaceHeight = 0;
}

