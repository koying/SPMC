package org.xbmc.kodi;

import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.app.Activity;
import android.app.SearchManager;
import android.util.Log;

public class XBMCSearchableActivity extends Activity
{

  private static final String TAG = "KodiSearch";

  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    this.setContentView(R.layout.search_result_layout);

    handleIntent(getIntent());
  }

  public void onNewIntent(Intent intent)
  {
    if (Intent.ACTION_SEARCH.equals(intent.getAction())) 
    {
      setIntent(intent);
      handleIntent(intent);
    }
  }

  private void handleIntent(Intent intent)
  {
    String query = intent.getStringExtra(SearchManager.QUERY);
    Log.d(TAG, "NEW INTENT: " + intent.getAction() + "; Q=" + query);

    String movieId = intent.getData().getLastPathSegment();
    Cursor c = getContentResolver()
        .query(
            Uri.parse("content://net.floating_systems.kodiplay.VideoContentProvider/id/"
                + movieId), null, null, null, null);

    if (c.getCount() > 0)
    {
      c.moveToFirst();
      String moviePath = c.getString(c.getColumnIndex("COLUMN_FULL_PATH"));

      c.close();

      try
      {
        Intent intent2 = new Intent(Intent.ACTION_VIEW, Uri.parse(moviePath));
        Uri videoUri = Uri.parse(moviePath);

        intent2.setDataAndType(videoUri, "video/*");

        // intent2.putExtra("playoffset",600.0f); //DOESNTWORK

        intent2.setPackage("org.xbmc.kodi");
        startActivity(intent2);
      } catch (Exception e)
      {
        Log.e(TAG, "ERROR EXECUTING KODI", e);
      }

      finish();
    }

  }
}