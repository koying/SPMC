package com.semperpax.spmc;

import android.app.NativeActivity;
import android.content.Intent;
import android.os.Bundle;
import android.media.AudioManager;
import android.content.Context;
import android.util.Log;

public class Main extends NativeActivity implements AudioManager.OnAudioFocusChangeListener
{
  native void _onNewIntent(Intent intent);
  
  private static final String TAG = "Main";

  public Main() 
  {
    super();
  }

  @Override
  public void onAudioFocusChange(int focusChange) 
  {
    if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT)
    {
      Log.i(TAG, "Lost audio focus: transient");
    } else if (focusChange == AudioManager.AUDIOFOCUS_GAIN) 
    {
      Log.i(TAG, "Got back audio focus");
    } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS)
    {
      Log.w(TAG, "Lost audio focus");
      AudioManager am = (AudioManager)this.getSystemService(Context.AUDIO_SERVICE);
      am.abandonAudioFocus(this);
    }
  }

  @Override
  public void onCreate(Bundle savedInstanceState) 
  {
    super.onCreate(savedInstanceState);

    AudioManager am = (AudioManager)this.getSystemService(Context.AUDIO_SERVICE);

    int result = am.requestAudioFocus(this,
                                     // Use the music stream.
                                     AudioManager.STREAM_MUSIC,
                                     // Request permanent focus.
                                     AudioManager.AUDIOFOCUS_GAIN);

    if (result != AudioManager.AUDIOFOCUS_REQUEST_GRANTED)
    {
      Log.w(TAG, "Cannot get audio focus");
    }
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
    
    AudioManager am = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
    am.abandonAudioFocus(this);
    
    super.onDestroy();
  }
}
