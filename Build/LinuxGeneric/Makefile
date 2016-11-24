
#
# File			Makefile
# Title			Example makefile
#
# Copyright		Copyright (C) Imagination Technologies Limited.
#

.PHONY: clean

SDKDIR  = ../../../../../..
OUTNAME = MyDemo
DEPENDS =  PVRCore PVRAssets PVREgl PVRNativeGles PVRShell
LIBRARIES =  PVRShell PVRNativeGles PVREgl PVRAssets PVRCore
LIBPATHS =  ../../../../../../Framework/Bin/Linux_$(PLAT_SUFFIX)/$(DEBUG_RELEASE)$(WS)

OBJECTS +=	\
			OGLES/MyDemo.o

RESOURCES += \
			 FragShader_ES3.fsh\
			 VertShader_ES3.vsh\

COMMON_RESOURCES += \
					Marble.pvr\


include $(SDKDIR)/Builds/Linux/make_example.mak
