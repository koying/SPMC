package org.xbmc.kodi;

import java.io.File;
import java.io.FileInputStream;
import java.util.Properties;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.util.Log;

public class XBMCBroadcastReceiver extends BroadcastReceiver
{
  native void _onReceive(Intent intent);

  private static final String TAG = "KodiReceiver";

  @Override
  public void onReceive(Context context, Intent intent)
  {
    Log.d("XBMCBroadcastReceiver", "Received Intent");
    if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction()))
    {
      File fProp = new File(Environment.getExternalStorageDirectory()
          .getAbsolutePath() + "/xbmc_env.properties");
      if (!fProp.exists())
        return;

      Log.i(TAG, "Loading xbmc_env.properties");
      try
      {
        Properties sysProp = new Properties(System.getProperties());
        FileInputStream xbmcenvprop = new FileInputStream(fProp);
        sysProp.load(xbmcenvprop);
        System.setProperties(sysProp);
      } catch (Exception e)
      {
        Log.e(TAG, "Error loading xbmc_env.properties (" + e.getMessage() + ")");
        return;
      }
      
      String sXbmcAutostart = System.getProperty("xbmc.autostart", "");
      if (sXbmcAutostart.equalsIgnoreCase("yes"))
      {
        // Run Kodi
        Intent i = new Intent();
        PackageManager manager = context.getPackageManager();
        i = manager.getLaunchIntentForPackage("org.xbmc.kodi");
        i.addCategory(Intent.CATEGORY_LAUNCHER);
        context.startActivity(i);
      }
    }
    else
    {
      try {
        _onReceive(intent);
      } catch (UnsatisfiedLinkError e) {
        Log.e(TAG, "Native not registered");
      }
    }
  }
}
