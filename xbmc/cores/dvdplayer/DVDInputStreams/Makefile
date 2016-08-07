CXXFLAGS += -D__STDC_FORMAT_MACROS \
          -DENABLE_DVDINPUTSTREAM_STACK \

SRCS=	DVDFactoryInputStream.cpp \
	DVDInputStream.cpp \
	DVDInputStreamBluray.cpp \
	DVDInputStreamFFmpeg.cpp \
	DVDInputStreamFile.cpp \
	DVDInputStreamHttp.cpp \
	DVDInputStreamMemory.cpp \
	DVDInputStreamNavigator.cpp \
	DVDInputStreamRTMP.cpp \
	DVDInputStreamPVRManager.cpp \
	DVDInputStreamStack.cpp \
	DVDStateSerializer.cpp \

# MPD
SRCS += \
        DVDInputStreamMPD.cpp\
        dash/DASHStream.cpp \
        dash/DASHTree.cpp \
        dash/DASHCodecHandler.cpp \
        dash/DASHByteStream.cpp \
        dash/DASHFragmentedSampleReader.cpp \
        dash/DASHSession.cpp \
        dash/helpers.cpp \
        dash/oscompat.cpp

LIB=	DVDInputStreams.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

