set(SOURCES 
  DVDSubtitleParserSubrip.cpp
  DVDSubtitleTagSami.cpp
  DVDSubtitleParserSSA.cpp
  DVDSubtitleParserSami.cpp
  DVDSubtitleParserMicroDVD.cpp
  DVDSubtitleParserMPL2.cpp
  DVDSubtitlesLibass.cpp
  DVDSubtitleTagMicroDVD.cpp
  DVDSubtitleParserVplayer.cpp
  DVDSubtitleLineCollection.cpp
  DVDFactorySubtitle.cpp
  DVDSubtitleStream.cpp

)

set(HEADERS 
  DVDSubtitleTagSami.h
  DVDSubtitleTagMicroDVD.h
  DVDSubtitleParserSSA.h
  DVDSubtitleParserSami.h
  DVDSubtitleParserVplayer.h
  DVDSubtitleParser.h
  DVDSubtitlesLibass.h
  DVDSubtitleParserSubrip.h
  DllLibass.h
  DVDSubtitleParserMicroDVD.h
  DVDFactorySubtitle.h
  DVDSubtitleStream.h
  DVDSubtitleParserMPL2.h
  DVDSubtitleLineCollection.h

)

core_add_library(dvdsubtitles)
