
LOCAL_PATH := $(call my-dir)/../../src
SUPERPOWERED_PATH := $(call my-dir)/../../../Superpowered

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

FILE_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_C_INCLUDES :=\
$(SUPERPOWERED_PATH) \
$(LOCAL_PATH)

LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_STATIC_LIBRARIES := Superpowered
LOCAL_MODULE := AudioPluginSuperpoweredSpatializer
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_CFLAGS = -mfloat-abi=softfp -mfpu=neon -O3 -DHAVE_NEON=1
else
	LOCAL_CFLAGS = -O3 -fno-stack-protector
endif

include $(BUILD_SHARED_LIBRARY)
