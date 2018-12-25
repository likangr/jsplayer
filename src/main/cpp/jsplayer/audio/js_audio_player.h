#ifndef JS_AUDIOPLAYER_H
#define JS_AUDIOPLAYER_H

#include "js_constant.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>


class JSAudioPlayer {

public :
    JSAudioPlayer();

    ~JSAudioPlayer();

    JS_RET create_audio_player_engine();

    JS_RET create_audio_player(int rate, int channel, int bit_per_sample, void *data);

    JS_RET enqueue_buffer(const void *buffer, unsigned int size);

    int64_t get_position();

    JS_RET clear();

    void set_mute(bool mute);

    void set_volume(int volume);

    void set_channel_mute(int channel, bool mute);

    void set_audio_buffer_queue_callback(slAndroidSimpleBufferQueueCallback callback);

    void reset();

    // engine interfaces
    SLObjectItf m_player_engine_object_itf = NULL;
    SLEngineItf m_player_engine_itf = NULL;

    // player interfaces
    SLObjectItf m_player_object_itf = NULL;
    SLPlayItf m_player_itf = NULL;
    SLAndroidSimpleBufferQueueItf m_player_buffer_queue_itf = NULL;
    SLMuteSoloItf m_player_mute_solo_itf = NULL;;
    SLEffectSendItf m_player_effect_send_itf = NULL;
    SLVolumeItf m_player_volume_itf = NULL;

    // output mix interfaces
    SLObjectItf m_output_mix_object_itf = NULL;
    SLEnvironmentalReverbItf m_output_mix_environmental_reverb_itf = NULL;

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
