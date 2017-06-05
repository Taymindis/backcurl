# APP_STL := gnustl_static
APP_STL := c++_static
APP_CPPFLAGS += -fexceptions
APP_CPPFLAGS += -frtti
APP_CPPFLAGS += -std=c++11
APP_ABI := all # or armeabi
APP_MODULES := libbackcurl