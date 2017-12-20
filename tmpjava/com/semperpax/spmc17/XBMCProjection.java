package com.semperpax.spmc17;

import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.os.Handler;
import android.os.Looper;

import java.io.FileOutputStream;
import java.nio.ByteBuffer;

/**
 * Created by cbro on 9/27/16.
 */
public class XBMCProjection implements ImageReader.OnImageAvailableListener
{
  private Main mActivity;
  private MediaProjection mMediaProjection;
  private VirtualDisplay mVirtualDisplay;
  private ImageReader mImageReader;

  private Handler mHandler;

  private int mWidth;
  private int mHeight;
  private boolean mScreenshotMode;
  private boolean mSaveCaptures;

  private static final String STORE_DIRECTORY = "/sdcard";

  public XBMCProjection(Main act, MediaProjection proj)
  {
    mActivity = act;
    mMediaProjection = proj;
    mSaveCaptures = XBMCProperties.getBoolProperty("xbmc.savecaptures");

        // start capture handling thread
        new Thread() {
            @Override
            public void run() {
                Looper.prepare();
                mHandler = new Handler();
                Looper.loop();
            }
        }.start();
  }

  private VirtualDisplay createVirtualDisplay()
  {
    Point size = new Point();
    mActivity.getWindowManager().getDefaultDisplay().getSize(size);
    return createVirtualDisplay(size.x, size.y);
  }

  private VirtualDisplay createVirtualDisplay(int width, int height)
  {
    Point size = new Point();
    mActivity.getWindowManager().getDefaultDisplay().getSize(size);
    if (size.x != width || size.y != height)
    {
      float dratio = (float)size.x / size.y;  // 1.7
      mWidth = width;
      mHeight = (int) (mWidth / dratio);
    }
    else
    {
      mWidth = width;
      mHeight = height;
    }

    if (mImageReader != null && (mImageReader.getWidth() != mWidth || mImageReader.getHeight() != mHeight))
    {
      mImageReader.close();
      mImageReader = null;
    }
    if (mImageReader == null)
    {
      mImageReader = ImageReader.newInstance(mWidth, mHeight, PixelFormat.RGBA_8888, 4);
      mImageReader.setOnImageAvailableListener(this, mHandler);
    }
    return mMediaProjection.createVirtualDisplay("rendercapture",
            mWidth, mHeight, mActivity.getResources().getDisplayMetrics().densityDpi,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY,
            mImageReader.getSurface(), null, mHandler);
  }

  @Override
  public void onImageAvailable(ImageReader reader)
  {
    try
    {
      final Image img = mImageReader.acquireNextImage();
      if (img == null)
        return;

      if (mScreenshotMode)
      {
        mActivity._onScreenshotAvailable(img);
        stopCapture();
      }
      else
      {
        if (mSaveCaptures)
        {
          Long tsLong = System.currentTimeMillis();
          String ts = tsLong.toString();
          String fn = "cap_" + ts;

          Image.Plane[] planes = img.getPlanes();
          ByteBuffer buffer = planes[0].getBuffer();
          int pixelStride = planes[0].getPixelStride();
          int rowStride = planes[0].getRowStride();
          int rowPadding = rowStride - pixelStride * mWidth;

          // create bitmap
          Bitmap bitmap = Bitmap.createBitmap(mWidth + rowPadding / pixelStride, mHeight, Bitmap.Config.ARGB_8888);
          bitmap.copyPixelsFromBuffer(buffer);

          // write bitmap to a file
          FileOutputStream fos = new FileOutputStream(STORE_DIRECTORY + "/" + fn + ".png");
          bitmap.compress(Bitmap.CompressFormat.JPEG, 100, fos);
        }
        mActivity._onCaptureAvailable(img);
      }
    }
    catch (Exception e)
    {}
  }

  /***************/

  public void takeScreenshot(int width, int height)
  {
    if (mMediaProjection == null)
      return;

    mScreenshotMode = true;
    mVirtualDisplay = createVirtualDisplay(width, height);
  }

  public void takeScreenshot()
  {
    if (mMediaProjection == null)
      return;

    mScreenshotMode = true;
    mVirtualDisplay = createVirtualDisplay();
  }

  public void startCapture(int width, int height)
  {
    if (mMediaProjection == null)
      return;

    if (mVirtualDisplay != null)
      return;

    mScreenshotMode = false;
    mVirtualDisplay = createVirtualDisplay(width, height);
  }

  public void stopCapture()
  {
    if (mVirtualDisplay != null)
    {
      mVirtualDisplay.release();
      mVirtualDisplay = null;
    }
  }

  public void stopProjection()
  {
    mHandler.post(new Runnable()
    {
      @Override
      public void run()
      {
        if (mMediaProjection != null)
        {
          mMediaProjection.stop();
        }
      }
    });
  }

}

