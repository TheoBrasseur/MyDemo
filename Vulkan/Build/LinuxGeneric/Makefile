#
# File			Makefile
# Title			Example makefile
#
# Copyright		Copyright (C) Imagination Technologies Limited.
#

.PHONY: clean

SDKDIR  = ../../../../../..
OUTNAME = VulkanMyDemo
DEPENDS =  PVRCore PVRAssets PVRVulkanGlue PVRNativeVulkan PVRVulkan PVRShell PVRUIRenderer
LIBRARIES =  PVRUIRenderer PVRShell PVRVulkan PVRNativeVulkan PVRVulkanGlue PVRAssets PVRCore
LIBPATHS =  ../../../../../../Framework/Bin/Linux_$(PLAT_SUFFIX)/$(DEBUG_RELEASE)$(WS)

OBJECTS +=	\
           Vulkan/VulkanMyDemo.o

RESOURCES += \
           FragShader_vk.fsh\
           FragShader_vk.fsh.spv\
           VertShader_vk.vsh\
           VertShader_vk.vsh.spv\

COMMON_RESOURCES += \
           teapot.pod\


include $(SDKDIR)/Builds/Linux/make_example.mak
