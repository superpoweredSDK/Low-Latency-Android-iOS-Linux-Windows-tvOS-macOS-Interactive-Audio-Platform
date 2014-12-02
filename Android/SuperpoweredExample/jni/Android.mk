LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := Superpowered
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_SRC_FILES := libSuperpoweredARM.a
else
	LOCAL_SRC_FILES := libSuperpoweredX86.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)  
LOCAL_MODULE := SuperpoweredExample  
LOCAL_SRC_FILES := SuperpoweredExample.cpp
LOCAL_LDLIBS := -llog -landroid -lOpenSLES 
LOCAL_STATIC_LIBRARIES := Superpowered
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_CFLAGS = -mfloat-abi=softfp -mfpu=neon -O3 -DHAVE_NEON=1
else
	LOCAL_CFLAGS = -O3
endif
include $(BUILD_SHARED_LIBRARY)
