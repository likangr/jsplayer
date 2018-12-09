#include <js_player.h>
#include "js_event_handler.h"
#include "js_player_jni.h"

extern "C" {
#include "util/js_jni.h"
#include "util/js_log.h"
}


JSEventHandler::JSEventHandler(jobject java_js_player, JSPlayer *native_js_player) {
    m_java_js_player = java_js_player;
    m_native_js_player = native_js_player;
}

JSEventHandler::~JSEventHandler() {
}


void JSEventHandler::call_on_intercepted_pcm_data(uint8_t *pcm_data, int len,
                                                  int channel_num) {//fixme func pointer.

    int sample_num = len / 2;
    LOGE("%s sample_num=%d,channel_num=%d", __func__,
         sample_num, channel_num);

    if (m_native_js_player->native_intercepted_pcm_data_callback) {
        m_native_js_player->native_intercepted_pcm_data_callback((jlong) m_native_js_player,
                                                                 (short *) pcm_data,
                                                                 sample_num,
                                                                 channel_num);

    } else {
        JNIEnv *env = js_jni_get_env(NULL);
        jshortArray pcm_short_array = env->NewShortArray(sample_num);
        env->SetShortArrayRegion(pcm_short_array, 0, sample_num, (jshort *) pcm_data);

        env->CallVoidMethod(m_java_js_player, midOnInterceptedPcmData,
                            pcm_short_array, sample_num,
                            channel_num);
        env->DeleteLocalRef(pcm_short_array);
    }
}

void JSEventHandler::call_on_parse_data_from_video_packet(uint8_t *data, int size) {
    if (m_native_js_player->native_parse_data_from_video_packet_callback) {
        m_native_js_player->native_parse_data_from_video_packet_callback(data, size);
    }
}

void JSEventHandler::call_on_prepared() {
    js_jni_get_env(NULL)->CallVoidMethod(m_java_js_player, midOnPrepared);
}


void JSEventHandler::call_on_error(int what, int arg1, int arg2) {
    js_jni_get_env(NULL)->CallVoidMethod(m_java_js_player, midOnError, what, arg1,
                                         arg2);

}

void JSEventHandler::call_on_info(int what, int arg1, int arg2) {
    js_jni_get_env(NULL)->CallVoidMethod(m_java_js_player, midOnInfo, what, arg1,
                                         arg2);

}

void JSEventHandler::call_on_completed() {
    js_jni_get_env(NULL)->CallVoidMethod(m_java_js_player, midOnCompleted);
}

void JSEventHandler::call_on_buffering(bool is_buffering) {
    js_jni_get_env(NULL)->CallVoidMethod(m_java_js_player, midOnBuffering, is_buffering);
}


