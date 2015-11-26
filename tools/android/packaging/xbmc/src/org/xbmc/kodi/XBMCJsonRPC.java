package org.xbmc.kodi;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.StatusLine;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.protocol.HTTP;
import org.json.JSONArray;
import org.json.JSONObject;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.TaskStackBuilder;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.util.Log;

public class XBMCJsonRPC
{
  private static String TAG = "kodi";
  private String m_jsonURL = "http://localhost:8080";
  private static final int MAX_RECOMMENDATIONS = 3;

  private NotificationManager mNotificationManager;
  
  public JSONObject request(String jsonRequest)
  {
    try
    {
      Log.d(TAG, "JSON in: " + jsonRequest);

      StringBuilder strbuilder = new StringBuilder();
      HttpClient client = new DefaultHttpClient();
      HttpPost httpPost = new HttpPost(m_jsonURL + "/jsonrpc");
      httpPost.setHeader(HTTP.CONTENT_TYPE, "application/json");
      httpPost.setEntity(new StringEntity(jsonRequest));
      try
      {
        HttpResponse response = client.execute(httpPost);
        StatusLine statusLine = response.getStatusLine();
        int statusCode = statusLine.getStatusCode();
        if (statusCode == 200)
        {
          HttpEntity entity = response.getEntity();
          InputStream content = entity.getContent();

          BufferedReader reader = new BufferedReader(new InputStreamReader(
              content));
          String line;
          while ((line = reader.readLine()) != null)
          {
            strbuilder.append(line);
          }

          Log.d(TAG, "JSON out: " + strbuilder.toString());

          try
          {
            JSONObject resp = new JSONObject(strbuilder.toString());
            return resp;
          } catch (Exception e)
          {
            Log.e(TAG, "Failed to parse JSON");
            e.printStackTrace();
            return null;
          }

        } else
        {
          Log.e(TAG, "Failed to read JSON");
          return null;
        }
      } catch (Exception e)
      {
        e.printStackTrace();
        return null;
      }
    } catch (Exception e)
    {
      Log.e(TAG, "Failed to read JSON");
      e.printStackTrace();
      return null;
    }
  }

  public Bitmap getBitmap(String src)
  {
    try
    {
      JSONObject req = request("{\"jsonrpc\": \"2.0\", \"method\": \"Files.PrepareDownload\", \"params\": { \"path\": \""
          + src + "\"}, \"id\": \"1\"}");
      if (req == null)
        return null;

      JSONObject result = req.getJSONObject("result");
      String surl = result.getJSONObject("details").getString("path");

      URL url = new URL(m_jsonURL + "/" + surl);
      HttpURLConnection connection = (HttpURLConnection) url.openConnection();
      connection.setDoInput(true);
      connection.connect();
      InputStream input = connection.getInputStream();
      Bitmap myBitmap = BitmapFactory.decodeStream(input);
      return myBitmap;
    } catch (Exception e)
    {
      e.printStackTrace();
      return null;
    }
  }

  public String getBitmapUrl(String src)
  {
    try
    {
      JSONObject req = request("{\"jsonrpc\": \"2.0\", \"method\": \"Files.PrepareDownload\", \"params\": { \"path\": \""
          + src + "\"}, \"id\": \"1\"}");
      if (req == null)
        return null;

      JSONObject result = req.getJSONObject("result");
      String surl = result.getJSONObject("details").getString("path");

      return (m_jsonURL + "/" + surl);
    } catch (Exception e)
    {
      e.printStackTrace();
      return null;
    }
  }
  
  public void updateLeanback(Context ctx, String jsonRequest)
  {
    if (mNotificationManager == null)
    {
      mNotificationManager = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    XBMCRecommendationBuilder builder = new XBMCRecommendationBuilder()
        .setContext(ctx)
        .setSmallIcon(R.drawable.notif_icon);

    JSONObject rep = request(jsonRequest);
    if (rep == null)
      return;

    try
    {
      JSONObject results = rep.getJSONObject("result");
      JSONArray movies = results.getJSONArray("movies");

      int count = 0;
      for (int i = 0; i < movies.length(); ++i)
      {
        JSONObject movie = movies.getJSONObject(i);
        int id = Integer.parseInt(movie.getString("imdbnumber").replace("tt",
            ""));

        final XBMCRecommendationBuilder notificationBuilder = builder
            .setBackground(
                XBMCImageContentProvider.GetImageUri(
                    getBitmapUrl(movie.getString("fanart"))).toString())
            .setId(id).setPriority(MAX_RECOMMENDATIONS - count)
            .setTitle(movie.getString("title"))
            .setDescription(movie.getString("tagline"))
            .setIntent(buildPendingIntent(ctx, movie));

        Bitmap bitmap = getBitmap(movie.getString("thumbnail"));
        notificationBuilder.setBitmap(bitmap);
        Notification notification = notificationBuilder.build();
        mNotificationManager.notify(id, notification);
        ++count;
      }
    } catch (Exception e)
    {
      e.printStackTrace();
    }
  
    }
  
    private PendingIntent buildPendingIntent(Context ctx, JSONObject movie)
    {
      try
      {
        Intent detailsIntent = new Intent(ctx, Splash.class);
        detailsIntent.setAction(Intent.ACTION_VIEW);
        detailsIntent.setData(Uri.parse("videodb://movies/" + movie.getString("movieid")));
        //detailsIntent.putExtra(MovieDetailsActivity.MOVIE, movie);
        //detailsIntent.putExtra(MovieDetailsActivity.NOTIFICATION_ID, id);
  
        return PendingIntent.getActivity(ctx, 0, detailsIntent, PendingIntent.FLAG_CANCEL_CURRENT);
      } catch (Exception e)
      {
        e.printStackTrace();
        return null;
      }
    }
}
