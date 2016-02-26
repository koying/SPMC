SRCS=VideoSyncGLX.cpp \
     VideoSyncOsx.cpp \
     VideoSyncIos.cpp \
     VideoSyncDRM.cpp \
     VideoSyncPi.cpp \
     VideoSyncAndroid.cpp \

LIB=videosync.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))