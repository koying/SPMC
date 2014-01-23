package com.semperpax.spmc;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.System;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.Properties;
import java.util.Enumeration;

import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.util.Log;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;

import android.os.Handler;
import android.os.Message;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.os.Environment;

public class Splash extends Activity {

  private static final int Uninitialized = 0;
  private static final int InError = 1;
  private static final int Checking = 2;
  private static final int ChecksDone = 3;
  private static final int Clearing = 4;
  private static final int Caching = 5;
  private static final int CachingDone = 6;
  private static final int WaitingStorageChecked = 7;
  private static final int StorageChecked = 8;
  private static final int StartingXBMC = 99;

  private static final String TAG = "XBMC";

  private String mCpuinfo = "";
  private String mErrorMsg = "";

  private ProgressBar mProgress = null;
  private TextView mTextView = null;
  
  private int mState = Uninitialized;
  public AlertDialog myAlertDialog;

  private String sPackagePath = "";
  private String sXbmcHome = "";
  private File fPackagePath = null;
  private File fXbmcHome = null;

  private BroadcastReceiver mExternalStorageReceiver = null;
  private boolean mExternalStorageChecked = false;
  private boolean mCachingDone = false;

  private class StateMachine extends Handler {

    private Splash mSplash = null;
    
    StateMachine(Splash a) {
      this.mSplash = a;
    }
    
    @Override
    public void handleMessage(Message msg) {
      mSplash.mState = msg.what;
      switch(mSplash.mState) {
        case InError:
          mSplash.finish();
          break;
        case Checking:
          break;
        case Clearing:
          mSplash.mTextView.setText("Clearing cache...");
          mSplash.mProgress.setVisibility(View.INVISIBLE);
          break;
        case CachingDone:
          mSplash.mCachingDone = true;
          if (mSplash.mExternalStorageChecked)
            sendEmptyMessage(StartingXBMC);
          else
            sendEmptyMessage(WaitingStorageChecked);
          break;
        case WaitingStorageChecked:
          mSplash.mTextView.setText("Waiting for external storage...");
          mSplash.mProgress.setVisibility(View.INVISIBLE);
          break;
        case StorageChecked:
          mExternalStorageChecked = true;
          mSplash.stopWatchingExternalStorage();
          if (mSplash.mCachingDone)
            sendEmptyMessage(StartingXBMC);
          break;
        case StartingXBMC:
          mSplash.mTextView.setText("Starting SPMC...");
          mSplash.mProgress.setVisibility(View.INVISIBLE);
          mSplash.startXBMC();
          break;
        default:
          break;
      }
    }
  }
  private StateMachine mStateMachine = new StateMachine(this);

  public void showErrorDialog(Context context, String title, String message) {
    if (myAlertDialog != null && myAlertDialog.isShowing())
      return;

    AlertDialog.Builder builder = new AlertDialog.Builder(context);
    builder.setTitle(title);
    builder.setIcon(android.R.drawable.ic_dialog_alert);
    builder.setMessage(Html.fromHtml(message));
    builder.setPositiveButton("Exit",
        new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int arg1) {
            dialog.dismiss();
          }
        });
    builder.setCancelable(false);
    myAlertDialog = builder.create();
    myAlertDialog.show();

    // Make links actually clickable
    ((TextView) myAlertDialog.findViewById(android.R.id.message))
        .setMovementMethod(LinkMovementMethod.getInstance());
  }
  
  private void SetupEnvironment() {
    File fProp = new File("/sdcard/xbmc_env.properties");
    if (fProp.exists()) {
      try {
        Properties sysProp = new Properties(System.getProperties());
        FileInputStream xbmcenvprop = new FileInputStream(fProp);
        sysProp.load(xbmcenvprop);
        System.setProperties(sysProp);

        sXbmcHome = System.getProperty("xbmc.home", "");
        fXbmcHome = new File(sXbmcHome);
        fXbmcHome.mkdir();
        if (!fXbmcHome.exists())
          sXbmcHome = "";

        String sXbmcdata = System.getProperty("xbmc.data", "");
        if (!sXbmcdata.isEmpty()) {
          File fXbmcData = new File(sXbmcdata);
          fXbmcData.mkdir();
          if (!fXbmcData.exists())
            sXbmcdata = "";
        }

      } catch (NotFoundException e) {
        Log.e(TAG, "Cannot find xbmc_env properties file");
      } catch (IOException e) {
        Log.e(TAG, "Failed to open xbmc_env properties file");
      }
    }
    if (sXbmcHome.isEmpty()) {
      File fCacheDir = getCacheDir();
      sXbmcHome = fCacheDir.getAbsolutePath() + "/apk";
    }

    sPackagePath = getPackageResourcePath();
    fPackagePath = new File(sPackagePath);
    fXbmcHome = new File(sXbmcHome);
  }

  private boolean ParseCpuFeature() {
    ProcessBuilder cmd;

    try {
      String[] args = { "/system/bin/cat", "/proc/cpuinfo" };
      cmd = new ProcessBuilder(args);

      Process process = cmd.start();
      InputStream in = process.getInputStream();
      byte[] re = new byte[1024];
      while (in.read(re) != -1) {
        mCpuinfo = mCpuinfo + new String(re);
      }
      in.close();
    } catch (IOException ex) {
      ex.printStackTrace();
      return false;
    }
    return true;
  }

  private boolean CheckCpuFeature(String feat) {
    final Pattern FeaturePattern = Pattern.compile(":.*?\\s" + feat
        + "(?:\\s|$)");
    Matcher m = FeaturePattern.matcher(mCpuinfo);
    return m.find();
  }

  void updateExternalStorageState() {
    String state = Environment.getExternalStorageState();
    if (Environment.MEDIA_CHECKING.equals(state)) {
        mExternalStorageChecked = false;
    } else {
        mStateMachine.sendEmptyMessage(StorageChecked);
    }
  }

  void startWatchingExternalStorage() {
    mExternalStorageReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        Log.i(TAG, "Storage: " + intent.getData());
        updateExternalStorageState();
      }
    };
    IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
    filter.addAction(Intent.ACTION_MEDIA_REMOVED);
    filter.addAction(Intent.ACTION_MEDIA_SHARED);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTABLE);
    filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
    registerReceiver(mExternalStorageReceiver, filter);
  }

  void stopWatchingExternalStorage() {
    if (mExternalStorageReceiver != null)
      unregisterReceiver(mExternalStorageReceiver);
  }

  class FillCache extends AsyncTask<Void, Integer, Integer> {

    private Splash mSplash = null;
    private int mProgressStatus = 0;

    public FillCache(Splash splash) {
      this.mSplash = splash;
    }

    void DeleteRecursive(File fileOrDirectory) {
      if (fileOrDirectory.isDirectory())
        for (File child : fileOrDirectory.listFiles())
          DeleteRecursive(child);

      fileOrDirectory.delete();
    }

    @Override
    protected Integer doInBackground(Void... param) {
      if (fXbmcHome.exists()) {
        // Remove existing files
        mStateMachine.sendEmptyMessage(Clearing);
        Log.d(TAG, "Removing existing " + fXbmcHome.toString());
        DeleteRecursive(fXbmcHome);
      }
      fXbmcHome.mkdirs();

      // Log.d(TAG, "apk: " + sPackagePath);
      // Log.d(TAG, "output: " + sXbmcHome);

      ZipFile zip;
      byte[] buf = new byte[4096];
      int n;
      try {
        zip = new ZipFile(sPackagePath);
        Enumeration<? extends ZipEntry> entries = zip.entries();
        mProgress.setProgress(0);
        mProgress.setMax(zip.size());

        mState = Caching;
        publishProgress(mProgressStatus);
        while (entries.hasMoreElements()) {
          // Update the progress bar
          publishProgress(++mProgressStatus);

          ZipEntry e = (ZipEntry) entries.nextElement();

          if (!e.getName().startsWith("assets/"))
            continue;
          if (e.getName().startsWith("assets/python2.6"))
            continue;

          String sFullPath = sXbmcHome + "/" + e.getName();
          File fFullPath = new File(sFullPath);
          if (e.isDirectory()) {
            // Log.d(TAG, "creating dir: " + sFullPath);
            fFullPath.mkdirs();
            continue;
          }

          fFullPath.getParentFile().mkdirs();

          try {
            InputStream in = zip.getInputStream(e);
            BufferedOutputStream out = new BufferedOutputStream(
                new FileOutputStream(sFullPath));
            while ((n = in.read(buf, 0, 4096)) > -1)
              out.write(buf, 0, n);

            in.close();
            out.close();
          } catch (IOException e1) {
            e1.printStackTrace();
          }
        }

        zip.close();

        fXbmcHome.setLastModified(fPackagePath.lastModified());

      } catch (FileNotFoundException e1) {
        e1.printStackTrace();
        mErrorMsg = "Cannot find package.";
        return -1;
      } catch (IOException e) {
        e.printStackTrace();
        mErrorMsg = "Cannot read package.";
        return -1;
      }

      mState = CachingDone;
      publishProgress(0);

      return 0;
    }

    @Override
    protected void onProgressUpdate(Integer... values) {
      switch (mState) {
      case Caching:
        mSplash.mTextView.setText("Preparing for first run. Please wait...");
        mSplash.mProgress.setVisibility(View.VISIBLE);
        mSplash.mProgress.setProgress(values[0]);
        break;
      case CachingDone:
        mSplash.mProgress.setVisibility(View.INVISIBLE);
        break;
      }
    }

    @Override
    protected void onPostExecute(Integer result) {
      super.onPostExecute(result);
      if (result < 0) {
        showErrorDialog(mSplash, "Error", mErrorMsg);
        mState = InError;
      }

      mStateMachine.sendEmptyMessage(mState);
    }
  }
  
  protected void startXBMC() {
    // NB: We only preload libxbmc to be able to get info on missing symbols.
    //     This is not normally needed
    System.loadLibrary("xbmc");
    
    // Run XBMC
    Intent intent = getIntent();
    intent.setClass(this, com.semperpax.spmc.Main.class);
    startActivity(intent);
    finish();
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // Check if XBMC is not already running
    ActivityManager activityManager = (ActivityManager) getBaseContext()
        .getSystemService(Context.ACTIVITY_SERVICE);
    List<RunningTaskInfo> tasks = activityManager
        .getRunningTasks(Integer.MAX_VALUE);
    for (RunningTaskInfo task : tasks)
      if (task.topActivity.toString().equalsIgnoreCase(
          "ComponentInfo{com.semperpax.spmc/com.semperpax.spmc.Main}")) {
        // XBMC already running; just activate it
        startXBMC();
        return;
      }

    mStateMachine.sendEmptyMessage(Checking);

    String curArch = "";
    try {
      curArch = Build.CPU_ABI.substring(0,3);
    } catch (IndexOutOfBoundsException e) {
      mErrorMsg = "Error! Unexpected architecture: " + Build.CPU_ABI;
      Log.e(TAG, mErrorMsg);
      mState = InError;
   }
    
    if (mState != InError) {
      // Check if we are on the proper arch

      // Read the properties
      try {
        Resources resources = this.getResources();
        InputStream xbmcprop = resources.openRawResource(R.raw.xbmc);
        Properties properties = new Properties();
        properties.load(xbmcprop);

        if (!curArch.equalsIgnoreCase(properties.getProperty("native_arch"))) {
          mErrorMsg = "This XBMC package is not compatible with your device (" + curArch + " vs. " + properties.getProperty("native_arch") +").\nPlease check the <a href=\"http://wiki.xbmc.org/index.php?title=XBMC_for_Android_specific_FAQ\">XBMC Android wiki</a> for more information.";
          Log.e(TAG, mErrorMsg);
          mState = InError;
        }
      } catch (NotFoundException e) {
        mErrorMsg = "Cannot find properties file";
        Log.e(TAG, mErrorMsg);
        mState = InError;
      } catch (IOException e) {
        mErrorMsg = "Failed to open properties file";
        Log.e(TAG, mErrorMsg);
        mState = InError;
      }
    }
    
    if (mState != InError) {
      if (curArch.equalsIgnoreCase("arm")) {
        // arm arch: check if the cpu supports neon
        boolean ret = ParseCpuFeature();
        if (!ret) {
          mErrorMsg = "Error! Cannot parse CPU features.";
          Log.e(TAG, mErrorMsg);
          mState = InError;
        } else {
          ret = CheckCpuFeature("neon");
          if (!ret) {
            mErrorMsg = "This XBMC package is not compatible with your device (NEON).\nPlease check the <a href=\"http://wiki.xbmc.org/index.php?title=XBMC_for_Android_specific_FAQ\">XBMC Android wiki</a> for more information.";
            Log.e(TAG, mErrorMsg);
            mState = InError;
          }
        }
      }
    }
    
    if (mState != InError) {
      mState = ChecksDone;
      
      SetupEnvironment();
      if (fXbmcHome.exists()
          && fXbmcHome.lastModified() >= fPackagePath.lastModified()) {
        mState = CachingDone;
        mCachingDone = true;
      }
    }
    if (!Environment.MEDIA_CHECKING.equals(Environment.getExternalStorageState()))
      mExternalStorageChecked = true;

    if (mCachingDone && mExternalStorageChecked) {
      startXBMC();
      return;
    }

    setContentView(R.layout.activity_splash);
    mProgress = (ProgressBar) findViewById(R.id.progressBar1);
    mTextView = (TextView) findViewById(R.id.textView1);

    if (mState == InError) {
      showErrorDialog(this, "Error", mErrorMsg);
      return;
    }
          
    if (!mExternalStorageChecked) {
      startWatchingExternalStorage();
      mStateMachine.sendEmptyMessage(WaitingStorageChecked);
    }

    if (!mCachingDone)
      new FillCache(this).execute();
  }

}
