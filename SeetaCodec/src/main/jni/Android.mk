LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE := opencv-prebuilt
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../../SeetaFaceSDK/android/SeetaSdk/third-party/opencv/libs/$(TARGET_ARCH_ABI)/libopencv_java3.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../SeetaFaceSDK/android/SeetaSdk/third-party/opencv/jni/include/
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := live555
LOCAL_SRC_FILES := $(LOCAL_PATH)/live555/libs/$(TARGET_ARCH_ABI)/liblive555.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)


LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := rtsp_client

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
APP_CPPFLAGS += -mfloat-abi=softfp -mfpu=neon -fpermissive
endif

MY_CPP_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)
#MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/live555/*.cpp)
#MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/live555/BasicUsageEnvironment/*.cpp)
#MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/live555/UsageEnvironment/*.cpp)
#MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/live555/UsageEnvironment/*.c)
#MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/live555/groupsock/*.cpp)
#MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/live555/groupsock/*.c)
#MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/live555/liveMedia/*.cpp)
#MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/live555/liveMedia/*.c)

LOCAL_SRC_FILES := $(MY_CPP_LIST:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/\
	$(LOCAL_PATH)/live555 \
	$(LOCAL_PATH)/live555/include/BasicUsageEnvironment/include \
	$(LOCAL_PATH)/live555/include/BasicUsageEnvironment \
	$(LOCAL_PATH)/live555/include/UsageEnvironment/include \
	$(LOCAL_PATH)/live555/include/UsageEnvironment \
	$(LOCAL_PATH)/live555/include/groupsock/include \
	$(LOCAL_PATH)/live555/include/groupsock \
	$(LOCAL_PATH)/live555/include/liveMedia/include \
	$(LOCAL_PATH)/live555/include/liveMedia \

LOCAL_STATIC_LIBRARIES := live555
LOCAL_SHARED_LIBRARIES := opencv-prebuilt
LOCAL_LDLIBS := -llog -lz -lmediandk -lOpenMAXAL -landroid

include $(BUILD_SHARED_LIBRARY)
