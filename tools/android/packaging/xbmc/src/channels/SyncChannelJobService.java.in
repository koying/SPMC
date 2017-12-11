/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.semperpax.spmc17.channels;

import java.util.List;

import android.app.job.JobParameters;
import android.app.job.JobService;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.media.tv.TvContract;
import android.net.Uri;
import android.os.AsyncTask;
import android.support.annotation.Nullable;
import android.support.media.tv.Channel;
import android.support.media.tv.ChannelLogoUtils;
import android.support.media.tv.TvContractCompat;
import android.util.Log;

import com.semperpax.spmc17.XBMCJsonRPC;
import com.semperpax.spmc17.channels.model.Subscription;
import com.semperpax.spmc17.channels.model.XBMCDatabase;
import com.semperpax.spmc17.channels.util.SharedPreferencesHelper;
import com.semperpax.spmc17.channels.util.TvUtil;

import java.util.List;

/**
 * A service that will populate the TV provider with channels that every user should have. Once a
 * channel is created, it trigger another service to add programs.
 */
public class SyncChannelJobService extends JobService
{

  private static final String TAG = "RecommendChannelJobSvc";

  private SyncChannelTask mSyncChannelTask;

  @Override
  public boolean onStartJob(final JobParameters jobParameters)
  {
    Log.d(TAG, "Starting channel creation job");

    mSyncChannelTask =
            new SyncChannelTask(getApplicationContext())
            {
              @Override
              protected void onPostExecute(Boolean success)
              {
                super.onPostExecute(success);
                jobFinished(jobParameters, !success);
              }
            };
    mSyncChannelTask.execute();
    return true;
  }

  @Override
  public boolean onStopJob(JobParameters jobParameters)
  {
    if (mSyncChannelTask != null)
    {
      mSyncChannelTask.cancel(true);
    }
    return true;
  }

  private static class SyncChannelTask extends AsyncTask<Void, Void, Boolean>
  {

    private final Context mContext;

    SyncChannelTask(Context context)
    {
      this.mContext = context;
    }

    @Override
    protected Boolean doInBackground(Void... voids)
    {
      List<Subscription> subscriptions = XBMCDatabase.getSubscriptions(mContext);

      // Kick off a job to update default programs.
      // The program job should verify if the channel is visible before updating programs.
      for (Subscription channel : subscriptions)
      {
        TvUtil.scheduleSyncingProgramsForChannel(mContext, channel.getChannelId());
      }

      return true;
    }
  }
}
