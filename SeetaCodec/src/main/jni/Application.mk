APP_STL := gnustl_shared
NDK_TOOLCHAIN_VERSION := 4.9
APP_CPPFLAGS := -std=c++11
APP_CPPFLAGS += -std=c++0x
APP_CPPFLAGS += -fexceptions -fpermissive  -Wnarrowing
APP_CPPFLAGS += -fPIC -frtti
APP_ABI :=  arm64-v8a armeabi-v7a
APP_PLATFORM := android-21
APP_OPTIM := debug
