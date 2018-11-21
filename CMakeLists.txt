# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.
#set(CMAKE_BUILD_TYPE "Debug")
set(jsplayer_src_dir src/main/cpp/jsplayer)

add_library( # Sets the name of the library.
             jsplayer

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             # Associated headers in the same location as their source
             # file are automatically included.

             ${jsplayer_src_dir}/js_player.cpp
             ${jsplayer_src_dir}/js_player_jni.cpp
             ${jsplayer_src_dir}/audio/js_audio_player.cpp
             ${jsplayer_src_dir}/render/js_egl_renderer.cpp
             ${jsplayer_src_dir}/converter/js_media_converter.cpp
             ${jsplayer_src_dir}/decoder/js_media_decoder.cpp
             ${jsplayer_src_dir}/decoder/js_media_hw_decoder_context.cpp
             ${jsplayer_src_dir}/decoder/js_ndk_mediacodec_proxy.c
             ${jsplayer_src_dir}/decoder/js_java_mediacodec_wrapper.c
             ${jsplayer_src_dir}/event/js_event_handler.cpp
             ${jsplayer_src_dir}/util/js_jni.c
             ${jsplayer_src_dir}/util/js_log.c
             )


#Add other prebuilt libraries

 #jsyuv
 set(jsyuv_inc_dir src/main/cpp/jsyuv/include)
 add_library(jsyuv SHARED IMPORTED )
 set_target_properties(jsyuv PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libjsyuv.so)

 #jsffmpeg
 set(jsffmpeg_inc_dir src/main/cpp/jsffmpeg/include)
 add_library(jsffmpeg SHARED IMPORTED )
 set_target_properties(jsffmpeg PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libjsffmpeg.so)


# Specifies a path to native header files.

include_directories(${jsplayer_src_dir})
include_directories(${jsyuv_inc_dir})
include_directories(${jsffmpeg_inc_dir})

# Searches for a specified prebuilt library and stores the path as a
# variable. Because system libraries are included in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in the
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.

                       jsplayer
                       jsyuv
                       jsffmpeg
                       android
                       GLESv2
                       EGL
                       OpenSLES
                       log
                      )
