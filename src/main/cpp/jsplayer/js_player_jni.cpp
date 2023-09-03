#include "js_player_jni.h"
#include "js_player.h"
#include <android/native_window_jni.h>

extern "C" {
#include "decoder/js_ndk_mediacodec_proxy.h"
#include "util/js_log.h"
#include "util/js_jni.h"
}

jmethodID midOnPrepared;
jmethodID midOnInterceptedPcmData;
jmethodID midOnError;
jmethodID midOnInfo;
jmethodID midOnCompleted;
jmethodID midOnBuffering;

static JNINativeMethod js_player_methods[] = {
        {"nativeSetupJNI",                                  "()V",                                      (void *) native_setup_jni},
        {"setLoggable",                                     "(Z)V",                                     (void *) set_loggable},
        {"getLoggable",                                     "()Z",                                      (jboolean *) get_loggable},
        {"setIsWriteLogToFile",                             "(Z)V",                                     (void *) set_is_write_log_to_file},
        {"getIsWriteLogToFile",                             "()Z",                                      (jboolean *) get_is_write_log_to_file},
        {"setLogFileSavePath",                              "(Ljava/lang/String;)V",                    (void *) set_log_file_save_path},
        {"getLogFileSavePath",                              "()Ljava/lang/String;",                     (jstring *) get_log_file_save_path},
        {"create",                                          "()J",                                      (jlong *) create},
        {"setOption",                                       "(JLjava/lang/String;Ljava/lang/String;)V", (void *) set_option},
        {"getOption",                                       "(JLjava/lang/String;)Ljava/lang/String;",  (jstring *) get_option},
        {"setUrl",                                          "(JLjava/lang/String;)V",                   (void *) set_url},
        {"prepare",                                         "(J)V",                                     (void *) prepare},
        {"play",                                            "(J)V",                                     (void *) play},
        {"pause",                                           "(J)V",                                     (void *) pause},
        {"resume",                                          "(J)V",                                     (void *) resume},
        {"stop",                                            "(J)V",                                     (void *) stop},
        {"reset",                                           "(J)V",                                     (void *) reset},
        {"destroy",                                         "(J)V",                                     (void *) destroy},
        {"onSurfaceCreated",                                "(JLandroid/view/Surface;)V",               (void *) on_surface_created},
        {"onSurfaceChanged",                                "(JII)V",                                   (void *) on_surface_changed},
        {"onSurfaceDestroyed",                              "(J)V",                                     (void *) on_surface_destroyed},
        {"setMute",                                         "(JZ)V",                                    (void *) set_mute},
        {"getMute",                                         "(J)Z",                                     (jboolean *) get_mute},
        {"setVolume",                                       "(JI)V",                                    (void *) set_volume},
        {"getVolume",                                       "(J)I",                                     (jint *) get_volume},
        {"setChannelMute",                                  "(JIZ)V",                                   (void *) set_channel_mute},
        {"getPlayStatus",                                   "(J)I",                                     (jint *) get_play_status},
        {"getDuration",                                     "(J)J",                                     (jlong *) get_duration},
        {"getCurrentPosition",                              "(J)J",                                     (jlong *) get_current_position},
        {"seekTo",                                          "(JJ)V",                                    (void *) seek_to},
        {"interceptPcmData",                                "(JZ)V",                                    (void *) intercept_audio},
        {"parseDataFromVideoPacket",                        "(JZ)V",                                    (void *) parse_data_from_video_packet},
        {"setNativeInterceptedPcmDataCallbackHandle",       "(JJ)V",                                    (void *) set_native_intercepted_pcm_data_callback_handle},
        {"setNativeParseDataFromVideoPacketCallbackHandle", "(JJ)V",                                    (void *) set_native_parse_data_from_video_packet_callback_handle},
};


JNIEXPORT jint JS_JNI_CALL JNI_OnLoad(
        JavaVM *vm, void *reserved) {

//    __JS_ANDROID_SDK_VERSION__ = js_get_sdk_version();
//    if (__JS_ANDROID_SDK_VERSION__ >= __JS_NATIVE_MEDIACODEC_API_LEVEL__) {
//        if (link_ndk_mediacodec() == JS_OK) {
//            __JS_NDK_MEDIACODEC_LINKED__ = 1;
//        }
//    }

    js_jni_set_java_vm(vm, NULL);
    register_js_player_methods(js_jni_get_env(NULL));
    init_logger();
    return JNI_VERSION_1_6;
}

JNIEXPORT void JS_JNI_CALL JNI_OnUnload(JavaVM *vm, void *reserved) {
//    if (__JS_NDK_MEDIACODEC_LINKED__) {
//        close_ndk_mediacodec();
//    }

    avformat_network_deinit();
    deinit_logger();
}


void JS_JNI_CALL native_setup_jni(JNIEnv *env, jclass cls) {

    av_register_all();
    avformat_network_init();
}

void JS_JNI_CALL set_loggable(JNIEnv *env, jclass cls, jboolean loggable) {
    set_loggable_(loggable);
}


jboolean JS_JNI_CALL get_loggable(JNIEnv *env, jclass cls) {
    return get_loggable_();
}

void JS_JNI_CALL set_is_write_log_to_file(JNIEnv *env, jclass clz, jboolean is_write_log_to_file) {
    set_is_write_log_to_file_(is_write_log_to_file);
}

jboolean JS_JNI_CALL get_is_write_log_to_file(JNIEnv *env, jclass clz) {
    return get_is_write_log_to_file_();
}

void JS_JNI_CALL
set_log_file_save_path(JNIEnv *env, jclass cls, jstring log_file_save_path) {
    char *path = js_jni_jstring_to_utf_chars(js_jni_get_env(NULL), log_file_save_path, NULL);
    set_log_file_save_path_(path);
    free(path);
}

jstring JS_JNI_CALL get_log_file_save_path(JNIEnv *env, jclass clz) {
    char path[256] = {0};
    get_log_file_save_path_(path);
    return env->NewStringUTF(path);
}


jlong JS_JNI_CALL create(JNIEnv *env, jobject obj) {

    JSPlayer *player = new JSPlayer(obj);
    return (jlong) player;
}

void JS_JNI_CALL set_option(JNIEnv *env, jobject obj, jlong handle, jstring key_, jstring value_) {
    const char *key = NULL;
    const char *value = NULL;
    JSPlayer *player = (JSPlayer *) handle;

    if (!key_) {
        LOGW("%s key is null.", __func__);
        return;
    }

    key = env->GetStringUTFChars(key_, 0);
    if (!key) {
        LOGW("%s get key is failed.", __func__);
        goto end;
    }
    value = env->GetStringUTFChars(value_, 0);
    if (!value) {
        LOGW("%s get value is failed.", __func__);
        goto end;
    }

    av_dict_set(&player->m_options, key, value, 0);

    end:
    if (key) {
        env->ReleaseStringUTFChars(key_, key);
    }
    if (value) {
        env->ReleaseStringUTFChars(value_, value);
    }
}

jstring JS_JNI_CALL get_option(JNIEnv *env, jobject obj, jlong handle, jstring key_) {
    const char *key = NULL;
    JSPlayer *player = (JSPlayer *) handle;
    AVDictionaryEntry *entry = NULL;

    if (!key_) {
        LOGW("%s key is null.", __func__);
        return NULL;
    }
    key = env->GetStringUTFChars(key_, 0);
    if (!key) {
        LOGW("%s get key is failed.", __func__);
        return NULL;
    }

    entry = av_dict_get(player->m_options, key, NULL, AV_DICT_IGNORE_SUFFIX);

    env->ReleaseStringUTFChars(key_, key);
    if (entry) {
        return env->NewStringUTF(entry->value);
    } else {
        return NULL;
    }
}

void JS_JNI_CALL prepare(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    if (!player->m_url) {
        LOGE("%s failed, url is null...", __func__);
        return;
    }
    LOGD("%s prepare current url is %s, url length=%d", __func__, player->m_url,
         strlen(player->m_url));
    player->prepare();
}


void JS_JNI_CALL play(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    player->play(false);
}


void JS_JNI_CALL pause(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    player->pause();
}

void JS_JNI_CALL resume(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    player->resume();
}

void JS_JNI_CALL stop(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    player->stop();
}

void JS_JNI_CALL reset(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    player->reset();
}


void JS_JNI_CALL destroy(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    player->destroy();
}


void JS_JNI_CALL on_surface_created(JNIEnv *env, jobject obj, jlong handle, jobject surface) {

    JSPlayer *player = (JSPlayer *) handle;
    if (player->m_has_video_stream && player->m_video_decoded_que != NULL) {
        player->m_video_decoded_que->clear_abort_get();
    }
    player->m_egl_renderer->create_surface(surface);
    player->on_surface_hold_state_changed();

}

void JS_JNI_CALL
on_surface_changed(JNIEnv *env, jobject obj, jlong handle, jint width, jint height) {
//    JSPlayer *player = (JSPlayer *) handle;
//    player->m_egl_renderer->window_size_changed(width, height);
}

void JS_JNI_CALL on_surface_destroyed(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;

    if (player->m_has_video_stream && player->m_video_decoded_que != NULL) {
        player->m_video_decoded_que->abort_get();
    }
    player->m_egl_renderer->destroy_surface();
    player->on_surface_hold_state_changed();
}


void JS_JNI_CALL set_url(JNIEnv *env, jobject obj, jlong handle, jstring url) {
    JSPlayer *player = (JSPlayer *) handle;
    if (player->m_url) {
        free((void *) player->m_url);
    }
    player->m_url = js_jni_jstring_to_utf_chars(env, url, NULL);
}

void JS_JNI_CALL set_mute(JNIEnv *env, jobject obj, jlong handle, jboolean mute) {

    JSPlayer *player = (JSPlayer *) handle;
    player->set_mute(mute);
}


void JS_JNI_CALL
set_channel_mute(JNIEnv *env, jobject obj, jlong handle, jint channel, jboolean mute) {

    JSPlayer *player = (JSPlayer *) handle;
    player->set_channel_mute(channel, mute);
}


void JS_JNI_CALL set_volume(JNIEnv *env, jobject obj, jlong handle, jint volume) {

    JSPlayer *player = (JSPlayer *) handle;
    player->m_audio_player->set_volume(volume);
}

void JS_JNI_CALL
intercept_audio(JNIEnv *env, jobject obj, jlong handle, jboolean is_intercept) {

    JSPlayer *player = (JSPlayer *) handle;
    player->set_is_intercept_audio(is_intercept);

}

void JS_JNI_CALL
parse_data_from_video_packet(JNIEnv *env, jobject obj, jlong handle, jboolean is_parse) {

    JSPlayer *player = (JSPlayer *) handle;
    player->m_is_parse_data_from_video_packet = is_parse;

}

void JS_JNI_CALL
set_native_intercepted_pcm_data_callback_handle(JNIEnv *env, jobject obj, jlong handle,
                                                jlong callback_handle) {
    JSPlayer *player = (JSPlayer *) handle;
    player->native_intercepted_pcm_data_callback = (void (*)(int64_t, short *, int,
                                                             int)) (callback_handle);
}

void JS_JNI_CALL
set_native_parse_data_from_video_packet_callback_handle(JNIEnv *env, jobject obj, jlong handle,
                                                        jlong callback_handle) {
    JSPlayer *player = (JSPlayer *) handle;
    player->native_parse_data_from_video_packet_callback = (void (*)(jlong, uint8_t *,
                                                                     int)) (callback_handle);
}


jboolean JS_JNI_CALL get_mute(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    return player->get_mute();
}


int JS_JNI_CALL get_volume(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    return player->m_audio_player->m_volume;

}

int JS_JNI_CALL get_play_status(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    return player->m_cur_play_status;
}

jlong JS_JNI_CALL get_duration(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    return player->get_duration();
}

jlong JS_JNI_CALL get_current_position(JNIEnv *env, jobject obj, jlong handle) {

    JSPlayer *player = (JSPlayer *) handle;
    return player->get_current_position();
}


void JS_JNI_CALL seek_to(JNIEnv *env, jobject obj, jlong handle, jlong timestamp) {

    JSPlayer *player = (JSPlayer *) handle;
    player->seek_to(timestamp);
}

void JS_JNI_CALL register_js_player_methods(JNIEnv *env) {
    jclass cls = env->FindClass(JS_PLAYER_CLASS);
    env->RegisterNatives(cls, js_player_methods, METHOD_ARRAY_ELEMS(js_player_methods));
    midOnPrepared = env->GetMethodID(cls, "onPrepared", "()V");
    midOnInterceptedPcmData = env->GetMethodID(cls, "onInterceptedPcmData", "([SII)V");
    midOnError = env->GetMethodID(cls, "onError", "(III)V");
    midOnInfo = env->GetMethodID(cls, "onInfo", "(III)V");
    midOnCompleted = env->GetMethodID(cls, "onCompleted", "()V");
    midOnBuffering = env->GetMethodID(cls, "onBuffering", "(Z)V");

}
//
//#include "libavutil/log.h"
////    av_log_set_callback(ffp_log_callback_report);
//void ffp_log_callback_report(void *ptr, int level, const char *fmt, va_list vl) {
//
//    int ffplv;
//    if (level <= AV_LOG_ERROR)
//        ffplv = ANDROID_LOG_ERROR;
//    else if (level <= AV_LOG_WARNING)
//        ffplv = ANDROID_LOG_WARN;
//    else if (level <= AV_LOG_INFO)
//        ffplv = ANDROID_LOG_INFO;
//    else if (level <= AV_LOG_VERBOSE)
//        ffplv = ANDROID_LOG_VERBOSE;
//    else
//        ffplv = ANDROID_LOG_DEBUG;
//
//    va_list vl2;
//    char line[1024];
//    static int print_prefix = 1;
//
//    va_copy(vl2, vl);
//    av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
//    va_end(vl2);
//
//    printf_log(ffplv, line);
//}