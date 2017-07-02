set(SOURCES 
  DVDAudioCodecPassthrough.cpp
  DVDAudioCodecFFmpeg.cpp

)

set(HEADERS 
  DVDAudioCodecFFmpeg.h
  DVDAudioCodec.h
  DVDAudioCodecPassthrough.h

)

core_add_library(dvdcodecsaudio)
