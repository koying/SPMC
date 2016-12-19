<div markdown="1">
  <a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=E9WN2M4HJAQZS"><img align="right" src="https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif" alt="Donate" /></a>
</div>

# Jarvis

As far as I'm concerned, Nvidia Shield has basically cornered the market, so is my main development device. More generally, Android TV devices, amlogic devices and Amazon FireTV *might* be getting some love ;)  
**Important: No more Rockchip specific support as of this version**

**Note 1:** As of 16.5.0, Android lollipop (API 21) is required  
**Note 2:** For recommendations and android voice search, SPMC must be already running.  

Latest Jarvis build is: **16.5.2** [link](https://github.com/koying/SPMC/releases/tag/16.5.2-spmc)   
APK: [link](https://github.com/koying/SPMC/releases/download/16.5.2-spmc/SPMC-16.5.2-spmc-2d86e3f-armeabi-v7a.apk)  
Launcher APK: [link](https://github.com/koying/SPMC/releases/download/16.5.2-spmc/SPMC-16.5.2-spmc-2d86e3f-armeabi-v7a_launcher.apk)  
X86 APK: [link](https://github.com/koying/SPMC/releases/download/16.5.2-spmc/SPMC-16.5.2-spmc-2d86e3f-x86.apk)  
Google Play: [link](https://play.google.com/store/apps/details?id=com.semperpax.spmc16)  
Issues and questions: [github](https://github.com/koying/SPMC/issues)  
FAQ: [wiki] (https://github.com/koying/SPMC/wiki)

Changelog:

### 16.5.2 (2016/12/11):

- FIX: Regressions
- ADD: SD h/w setting for HEVC

### 16.5.1 (2016/12/08):

- FIX: 4K on AFTV2
- FIX: Crash in new Network code
- FIX: Crash when exiting on aarch64
- FIX: Crash on AML 6.01 FW
- ADD: Limit AC3 encoder to 384kb/s with Quality < High

### 16.5.0 (2016/11/30):

- CHG: API 21+ (Android 5.0) only, sorry.
- ADD: 4K on AFTV2
- ADD: refresh rate sync on AFTV
- ADD: DASH support (aka 1080p/4K youtube)
- ADD: Builtin log (Settings-System-Logging-Upload...)
- ADD: crash handler (no need for logcat)
- ADD: rendercapture (boblight, Hue, ...) support for Surface
- FIX: Only stop Surface when really necessary
- CHG: Revert to Surface as standard
- ADD: acceleration setting for hevc

### 16.4.2 (2016/08/15):

- REVERT: crash when enumerating sound devices during a refresh rate change
- FIX: Missing start of video with large buffer

### 16.4.1 (2016/08/01):

- FIX: stalls after seek
- FIX: [includeall] avoid double episodes

### 16.4.0 (2016/07/14):

- **CHG: Enable YADIF for arm**
- ADD: Export single item to NFO via context menu
- FIX: Properly cache if both audio and video are starving
- FIX: GLES font corruption inevitable after few days runtime
- FIX: [shield] properly handle wifi direct headset
- CHG: [leanback] force recommendation update every hour

### 16.3.0 (2016/05/01):

- **ADD: HQ scalers (Shield only, afaik)**
- **ADD: GUI selection of H/W decoders**
- **ADD: [AFTV(S)] get real HDMI refresh rate**
- **ADD: additional "ffmpeg" s/w deinterlacer**
- FIX: Save skin settings immediately
- FIX: lockup with Twitch addon
- FIX: stream details issues
- FIX: sorting issues
- FIX: background music issues
- Merge Kodi 16.1

### 16.2.1 (2016/04/14):

- ADD: Boost center channel on downmix
- FIX: UTF8 formatted labels
- FIX: stream details detection
- FIX: "block" fonts

### 16.2.0 (2016/04/02):

- **Add voice recognition to soft keyboard** (cf. [faq] (https://github.com/koying/SPMC/wiki/How-to-use-the-voice-recognition-feature) )
- **Add Thumbnails cleanup** (cf. [faq] (https://github.com/koying/SPMC/wiki/Where-is-the-%22Clean-Thumbnails%22-option%3F) )
- [Shield] fallback to s/w for 3D (still issues with HTAB/HSBS)
- (re-)Add reboot & shutdown on rooted devices
- Add support for AudioFormat::ENCODING_IEC61937 (Android N IEC passthrough)
- Expand recommendations and voice search to tv shows and music albums
- Fix IEC passthrough volume issue
- TrueHD PT buffer tweaks
- Backport "disable PT when sync to display is enabled"

### 16.1.2 (2016/03/20):

- Fix execution on Android < 4.4
- Fix voice search (+ enable Web server by default)
- Add spmc_env.properties support; fallback on xbmc_env.properties
- Add possibility to choose IEC or RAW passthrough (via passthrough device)
- Fix low sample rates audio
- Revert to using xbmclogs.com

### 16.1.1 (2016/03/13):

- Fix DTS issue
- Fix Mediacodec surface in 4K GUI
- "Back" = Stop in fullscreen video

### 16.1.0 (2016/03/13):

- Based on Kodi 16.1
- HD Audio passthrough (Shield + some aml, Minix U1 & Wetek Core at least)
- Autostart at boot (xbmc.autostart=true in xbmc_env.properties)
- GUI size setting (Allow 4k gui on Shield)
- Only give codec priority to Mediacodec(Surface) for 4K
- Only stop video on minimize for Mediacodec(Surface)
- "System" screensaver to enable daydream; all other screensavers disable daydream
- Get removable storage names from system
- Basic Bluray 3D iso playback (experimental; amlogic)
- Android TV recommendations and voice search (experimental)
- SSL enabled MySql
- Stop video playback when screen goes off
- Setting to force SMB v1 (might solve windows connection issues)
- The ever good ol' "Import All" option

# Isengard

By popular demand...   
Most of SPMC is merged in Kodi besides the rockchip specifics.

Latest Helix build is: **15.0.0** [link](https://github.com/koying/SPMC/releases/tag/15.0.0-spmc)   
APK: [link](https://github.com/koying/SPMC/releases/download/15.0.0-spmc/spmc-armeabi-v7a_15.0.0.apk)  
launcher APK: [link](https://github.com/koying/SPMC/releases/download/15.0.0-spmc/spmc-armeabi-v7a_15.0.0_launcher.apk)  
X86 APK: [link](https://github.com/koying/SPMC/releases/download/15.0.0-spmc/spmc-x86_15.0.0.apk)  

Changelog:

### 15.0.0 (2015/07/25):

# Helix
Those Helix builds will contain XBMC patches which were not accepted, or are not acceptable, in the main XBMC tree. 

Latest Helix build is: **14.2.0** [link](https://github.com/koying/SPMC/releases/tag/14.2.0-spmc)   
APK: [link](https://github.com/koying/SPMC/releases/download/14.2.0-spmc/spmc-armeabi-v7a_14.2.0.apk)  
launcher APK: [link](https://github.com/koying/SPMC/releases/download/14.2.0-spmc/spmc-armeabi-v7a_14.2.0_launcher.apk)  
X86 APK: [link](https://github.com/koying/SPMC/releases/download/14.2.0-spmc/spmc-x86_14.2.0.apk)  

Google play store:
SPMC Helix is in the Beta channel. To get access:
- Join the [G+ community here](https://plus.google.com/communities/113692037565988860301)
- Go to https://play.google.com/apps/testing/com.semperpax.spmc to enroll

Changelog:

### 14.2.0 (2015/04/04):
- Rebased on last Kodi Helix (14.2)
- Backport of Incremental seeking
- Use the device volume controls (if any)
- Use <TAB> in virtual keyboard to toggle use of left/right/enter the keyboard way
- Generate minidumps in case of crashes (no more logcats :) )
- Use Samba 3.6 (gplv3)
- Release build :D

### 14.1.0 (2015/02/07):
- Rebased on Kodi Helix 14.1
- Added BOB de-interlacing
- Fixed Video settings availability
- Removed hardcoded 250ms audio delay on AML
- Auto-3D switching on AFTV
- Add "uhd" (4K) to codec selection in Advanced Settings

# Gotham

**Warning: as of 13.3.3, the non-launcher version is default, including in stores**

Last Gotham build is: **13.4.0** [link](https://github.com/koying/SPMC/releases/tag/13.4.0-spmc)   
APK: [link](https://github.com/koying/SPMC/releases/download/13.4.0-spmc/spmc-armeabi-v7a_13.4.0.apk)  
launcher APK: [link](https://github.com/koying/SPMC/releases/download/13.4.0-spmc/spmc-armeabi-v7a_13.4.0_launcher.apk)

Google Play: [link](https://play.google.com/store/apps/details?id=com.semperpax.spmc)  
Amazon Appstore: [link](http://www.amazon.com/Semperpax-SPMC/dp/B00MK49LL8/ref=sr_1_1?s=mobile-apps&ie=UTF8&qid=1407661207&sr=1-1&keywords=spmc)

Changelog:

### 13.4.0 (2014/12/14):
- Improved Rockchip support
  - overlay, hevc + true 4K on rk3288
- Auto-framerate switching for Amlogic
  - (Needs 666 on /sys/class/display/mode)
- Auto-framerate switching for Rockchip
  - (Needs 666 on /sys/class/display/display0.HDMI/mode)
- Allow forcing passthrough hack on Rockchip via AS (<libMediaPassThroughHack>)
- Better Android apps icons
- Add direct access to common Android settings
- Android TV banner
- Fix Nexus 9
- Bump libbluray & ffmpeg
- Fix amlogic black screen on recent SDK
- ... and the usual bunch of bugfixes ;)

### 13.3.3 (2014/10/26):
- Preliminary support for h265 (including RK3288)
- Preliminary support for Android TV
- Allow sleep and Daydream to kick in (e.g. on Amazon Fire TV)
- Add setting to scrape all videos, even if they failed 
- Add "sd" & "hd" to the advancedsettings of libstagefright (and mediacodec) for fine tuning
- Allow fullscreen on JB
- Hide irrelevant video & audio settings
- Another possible fix for start crash
- Make the "no launcher" version default

### 13.3.2 (2014/08/15):
- Backport ffmpeg 2.3
- Fix suspend
- Fix SPMC closing when adding/removing a keyboard with touchpad
- Possible fix for hiccups after pausing for a long time on amlogic
- Fix passthrough not working on Rockchip (with media hack): remove minix check 
- Do not pause music when going to background
- Allow multi-threaded in fallback s/w decoding
- Add option to wait for network (via xbmc_env.properties)
- Possible fix for crash on startup (previous one didn't solve it)
- disable libstagefright on Nvidia Shield (crash)
- Fix Python PIL

### 13.3.1 (2014/08/06):
- Rebased to latest XBMC 13.2
- Add suspend on rooted devices (thanks to @elmerohueso)
- Possible fix for recurrent startup crash
- Switch TV to 3D automatically on amlogic
- Allow mouse long left click to bring up context menu

### 13.3.0 (2014/07/26):
- Rebased to latest XBMC 13.2
- Fix remaining issue with some h264 BD rips
- Fix compatibility with KitKat releases on Rockchip
- Remove "su" tinkering on Amlogic. This should be fixed in firmware
- Fix crash when Android apps are favourited, then uninstalled. 
Probably also fixes crash issue with "amber"-like skins which do not have a dedicated icon for the "Android app" category.
- Fix issue where scanning from local nfo did not work for episodes anymore

### 13.2.1 (2014/07/16):
- New graphics (thanks to @Tinwarble from OuyaForum)
- Register as launcher
- Important fix for some MakeMKV et al 1-to-1 BD rips

### 13.2.0 (2014/07/10):
- Initial Gotham release
- OUYA specifics
  - OUYA app specifics
  - AC3/DTS passthrough
- Rockchip enhancements
  - Better H/W acceleration via private API
- AML enhacements
  - 4K on M8
- Change data location:  
  https://github.com/koying/SPMC/wiki/How-to-specify-the-location-of-SPMC-data%3F

See https://github.com/koying/SPMC/commits/gotham for the full list.

# Frodo

Last Frodo build is: **12.4.2**  
Torrent: http://filez.semperpax.com/ykyk6hye  
APK: http://filez.semperpax.com/5r4ln85l  
Sources: https://github.com/koying/SPMC

Changelog:  
### 12.4.2:
- Pack bluray library
- Fix libstagefright bug
- Remove unnecessary joystick keymaps

### 12.4.1:
- Added mouse wheel and proper button support
- Allow reboot/restart on rooted devices
- Runs on kitkat