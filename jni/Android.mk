LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OPENCV_CAMERA_MODULES:=on
OPENCV_INSTALL_MODULES:=on

include /Users/john/Development/OpenCV-2.4.5-Tegra-sdk-r2/sdk/native/jni/OpenCV.mk

LOCAL_C_INCLUDES += ptam 
LOCAL_CPP_EXTENSION := .cc .cpp
LOCAL_MODULE    := native_activity
LOCAL_SRC_FILES := native.cpp \
open_cv_helpers.cpp      \
ptam/ATANCamera.cc       \
ptam/Bundle.cc           \
ptam/HomographyInit.cc   \
ptam/KeyFrame.cc         \
ptam/Map.cc              \
ptam/MapMaker.cc         \
ptam/MapPoint.cc         \
ptam/MiniPatch.cc        \
ptam/PatchFinder.cc      \
ptam/Relocaliser.cc      \
ptam/ShiTomasi.cc        \
ptam/SmallBlurryImage.cc \
ptam/Tracker.cc          \



LOCAL_LDLIBS    += -lm -llog -landroid -lGLESv2
LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_STATIC_LIBRARIES += cvd
LOCAL_STATIC_LIBRARIES += gvars3

include $(BUILD_SHARED_LIBRARY)

$(call import-module,gvars3)
$(call import-module,cvd)
$(call import-module,TooN)
$(call import-module,android/native_app_glue)
