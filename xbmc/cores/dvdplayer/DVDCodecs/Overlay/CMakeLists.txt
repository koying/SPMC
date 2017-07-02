set(SOURCES 
  DVDOverlayCodecFFmpeg.cpp
  DVDOverlayCodecText.cpp
  DVDOverlayCodecTX3G.cpp
  DVDOverlayCodecSSA.cpp
  DVDOverlayCodec.cpp

)

set(HEADERS 
  DVDOverlay.h
  DVDOverlaySSA.h
  DVDOverlayCodecFFmpeg.h
  DVDOverlayImage.h
  DVDOverlayText.h
  DVDOverlayCodecSSA.h
  DVDOverlaySpu.h
  DVDOverlayCodec.h
  DVDOverlayCodecText.h
  DVDOverlayCodecTX3G.h

)

core_add_library(dvdcodecsoverlay)
