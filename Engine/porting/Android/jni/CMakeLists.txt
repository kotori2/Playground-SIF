# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

#set(DEBUG_FLAGS "${DEBUG_FLAGS} -DDEBUG_MODIFIER")
#set(DEBUG_FLAGS "${DEBUG_FLAGS} -DDEBUG_PERFORMANCE")
#set(DEBUG_FLAGS "${DEBUG_FLAGS} -DDEBUG_MEMORY")
#set(DEBUG_FLAGS "${DEBUG_FLAGS} -DDEBUG_LUA")
set(DEBUG_FLAGS "${DEBUG_FLAGS} -DDEBUG_NULLTASK")
set(DEBUG_FLAGS "${DEBUG_FLAGS} -DDEBUG_MENU")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

if( ${ANDROID_ABI} STREQUAL "armeabi-v7a" )
    link_directories(../obj/local/armeabi)
elseif( ${ANDROID_ABI} STREQUAL "x86" )
    link_directories(../obj/local/x86)
endif()

set(LOCAL_LDLIB ${LOCAL_LDLIB} log curl GLESv1_CM dl z OpenSLES freetype2) #GLESv2
# set(DEBUG_FLAGS "${DEBUG_FLAGS} -DOPENGL2")

if (WIN32)
    execute_process(COMMAND CMD /c git log --pretty=format:%h -1 OUTPUT_VARIABLE ENGINE_GITHASH)
else()
    execute_process(COMMAND git log --pretty=format:%h -1 OUTPUT_VARIABLE ENGINE_GITHASH)
endif (WIN32)

include_directories(
        ./Android
        ./include
        ./libs/curl-7.29.0/include/include/curl
        ./libs/JSonParser
        ./libs/JSonParser/api
        ./libs/libfreetype2/include
        ./libs/libfreetype2/include/freetype
        ./libs/libfreetype2/include/freetype/config
        ./libs/libfreetype2/include/freetype/internal
        ./libs/libfreetype2/include/freetype/internal/services
        ./libs/lua
        ./libs/minizip
        ./libs/mono/jit
        ./libs/mono/metadata
        ./libs/mono/utils
        ./libs/sha1
        ./libs/SQLite
        ./libs/Tremolo
        ./libs/utf8_converter
        ./proxy
        ./source/Animation
        ./source/Assets
        ./source/Core
        ./source/Core/HonokaMiku
        ./source/Database
        ./source/HTTP
        ./source/include
        ./source/LuaLib
        ./source/Rendering
        ./source/SceneGraph
        ./source/Scripting
        ./source/Sound
        ./source/SystemTask
        ./source/UISystem
)

file(GLOB_RECURSE SUB_SOURCES "./libs/*.cpp" "./libs/*.c" "./libs/*.cc")
set(SOURCE ${SOURCE} ${SUB_SOURCES})
file(GLOB_RECURSE SUB_SOURCES "./source/*.cpp" "./source/*.c" "./source/*.cc")
set(SOURCE ${SOURCE} ${SUB_SOURCES})
file(GLOB_RECURSE SUB_SOURCES "./Android/*.cpp" "./Android/*.c" "./Android/*.cc")
set(SOURCE ${SOURCE} ${SUB_SOURCES})
file(GLOB_RECURSE SUB_SOURCES "./UserTask/*.cpp" "./UserTask/*.c" "./UserTask/*.cc")
set(SOURCE ${SOURCE} ${SUB_SOURCES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLUA_ANSI -DIOAPI_NO_64 ${DEBUG_FLAGS} -fPIC -DENGINE_GITHASH='\"${ENGINE_GITHASH}\"' -DSQLITE_THREADSAFE=1 ${SQLITE_ENABLE_COLUMN_METADATA} -DONLY_C -frtti")
set(CMAKE_C_FLAGS_DEBUG "-DDEBUG")
set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS} -fexceptions -frtti")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")

add_library( # Sets the name of the library.
        Game

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${SOURCE})

# FOR DEBUG
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        Game
        ${LOCAL_LDLIB})


# --------------------- jniproxy
include_directories(./proxy)
add_library( # Sets the name of the library.
        jniproxy

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        proxy/jniproxy.cpp)
target_link_libraries( # Specifies the target library.
        jniproxy
        log)
# ---------------------