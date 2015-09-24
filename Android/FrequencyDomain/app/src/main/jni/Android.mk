LOCAL_PATH := $(call my-dir)
SUPERPOWERED_PATH := ../../../../../../Superpowered

include $(CLEAR_VARS)
LOCAL_MODULE := Superpowered
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_SRC_FILES := $(SUPERPOWERED_PATH)/libSuperpoweredAndroidARM.a
else
	ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
		LOCAL_SRC_FILES := $(SUPERPOWERED_PATH)/libSuperpoweredAndroidARM64.a
	else
		ifeq ($(TARGET_ARCH_ABI),x86_64)
			LOCAL_SRC_FILES := $(SUPERPOWERED_PATH)/libSuperpoweredAndroidX86_64.a
		else
			LOCAL_SRC_FILES := $(SUPERPOWERED_PATH)/libSuperpoweredAndroidX86.a
		endif
	endif
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)  
LOCAL_MODULE := FrequencyDomain

LOCAL_SRC_FILES :=  \
$(SUPERPOWERED_PATH)/SuperpoweredAndroidAudioIO.cpp  \
FrequencyDomain.cpp
LOCAL_C_INCLUDES += $(SUPERPOWERED_PATH)

LOCAL_LDLIBS := -llog -landroid -lOpenSLES 
LOCAL_STATIC_LIBRARIES := Superpowered
LOCAL_CFLAGS = -O3
include $(BUILD_SHARED_LIBRARY)
