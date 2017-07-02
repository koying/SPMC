set(SOURCES 
  DVDClock.cpp
  DVDDemuxSPU.cpp
  DVDOverlayRenderer.cpp
  DVDPlayerSubtitle.cpp
  DVDMessageQueue.cpp
  DVDStreamInfo.cpp
  Edl.cpp
  DVDFileInfo.cpp
  DVDPlayerVideo.cpp
  DVDOverlayContainer.cpp
  DVDAudio.cpp
  DVDPlayerAudio.cpp
  DVDPlayer.cpp
  DVDMessage.cpp
  DVDPlayerRadioRDS.cpp
  DVDTSCorrection.cpp
  DVDPlayerTeletext.cpp

)

set(HEADERS 
  DVDDemuxSPU.h
  DVDOverlayRenderer.h
  DVDMessage.h
  DVDPlayerVideo.h
  Edl.h
  DVDPlayerAudio.h
  DVDMessageQueue.h
  DVDResource.h
  DVDPlayerRadioRDS.h
  IDVDPlayer.h
  DVDClock.h
  DVDTSCorrection.h
  DVDPlayerSubtitle.h
  DVDOverlayContainer.h
  DVDStreamInfo.h
  DVDFileInfo.h
  DVDAudio.h
  DVDPlayer.h
  DVDPlayerTeletext.h

)

core_add_library(droidVplayer)
