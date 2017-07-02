set(SOURCES 
  DVDDemuxCDDA.cpp
  DVDDemuxUtils.cpp
  DVDDemux.cpp
  DVDDemuxPVRClient.cpp
  DVDDemuxCC.cpp
  DVDDemuxVobsub.cpp
  DVDDemuxFFmpeg.cpp
  DVDFactoryDemuxer.cpp
  DVDDemuxBXA.cpp
  DVDDemuxShoutcast.cpp

)

set(HEADERS 
  DVDDemuxUtils.h
  DVDDemuxFFmpeg.h
  DVDDemuxPVRClient.h
  DVDDemuxShoutcast.h
  DVDDemuxBXA.h
  DVDDemuxCC.h
  DVDDemuxPacket.h
  DVDFactoryDemuxer.h
  DVDDemuxVobsub.h
  DVDDemux.h
  DVDDemuxCDDA.h

)

core_add_library(dvddemuxers)
