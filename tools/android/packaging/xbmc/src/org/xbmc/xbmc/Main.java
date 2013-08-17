package tv.ouya.xbmc;

import android.app.NativeActivity;
import android.content.Intent;
import android.os.Bundle;

public class Main extends NativeActivity 
{
  native void _onNewIntent(Intent intent);
  public Main() 
  {
    super();
  }

  @Override
  public void onCreate(Bundle savedInstanceState) 
  {
    super.onCreate(savedInstanceState);
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
  }

  @Override
  public void onDestroy() {
    new Thread() {
      public void run() {
        try
        {
          sleep(1000);
          android.os.Process.killProcess(android.os.Process.myPid());
        } catch (Exception e) {}
      }
    }.start();
    super.onDestroy();
  }
}
