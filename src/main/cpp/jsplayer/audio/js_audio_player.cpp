#include "js_audio_player.h"

extern "C" {
#include "util/js_log.h"
}


JSAudioPlayer::JSAudioPlayer() {

}

JSAudioPlayer::~JSAudioPlayer() {
    sl_android_simple_buffer_queue_callback = NULL;
}

JS_RET JSAudioPlayer::create_audio_player_engine() {

    SLresult ret;
    // create engine
    ret = slCreateEngine(&m_player_engine_object_itf, 0, NULL, 0, NULL, NULL);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("%s create engine failed: errcode=%d", __func__, ret);
        goto fail;
    }

    // realize the engine
    ret = (*m_player_engine_object_itf)->Realize(m_player_engine_object_itf, SL_BOOLEAN_FALSE);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("%s realize engine failed: errcode=%d", __func__, ret);
        goto fail;
    }
    // get the engine interface, which is needed in order to create other objects
    ret = (*m_player_engine_object_itf)->GetInterface(m_player_engine_object_itf, SL_IID_ENGINE,
                                                      &m_player_engine_itf);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("%s get engine interface failed: errcode=%d", __func__, ret);
        goto fail;
    }


    // create output mix, with environmental reverb specified as a non-required interface
    ret = (*m_player_engine_itf)->CreateOutputMix(m_player_engine_itf, &m_output_mix_object_itf, 0,
                                                  0, 0);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("%s create output mix failed: errcode=%d", __func__, ret);
        goto fail;
    }

    // realize the output mix
    ret = (*m_output_mix_object_itf)->Realize(m_output_mix_object_itf, SL_BOOLEAN_FALSE);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("%s realize output mix failed: errcode=%d", __func__, ret);
        goto fail;
    }

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    ret = (*m_output_mix_object_itf)->GetInterface(m_output_mix_object_itf,
                                                   SL_IID_ENVIRONMENTALREVERB,
                                                   &m_output_mix_environmental_reverb_itf);
    if (ret == SL_RESULT_SUCCESS) {
        LOGE("%s get m_output_mix_environmental_reverb_itf interface failed: errcode=%d", __func__,
             ret);
        (*m_output_mix_environmental_reverb_itf)->SetEnvironmentalReverbProperties(
                m_output_mix_environmental_reverb_itf, &m_reverb_settings);
    }

    return JS_OK;

    fail:
    reset();
    return JS_ERR;
}

// create buffer m_dequeue audio player
JS_RET JSAudioPlayer::create_audio_player(int rate, int channel, int bit_per_sample, void *data) {


    LOGD("%s create_audio_player:%d,%d", __func__, rate, channel);

    if (create_audio_player_engine() != JS_OK) {
        LOGE("%s create_audio_player failed to create engine", __func__);
        return JS_ERR;
    }

    SLresult ret;
    m_channel_count = channel;
    m_rate = rate * 1000;
    m_bits_per_sample = bit_per_sample;


    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = (SLuint32) (m_channel_count);
    format_pcm.samplesPerSec = (SLuint32) m_rate;
    format_pcm.bitsPerSample = (SLuint32) m_bits_per_sample;
    format_pcm.containerSize = (SLuint32) m_bits_per_sample;

    if (m_channel_count == 1) {
        format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    } else {
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    }
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;


    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, m_output_mix_object_itf};
    SLDataSink audioSnk = {&loc_outmix, NULL};

//    // create audio player
    if (m_channel_count > 1) {

        const SLInterfaceID ids[4] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
                                      SL_IID_MUTESOLO, SL_IID_VOLUME};
        const SLboolean req[4] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
                                  SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
        ret = (*m_player_engine_itf)->CreateAudioPlayer(m_player_engine_itf, &m_player_object_itf,
                                                        &audioSrc,
                                                        &audioSnk,
                                                        4, ids, req);
    } else {
        const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
        const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
                                  SL_BOOLEAN_TRUE};
        ret = (*m_player_engine_itf)->CreateAudioPlayer(m_player_engine_itf, &m_player_object_itf,
                                                        &audioSrc,
                                                        &audioSnk,
                                                        3, ids, req);
    }


    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s createAudioPlayer failed: errcode=%d", __func__, ret);
        goto fail;
    }


    // realize the player
    ret = (*m_player_object_itf)->Realize(m_player_object_itf, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s realize player failed: errcode=%d", __func__, ret);
        goto fail;
    }

    // get the play interface
    ret = (*m_player_object_itf)->GetInterface(m_player_object_itf, SL_IID_PLAY, &m_player_itf);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s get play interface failed: errcode=%d", __func__, ret);
        goto fail;
    }

    // get the buffer m_dequeue interface
    ret = (*m_player_object_itf)->GetInterface(m_player_object_itf, SL_IID_BUFFERQUEUE,
                                               &m_player_buffer_queue_itf);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s get buffer m_dequeue interface failed: errcode=%d", __func__, ret);
        goto fail;
    }

    if (sl_android_simple_buffer_queue_callback == NULL) {
        LOGE("%s sl_android_simple_buffer_queue_callback is null", __func__);
        goto fail;
    }

    // register callback on the bufferQueue
    ret = (*m_player_buffer_queue_itf)->RegisterCallback(m_player_buffer_queue_itf,
                                                         sl_android_simple_buffer_queue_callback,
                                                         data);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s register sl_android_simple_buffer_queue_callback on the buffer m_dequeue failed: errcode=%d",
             __func__,
             ret);
        goto fail;
    }

    // get the effect send interface
    ret = (*m_player_object_itf)->GetInterface(m_player_object_itf, SL_IID_EFFECTSEND,
                                               &m_player_effect_send_itf);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s get effect send interface failed: errcode=%d", __func__, ret);
        goto fail;
    }
    // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    if (m_channel_count > 1) {
        ret = (*m_player_object_itf)->GetInterface(m_player_object_itf, SL_IID_MUTESOLO,
                                                   &m_player_mute_solo_itf);
        if (SL_RESULT_SUCCESS != ret) {
            LOGE("%s get m_player_mute_solo_itf interface failed: errcode=%d", __func__, ret);
        }
    }
    // get the volume interface
    ret = (*m_player_object_itf)->GetInterface(m_player_object_itf, SL_IID_VOLUME,
                                               &m_player_volume_itf);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s get the m_player_volume_itf interface failed: errcode=%d", __func__, ret);
        goto fail;
    }

    // set the player's state to playing
    ret = (*m_player_itf)->SetPlayState(m_player_itf, SL_PLAYSTATE_PLAYING);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s set the player's state to playing failed: errcode=%d", __func__, ret);
        goto fail;
    }


    m_is_created_audio_player = true;

    set_volume(m_volume);
    set_mute(m_mute);

    if (m_channel_count == 2) {
        set_channel_mute(0, m_left_channel_mute);
        set_channel_mute(1, m_right_channel_mute);
    }
    return JS_OK;

    fail:
    reset();
    return JS_ERR;
}


JS_RET JSAudioPlayer::enqueue_buffer(const void *buffer, unsigned int size) {

    SLresult ret;
    // enqueue_buffer another buffer
    ret = (*m_player_buffer_queue_itf)->Enqueue(m_player_buffer_queue_itf, buffer,
                                                size);

    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s enqueue_buffer failed: errcode=%d", __func__, ret);
        return JS_ERR;
    }
    return JS_OK;
}

int64_t JSAudioPlayer::get_position() {
    SLresult ret;
    SLmillisecond sec;
    ret = (*m_player_itf)->GetPosition(m_player_itf, &sec);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s GetPosition failed: errcode=%d", __func__, ret);
        return -1;
    }
    int64_t position = sec * 1000;
    if (m_paused_position != -1) {
        position = m_paused_position + position;
    }
    return position;
}

void JSAudioPlayer::update_paused_position() {
    SLresult ret;
    SLmillisecond sec;
    ret = (*m_player_itf)->GetPosition(m_player_itf, &sec);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s GetPosition failed: errcode=%d", __func__, ret);
        return;
    }
    if (m_paused_position == -1) {
        m_paused_position = sec * 1000;
    } else {
        m_paused_position = sec * 1000 + m_paused_position;
    }
}

JS_RET JSAudioPlayer::clear() {
    SLresult ret;
    ret = (*m_player_buffer_queue_itf)->Clear(m_player_buffer_queue_itf);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("%s clear failed: errcode=%d", __func__, ret);
        return JS_ERR;
    }
    return JS_OK;
}

void JSAudioPlayer::set_audio_buffer_queue_callback(slAndroidSimpleBufferQueueCallback callback) {
    sl_android_simple_buffer_queue_callback = callback;
}


void JSAudioPlayer::set_mute(bool mute) {

    if (m_is_created_audio_player) {
        SLresult ret = (*m_player_volume_itf)->
                SetMute(m_player_volume_itf,
                        mute);
        if (SL_RESULT_SUCCESS != ret) {
            LOGE("%s set_mute failed: errcode=%d", __func__, ret);
            return;
        }
    }
    m_mute = mute;
}

void JSAudioPlayer::set_volume(int volume) {

    if (m_is_created_audio_player) {
        if (volume > 100 || volume < 0) {
            LOGE("%s set_volume want m_volume is %d ,not in [0, 100]", __func__, volume);
            return;
        }

        SLresult ret = (*m_player_volume_itf)->
                SetVolumeLevel(m_player_volume_itf,
                               (SLmillibel) ((1.0f - volume / 100.0f) * -5000));
        if (SL_RESULT_SUCCESS != ret) {
            LOGE("%s set_volume failed: errcode=%d", __func__, ret);
            return;
        }
    }
    m_volume = volume;
}

//todo 目前支持2个声道音频调用。
void JSAudioPlayer::set_channel_mute(int channel, bool mute) {

    if (m_channel_count != 2) {
        LOGW("%s unsupport operation: set_channel_mute when channel count != 2", __func__);
        return;
    }
    if (m_is_created_audio_player) {
        if (NULL != m_player_mute_solo_itf) {
            SLresult ret = (*m_player_mute_solo_itf)->SetChannelMute(m_player_mute_solo_itf,
                                                                     channel,
                                                                     mute);
            if (SL_RESULT_SUCCESS != ret) {
                LOGE("%s set_channel_mute failed: errcode=%d", __func__, ret);
                return;
            }
        } else {
            LOGE("%s set_channel_mute failed m_player_mute_solo_itf is null", __func__);
        }
    }

    if (channel == 0) {
        m_left_channel_mute = mute;
    } else if (channel == 1) {
        m_right_channel_mute = mute;
    }

}

void JSAudioPlayer::reset() {
    m_is_created_audio_player = false;
    m_channel_count = 0;
    m_rate = 0;
    m_bits_per_sample = 0;
    m_paused_position = -1;

    // destroy buffer m_dequeue audio player object, and invalidate all associated interfaces
    if (m_player_object_itf != NULL) {
        (*m_player_object_itf)->Destroy(m_player_object_itf);
        m_player_object_itf = NULL;
        m_player_itf = NULL;
        m_player_buffer_queue_itf = NULL;
        m_player_effect_send_itf = NULL;
        m_player_mute_solo_itf = NULL;
        m_player_volume_itf = NULL;
    }
    // destroy output mix object, and invalidate all associated interfaces
    if (m_output_mix_object_itf != NULL) {
        (*m_output_mix_object_itf)->Destroy(m_output_mix_object_itf);
        m_output_mix_object_itf = NULL;
        m_output_mix_environmental_reverb_itf = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (m_player_engine_object_itf != NULL) {
        (*m_player_engine_object_itf)->Destroy(m_player_engine_object_itf);
        m_player_engine_object_itf = NULL;
        m_player_engine_itf = NULL;
    }

}
