set(SOURCES 
  helpers.cpp
  DASHByteStream.cpp
  DASHSession.cpp
  oscompat.cpp
  DASHCodecHandler.cpp
  DASHStream.cpp
  DASHFragmentedSampleReader.cpp

  common/AdaptiveTree.cpp
  parsers/DASHTree.cpp
  parsers/SmoothTree.cpp
)

set(HEADERS 
  DASHStream.h
  DASHFragmentObserver.h
  oscompat.h
  helpers.h
  DASHSession.h
  DASHFragmentedSampleReader.h
  DASHByteStream.h
  DASHCodecHandler.h

  common/AdaptiveTree.h
  parsers/DASHTree.h
  parsers/SmoothTree.h
)

core_add_library(droidVadaptive)
