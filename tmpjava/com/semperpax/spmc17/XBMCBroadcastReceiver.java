package com.semperpax.spmc17;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.util.Log;

public class XBMCBroadcastReceiver extends BroadcastReceiver
{
  native void _onReceive(Intent intent);

  private static final String TAG = "SPMCReceiver";

  @Override
  public void onReceive(Context context, Intent intent)
  {
    if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction()))
    {
      if (XBMCProperties.getBoolProperty("spmc.autostart"))
      {
        // Run SPMC
        Intent i = new Intent();
        PackageManager manager = context.getPackageManager();
        i = manager.getLaunchIntentForPackage("com.semperpax.spmc17");
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
