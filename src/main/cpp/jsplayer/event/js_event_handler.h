#ifndef JS_EVENT_HANDLER_H
#define JS_EVENT_HANDLER_H

#include <jni.h>

class JSPlayer;

class JSEventHandler {

public:

    JSEventHandler(jobject java_js_player, JSPlayer *player);

    ~JSEventHandler();


    void call_on_intercepted_pcm_data(short *pcm_data, int pcm_data_size, int channel_num);

    void
    call_on_parse_data_from_video_packet(uint8_t *video_packet_data, int video_packet_data_size);


    void call_on_prepared();

    void call_on_error(int what, int arg1, int arg2);

    void call_on_info(int what, int arg1, int arg2);

    void call_on_completed();

    void call_on_buffering(bool is_buffering);

private:

    jobject m_js_player_obj;
    JSPlayer *m_js_player;
};


#endif //JS_EVENT_HANDLER_H
