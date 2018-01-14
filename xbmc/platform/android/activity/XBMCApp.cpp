/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XBMCApp.h"

#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#include <jni.h>
#include <android/configuration.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <androidjni/ApplicationInfo.h>
#include <androidjni/BitmapFactory.h>
#include <androidjni/BroadcastReceiver.h>
#include <androidjni/Build.h>
#include <androidjni/CharSequence.h>
#include <androidjni/ComponentName.h>
#include <androidjni/ConnectivityManager.h>
#include <androidjni/ContentResolver.h>
#include <androidjni/Context.h>
#include <androidjni/Cursor.h>
#include <androidjni/Display.h>
#include <androidjni/Environment.h>
#include <androidjni/File.h>
#include <androidjni/Intent.h>
#include <androidjni/IntentFilter.h>
#include <androidjni/JNIThreading.h>
#include <androidjni/KeyEvent.h>
#include <androidjni/MediaStore.h>
#include <androidjni/NetworkInfo.h>
#include <androidjni/PackageManager.h>
#include <androidjni/PowerManager.h>
#include <androidjni/ResolveInfo.h>
#include <androidjni/StatFs.h>
#include <androidjni/System.h>
#include <androidjni/SystemClock.h>
#include <androidjni/URI.h>
#include <androidjni/View.h>
#include <androidjni/WakeLock.h>
#include <androidjni/Window.h>
#include <androidjni/WindowManager.h>

#include "AndroidKey.h"
#include "settings/AdvancedSettings.h"
#include "cores/AudioEngine/AEFactory.h"
#include "AndroidFeatures.h"
#include "Application.h"
#include "AppParamParser.h"
#include "messaging/ApplicationMessenger.h"
#include "CompileInfo.h"
#include "settings/DisplaySettings.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "guiinfo/GUIInfoLabels.h"
#include "platform/android/activity/IInputDeviceCallbacks.h"
#include "platform/android/activity/IInputDeviceEventHandler.h"
#include "input/Key.h"
#include "utils/log.h"
#include "input/MouseStat.h"
#include "network/android/NetworkAndroid.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "filesystem/SpecialProtocol.h"
#include "TextureCache.h"
#include "utils/StringUtils.h"
#include "filesystem/VideoDatabaseFile.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/videosync/VideoSyncAndroid.h"
#include "windowing/WinEvents.h"

#include "CompileInfo.h"
#include "interfaces/AnnouncementManager.h"

#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif

#define CAPTURE_QUEUE_MAXDEPTH 3

#define ACTION_XBMC_RESUME "android.intent.XBMC_RESUME"

#define PLAYBACK_STATE_STOPPED  0x0000
#define PLAYBACK_STATE_PLAYING  0x0001
#define PLAYBACK_STATE_VIDEO    0x0100
#define PLAYBACK_STATE_AUDIO    0x0200
#define PLAYBACK_STATE_CANNOT_PAUSE 0x0400

using namespace KODI::MESSAGING;
using namespace ANNOUNCEMENT;
using namespace jni;

CXBMCApp* CXBMCApp::m_xbmcappinstance = NULL;
CCriticalSection CXBMCApp::m_LayoutMutex;

std::unique_ptr<CJNIXBMCMainView> CXBMCApp::m_mainView;
ANativeActivity *CXBMCApp::m_activity = NULL;
CJNIWakeLock *CXBMCApp::m_wakeLock = NULL;
ANativeWindow* CXBMCApp::m_window = NULL;
int CXBMCApp::m_batteryLevel = 0;
bool CXBMCApp::m_hasFocus = false;
bool CXBMCApp::m_isResumed = false;
bool CXBMCApp::m_hasAudioFocus = false;
bool CXBMCApp::m_headsetPlugged = false;
bool CXBMCApp::m_hdmiPlugged = true;
bool CXBMCApp::m_hasReqVisible = false;
bool CXBMCApp::m_hasPIP = false;
CCriticalSection CXBMCApp::m_applicationsMutex;
std::vector<androidPackage> CXBMCApp::m_applications;

CCriticalSection CXBMCApp::m_captureMutex;
CCaptureEvent CXBMCApp::m_captureEvent;
std::queue<CJNIImage> CXBMCApp::m_captureQueue;

uint64_t CXBMCApp::m_vsynctime = 0;
CEvent CXBMCApp::m_vsyncEvent;
std::vector<CActivityResultEvent*> CXBMCApp::m_activityResultEvents;
std::vector<GLuint> CXBMCApp::m_texturePool;

CRect CXBMCApp::m_droid2guiRatio(0.0, 0.0, 1.0, 1.0);

uint32_t CXBMCApp::m_playback_state = PLAYBACK_STATE_STOPPED;
CRect CXBMCApp::m_surface_rect;

CXBMCApp::CXBMCApp(ANativeActivity* nativeActivity)
  : CJNIBase()
  , CJNIMainActivity(nativeActivity->clazz)
  , CJNIXBMCInputDeviceListener()
  , CJNIBroadcastReceiver()
  , m_videosurfaceInUse(false)
  , m_inputDeviceCallbacks(nullptr)
  , m_inputDeviceEventHandler(nullptr)

{
  m_xbmcappinstance = this;
  m_activity = nativeActivity;
  if (m_activity == NULL)
  {
    android_printf("CXBMCApp: invalid ANativeActivity instance");
    exit(1);
    return;
  }
  m_audioFocusListener.reset(new CJNIXBMCAudioManagerOnAudioFocusChangeListener());
  m_broadcastReceiver.reset(new CJNIXBMCBroadcastReceiver(this));
  m_mainView.reset(new CJNIXBMCMainView(this));
  m_firstActivityRun = true;
  m_exiting = false;
  android_printf("CXBMCApp: Created");
}

CXBMCApp::~CXBMCApp()
{
  if (m_wakeLock->isHeld())
    m_wakeLock->release();
  delete m_wakeLock;
  m_xbmcappinstance = NULL;
}

void CXBMCApp::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (strcmp(sender, "xbmc") != 0)
    return;

  if (flag & Input)
  {
    if (strcmp(message, "OnInputRequested") == 0)
      CAndroidKey::SetHandleSearchKeys(true);
    else if (strcmp(message, "OnInputFinished") == 0)
      CAndroidKey::SetHandleSearchKeys(false);
  }
  else if (flag & Player)
  {
     if (strcmp(message, "OnPlay") == 0)
      OnPlayBackStarted();
    else if (strcmp(message, "OnPause") == 0)
      OnPlayBackPaused();
    else if (strcmp(message, "OnStop") == 0)
      OnPlayBackStopped();
     else if (strcmp(message, "OnSeek") == 0)
       UpdateSessionState();
     else if (strcmp(message, "OnSpeedChanged") == 0)
       UpdateSessionState();
  }
  else if (flag & Info)
  {
     if (strcmp(message, "OnChanged") == 0)
      UpdateSessionMetadata();
  }
  else if (flag & GUI)
  {
     if (strcmp(message, "OnVideoResolutionChanged") == 0)
      CalculateGUIRatios();
  }
}

void CXBMCApp::onStart()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

#if defined(HAS_LIBAMCODEC)
  if (aml_permissions())
  {
    // non-aml boxes will ignore this intent broadcast.
    // setup aml scalers to play video as is, unscaled.
    CJNIIntent intent_aml_video_on = CJNIIntent("android.intent.action.REALVIDEO_ON");
    sendBroadcast(intent_aml_video_on);
  }
#endif

  if (m_firstActivityRun)
  {
    if (!g_application.IsInitialized())
      abort();

    CApplicationMessenger::GetInstance().PostMsg(TMSG_DISPLAY_INIT);

    // Some intent filters MUST be registered in code rather than through the manifest
    CJNIIntentFilter intentFilter;
    intentFilter.addAction("android.intent.action.BATTERY_CHANGED");
    intentFilter.addAction("android.intent.action.SCREEN_ON");
    intentFilter.addAction("android.intent.action.SCREEN_OFF");
    intentFilter.addAction("android.intent.action.HEADSET_PLUG");
    intentFilter.addAction("android.media.action.HDMI_AUDIO_PLUG");
    intentFilter.addAction("android.net.conn.CONNECTIVITY_CHANGE");
    registerReceiver(*m_broadcastReceiver, intentFilter);

    m_mediaSession.reset(new CJNIXBMCMediaSession());

    m_firstActivityRun = false;
  }
}

void CXBMCApp::onResume()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

  if (!g_application.IsInScreenSaver())
    EnableWakeLock(true);
  else
    g_application.WakeUpScreenSaverAndDPMS();

  CheckHeadsetPlugged();

  // Clear the applications cache. We could have installed/deinstalled apps
  {
    CSingleLock lock(m_applicationsMutex);
    m_applications.clear();
  }

  m_hasReqVisible = false;
  // Re-request Visible Behind
  if ((m_playback_state & PLAYBACK_STATE_PLAYING) && (m_playback_state & PLAYBACK_STATE_VIDEO))
    RequestVisibleBehind(true);

  m_isResumed = true;
}

void CXBMCApp::onPause()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

#if defined(HAS_LIBAMCODEC)
  if (aml_permissions())
  {
    // non-aml boxes will ignore this intent broadcast.
    CJNIIntent intent_aml_video_off = CJNIIntent("android.intent.action.REALVIDEO_OFF");
    sendBroadcast(intent_aml_video_off);
  }
#endif

  EnableWakeLock(false);
  m_isResumed = false;
}

void CXBMCApp::onStop()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);

  if ((m_playback_state & PLAYBACK_STATE_PLAYING) && !m_hasReqVisible)
  {
    if (m_playback_state & PLAYBACK_STATE_CANNOT_PAUSE)
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
    else if (m_playback_state & PLAYBACK_STATE_VIDEO)
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
  }
}

void CXBMCApp::onDestroy()
{
  android_printf("%s", __PRETTY_FUNCTION__);

  unregisterReceiver(*m_broadcastReceiver);

  m_mediaSession.release();
}

void CXBMCApp::onSaveState(void **data, size_t *size)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  // no need to save anything as XBMC is running in its own thread
}

void CXBMCApp::onConfigurationChanged()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  // ignore any configuration changes like screen rotation etc
}

void CXBMCApp::onLowMemory()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  // can't do much as we don't want to close completely
}

void CXBMCApp::onCreateWindow(ANativeWindow* window)
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::onResizeWindow()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_window = NULL;
  // no need to do anything because we are fixed in fullscreen landscape mode
}

void CXBMCApp::onDestroyWindow()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
}

void CXBMCApp::onGainFocus()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_hasFocus = true;
  g_application.WakeUpScreenSaverAndDPMS();
}

void CXBMCApp::onLostFocus()
{
  android_printf("%s: ", __PRETTY_FUNCTION__);
  m_hasFocus = false;
}

void CXBMCApp::Initialize()
{
  // Allocate a pool of texture for MediaCodec non-surface
  GLuint texture_ids[5];
  glGenTextures(5, texture_ids);
  for (int i=0; i<5; ++i)
    m_texturePool.push_back(texture_ids[i]);

  g_application.m_ServiceManager->GetAnnouncementManager().AddAnnouncer(this);
}

void CXBMCApp::Deinitialize(int status)
{
  while(!m_texturePool.empty())
  {
    GLuint texture_id = m_texturePool.back();
    glDeleteTextures(1, &texture_id);
    m_texturePool.pop_back();
  }

  // Pass the return code to Java
  set_field(m_object, "mExitCode", status);

  // If we are have not been force by Android to exit, notify its finish routine.
  // This will cause android to run through its teardown events, it calls:
  // onPause(), onLostFocus(), onDestroyWindow(), onStop(), onDestroy().
  ANativeActivity_finish(m_activity);
  m_exiting=true;

}

bool CXBMCApp::EnableWakeLock(bool on)
{
  if (!m_wakeLock)
  {
    std::string appName = CCompileInfo::GetAppName();
    StringUtils::ToLower(appName);
    std::string className = CCompileInfo::GetPackage();
    // SCREEN_BRIGHT_WAKE_LOCK is marked as deprecated but there is no real alternatives for now
    m_wakeLock = new CJNIWakeLock(CJNIPowerManager(getSystemService("power")).newWakeLock(CJNIPowerManager::SCREEN_BRIGHT_WAKE_LOCK, className.c_str()));
    if (m_wakeLock)
      m_wakeLock->setReferenceCounted(false);
    else
      return false;
  }

  if (on)
  {
    if (!m_wakeLock->isHeld())
    {
      m_wakeLock->acquire();
      android_printf("%s: %s", __PRETTY_FUNCTION__, "acquired");
    }
  }
  else
  {
    if (m_wakeLock->isHeld())
    {
      m_wakeLock->release();
      android_printf("%s: %s", __PRETTY_FUNCTION__, "released");
    }
  }

  return true;
}

bool CXBMCApp::AcquireAudioFocus()
{
  if (!m_xbmcappinstance)
    return false;

  if (m_hasAudioFocus)
    return true;

  CJNIAudioManager audioManager(getSystemService("audio"));
  if (!audioManager)
  {
    CXBMCApp::android_printf("Cannot get audiomanger");
    return false;
  }

  // Request audio focus for playback
  int result = audioManager.requestAudioFocus(*m_audioFocusListener,
                                              // Use the music stream.
                                              CJNIAudioManager::STREAM_MUSIC,
                                              // Request permanent focus.
                                              CJNIAudioManager::AUDIOFOCUS_GAIN);

  if (result != CJNIAudioManager::AUDIOFOCUS_REQUEST_GRANTED)
  {
    CXBMCApp::android_printf("Audio Focus request failed");
    return false;
  }
  m_hasAudioFocus = true;
  return true;
}

bool CXBMCApp::ReleaseAudioFocus()
{
  if (!m_xbmcappinstance)
    return false;

  if (!m_hasAudioFocus)
    return true;

  CJNIAudioManager audioManager(getSystemService("audio"));
  if (!audioManager)
  {
    CXBMCApp::android_printf("Cannot get audiomanger");
    return false;
  }

  // Release audio focus after playback
  int result = audioManager.abandonAudioFocus(*m_audioFocusListener);
  if (result != CJNIAudioManager::AUDIOFOCUS_REQUEST_GRANTED)
  {
    CXBMCApp::android_printf("Audio Focus abandon failed");
    return false;
  }
  m_hasAudioFocus = false;
  return true;
}

void CXBMCApp::RequestVisibleBehind(bool requested)
{
  if (requested == m_hasReqVisible)
    return;

  m_hasReqVisible = requestVisibleBehind(requested);
  CLog::Log(LOGDEBUG, "Visible Behind request: %s", m_hasReqVisible ? "true" : "false");
}

void CXBMCApp::RequestPictureInPictureMode()
{
  // PIP and VisbleBehind are exclusive
  if (m_hasReqVisible)
    RequestVisibleBehind(false);

  enterPictureInPictureMode();
  CLog::Log(LOGDEBUG, "Entering PIP mode");
}

void CXBMCApp::CheckHeadsetPlugged()
{
  bool oldstate = m_headsetPlugged;

  CJNIAudioManager audioManager(getSystemService("audio"));
  m_headsetPlugged = audioManager.isWiredHeadsetOn() || audioManager.isBluetoothA2dpOn();

  if (m_headsetPlugged != oldstate)
    CAEFactory::DeviceChange();
}

bool CXBMCApp::IsHeadsetPlugged()
{
  return m_headsetPlugged;
}

bool CXBMCApp::IsHDMIPlugged()
{
  return m_hdmiPlugged;
}

int CXBMCApp::SetBuffersGeometry(int width, int height)
{
  return ANativeWindow_setBuffersGeometry(m_window, width, height, WINDOW_FORMAT_RGBA_8888);
}

#include "threads/Event.h"
#include <time.h>

void CXBMCApp::SetRefreshRateCallback(CVariant* rateVariant)
{
  float rate = rateVariant->asFloat();
  delete rateVariant;

  CJNIWindow window = CXBMCApp::get()->getWindow();
  if (window)
  {
    CJNIWindowManagerLayoutParams params = window.getAttributes();
    if (params.getpreferredRefreshRate() != rate)
    {
      params.setpreferredRefreshRate(rate);
      if (params.getpreferredRefreshRate() > 0.0)
        window.setAttributes(params);
    }
  }
}

void CXBMCApp::SetDisplayModeCallback(CVariant* modeVariant)
{
  int mode = modeVariant->asFloat();
  delete modeVariant;

  CJNIWindow window = CXBMCApp::get()->getWindow();
  if (window)
  {
    CJNIWindowManagerLayoutParams params = window.getAttributes();
    if (params.getpreferredDisplayModeId() != mode)
    {
      params.setpreferredDisplayModeId(mode);
      window.setAttributes(params);
    }
  }
}

void CXBMCApp::SetRefreshRate(float rate)
{
  if (rate < 1.0)
    return;

  CVariant *variant = new CVariant(rate);
  runNativeOnUiThread(SetRefreshRateCallback, variant);
}

void CXBMCApp::SetDisplayMode(int mode)
{
  if (mode < 1.0)
    return;

  CVariant *variant = new CVariant(mode);
  runNativeOnUiThread(SetDisplayModeCallback, variant);
}

int CXBMCApp::android_printf(const char *format, ...)
{
  // For use before CLog is setup by XBMC_Run()
  va_list args;
  va_start(args, format);
  int result = __android_log_vprint(ANDROID_LOG_DEBUG, CCompileInfo::GetAppName(), format, args);
  va_end(args);
  return result;
}

void CXBMCApp::BringToFront()
{
  if (!m_isResumed)
  {
    CLog::Log(LOGERROR, "CXBMCApp::BringToFront");
    StartActivity(getPackageName());
  }
}

void CXBMCApp::Minimize()
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_DISPLAY_DESTROY);
  moveTaskToBack(true);
}

GLuint CXBMCApp::pullTexture()
{
  if (m_texturePool.empty())
    return (GLuint) -1;
  GLuint tex = m_texturePool.back();
  m_texturePool.pop_back();
  return tex;
}

void CXBMCApp::pushTexture(GLuint tex)
{
  m_texturePool.push_back(tex);
}

int CXBMCApp::GetDPI()
{
  if (m_activity == NULL || m_activity->assetManager == NULL)
    return 0;

  // grab DPI from the current configuration - this is approximate
  // but should be close enough for what we need
  AConfiguration *config = AConfiguration_new();
  AConfiguration_fromAssetManager(config, m_activity->assetManager);
  int dpi = AConfiguration_getDensity(config);
  AConfiguration_delete(config);

  return dpi;
}

CRect CXBMCApp::GetSurfaceRect()
{
  CSingleLock lock(m_LayoutMutex);

  return m_surface_rect;
}

CRect CXBMCApp::MapRenderToDroid(const CRect& srcRect)
{
  CSingleLock lock(m_LayoutMutex);

  return CRect(srcRect.x1 / m_droid2guiRatio.x2, srcRect.y1 / m_droid2guiRatio.y2, srcRect.x2 / m_droid2guiRatio.x2, srcRect.y2 / m_droid2guiRatio.y2);
}

CPoint CXBMCApp::MapDroidToGui(const CPoint& src)
{
  CSingleLock lock(m_LayoutMutex);

  return CPoint((src.x - m_droid2guiRatio.x1) * m_droid2guiRatio.x2, (src.y - m_droid2guiRatio.y1) * m_droid2guiRatio.y2);
}

void CXBMCApp::UpdateSessionMetadata()
{
  CJNIMediaMetadataBuilder builder;
  builder
      .putString(CJNIMediaMetadata::METADATA_KEY_DISPLAY_TITLE, g_infoManager.GetLabel(PLAYER_TITLE))
      .putString(CJNIMediaMetadata::METADATA_KEY_TITLE, g_infoManager.GetLabel(PLAYER_TITLE))
      .putLong(CJNIMediaMetadata::METADATA_KEY_DURATION, g_application.m_pPlayer->GetTotalTime())
//      .putString(CJNIMediaMetadata::METADATA_KEY_ART_URI, thumb)
//      .putString(CJNIMediaMetadata::METADATA_KEY_DISPLAY_ICON_URI, thumb)
//      .putString(CJNIMediaMetadata::METADATA_KEY_ALBUM_ART_URI, thumb)
      ;

  std::string thumb;
  if (m_playback_state & PLAYBACK_STATE_VIDEO)
  {
    builder
        .putString(CJNIMediaMetadata::METADATA_KEY_DISPLAY_SUBTITLE, g_infoManager.GetLabel(VIDEOPLAYER_TAGLINE))
        .putString(CJNIMediaMetadata::METADATA_KEY_ARTIST, g_infoManager.GetLabel(VIDEOPLAYER_DIRECTOR))
        ;
    thumb = g_infoManager.GetImage(VIDEOPLAYER_COVER, -1);
  }
  else if (m_playback_state & PLAYBACK_STATE_AUDIO)
  {
    builder
        .putString(CJNIMediaMetadata::METADATA_KEY_DISPLAY_SUBTITLE, g_infoManager.GetLabel(MUSICPLAYER_ARTIST))
        .putString(CJNIMediaMetadata::METADATA_KEY_ARTIST, g_infoManager.GetLabel(MUSICPLAYER_ARTIST))
        ;
    thumb = g_infoManager.GetImage(MUSICPLAYER_COVER, -1);
  }
  bool needrecaching = false;
  std::string cachefile = CTextureCache::GetInstance().CheckCachedImage(thumb, needrecaching);
  if (!cachefile.empty())
  {
    std::string actualfile = CSpecialProtocol::TranslatePath(cachefile);
    CJNIBitmap bmp = CJNIBitmapFactory::decodeFile(actualfile);
    if (bmp)
      builder.putBitmap(CJNIMediaMetadata::METADATA_KEY_ART, bmp);
  }
  m_mediaSession->updateMetadata(builder.build());
}

void CXBMCApp::UpdateSessionState()
{
  CJNIPlaybackStateBuilder builder;
  int state = CJNIPlaybackState::STATE_NONE;
  int64_t pos = 0;
  float speed = 0.0;
  if (m_playback_state != PLAYBACK_STATE_STOPPED)
  {
    if (g_application.m_pPlayer->HasVideo())
      m_playback_state |= PLAYBACK_STATE_VIDEO;
    else
      m_playback_state &= ~PLAYBACK_STATE_VIDEO;
    if (g_application.m_pPlayer->HasAudio())
      m_playback_state |= PLAYBACK_STATE_AUDIO;
    else
      m_playback_state &= ~PLAYBACK_STATE_AUDIO;
    pos = g_application.m_pPlayer->GetTime();
    speed = g_application.m_pPlayer->GetPlaySpeed();
    if (m_playback_state & PLAYBACK_STATE_PLAYING)
      state = CJNIPlaybackState::STATE_PLAYING;
    else
      state = CJNIPlaybackState::STATE_PAUSED;
  }
  else
    state = CJNIPlaybackState::STATE_STOPPED;
  builder
      .setState(state, pos, speed, CJNISystemClock::elapsedRealtime())
      .setActions(0xffffffffffffffff)
      ;
  m_mediaSession->updatePlaybackState(builder.build());
}

void CXBMCApp::OnPlayBackStarted()
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  m_playback_state = PLAYBACK_STATE_PLAYING;
  if (g_application.m_pPlayer->HasVideo())
    m_playback_state |= PLAYBACK_STATE_VIDEO;
  if (g_application.m_pPlayer->HasAudio())
    m_playback_state |= PLAYBACK_STATE_AUDIO;
  if (!g_application.m_pPlayer->CanPause())
    m_playback_state |= PLAYBACK_STATE_CANNOT_PAUSE;

  m_mediaSession->activate(true);
  UpdateSessionState();

  CJNIIntent intent(ACTION_XBMC_RESUME, CJNIURI::EMPTY, *this, get_class(CJNIContext::get_raw()));
  m_mediaSession->updateIntent(intent);

  m_xbmcappinstance->AcquireAudioFocus();
  CAndroidKey::SetHandleMediaKeys(false);

  if (m_isResumed)
    RequestVisibleBehind(true);
}

void CXBMCApp::OnPlayBackPaused()
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  m_playback_state &= ~PLAYBACK_STATE_PLAYING;
  UpdateSessionState();

  RequestVisibleBehind(false);
  m_xbmcappinstance->ReleaseAudioFocus();
}

void CXBMCApp::OnPlayBackStopped()
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);

  m_playback_state = PLAYBACK_STATE_STOPPED;
  UpdateSessionState();
  m_mediaSession->activate(false);

  RequestVisibleBehind(false);
  CAndroidKey::SetHandleMediaKeys(true);
  m_xbmcappinstance->ReleaseAudioFocus();
}

const CJNIViewInputDevice CXBMCApp::GetInputDevice(int deviceId)
{
  CJNIInputManager inputManager(getSystemService("input"));
  return inputManager.getInputDevice(deviceId);
}

std::vector<int> CXBMCApp::GetInputDeviceIds()
{
  CJNIInputManager inputManager(getSystemService("input"));
  return inputManager.getInputDeviceIds();
}

void CXBMCApp::ProcessSlow()
{
  if ((m_playback_state & PLAYBACK_STATE_PLAYING) && m_mediaSession->isActive())
    UpdateSessionState();
}

std::vector<androidPackage> CXBMCApp::GetApplications()
{
  CSingleLock lock(m_applicationsMutex);
  if (m_applications.empty())
  {
    std::map<std::string, androidPackage> applications;
    CJNIIntent main(CJNIIntent::ACTION_MAIN, CJNIURI());

    if (CAndroidFeatures::IsLeanback())  // First try leanback
    {
      main.addCategory(CJNIIntent::CATEGORY_LEANBACK_LAUNCHER);

      CJNIList<CJNIResolveInfo> launchables = GetPackageManager().queryIntentActivities(main, 0);
      int numPackages = launchables.size();
      for (int i = 0; i < numPackages; i++)
      {
        CJNIResolveInfo launchable = launchables.get(i);
        CJNIActivityInfo activity = launchable.activityInfo;

        androidPackage newPackage;
        newPackage.packageName = activity.applicationInfo.packageName;
        newPackage.className = activity.name;
        newPackage.packageLabel = launchable.loadLabel(GetPackageManager()).toString();
        newPackage.icon = activity.applicationInfo.icon;
        applications.insert(std::make_pair(newPackage.packageName, newPackage));
      }
    }

    main.removeCategory(CJNIIntent::CATEGORY_LEANBACK_LAUNCHER);
    main.addCategory(CJNIIntent::CATEGORY_LAUNCHER);

    CJNIList<CJNIResolveInfo> launchables = GetPackageManager().queryIntentActivities(main, 0);
    int numPackages = launchables.size();
    for (int i = 0; i < numPackages; i++)
    {
      CJNIResolveInfo launchable = launchables.get(i);
      CJNIActivityInfo activity = launchable.activityInfo;

      if (applications.find(activity.applicationInfo.packageName) == applications.end())
      {
        androidPackage newPackage;
        newPackage.packageName = activity.applicationInfo.packageName;
        newPackage.className = activity.name;
        newPackage.packageLabel = launchable.loadLabel(GetPackageManager()).toString();
        newPackage.icon = activity.applicationInfo.icon;
        applications.insert(std::make_pair(newPackage.packageName, newPackage));
      }
    }

    for(auto it : applications)
      m_applications.push_back(it.second);
  }

  return m_applications;
}

bool CXBMCApp::HasLaunchIntent(const std::string &package)
{
  return (GetPackageManager().getLaunchIntentForPackage(package) != NULL);
}

bool CXBMCApp::StartAppActivity(const std::string &package, const std::string &cls)
{
  CJNIComponentName name(package, cls);
  CJNIIntent newIntent(CJNIIntent::ACTION_MAIN);

  newIntent.addCategory(CJNIIntent::CATEGORY_LAUNCHER);
  newIntent.setFlags(CJNIIntent::FLAG_ACTIVITY_NEW_TASK | CJNIIntent::FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
  newIntent.setComponent(name);

  startActivity(newIntent);
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CXBMCApp::StartActivity - ExceptionOccurred launching %s", package.c_str());
    xbmc_jnienv()->ExceptionClear();
    return false;
  }

  return true;
}

// Note intent, dataType, dataURI all default to ""
bool CXBMCApp::StartActivity(const std::string &package, const std::string &intent, const std::string &dataType, const std::string &dataURI)
{
  if (package.find('/') != std::string::npos)
  {
    std::vector<std::string> split = StringUtils::Split(package, '/');
    return StartAppActivity(split[0], split[1]);
  }

  CJNIIntent newIntent;
  if (intent.empty())
  {
    if (CAndroidFeatures::IsLeanback())
      newIntent = GetPackageManager().getLeanbackLaunchIntentForPackage(package);
    if (!newIntent)
      newIntent = GetPackageManager().getLaunchIntentForPackage(package);
  }
  else
    newIntent = CJNIIntent(intent);

  if (!newIntent)
    return false;

  if (!dataURI.empty())
  {
    CJNIURI jniURI = CJNIURI::parse(dataURI);

    if (!jniURI)
      return false;

    newIntent.setDataAndType(jniURI, dataType);
  }

  newIntent.setPackage(package);
  startActivity(newIntent);
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CXBMCApp::StartActivity - ExceptionOccurred launching %s", package.c_str());
    xbmc_jnienv()->ExceptionClear();
    return false;
  }

  return true;
}

int CXBMCApp::GetBatteryLevel()
{
  return m_batteryLevel;
}

// Used in Application.cpp to figure out volume steps
int CXBMCApp::GetMaxSystemVolume()
{
  JNIEnv* env = xbmc_jnienv();
  static int maxVolume = -1;
  if (maxVolume == -1)
  {
    maxVolume = GetMaxSystemVolume(env);
  }
  //android_printf("CXBMCApp::GetMaxSystemVolume: %i",maxVolume);
  return maxVolume;
}

int CXBMCApp::GetMaxSystemVolume(JNIEnv *env)
{
  CJNIAudioManager audioManager(getSystemService("audio"));
  if (audioManager)
    return audioManager.getStreamMaxVolume();
  android_printf("CXBMCApp::SetSystemVolume: Could not get Audio Manager");
  return 0;
}

float CXBMCApp::GetSystemVolume()
{
  CJNIAudioManager audioManager(getSystemService("audio"));
  if (audioManager)
    return (float)audioManager.getStreamVolume() / GetMaxSystemVolume();
  else
  {
    android_printf("CXBMCApp::GetSystemVolume: Could not get Audio Manager");
    return 0;
  }
}

void CXBMCApp::SetSystemVolume(float percent)
{
  CJNIAudioManager audioManager(getSystemService("audio"));
  int maxVolume = (int)(GetMaxSystemVolume() * percent);
  if (audioManager)
    audioManager.setStreamVolume(maxVolume);
  else
    android_printf("CXBMCApp::SetSystemVolume: Could not get Audio Manager");
}


void CXBMCApp::onReceive(CJNIIntent intent)
{
  std::string action = intent.getAction();
  CLog::Log(LOGDEBUG, "CXBMCApp::onReceive Got intent. Action: %s", action.c_str());
  if (action == "android.intent.action.BATTERY_CHANGED")
    m_batteryLevel = intent.getIntExtra("level",-1);
  else if (action == "android.intent.action.DREAMING_STOPPED" || action == "android.intent.action.SCREEN_ON")
  {
    if (HasFocus())
      g_application.WakeUpScreenSaverAndDPMS();
  }
  else if (action == "android.intent.action.SCREEN_OFF")
  {
    if (m_playback_state & PLAYBACK_STATE_VIDEO)
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
  }
  else if (action == "android.intent.action.HEADSET_PLUG" || action == "android.bluetooth.a2dp.profile.action.CONNECTION_STATE_CHANGED")
  {
    CheckHeadsetPlugged();
  }
  else if (action == "android.media.action.HDMI_AUDIO_PLUG")
  {
    bool newstate;
    newstate = (intent.getIntExtra("android.media.extra.AUDIO_PLUG_STATE", 0) != 0);

    if (newstate != m_hdmiPlugged)
    {
      CLog::Log(LOGDEBUG, "-- HDMI state: %s",  newstate ? "on" : "off");
      m_hdmiPlugged = newstate;
      CAEFactory::DeviceChange();
    }
  }
  else if (action == "android.intent.action.MEDIA_BUTTON")
  {
    if (m_playback_state == PLAYBACK_STATE_STOPPED)
    {
      CLog::Log(LOGINFO, "Ignore MEDIA_BUTTON intent: no media playing");
      return;
    }
    CJNIKeyEvent keyevt = (CJNIKeyEvent)intent.getParcelableExtra(CJNIIntent::EXTRA_KEY_EVENT);

    int keycode = keyevt.getKeyCode();
    bool up = (keyevt.getAction() == CJNIKeyEvent::ACTION_UP);

    CLog::Log(LOGINFO, "Got MEDIA_BUTTON intent: %d, up:%s", keycode, up ? "true" : "false");
    if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_RECORD)
      CAndroidKey::XBMC_Key(keycode, XBMCK_RECORD, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_EJECT)
      CAndroidKey::XBMC_Key(keycode, XBMCK_EJECT, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_FAST_FORWARD)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_FASTFORWARD, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_NEXT)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_NEXT_TRACK, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_PAUSE)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_PLAY_PAUSE, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_PLAY)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_PLAY_PAUSE, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_PLAY_PAUSE)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_PLAY_PAUSE, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_PREVIOUS)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_PREV_TRACK, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_REWIND)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_REWIND, 0, 0, up);
    else if (keycode == CJNIKeyEvent::KEYCODE_MEDIA_STOP)
      CAndroidKey::XBMC_Key(keycode, XBMCK_MEDIA_STOP, 0, 0, up);
  }
  else if (action == "android.net.conn.CONNECTIVITY_CHANGE")
  {
    if (g_application.IsInitialized())
    {
      CNetwork& net = g_application.getNetwork();
      CNetworkAndroid* netdroid = static_cast<CNetworkAndroid*>(&net);
      netdroid->RetrieveInterfaces();
    }
  }
}

void CXBMCApp::onNewIntent(CJNIIntent intent)
{
  std::string action = intent.getAction();
  CLog::Log(LOGDEBUG, "CXBMCApp::onNewIntent - Got intent. Action: %s", action.c_str());
  std::string targetFile = GetFilenameFromIntent(intent);
  if (!targetFile.empty() &&  (action == "android.intent.action.VIEW" || action == "android.intent.action.GET_CONTENT"))
  {
    CLog::Log(LOGDEBUG, "-- targetFile: %s", targetFile.c_str());

    CURL targeturl(targetFile);
    std::string value;
    if (action == "android.intent.action.GET_CONTENT" || (targeturl.GetOption("showinfo", value) && value == "true"))
    {
      if (targeturl.IsProtocol("videodb")
          || (targeturl.IsProtocol("special") && targetFile.find("playlists/video") != std::string::npos)
          || (targeturl.IsProtocol("special") && targetFile.find("playlists/mixed") != std::string::npos)
          )
      {
        std::vector<std::string> params;
        params.push_back(targeturl.Get());
        params.push_back("return");
        CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTIVATE_WINDOW, WINDOW_VIDEO_NAV, 0, nullptr, "", params);
      }
      else if (targeturl.IsProtocol("musicdb")
               || (targeturl.IsProtocol("special") && targetFile.find("playlists/music") != std::string::npos))
      {
        std::vector<std::string> params;
        params.push_back(targeturl.Get());
        params.push_back("return");
        CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTIVATE_WINDOW, WINDOW_MUSIC_NAV, 0, nullptr, "", params);
      }
    }
    else
    {
      CFileItem* item = new CFileItem(targetFile, false);
      if (item->IsVideoDb())
      {
        *(item->GetVideoInfoTag()) = XFILE::CVideoDatabaseFile::GetVideoTag(CURL(item->GetPath()));
        item->SetPath(item->GetVideoInfoTag()->m_strFileNameAndPath);
      }
      CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));
    }
  }
  else if (action == ACTION_XBMC_RESUME)
  {
    if (m_playback_state != PLAYBACK_STATE_STOPPED)
    {
      if (m_playback_state & PLAYBACK_STATE_VIDEO)
        RequestVisibleBehind(true);
      if (!(m_playback_state & PLAYBACK_STATE_PLAYING))
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
    }
  }
}

void CXBMCApp::onActivityResult(int requestCode, int resultCode, CJNIIntent resultData)
{
  for (auto it = m_activityResultEvents.begin(); it != m_activityResultEvents.end(); ++it)
  {
    if ((*it)->GetRequestCode() == requestCode)
    {
      m_activityResultEvents.erase(it);
      (*it)->SetResultCode(resultCode);
      (*it)->SetResultData(resultData);
      (*it)->Set();
      break;
    }
  }
}

bool CXBMCApp::GetCapture(CJNIImage& img)
{
  CSingleLock lock(m_captureMutex);
  if (m_captureQueue.empty())
    return false;

  img = m_captureQueue.front();
  m_captureQueue.pop();
  return true;
}

void CXBMCApp::TakeScreenshot()
{
  takeScreenshot();
}

void CXBMCApp::StopCapture()
{
  CSingleLock lock(m_captureMutex);
  while (!m_captureQueue.empty())
  {
    CJNIImage img = m_captureQueue.front();
    img.close();
    m_captureQueue.pop();
  }
  CJNIMainActivity::stopCapture();
}

void CXBMCApp::onCaptureAvailable(CJNIImage image)
{
  CSingleLock lock(m_captureMutex);

  m_captureQueue.push(image);
  if (m_captureQueue.size() > CAPTURE_QUEUE_MAXDEPTH)
  {
    CJNIImage img = m_captureQueue.front();
    img.close();
    m_captureQueue.pop();
  }
}

void CXBMCApp::onScreenshotAvailable(CJNIImage image)
{
  CSingleLock lock(m_captureMutex);

  m_captureEvent.SetImage(image);
  m_captureEvent.Set();
}

void CXBMCApp::onVisibleBehindCanceled()
{
  CLog::Log(LOGDEBUG, "Visible Behind Cancelled");
  m_hasReqVisible = false;

  // Pressing the pause button calls OnStop() (cf. https://code.google.com/p/android/issues/detail?id=186469)
  if ((m_playback_state & PLAYBACK_STATE_PLAYING))
  {
    if (m_playback_state & PLAYBACK_STATE_CANNOT_PAUSE)
      CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
    else if (m_playback_state & PLAYBACK_STATE_VIDEO)
      CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
  }
}

void CXBMCApp::onMultiWindowModeChanged(bool isInMultiWindowMode)
{
  CLog::Log(LOGDEBUG, "%s: %s", __PRETTY_FUNCTION__, isInMultiWindowMode ? "true" : "false");

}

void CXBMCApp::onPictureInPictureModeChanged(bool isInPictureInPictureMode)
{
  android_printf("%s: %s", __PRETTY_FUNCTION__, isInPictureInPictureMode ? "true" : "false");
  m_hasPIP = isInPictureInPictureMode;
}

void CXBMCApp::onAudioDeviceAdded(CJNIAudioDeviceInfos devices)
{
}

void CXBMCApp::onAudioDeviceRemoved(CJNIAudioDeviceInfos devices)
{
}

int CXBMCApp::WaitForActivityResult(const CJNIIntent &intent, int requestCode, CJNIIntent &result)
{
  int ret = 0;
  CActivityResultEvent* event = new CActivityResultEvent(requestCode);
  m_activityResultEvents.push_back(event);
  startActivityForResult(intent, requestCode);
  if (event->Wait())
  {
    result = event->GetResultData();
    ret = event->GetResultCode();
  }
  delete event;
  return ret;
}

bool CXBMCApp::WaitForCapture(CJNIImage& image)
{
  bool ret = false;
  if (m_captureEvent.WaitMSec(2000))
  {
    image = m_captureEvent.GetImage();
    ret = true;
  }
  m_captureEvent.Reset();
  return ret;
}

void CXBMCApp::onVolumeChanged(int volume)
{
  // System volume was used; Reset Kodi volume to 100% if it'not, already
  if (g_application.GetVolume(false) != 1.0)
    CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(
                                                 new CAction(ACTION_VOLUME_SET, static_cast<float>(CXBMCApp::GetMaxSystemVolume()))));
}

void CXBMCApp::onAudioFocusChange(int focusChange)
{
  CLog::Log(LOGDEBUG, "Audio Focus changed: %d", focusChange);
  if (focusChange == CJNIAudioManager::AUDIOFOCUS_LOSS ||
      focusChange == CJNIAudioManager::AUDIOFOCUS_LOSS_TRANSIENT ||
      focusChange == CJNIAudioManager::AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK
      )
  {
    m_hasAudioFocus = false;

    if ((m_playback_state & PLAYBACK_STATE_PLAYING))
    {
      if (m_playback_state & PLAYBACK_STATE_CANNOT_PAUSE)
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_STOP)));
      else
        CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PAUSE)));
    }
  }
/*
  else if (focusChange == CJNIAudioManager::AUDIOFOCUS_GAIN)
  {
    m_hasAudioFocus = true;
    registerMediaButtonEventReceiver();
    if (m_playback_state && !(m_playback_state & PLAYBACK_STATE_PLAYING))
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PLAYER_PLAY)));
  }
*/
}

void CXBMCApp::doFrame(int64_t frameTimeNanos)
{
  m_vsynctime = frameTimeNanos;
  m_vsyncEvent.Set();
}

bool CXBMCApp::WaitVSync(unsigned int milliSeconds)
{
  return m_vsyncEvent.WaitMSec(milliSeconds);
}

std::string CXBMCApp::GetFilenameFromIntent(const CJNIIntent &intent)
{
    std::string ret;
    if (!intent)
      return ret;
    CJNIURI data = intent.getData();
    if (!data)
      return ret;
    std::string scheme = data.getScheme();
    StringUtils::ToLower(scheme);
    if (scheme == "content")
    {
      std::vector<std::string> filePathColumn;
      filePathColumn.push_back(CJNIMediaStoreMediaColumns::DATA);
      CJNICursor cursor = getContentResolver().query(data, filePathColumn, std::string(), std::vector<std::string>(), std::string());
      if(cursor.moveToFirst())
      {
        int columnIndex = cursor.getColumnIndex(filePathColumn[0]);
        ret = cursor.getString(columnIndex);
      }
      cursor.close();
    }
    else if(scheme == "file")
      ret = data.getPath();
    else
      ret = data.toString();
  return ret;
}

const ANativeWindow** CXBMCApp::GetNativeWindow(int timeout)
{
  if (m_window)
    return (const ANativeWindow**)&m_window;

  if (m_mainView)
    m_mainView->waitForSurface(timeout);

  return (const ANativeWindow**)&m_window;
}

void CXBMCApp::RegisterInputDeviceCallbacks(IInputDeviceCallbacks* handler)
{
  if (handler == nullptr)
    return;

  m_inputDeviceCallbacks = handler;
}

void CXBMCApp::UnregisterInputDeviceCallbacks()
{
  m_inputDeviceCallbacks = nullptr;
}

void CXBMCApp::onInputDeviceAdded(int deviceId)
{
  CXBMCApp::android_printf("Input device added: %d", deviceId);

  if (m_inputDeviceCallbacks != nullptr)
    m_inputDeviceCallbacks->OnInputDeviceAdded(deviceId);
}

void CXBMCApp::onInputDeviceChanged(int deviceId)
{
  CXBMCApp::android_printf("Input device changed: %d", deviceId);

  if (m_inputDeviceCallbacks != nullptr)
    m_inputDeviceCallbacks->OnInputDeviceChanged(deviceId);
}

void CXBMCApp::onInputDeviceRemoved(int deviceId)
{
  CXBMCApp::android_printf("Input device removed: %d", deviceId);

  if (m_inputDeviceCallbacks != nullptr)
    m_inputDeviceCallbacks->OnInputDeviceRemoved(deviceId);
}

void CXBMCApp::RegisterInputDeviceEventHandler(IInputDeviceEventHandler* handler)
{
  if (handler == nullptr)
    return;

  m_inputDeviceEventHandler = handler;
}

void CXBMCApp::UnregisterInputDeviceEventHandler()
{
  m_inputDeviceEventHandler = nullptr;
}

bool CXBMCApp::onInputDeviceEvent(const AInputEvent* event)
{
  if (m_inputDeviceEventHandler != nullptr)
    return m_inputDeviceEventHandler->OnInputDeviceEvent(event);

  return false;
}

void CXBMCApp::CalculateGUIRatios()
{
  m_droid2guiRatio = CRect(0.0, 0.0, 1.0, 1.0);

  if(!m_xbmcappinstance || !m_surface_rect.Width() || !m_surface_rect.Height())
    return;

  RESOLUTION_INFO res_info = g_graphicsContext.GetResInfo();
  float curRatio = (float)res_info.iWidth / res_info.iHeight;
  float newRatio = (float)m_surface_rect.Width() / m_surface_rect.Height();

  res_info.fPixelRatio = newRatio / curRatio;
  g_graphicsContext.SetResInfo(g_graphicsContext.GetVideoResolution(), res_info);

  CRect gui = CRect(0, 0, res_info.iWidth, res_info.iHeight);
  m_droid2guiRatio.x1 = m_surface_rect.x1;
  m_droid2guiRatio.y1 = m_surface_rect.y1;
  m_droid2guiRatio.x2 = gui.Width() / (double)m_surface_rect.Width();
  m_droid2guiRatio.y2 = gui.Height() / (double)m_surface_rect.Height();

  CLog::Log(LOGDEBUG, "%s(gui scaling) - %f, %f", __PRETTY_FUNCTION__, m_droid2guiRatio.x2, m_droid2guiRatio.y2);
}


void CXBMCApp::surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height)
{
  CLog::Log(LOGDEBUG, "%s: %d x %d", __PRETTY_FUNCTION__, width, height);
}

void CXBMCApp::surfaceCreated(CJNISurfaceHolder holder)
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);
  m_window = ANativeWindow_fromSurface(xbmc_jnienv(), holder.getSurface().get_raw());
  if (m_window == NULL)
  {
    android_printf(" => invalid ANativeWindow object");
    return;
  }
  CApplicationMessenger::GetInstance().PostMsg(TMSG_DISPLAY_SETUP);
}

void CXBMCApp::surfaceDestroyed(CJNISurfaceHolder holder)
{
  CLog::Log(LOGDEBUG, "%s", __PRETTY_FUNCTION__);
  // If we have exited XBMC, it no longer exists.
  if (!m_exiting)
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_DISPLAY_CLEANUP);
    m_window = NULL;
  }
}

void CXBMCApp::onLayoutChange(int left, int top, int width, int height)
{
  CSingleLock lock(m_LayoutMutex);

  m_surface_rect.x1 = left;
  m_surface_rect.y1 = top;
  m_surface_rect.x2 = left + width;
  m_surface_rect.y2 = top + height;

  CLog::Log(LOGDEBUG, "%s: %f + %f - %f x %f", __PRETTY_FUNCTION__, m_surface_rect.x1, m_surface_rect.y1, m_surface_rect.Width(), m_surface_rect.Height());

  if (g_application.GetRenderGUI())
    CalculateGUIRatios();
}

