ARCH=arm

INCLUDES+=-I/src/spmc-isengard/xbmc/cores/dvdplayer
INCLUDES+=-I..

SRCS = StageFrightInterface.cpp
SRCS += RkStagefrightVideo.cpp
SRCS += StageFrightVideoPrivate.cpp
INCLUDES += -I${prefix}/opt/android-source/frameworks/base/include
INCLUDES += -I${prefix}/opt/android-source/frameworks/base/native/include
INCLUDES += -I${prefix}/opt/android-source/frameworks/base/include/media/stagefright
INCLUDES += -I${prefix}/opt/android-source/frameworks/base/include/media/stagefright/openmax
INCLUDES += -I${prefix}/opt/android-source/system/core/include
INCLUDES += -I${prefix}/opt/android-source/libhardware/include
INCLUDES += -I${prefix}/opt/rkvpu/src/common
INCLUDES += -I${prefix}/opt/rkvpu/src/common/include

LIBNAME=libXBMCvcodec_stagefrightRK
LIB_SHARED=/src/spmc-isengard/system/players/dvdplayer/$(LIBNAME)-$(ARCH).so

LIBS += -landroid -lEGL -lGLESv2 -L${prefix}/opt/android-libs -L${prefix}/opt/rkvpu/lib -lstdc++ -lutils -lcutils -lstagefright -lbinder -lui -lgui -lvpu -L/src/spmc-isengard -lspmc15

all: $(LIB_SHARED)

include /src/spmc-isengard/Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

JNICXXFLAGS = $(CXXFLAGS) -std=gnu++0x -Wextra
STFCXXFLAGS = $(CXXFLAGS) -Wno-multichar -fno-rtti -fPIC

$(LIB_SHARED): $(OBJS)
	$(CXX) $(STFCXXFLAGS) $(LDFLAGS) -shared -Wl,-no-undefined -g -o $(LIB_SHARED) $(OBJS) $(LIBS)

%.o : %.cpp
	$(CXX) -MF $*.d -MD -c $(STFCXXFLAGS) $(DEFINES) $(INCLUDES) $< -o $@

CLEAN_FILES = \
	$(LIB_SHARED) \

DISTCLEAN_FILES= \
	Makefile \

