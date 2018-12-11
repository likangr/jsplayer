#ifndef JS_AUDIOPLAYER_H
#define JS_AUDIOPLAYER_H

#include "js_constant.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define MAX_AUDIO_FRAME_SIZE        192000

class JSAudioPlayer {

public :
    JSAudioPlayer();

    ~JSAudioPlayer();

    JS_RET create_engine();

    JS_RET create_AudioPlayer(int rate, int channel, int bit_per_sample, void *data);

    JS_RET enqueue(const void *buffer, unsigned int size);

    JS_RET clear();

    void set_mute(bool mute);

    void set_volume(int volume);

    void set_channel_mute(int channel, bool mute);

    void set_audio_buffer_queue_callback(slAndroidSimpleBufferQueueCallback callback);

    void reset();


    // engine interfaces
    SLObjectItf m_engine_object = NULL;
    SLEngineItf m_engine_engine = NULL;

    // buffer m_dequeue player interfaces
    SLObjectItf m_bq_player_object = NULL;
    SLPlayItf m_bq_player_play = NULL;
    SLAndroidSimpleBufferQueueItf m_bq_player_buffer_queue = NULL;
    SLMuteSoloItf m_bq_player_mute_solo = NULL;;
    SLEffectSendItf m_bq_player_effect_send = NULL;
    SLVolumeItf m_bq_player_volume = NULL;

    // output mix interfaces
    SLObjectItf m_output_mix_object = NULL;
    SLEnvironmentalReverbItf m_output_mix_environmental_reverb = NULL;

    slAndroidSimpleBufferQueueCallback sl_android_simple_buffer_queue_callback = NULL;

    // aux effect on the output mix, used by the buffer m_dequeue player
    const SLEnvironmentalReverbSettings m_reverb_settings =
            SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    int m_volume = 100;
    bool m_mute = false;
    bool m_left_channel_mute = false;
    bool m_right_channel_mute = false;

    int m_channel_count = 0;
    int m_rate = 0;
    int m_bits_per_sample = 0;

    bool m_is_created_audio_player = false;


};

#endif //JS_AUDIOPLAYER_H
