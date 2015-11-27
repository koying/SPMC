package org.xbmc.kodi;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;

public class XBMCMediaContentProvider extends ContentProvider
{
  private static String TAG = "kodi";
  
  public static String AUTHORITY = "org.xbmc.kodi";
  public static String AUTHORITY_IMAGE = AUTHORITY + ".media";

  private XBMCJsonRPC mJsonRPC = null;

  @Override
  public int delete(Uri arg0, String arg1, String[] arg2)
  {
    // TODO Auto-generated method stub
    return 0;
  }

  @Override
  public String getType(Uri arg0)
  {
    // TODO Auto-generated method stub
    return null;
  }

  @Override
  public Uri insert(Uri arg0, ContentValues arg1)
  {
    // TODO Auto-generated method stub
    return null;
  }

  @Override
  public boolean onCreate()
  {
    mJsonRPC  = new XBMCJsonRPC();

    return true;
  }

  @Override
  public Cursor query(Uri uri, String[] projection, String selection,
          String[] selectionArgs, String sortOrder)
  {	  
    if(uri.toString().contains("/search"))
  	{
  	}

    return null;
  }

  @Override
  public int update(Uri arg0, ContentValues arg1, String arg2, String[] arg3)
  {
    // TODO Auto-generated method stub
    return 0;
  }

}
