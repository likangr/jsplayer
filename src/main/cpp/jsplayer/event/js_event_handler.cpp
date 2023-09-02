#include <js_player.h>
#include "js_event_handler.h"
#include "js_player_jni.h"

extern "C" {
#include "util/js_jni.h"
#include "util/js_log.h"
}


JSEventHandler::JSEventHandler(jobject java_js_player, JSPlayer *native_js_player) {
    m_js_player_obj = java_js_player;
    m_js_player = native_js_player;
}

JSEventHandler::~JSEventHandler() {
}


void JSEventHandler::call_on_intercepted_pcm_data(short *pcm_data, int pcm_data_size,
                                                  int channel_num) {//fixme func pointer.

//    LOGD("%s pcm_data_size=%ld,channel_num=%d", __func__, pcm_data_size, channel_num);
    if (m_js_player->native_intercepted_pcm_data_callback) {
        m_js_player->native_intercepted_pcm_data_callback((int64_t) m_js_player,
                                                          pcm_data, pcm_data_size,
                                                          channel_num);
    } else {
        int sample_num = pcm_data_size / DST_BYTES_PER_SAMPLE;
        JNIEnv *env = js_jni_get_env(NULL);
        jshortArray jpcm_data = env->NewShortArray((jsize) sample_num);
        env->SetShortArrayRegion(jpcm_data, 0, (jsize) sample_num, pcm_data);

        env->CallVoidMethod(m_js_player_obj, midOnInterceptedPcmData,
                            jpcm_data, sample_num,
                            channel_num);
        env->DeleteLocalRef(jpcm_data);
    }
}

void JSEventHandler::call_on_parse_data_from_video_packet(uint8_t *video_packet_data,
                                                          int video_packet_data_size) {
    if (m_js_player->native_parse_data_from_video_packet_callback) {
        m_js_player->native_parse_data_from_video_packet_callback(
                (int64_t) m_js_player,
                video_packet_data, video_packet_data_size);
    }
}

void JSEventHandler::call_on_prepared() {
    js_jni_get_env(NULL)->CallVoidMethod(m_js_player_obj, midOnPrepared);
}


void JSEventHandler::call_on_error(int what, int arg1, int arg2) {
    js_jni_get_env(NULL)->CallVoidMethod(m_js_player_obj, midOnError, what, arg1,
                                         arg2);

}

void JSEventHandler::call_on_info(int what, int arg1, int arg2) {
    js_jni_get_env(NULL)->CallVoidMethod(m_js_player_obj, midOnInfo, what, arg1,
                                         arg2);

}

void JSEventHandler::call_on_completed() {
//    js_jni_get_env(NULL)->CallVoidMethod(m_js_player_obj, midOnCompleted);
}

void JSEventHandler::call_on_buffering(bool is_buffering) {
    js_jni_get_env(NULL)->CallVoidMethod(m_js_player_obj, midOnBuffering, is_buffering);
}


