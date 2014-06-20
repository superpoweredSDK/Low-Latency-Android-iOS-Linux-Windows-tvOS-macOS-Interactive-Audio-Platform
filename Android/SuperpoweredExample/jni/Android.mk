LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := Superpowered
LOCAL_SRC_FILES := libSuperpoweredARM.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)  
LOCAL_MODULE := SuperpoweredExample  
LOCAL_SRC_FILES := SuperpoweredExample.cpp
LOCAL_LDLIBS := -llog -landroid -lOpenSLES 
LOCAL_STATIC_LIBRARIES := Superpowered
LOCAL_CFLAGS = -mfloat-abi=softfp -mfpu=neon -O3
include $(BUILD_SHARED_LIBRARY)
