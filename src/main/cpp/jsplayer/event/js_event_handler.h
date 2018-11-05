#ifndef JS_EVENT_HANDLER_H
#define JS_EVENT_HANDLER_H

#include <jni.h>

class JSPlayer;

class JSEventHandler {

public:

    JSEventHandler(jobject java_js_player, JSPlayer *player);

    ~JSEventHandler();


    void call_on_intercepted_pcm_data(uint8_t *data, int len, int channel_num);

    void call_on_parse_data_from_video_packet(uint8_t *data, int size);


    void call_on_prepared();

    void call_on_error(int what, int arg1, int arg2);

    void call_on_info(int what, int arg1, int arg2);

    void call_on_completed();

private:

    jobject m_java_js_player;
    JSPlayer *m_native_js_player;
};


#endif //JS_EVENT_HANDLER_H
