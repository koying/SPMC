set(SOURCES 
  DVDStateSerializer.cpp
  DVDInputStreamHttp.cpp
  DVDInputStreamPVRManager.cpp
  DVDInputStreamMemory.cpp
  DVDInputStreamFFmpeg.cpp
  DVDFactoryInputStream.cpp
  DVDInputStreamRTMP.cpp
  DVDInputStreamFile.cpp
  DVDInputStreamNavigator.cpp
  DVDInputStream.cpp
  DVDInputStreamStack.cpp

)

set(HEADERS 
  DVDStateSerializer.h
  DVDInputStream.h
  DVDFactoryInputStream.h
  DVDInputStreamFFmpeg.h
  DVDInputStreamNavigator.h
  DVDInputStreamRTMP.h
  DVDInputStreamPVRManager.h
  DVDInputStreamStack.h
  DllDvdNav.h
  DVDInputStreamFile.h
  DllLibRTMP.h
  DVDInputStreamMemory.h
  DVDInputStreamHttp.h

)

if(BLURAY_FOUND)
  list(APPEND SOURCES DVDInputStreamBluray.cpp)
  list(APPEND HEADERS DVDInputStreamBluray.h)
endif()

core_add_library(dvdinputstreams)
