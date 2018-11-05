#ifndef JS_PLAYER_JNI_H
#define JS_PLAYER_JNI_H

#include <jni.h>

#define JS_PLAYER_CLASS "com/joyshow/jsplayer/player/JSPlayer"
#define JS_JNI_CALL
#define METHOD_ARRAY_ELEMS(a)  (sizeof(a) / sizeof(a[0]))
extern jmethodID midOnPrepared;// 在头文件中声明,必须加上extern，否则就是重定义???
extern jmethodID midOnInterceptedPcmData;
extern jmethodID midOnError;
extern jmethodID midOnInfo;
extern jmethodID midOnCompleted;


void JS_JNI_CALL register_js_player_methods(JNIEnv *env);

void JS_JNI_CALL native_setup_jni(JNIEnv *env, jclass clz);

void JS_JNI_CALL set_loggable(JNIEnv *env, jclass clz, jboolean loggable);

jboolean JS_JNI_CALL get_loggable(JNIEnv *env, jclass clz);

void JS_JNI_CALL set_log_file_save_path(JNIEnv *env, jclass clz, jstring log_file_save_path);

jlong JS_JNI_CALL create(JNIEnv *env, jobject obj);

void JS_JNI_CALL set_option(JNIEnv *env, jobject obj, jlong handle, jstring key_, jstring value_);

jstring JS_JNI_CALL get_option(JNIEnv *env, jobject obj, jlong handle, jstring key_);

void JS_JNI_CALL prepare(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL play(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL pause(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL resume(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL stop(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL reset(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL destroy(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL on_surface_created(JNIEnv *env, jobject obj, jlong handle, jobject surface);

void JS_JNI_CALL
on_surface_changed(JNIEnv *env, jobject obj, jlong handle, jint width, jint height);

void JS_JNI_CALL on_surface_destroyed(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL set_url(JNIEnv *env, jobject obj, jlong handle, jstring url_);

void JS_JNI_CALL set_mute(JNIEnv *env, jobject obj, jlong handle, jboolean mute);

jboolean JS_JNI_CALL get_mute(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL set_volume(JNIEnv *env, jobject obj, jlong handle, jint volume);

jint JS_JNI_CALL get_volume(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL
set_channel_mute(JNIEnv *env, jobject obj, jlong handle, jint channel, jboolean mute);

jint JS_JNI_CALL get_play_status(JNIEnv *env, jobject obj, jlong handle);

void JS_JNI_CALL intercept_audio(JNIEnv *env, jobject obj, jlong handle, jboolean is_intercept);

void JS_JNI_CALL
parse_data_from_video_packet(JNIEnv *env, jobject obj, jlong handle, jboolean is_parse);

void JS_JNI_CALL
set_native_intercepted_pcm_data_callback_handle(JNIEnv *env, jobject obj, jlong handle,
                                                jlong callback_handle);

void JS_JNI_CALL
set_native_parse_data_from_video_packet_callback_handle(JNIEnv *env, jobject obj, jlong handle,
                                                        jlong callback_handle);

#endif // JS_PLAYER_JNI_H
