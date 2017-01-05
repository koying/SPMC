SRCS  = NetworkAndroid.cpp 
SRCS += ZeroconfAndroid.cpp 
SRCS += ZeroconfBrowserAndroid.cpp 

LIB=network_android.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
