#include "js_audio_player.h"

extern "C" {
#include "util/js_log.h"
}

/**
 *
（1）C 语言接口，兼容 C++，需要在 NDK 下开发，能更好地集成在 native 应用中
（2）运行于 native 层，需要自己管理资源的申请与释放，没有 Dalvik 虚拟机的垃圾回收机制
（3）支持 PCM 数据的采集，支持的配置：16bit 位宽，16000 Hz采样率，单通道。（其他的配置不能保证兼容所有平台）
（4）支持 PCM 数据的播放，支持的配置：8bit/16bit 位宽，单通道/双通道，小端模式，采样率（8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 Hz）
（5）支持播放的音频数据来源：res 文件夹下的音频、assets 文件夹下的音频、sdcard 目录下的音频、在线网络音频、代码中定义的音频二进制数据等等
 */

JSAudioPlayer::JSAudioPlayer() {

}

JSAudioPlayer::~JSAudioPlayer() {
    sl_android_simple_buffer_queue_callback = NULL;
}

JS_RET JSAudioPlayer::create_engine() {

    SLresult ret;
    // create engine
    ret = slCreateEngine(&m_engine_object, 0, NULL, 0, NULL, NULL);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("create engine failed: errcode=%d", ret);
        goto fail;
    }

    // realize the engine
    ret = (*m_engine_object)->Realize(m_engine_object, SL_BOOLEAN_FALSE);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("realize engine failed: errcode=%d", ret);
        goto fail;
    }
    // get the engine interface, which is needed in order to create other objects
    ret = (*m_engine_object)->GetInterface(m_engine_object, SL_IID_ENGINE, &m_engine_engine);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("get engine interface failed: errcode=%d", ret);
        goto fail;
    }


    // create output mix, with environmental reverb specified as a non-required interface
    ret = (*m_engine_engine)->CreateOutputMix(m_engine_engine, &m_output_mix_object, 0, 0, 0);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("create output mix failed: errcode=%d", ret);
        goto fail;
    }

    // realize the output mix
    ret = (*m_output_mix_object)->Realize(m_output_mix_object, SL_BOOLEAN_FALSE);
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("realize output mix failed: errcode=%d", ret);
        goto fail;
    }

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    ret = (*m_output_mix_object)->GetInterface(m_output_mix_object, SL_IID_ENVIRONMENTALREVERB,
                                               &m_output_mix_environmental_reverb);
    if (ret == SL_RESULT_SUCCESS) {
        LOGE("get m_output_mix_environmental_reverb interface failed: errcode=%d", ret);
        (*m_output_mix_environmental_reverb)->SetEnvironmentalReverbProperties(
                m_output_mix_environmental_reverb, &m_reverb_settings);
    }

    return JS_OK;

    fail:
    reset();
    return JS_ERR;
}

// create buffer m_dequeue audio player
JS_RET JSAudioPlayer::create_AudioPlayer(int rate, int channel, int bit_per_sample, void *data) {


    LOGD("create_AudioPlayer:%d,%d", rate, channel);

    if (create_engine() != JS_OK) {
        LOGE("create_AudioPlayer failed to create engine");
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
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, m_output_mix_object};
    SLDataSink audioSnk = {&loc_outmix, NULL};

//    // create audio player
    if (m_channel_count > 1) {

        const SLInterfaceID ids[4] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
                                      SL_IID_MUTESOLO, SL_IID_VOLUME};
        const SLboolean req[4] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
                                  SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
        ret = (*m_engine_engine)->CreateAudioPlayer(m_engine_engine, &m_bq_player_object, &audioSrc,
                                                    &audioSnk,
                                                    4, ids, req);
    } else {
        const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
        const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
                                  SL_BOOLEAN_TRUE};
        ret = (*m_engine_engine)->CreateAudioPlayer(m_engine_engine, &m_bq_player_object, &audioSrc,
                                                    &audioSnk,
                                                    3, ids, req);
    }


    if (SL_RESULT_SUCCESS != ret) {
        LOGE("createAudioPlayer failed: errcode=%d", ret);
        goto fail;
    }


    // realize the player
    ret = (*m_bq_player_object)->Realize(m_bq_player_object, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("realize player failed: errcode=%d", ret);
        goto fail;
    }

    // get the play interface
    ret = (*m_bq_player_object)->GetInterface(m_bq_player_object, SL_IID_PLAY, &m_bq_player_play);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("get play interface failed: errcode=%d", ret);
        goto fail;
    }

    // get the buffer m_dequeue interface
    ret = (*m_bq_player_object)->GetInterface(m_bq_player_object, SL_IID_BUFFERQUEUE,
                                              &m_bq_player_buffer_queue);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("get buffer m_dequeue interface failed: errcode=%d", ret);
        goto fail;
    }

    if (sl_android_simple_buffer_queue_callback == NULL) {
        LOGE("sl_android_simple_buffer_queue_callback is null ,please use set_audio_buffer_queue_callback to set the sl_android_simple_buffer_queue_callback first");
        goto fail;
    }

    // register callback on the bufferQueue
    ret = (*m_bq_player_buffer_queue)->RegisterCallback(m_bq_player_buffer_queue,
                                                        sl_android_simple_buffer_queue_callback,
                                                        data);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("register sl_android_simple_buffer_queue_callback on the buffer m_dequeue failed: errcode=%d",
             ret);
        goto fail;
    }

    // get the effect send interface
    ret = (*m_bq_player_object)->GetInterface(m_bq_player_object, SL_IID_EFFECTSEND,
                                              &m_bq_player_effect_send);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("get effect send interface failed: errcode=%d", ret);
        goto fail;
    }
    // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    if (m_channel_count > 1) {
        ret = (*m_bq_player_object)->GetInterface(m_bq_player_object, SL_IID_MUTESOLO,
                                                  &m_bq_player_mute_solo);
        if (SL_RESULT_SUCCESS != ret) {
            LOGE("get m_bq_player_mute_solo interface failed: errcode=%d", ret);
        }
    }
    // get the volume interface
    ret = (*m_bq_player_object)->GetInterface(m_bq_player_object, SL_IID_VOLUME,
                                              &m_bq_player_volume);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("get the m_bq_player_volume interface failed: errcode=%d", ret);
        goto fail;
    }

    // set the player's state to playing
    ret = (*m_bq_player_play)->SetPlayState(m_bq_player_play, SL_PLAYSTATE_PLAYING);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("set the player's state to playing failed: errcode=%d", ret);
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


JS_RET JSAudioPlayer::enqueue(const void *buffer, unsigned int size) {

    SLresult ret;
    // enqueue another buffer
    ret = (*m_bq_player_buffer_queue)->Enqueue(m_bq_player_buffer_queue, buffer,
                                               size);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("enqueue failed: errcode=%d", ret);
        return JS_ERR;
    }
    return JS_OK;
}

JS_RET JSAudioPlayer::clear() {
    SLresult ret;
    ret = (*m_bq_player_buffer_queue)->Clear(m_bq_player_buffer_queue);
    if (SL_RESULT_SUCCESS != ret) {
        LOGE("clear failed: errcode=%d", ret);
        return JS_ERR;
    }
    return JS_OK;
}

void JSAudioPlayer::set_audio_buffer_queue_callback(slAndroidSimpleBufferQueueCallback callback) {
    sl_android_simple_buffer_queue_callback = callback;
}


void JSAudioPlayer::set_mute(bool mute) {

    if (m_is_created_audio_player) {
        SLresult ret = (*m_bq_player_volume)->
                SetMute(m_bq_player_volume,
                        mute);
        if (SL_RESULT_SUCCESS != ret) {
            LOGE("set_mute failed: errcode=%d", ret);
            return;
        }
    }
    m_mute = mute;
}

void JSAudioPlayer::set_volume(int volume) {

    if (m_is_created_audio_player) {
        if (volume > 100 || volume < 0) {
            LOGE("set_volume want m_volume is %d ,not in [0, 100]", volume);
            return;
        }

        SLresult ret = (*m_bq_player_volume)->
                SetVolumeLevel(m_bq_player_volume,
                               (SLmillibel) ((1.0f - volume / 100.0f) * -5000));
        if (SL_RESULT_SUCCESS != ret) {
            LOGE("set_volume failed: errcode=%d", ret);
            return;
        }
    }
    m_volume = volume;
}

//todo 目前支持2个声道音频调用。
void JSAudioPlayer::set_channel_mute(int channel, bool mute) {

    if (m_channel_count != 2) {
        LOGW("unsupport operation: set_channel_mute when channel count != 2");
        return;
    }
    if (m_is_created_audio_player) {
        if (NULL != m_bq_player_mute_solo) {
            SLresult ret = (*m_bq_player_mute_solo)->SetChannelMute(m_bq_player_mute_solo, channel,
                                                                    mute);
            if (SL_RESULT_SUCCESS != ret) {
                LOGE("set_channel_mute failed: errcode=%d", ret);
                return;
            }
        } else {
            LOGE("set_channel_mute failed m_bq_player_mute_solo is null");
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

    // destroy buffer m_dequeue audio player object, and invalidate all associated interfaces
    if (m_bq_player_object != NULL) {
        (*m_bq_player_object)->Destroy(m_bq_player_object);
        m_bq_player_object = NULL;
        m_bq_player_play = NULL;
        m_bq_player_buffer_queue = NULL;
        m_bq_player_effect_send = NULL;
        m_bq_player_mute_solo = NULL;
        m_bq_player_volume = NULL;
    }
    // destroy output mix object, and invalidate all associated interfaces
    if (m_output_mix_object != NULL) {
        (*m_output_mix_object)->Destroy(m_output_mix_object);
        m_output_mix_object = NULL;
        m_output_mix_environmental_reverb = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (m_engine_object != NULL) {
        (*m_engine_object)->Destroy(m_engine_object);
        m_engine_object = NULL;
        m_engine_engine = NULL;
    }

}
