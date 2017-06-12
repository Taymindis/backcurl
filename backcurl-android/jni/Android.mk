LOCAL_PATH := $(call my-dir)

#cURL prebuilt
include $(CLEAR_VARS)
LOCAL_MODULE := curl-prebuilt
LOCAL_SRC_FILES := \
  ../../curl-android-ios/prebuilt-with-ssl/android/$(TARGET_ARCH_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)
################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := libbackcurl

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/../../backcurl-core/header \
		$(LOCAL_PATH)/../../curl-android-ios/prebuilt-with-ssl/android/include

LOCAL_SRC_FILES +=  ../../backcurl-core/src/BackCurl.cpp

#LOCAL_SHARED_LIBRARIES := -lz
LOCAL_STATIC_LIBRARIES := curl-prebuilt
# LOCAL_EXPORT_LDLIBS := -lz
# LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/.

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)