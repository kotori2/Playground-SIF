LOCAL_PATH:= $(call my-dir)


# ---------------------
include $(CLEAR_VARS)
#
DEBUG_FLAGS		:=
#DEBUG_FLAGS		+=	-DDEBUG_MODIFIER
#DEBUG_FLAGS		+=	-DDEBUG_PERFORMANCE
#DEBUG_FLAGS		+=	-DDEBUG_MEMORY
#DEBUG_FLAGS		+=	-DDEBUG_LUA
DEBUG_FLAGS		+=	-DDEBUG_NULLTASK
DEBUG_FLAGS		+=	-DDEBUG_MENU

ifeq ($(TARGET_ARCH),arm)
  LOCAL_LDLIBS	:= -L$(LOCAL_PATH)/../obj/local/armeabi -lcurl
endif

ifeq ($(TARGET_ARCH),x86)
  LOCAL_LDLIBS	:= -L$(LOCAL_PATH)/../obj/local/x86 -lcurl
endif

ifdef ($(USE_LUAJIT))
  LOCAL_LDLIBS  += -lluajit-5.1
endif

# basic libraries
LOCAL_LDLIBS    += -llog -lGLESv1_CM -ldl -lz -lOpenSLES

# OpenGL ES 2.0/3.0 support
ifdef ($(USE_GLES2))
  LOCAL_LDLIBS	+=	-lGLESv2
  DEBUG_FLAGS	+=	-DOPENGL2
endif
ifdef ($(USE_GLES3))
  LOCAL_LDLIBS	+=	-lGLESv3
  DEBUG_FLAGS	+=	-DOPENGL3
endif

# for freetype2
LOCAL_LDLIBS    += -lfreetype2

# all directories contain .h files is regarded as include dir
LOCAL_C_INCLUDES :=	$(ANDROID_NDK_ROOT)/sysroot/usr/include/
#LOCAL_C_INCLUDES +=	$(shell find ./jni -regex '.*\.h$$' | sed "s/\/[^\/]*$$//" | sort | uniq)
LOCAL_C_INCLUDES +=	./jni/Android
LOCAL_C_INCLUDES +=	./jni/include
LOCAL_C_INCLUDES +=	./jni/libs/curl-7.29.0/include/include/curl
LOCAL_C_INCLUDES +=	./jni/libs/JSonParser
LOCAL_C_INCLUDES +=	./jni/libs/JSonParser/api
LOCAL_C_INCLUDES +=	./jni/libs/libfreetype2/include
LOCAL_C_INCLUDES +=	./jni/libs/libfreetype2/include/freetype
LOCAL_C_INCLUDES +=	./jni/libs/libfreetype2/include/freetype/config
LOCAL_C_INCLUDES +=	./jni/libs/libfreetype2/include/freetype/internal
LOCAL_C_INCLUDES +=	./jni/libs/libfreetype2/include/freetype/internal/services
LOCAL_C_INCLUDES +=	./jni/libs/lua
LOCAL_C_INCLUDES +=	./jni/libs/minizip
LOCAL_C_INCLUDES +=	./jni/libs/mono/jit
LOCAL_C_INCLUDES +=	./jni/libs/mono/metadata
LOCAL_C_INCLUDES +=	./jni/libs/mono/utils
LOCAL_C_INCLUDES +=	./jni/libs/sha1
LOCAL_C_INCLUDES +=	./jni/libs/SQLite
LOCAL_C_INCLUDES +=	./jni/libs/Tremolo
LOCAL_C_INCLUDES +=	./jni/libs/utf8_converter
LOCAL_C_INCLUDES +=	./jni/proxy
LOCAL_C_INCLUDES +=	./jni/source/Animation
LOCAL_C_INCLUDES +=	./jni/source/Assets
LOCAL_C_INCLUDES +=	./jni/source/Core
#LOCAL_C_INCLUDES +=	./jni/source/Core/HonokaMiku
LOCAL_C_INCLUDES +=	./jni/source/Database
LOCAL_C_INCLUDES +=	./jni/source/HTTP
LOCAL_C_INCLUDES +=	./jni/source/include
LOCAL_C_INCLUDES +=	./jni/source/LuaLib
LOCAL_C_INCLUDES +=	./jni/source/Rendering
LOCAL_C_INCLUDES +=	./jni/source/SceneGraph
LOCAL_C_INCLUDES +=	./jni/source/Scripting
LOCAL_C_INCLUDES +=	./jni/source/Sound
LOCAL_C_INCLUDES +=	./jni/source/SystemTask
LOCAL_C_INCLUDES +=	./jni/source/UISystem

# all *.c/*.cpp files under src dirs are listed for building
SRC_DIRS    := ./jni/include
SRC_DIRS    += ./jni/libs
SRC_DIRS    += ./jni/source
#SRC_DIRS    += ./jni/custom
SRC_DIRS    += ./jni/Android
SRC_DIRS    += ./jni/UserTask 
SRC_FILES   :=	$(shell find $(SRC_DIRS) -name "*.c" -or -name "*.cpp" -or -name "*.cc")

LOCAL_SRC_FILES	:=	$(subst ./jni/,,$(SRC_FILES))

ENGINE_GITHASH  := $(shell git log --pretty=format:%h -1)
# force refresh engine hash
$(shell touch ./jni/Android/KLBPlatformMetrics.cpp)

LOCAL_CFLAGS    := -DDEBUG -DLUA_ANSI -DIOAPI_NO_64 $(DEBUG_FLAGS) -DENGINE_GITHASH='"$(ENGINE_GITHASH)"' -DSQLITE_THREADSAFE=1 $(SQLITE_ENABLE_COLUMN_METADATA)
LOCAL_CPPFLAGS  := -fexceptions -fno-rtti
LOCAL_MODULE    := libGame
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true

ifeq ($(TARGET_ARCH),arm)
# notice: these assembly files (*.s) generate compile-time warnings under Clang but they are harmless.
  #LOCAL_SRC_FILES += \
	./libs/Tremolo/bitwiseARM.s \
	./libs/Tremolo/dpen.s \
	./libs/Tremolo/floor1ARM.s \
	./libs/Tremolo/mdctARM.s
  #LOCAL_CFLAGS += -D_ARM_ASSEM_

  LOCAL_ARM_MODE  := arm
  LOCAL_ARM_NEON  := true
  TARGET_ARCH_ABI := armeabi-v7a
  LOCAL_CFLAGS    += -DONLY_C -O3 -mcpu=cortex-a8 -mfloat-abi=softfp -fPIC -march=armv7-a -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums
else
  LOCAL_CFLAGS += -DONLY_C
endif

include $(BUILD_SHARED_LIBRARY)
# ---------------------


# ---------------------
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH),arm)
  LOCAL_ARM_MODE  := arm
  LOCAL_ARM_NEON  := true
  TARGET_ARCH_ABI := armeabi-v7a
endif

LOCAL_MODULE    := libjniproxy
LOCAL_C_INCLUDES := ./jni/proxy
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := proxy/jniproxy.cpp
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)
# ---------------------

