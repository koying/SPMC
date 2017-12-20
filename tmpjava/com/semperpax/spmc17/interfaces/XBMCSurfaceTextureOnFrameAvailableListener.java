package com.semperpax.spmc17.interfaces;

import android.graphics.SurfaceTexture;
import android.graphics.SurfaceTexture.OnFrameAvailableListener;

public class XBMCSurfaceTextureOnFrameAvailableListener implements OnFrameAvailableListener
{
  native void _onFrameAvailable(SurfaceTexture surfaceTexture);

  @Override
  public void onFrameAvailable(SurfaceTexture surfaceTexture)
  {
  _onFrameAvailable(surfaceTexture);
  }
}
