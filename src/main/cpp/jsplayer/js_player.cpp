#include "js_player.h"
#include "converter/js_media_converter.h"

extern "C" {
#include "libavutil/time.h"
#include "util/js_jni.h"
#include "util/js_log.h"
}

//todo use func pointer reduce if-else

JSPlayer::JSPlayer(jobject java_js_player) {
    LOGD("%s new JSPlayer", __func__);

    m_java_js_player = (jobject) js_jni_get_env(NULL)->NewGlobalRef(java_js_player);
    m_js_event_handler = new JSEventHandler(m_java_js_player, this);

    m_egl_renderer = new JSEglRenderer(m_js_event_handler);
    m_egl_renderer->set_egl_buffer_queue_callback(egl_buffer_queue_cb, this);
    m_egl_renderer->create_renderer_thread();

    m_audio_player = new JSAudioPlayer();
    m_audio_player->set_audio_buffer_queue_callback(opensles_buffer_queue_cb);

    m_js_media_decoder = new JSMediaDecoder(m_js_event_handler);
    m_js_media_converter = new JSMediaCoverter();

    m_cur_play_status = PLAY_STATUS_CREATED;
}

JSPlayer::~JSPlayer() {
    LOGD("%s destroy JSPlayer", __func__);

    if (m_url != NULL) {
        free((void *) m_url);
    }
    av_dict_free(&m_options);
    js_jni_get_env(NULL)->DeleteGlobalRef(m_java_js_player);
    m_egl_renderer->destroy_renderer_thread();
    delete m_egl_renderer;
    delete m_audio_player;
    delete m_js_media_decoder;
    delete m_js_event_handler;
    delete m_js_media_converter;
}

void JSPlayer::prepare() {
    LOGD("%s prepare...", __func__);
    reset();
    m_io_time_out = DEFAULT_PREPARE_TIME_OUT_MICROSECONDS;
    if (!m_egl_renderer->m_is_renderer_thread_running) {
        m_egl_renderer->create_renderer_thread();
    }
    pthread_create(&m_prepare_tid, NULL, prepare_thread, this);
    m_cur_play_status = PLAY_STATUS_PREPARING;
}


void JSPlayer::play(bool is_to_resume) {
    LOGD("%s play...", __func__);
    m_is_playing = true;
    if (is_to_resume) {
        if (m_is_live) {
            start_read_frame();
        }
    } else {
        start_read_frame();
    }

    if (m_has_audio_stream) {
        m_audio_cached_que->clear_abort_get();
        m_audio_cached_que->clear_abort_put();
        m_audio_decoded_que->clear_abort_get();
        m_audio_decoded_que->clear_abort_put();

        start_decode_audio();

        pthread_create(&m_play_audio_tid, NULL, play_audio_thread,
                       this);
        LOGD("%s play_audio_thread...", __func__);
    }
    if (m_has_video_stream) {
        m_video_cached_que->clear_abort_get();
        m_video_cached_que->clear_abort_put();
        m_video_decoded_que->clear_abort_get();
        m_video_decoded_que->clear_abort_put();

        start_decode_video();

        m_egl_renderer->start_render();
        LOGD("%s start_render...", __func__);
    }
    m_cur_play_status = PLAY_STATUS_PLAYING;
    LOGD("%s playing.", __func__);
}

void JSPlayer::pause() {
    LOGD("%s pause...", __func__);
    stop_play(true);
//    m_cur_audio_position = -1;
    m_cur_play_status = PLAY_STATUS_PAUSED;
    LOGD("%s paused.", __func__);
}


void JSPlayer::resume() {
    LOGD("%s resume...", __func__);
    play(true);
    LOGD("%s resumed.", __func__);
}


void JSPlayer::stop() {
    LOGD("%s stop...", __func__);
    reset();
    m_cur_play_status = PLAY_STATUS_STOPPED;
    LOGD("%s stopped.", __func__);
}

void JSPlayer::reset() {
    LOGD("%s reset...", __func__);
    if (m_cur_play_status == PLAY_STATUS_PREPARING) {
        stop_prepare();
        free_res();
    } else if (m_cur_play_status == PLAY_STATUS_PREPARED) {
        free_res();
    } else if (m_cur_play_status == PLAY_STATUS_PLAYING) {
        stop_play(false);
        free_res();
    } else if (m_cur_play_status == PLAY_STATUS_PAUSED) {
        if (!m_is_live) {
            stop_read_frame();
        }
        free_res();
    }

    m_cur_play_status = PLAY_STATUS_CREATED;
    LOGD("%s reset.", __func__);
}


void JSPlayer::destroy() {
    LOGD("%s destroy...", __func__);
    reset();
    delete this;
    LOGD("%s destroyed.", __func__);
}


JS_RET JSPlayer::find_stream_info() {
    LOGD("%s find_stream_info...", __func__);
    JS_RET ret;
    m_format_ctx = avformat_alloc_context();
    if (!m_format_ctx) {
        LOGE("%s failed to avformat_alloc_context", __func__);
        return JS_ERR;
    }

    m_format_ctx->interrupt_callback.callback = io_interrupt_cb;
    m_format_ctx->interrupt_callback.opaque = this;

    ret = avformat_open_input(&m_format_ctx, m_url, NULL, NULL);

    if (ret != 0) {
        LOGE("%s couldn't open input stream! ret=%d:", __func__, ret);
        return JS_ERR;
    } else {
        LOGD("%s avformat_open_input success", __func__);
    }

    ret = avformat_find_stream_info(m_format_ctx, NULL);

    if (ret < 0) {
        LOGE("%s couldn't find open stream information! ret:%d:", __func__, ret);
        return JS_ERR;
    } else {
        LOGD("%s avformat_find_stream_info", __func__, ret);
    }

    m_video_stream_index = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    m_audio_stream_index = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_AUDIO, -1,
                                               m_video_stream_index, NULL, 0);

    m_has_audio_stream = m_audio_stream_index != AVERROR_STREAM_NOT_FOUND;
    m_has_video_stream = m_video_stream_index != AVERROR_STREAM_NOT_FOUND;

    if (!m_has_audio_stream && !m_has_video_stream) {
        LOGE("%s couldn't find any stream!", __func__);
        return JS_ERR;
    }

    m_is_live = is_live(m_format_ctx);
    LOGD("%s m_is_live=%d", __func__, m_is_live);

    if (m_is_live) {//todo can set  value in another func?
        if (m_min_cached_duration == -1) {
            m_min_cached_duration = DEFAULT_MIN_CACHED_DURATION_LIVE;
        }
        if (m_max_cached_duration == -1) {
            m_max_cached_duration = DEFAULT_MAX_CACHED_DURATION_LIVE;
        }
        if (m_min_decoded_duration == -1) {
            m_min_decoded_duration = DEFAULT_MIN_DECODED_DURATION_LIVE;
        }
        if (m_max_decoded_duration == -1) {
            m_max_decoded_duration = DEFAULT_MAX_DECODED_DURATION_LIVE;
        }

    } else {
        if (m_min_cached_duration == -1) {
            m_min_cached_duration = DEFAULT_MIN_CACHED_DURATION_RECORD;
        }
        if (m_max_cached_duration == -1) {
            m_max_cached_duration = DEFAULT_MAX_CACHED_DURATION_RECORD;
        }
        if (m_min_decoded_duration == -1) {
            m_min_decoded_duration = DEFAULT_MIN_DECODED_DURATION_RECORD;
        }
        if (m_max_decoded_duration == -1) {
            m_max_decoded_duration = DEFAULT_MAX_DECODED_DURATION_RECORD;
        }
    }

    LOGD("%s duration threshold ：m_min_cached_duration=%lld,"
         "m_max_cached_duration=%lld,"
         "m_min_decoded_duration=%lld,"
         "m_max_decoded_duration=%lld",
         __func__,
         m_min_cached_duration,
         m_max_cached_duration,
         m_min_decoded_duration,
         m_max_decoded_duration);

    LOGD("%s find_stream_info finish.", __func__);

    return JS_OK;
}

void JSPlayer::init_decoder() {

    AVDictionaryEntry *entry_decode_type = av_dict_get(m_options, JS_OPTION_DECODER_TYPE,
                                                       NULL,
                                                       AV_DICT_IGNORE_SUFFIX);
    if (entry_decode_type) {
        m_js_media_decoder->set_decoder_type(entry_decode_type->value);
    } else {
        m_js_media_decoder->set_decoder_type(JS_OPTION_DECODER_TYPE_HW);
    }
}

JS_RET JSPlayer::prepare_audio() {
    LOGD("%s prepare_audio...", __func__);
    JS_RET ret;
    m_audio_stream = m_format_ctx->streams[m_audio_stream_index];

    ret = (m_js_media_decoder->*m_js_media_decoder->m_create_decoder_by_av_stream)(m_audio_stream);
    if (ret != JS_OK) {
        LOGE("%s prepare_audio failed to m_create_decoder_by_av_stream", __func__);
        return JS_ERR;
    }

    ret = m_audio_player->create_audio_player(
            m_audio_stream->codecpar->sample_rate,
            m_audio_stream->codecpar->channels,
            DST_BITS_PER_SAMPLE,
            this);
    if (ret != JS_OK) {
        LOGE("%s unable to create_audio_player!", __func__);
        return JS_ERR;
    }

    m_audio_cached_que = new JSPacketQueue(QUEUE_TYPE_AUDIO,
                                           m_audio_stream->time_base,
                                           m_min_cached_duration,
                                           m_max_cached_duration,
                                           m_is_live);

    m_audio_cached_que->set_clear_callback(audio_cached_que_clear_callback, this);
    m_audio_cached_que->set_buffering_callback(audio_cached_que_buffering_callback, this);


    m_audio_decoded_que = new JSFrameQueue(QUEUE_TYPE_AUDIO,
                                           m_audio_stream->time_base,
                                           m_min_decoded_duration,
                                           m_max_decoded_duration,
                                           m_is_live);

    m_audio_decoded_que->set_clear_callback(audio_decoded_que_clear_callback, this);
    m_audio_cached_que->set_buffering_callback(audio_decoded_que_buffering_callback, this);

    if (m_is_live) {

        cache_audio_packet = &JSPlayer::cache_live_audio_packet;

    } else {
        cache_audio_packet = &JSPlayer::cache_record_audio_packet;
    }

    LOGD("%s prepare_audio finish.", __func__);
    return JS_OK;
}

JS_RET JSPlayer::prepare_video() {
    LOGD("%s prepare_video...", __func__);
    JS_RET ret;
    m_video_stream = m_format_ctx->streams[m_video_stream_index];

    ret = (m_js_media_decoder->*m_js_media_decoder->m_create_decoder_by_av_stream)(m_video_stream);
    if (ret != JS_OK) {
        LOGE("%s prepare_video failed to m_create_decoder_by_av_stream", __func__);
        return JS_ERR;
    }

    m_video_cached_que = new JSPacketQueue(QUEUE_TYPE_VIDEO,
                                           m_video_stream->time_base,
                                           m_min_cached_duration,
                                           m_max_cached_duration,
                                           m_is_live);

    m_video_cached_que->set_clear_callback(video_cached_que_clear_callback, this);
    m_video_cached_que->set_buffering_callback(video_cached_que_buffering_callback, this);

    m_video_decoded_que = new JSFrameQueue(QUEUE_TYPE_VIDEO,
                                           m_video_stream->time_base,
                                           m_min_decoded_duration,
                                           m_max_decoded_duration,
                                           m_is_live);

    m_video_decoded_que->set_clear_callback(video_decoded_que_clear_callback, this);
    m_video_decoded_que->set_buffering_callback(video_decoded_que_buffering_callback, this);

    if (m_is_live) {
        if (m_egl_renderer->m_is_hold_surface) {
            cache_video_packet = &JSPlayer::cache_live_video_packet_hold_surface;
        } else {
            cache_video_packet = &JSPlayer::cache_live_video_packet_not_hold_surface;
        }

    } else {
        cache_video_packet = &JSPlayer::cache_record_video_packet;
    }

    //===sync
    AVRational frame_rate = av_guess_frame_rate(m_format_ctx, m_video_stream, NULL);
    if (frame_rate.den && frame_rate.num) {
        m_frame_rate_duration =
                (int64_t) (av_q2d((AVRational) {frame_rate.den, frame_rate.num}) * 1000000);
        m_frame_rate = av_q2d(frame_rate);
    }
    LOGD("%s frame_rate.den=%d,frame_rate.num=%d,m_frame_rate=%f,m_frame_rate_duration=%lld",
         __func__,
         frame_rate.den,
         frame_rate.num,
         m_frame_rate,
         m_frame_rate_duration);
    //===sync

    LOGD("%s prepare_video finish.", __func__);
    return JS_OK;
}


void JSPlayer::start_read_frame() {
    LOGD("%s start_read_frame...", __func__);
    m_io_time_out = DEFAULT_READ_PKT_TIME_OUT_MICROSECONDS;
    pthread_create(&m_read_packet_tid, NULL, read_frame_thread, this);
    m_is_reading_frame = true;
}

void JSPlayer::start_decode_audio() {
    LOGD("%s start_decode_audio...", __func__);
    pthread_create(&m_decode_audio_tid, NULL, decode_audio_thread, this);
}

void JSPlayer::start_decode_video() {
    LOGD("%s start_decode_video...", __func__);
    pthread_create(&m_decode_video_tid, NULL, decode_video_thread, this);
}

void JSPlayer::stop_read_frame() {
    LOGD("%s stop_read_frame...", __func__);
    if (m_has_video_stream && !m_is_live) {
        m_video_cached_que->abort_put();
    }
    if (m_has_audio_stream && !m_is_live) {
        m_audio_cached_que->abort_put();
    }

    m_is_reading_frame = false;
    m_io_time_out = NO_TIME_OUT_MICROSECONDS;
    pthread_join(m_read_packet_tid, NULL);

    if (m_has_video_stream) {
        m_video_cached_que->clear(false);
        m_video_decoded_que->clear(false);
    }
    if (m_has_audio_stream) {
        m_audio_cached_que->clear(false);
        m_audio_decoded_que->clear(false);
    }
    LOGD("%s stop_read_frame finished.", __func__);
}


void JSPlayer::stop_play(bool is_to_pause) {
    LOGD("%s stop_play...", __func__);
    m_is_playing = false;

    if (m_has_video_stream) {
        m_video_cached_que->abort_get();
        m_video_decoded_que->abort_get();

        if (!m_is_live) {
            m_video_decoded_que->abort_put();
        }

        LOGD("%s stop_play stop decode video step1.", __func__);
        pthread_join(m_decode_video_tid, NULL);
        LOGD("%s stop_play stop decode video step2.", __func__);


        LOGD("%s stop_play stop render step1.", __func__);
        m_egl_renderer->stop_render();
        LOGD("%s stop_play stop render step2.", __func__);

    }

    if (m_has_audio_stream) {
        m_audio_cached_que->abort_get();
        m_audio_decoded_que->abort_get();

        if (!m_is_live) {
            m_audio_decoded_que->abort_put();
        }

        LOGD("%s stop_play stop decode audio step1.", __func__);
        pthread_join(m_decode_audio_tid, NULL);
        LOGD("%s stop_play stop decode audio step2.", __func__);

        LOGD("%s stop_play stop play audio step1.", __func__);
        while (m_is_audio_data_consuming);
        LOGD("%s stop_play stop play audio step2.", __func__);
    }

    if (!is_to_pause || m_is_live) {

        stop_read_frame();
    }

    LOGD("%s stop_play finished.", __func__);
}


void JSPlayer::stop_prepare() {
    LOGD("%s stop_prepare...", __func__);
    m_io_time_out = NO_TIME_OUT_MICROSECONDS;
    pthread_join(m_prepare_tid, NULL);
    LOGD("%s stop_prepare finished.", __func__);
}


void JSPlayer::free_res() {
    LOGD("%s free_res...", __func__);

    m_is_eof = false;

    m_video_stream_index = -1;
    m_audio_stream_index = -1;

    m_has_audio_stream = false;
    m_has_video_stream = false;

    m_audio_stream = NULL;
    m_video_stream = NULL;

    m_is_live = false;

    m_min_cached_duration = -1;
    m_max_cached_duration = -1;
    m_min_decoded_duration = -1;
    m_max_decoded_duration = -1;

    m_frame_rate_duration = -1;
    m_frame_rate = -1.0;
    m_cur_video_pts = -1;
    m_cur_audio_pts = -1;
    m_last_video_pts = -1;
    m_first_video_pts = -1;

    if (m_video_cached_que != NULL) {
        delete m_video_cached_que;
        m_video_cached_que = NULL;
    }

    if (m_audio_cached_que != NULL) {
        delete m_audio_cached_que;
        m_audio_cached_que = NULL;
    }

    if (m_video_decoded_que != NULL) {
        delete m_video_decoded_que;
        m_video_decoded_que = NULL;
    }

    if (m_audio_decoded_que != NULL) {
        delete m_audio_decoded_que;
        m_audio_decoded_que = NULL;
    }

    if (m_format_ctx != NULL) {
        m_format_ctx->interrupt_callback.callback = NULL;
        m_format_ctx->interrupt_callback.opaque = NULL;
        avformat_close_input(&m_format_ctx);
        avformat_free_context(m_format_ctx);
        m_format_ctx = NULL;
    }

    m_audio_player->reset();
    m_js_media_decoder->reset();
    LOGD("%s free_res finished.", __func__);
}


void JSPlayer::set_cur_timing() {
    m_timing = av_gettime();
}

void JSPlayer::set_is_intercept_audio(bool is_intercept_audio) {

    if (m_is_intercept_audio == is_intercept_audio) {
        LOGD("%s has already in this status.m_is_intercept_audio=%d", __func__,
             m_is_intercept_audio);
        return;
    }
    m_is_intercept_audio = is_intercept_audio;
    LOGD("%s stop play audio step0. m_is_intercept_audio=%d", __func__, m_is_intercept_audio);
    if (!m_has_audio_stream || !m_is_audio_data_consuming) {
        return;
    }

    //replay audio use new engine.
    LOGD("%s stop play audio step1.", __func__);
    m_audio_decoded_que->abort_get();
    LOGD("%s stop play audio step2.", __func__);
    while (m_is_audio_data_consuming);
    LOGD("%s stop play audio step3.", __func__);
    m_audio_decoded_que->clear_abort_get();
    pthread_create(&m_play_audio_tid, NULL, play_audio_thread,
                   this);
    if (m_is_intercept_audio) {
        //todo audiotrack.clear.
        m_audio_player->clear();
    }
}

void JSPlayer::on_surface_hold_state_changed() {

    if ((m_cur_play_status > PLAY_STATUS_PREPARED
         && m_cur_play_status < PLAY_STATUS_STOPPED) &&
        m_has_video_stream && m_is_live) {

        if (m_egl_renderer->m_is_hold_surface) {
            cache_video_packet = &JSPlayer::cache_live_video_packet_hold_surface;
        } else {
            m_is_cleared_video_frame = false;
            cache_video_packet = &JSPlayer::cache_live_video_packet_not_hold_surface;
        }
    }

}

JS_RET JSPlayer::cache_record_video_packet(AVPacket *src) {
    return (m_video_cached_que->*m_video_cached_que->put)(src);
}

JS_RET JSPlayer::cache_live_video_packet_hold_surface(AVPacket *src) {
    if (m_is_parse_data_from_video_packet) {
        m_js_event_handler->call_on_parse_data_from_video_packet(src->data,
                                                                 src->size);
    }
    return (m_video_cached_que->*m_video_cached_que->put)(src);
}

JS_RET JSPlayer::cache_live_video_packet_not_hold_surface(AVPacket *src) {
    if (m_is_parse_data_from_video_packet) {
        m_js_event_handler->call_on_parse_data_from_video_packet(src->data,
                                                                 src->size);
    }
    av_packet_unref(src);

    if (!m_is_cleared_video_frame) {
        m_video_cached_que->clear(false);
        LOGD("%s *drop* clear video packet.", __func__);
        m_video_decoded_que->clear(false);
        LOGD("%s *drop* clear video frame.", __func__);
        if (JS_OK != m_video_cached_que->put_flush_avpkt()) {
            LOGE("%s can't put video flush packet.", __func__);
            m_js_event_handler->call_on_error(JS_ERR_CACHE_PACKET_FAILED, 0, 0);
        }
        m_is_cleared_video_frame = true;
    }

    return JS_OK;
}

JS_RET JSPlayer::cache_record_audio_packet(AVPacket *src) {
    return (m_audio_cached_que->*m_audio_cached_que->put)(src);
}

JS_RET JSPlayer::cache_live_audio_packet(AVPacket *src) {
    return (m_audio_cached_que->*m_audio_cached_que->put)(src);
}

void JSPlayer::set_mute(bool mute) {
    m_mute = mute;
    m_audio_player->set_mute(m_mute);
}

bool JSPlayer::get_mute() {
    return m_mute;
}

void JSPlayer::set_channel_mute(int channel, bool mute) {
    if (m_audio_stream->codecpar->channels != 2) {
        LOGW("%s unsupport operation: set_channel_mute when channel count != 2", __func__);
        return;
    }
    if (channel == 0) {
        m_left_channel_mute = mute;
    } else if (channel == 1) {
        m_right_channel_mute = mute;
    }
    m_audio_player->set_channel_mute(channel, mute);
}


void *prepare_thread(void *data) {

    pthread_setname_np(pthread_self(), __func__);

    JSPlayer *player = (JSPlayer *) data;

    player->set_cur_timing();
    int ret = player->find_stream_info();
    if (ret != JS_OK) {
        LOGE("%s find_stream_info error", __func__);
        goto fail;
    }

    player->init_decoder();

    if (player->m_has_audio_stream) {
        ret = player->prepare_audio();

        if (ret != JS_OK) {
            LOGE("%s prepare_audio error", __func__);
            goto fail;

        }
    } else {
        LOGW("%s prepare_audio didn't find a audio stream", __func__);
    }

    if (player->m_has_video_stream) {

        ret = player->prepare_video();

        if (ret != JS_OK) {
            LOGE("%s prepare_video error", __func__);
            goto fail;

        }
    } else {
        LOGW("%s prepare_video didn't find a video stream", __func__);
    }

    player->m_cur_play_status = PLAY_STATUS_PREPARED;
    player->m_js_event_handler->call_on_prepared();
    pthread_exit(0);

    fail:
    if (player->m_io_time_out != NO_TIME_OUT_MICROSECONDS) {
        player->m_js_event_handler->call_on_error(JS_ERR_PREPARE_FAILED, 0, 0);
    }
    pthread_exit(0);
}

void *play_audio_thread(void *data) {

    JSPlayer *player = (JSPlayer *) data;
    player->m_is_audio_data_consuming = true;

    if (player->m_is_intercept_audio) {
        pthread_setname_np(pthread_self(), "play_audio_consume_audio_data_by_interceptor");
        consume_audio_data_by_interceptor(player);
    } else {
        //opensles
        pthread_setname_np(pthread_self(), "play_audio_consume_audio_data_by_opensles");
        consume_audio_data_by_opensles(player);
        //audiotrack.
//        pthread_setname_np(pthread_self(), "play_audio_consume_audio_data_by_audiotrack");
//        consume_audio_data_by_audiotrack(data);
    }

    pthread_exit(0);
}


void *read_frame_thread(void *data) {

    pthread_setname_np(pthread_self(), __func__);
    JSPlayer *player = (JSPlayer *) data;
    JSEventHandler *js_event_handler = player->m_js_event_handler;
    AVFormatContext *format_ctx = player->m_format_ctx;

    AVPacket packet0, *packet = &packet0;

    while (player->m_is_reading_frame) {

        player->set_cur_timing();
        packet->flags = 0;
        int ret = av_read_frame(format_ctx, packet);

        if (ret != 0) {

            if (player->m_io_time_out != NO_TIME_OUT_MICROSECONDS) {
                if (ret == AVERROR_EOF || avio_feof(format_ctx->pb)) {

                    if (player->m_has_audio_stream) {

                        if (JS_OK != player->m_audio_cached_que->put_eof_avpkt()) {
                            LOGE("%s can't put audio eof packet.", __func__);
                            js_event_handler->call_on_error(JS_ERR_CACHE_PACKET_FAILED, 0, 0);
                            break;
                        }
                    }

                    if (player->m_has_video_stream) {
                        if (JS_OK != player->m_video_cached_que->put_eof_avpkt()) {
                            LOGE("%s can't put video eof packet.", __func__);
                            js_event_handler->call_on_error(JS_ERR_CACHE_PACKET_FAILED, 0, 0);
                            break;
                        }
                    }

                } else {
                    js_event_handler->call_on_error(JS_ERR_READ_FRAME_FAILED, 0, 0);
                }
            }

            break;
        }

        if (packet->stream_index == player->m_audio_stream_index) {
            if (JS_OK != (player->*player->cache_audio_packet)(packet)) {
                LOGE("%s can't cache audio packet.", __func__);
                av_packet_unref(packet);
                js_event_handler->call_on_error(JS_ERR_CACHE_PACKET_FAILED, 0, 0);
                break;
            }
        } else if (packet->stream_index == player->m_video_stream_index) {
            if (JS_OK != (player->*player->cache_video_packet)(packet)) {
                LOGE("%s can't cache video packet.", __func__);
                av_packet_unref(packet);
                js_event_handler->call_on_error(JS_ERR_CACHE_PACKET_FAILED, 0, 0);
                break;
            }
        } else {

            LOGD("%s *drop* drop a packet unsupported.", __func__);
            av_packet_unref(packet);
        }
    }
    pthread_exit(0);
}

void *decode_video_thread(void *data) {

    pthread_setname_np(pthread_self(), __func__);
    JSPlayer *player = (JSPlayer *) data;

    JS_RET ret;
    AVPacket *avpkt = NULL;
    AVFrame *frame = av_frame_alloc();

    if (frame == NULL) {
        LOGE("%s unable to allocate an AVFrame!", __func__);
        player->m_js_event_handler->call_on_error(JS_ERR_EXTERNAL, 0, 0);
        goto end;
    }

    while (player->m_is_playing) {

        avpkt = player->m_video_cached_que->get();
        if (avpkt == NULL) {
            break;
        }
        if (avpkt->data == player->m_video_cached_que->m_flush_pkt->data) {
            (player->m_js_media_decoder->*player->m_js_media_decoder->video_decoder_flush)();
            av_packet_free(&avpkt);
            LOGD("%s video_decoder_flush", __func__);
            continue;
        }

        while (player->m_is_playing) {
            ret = (player->m_js_media_decoder->*player->
                    m_js_media_decoder->
                    decode_video_packet)(avpkt, frame);

            if (ret >= 0) {


//                LOGD("%s player->m_video_decoded_que->get_num()=%d", __func__,
//                     player->m_video_decoded_que->get_num());
                //ffplay
//                ﻿    if (decoder_reorder_pts == -1) {
//                LOGD("%s frame->pts=%lld,frame->best_effort_timestamp=%lld", __func__,
//                     frame->pts,
//                     frame->best_effort_timestamp);

                if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                    frame->pts = frame->best_effort_timestamp;//fixme why?
                }
//                } else if (!decoder_reorder_pts) {
//                    frame->pts = frame->pkt_dts;
//            }

                if (JS_OK !=
                    (player->m_video_decoded_que->*player->m_video_decoded_que->put)(frame)) {
                    player->m_js_event_handler->call_on_error(JS_ERR_CACHE_FRAME_FAILED, 0, 0);
                    goto end;
                }

                if (ret == JS_OK) {
                    av_packet_free(&avpkt);
                    break;
                }
            } else if (ret == JS_ERR_EOF) {
                // fixme invoke once.
                player->m_js_event_handler->call_on_completed();
                goto end;
            } else if (ret == JS_ERR_TRY_TO_USE_SW_DECODER) {
                LOGD("%s JS_ERR_TRY_TO_USE_SW_DECODER", __func__);
                //fixme data can't serial.
                continue;
            } else if (ret == JS_ERR_NEED_SEND_THIS_PACKET_AGAIN) {
                LOGD("%s JS_ERR_NEED_SEND_THIS_PACKET_AGAIN", __func__);
                continue;
            } else if (ret == JS_ERR_NEED_SEND_NEW_PACKET_AGAIN) {
                av_packet_free(&avpkt);
                break;
            } else {
                player->m_js_event_handler->call_on_error(ret, 0, 0);
                goto end;
            }
        }
    }

    end:
    if (avpkt) {
        av_packet_free(&avpkt);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    pthread_exit(0);
}

void *decode_audio_thread(void *data) {

    pthread_setname_np(pthread_self(), __func__);
    JSPlayer *player = (JSPlayer *) data;
    AVCodecParameters *audio_stream_param = player->m_audio_stream->codecpar;
    JS_RET ret;
    AVPacket *avpkt = NULL;
    AVFrame *converted_frame = NULL;
    AVFrame *frame = av_frame_alloc();

    if (frame == NULL) {
        LOGE("%s unable to allocate an AVFrame!", __func__);
        player->m_js_event_handler->call_on_error(JS_ERR_EXTERNAL, 0, 0);
        goto end;
    }

    if (audio_stream_param->format != DEFAULT_AV_SAMPLE_FMT) {

        converted_frame = av_frame_alloc();
        if (converted_frame == NULL) {
            LOGE("%s unable to allocate an AVFrame!", __func__);
            player->m_js_event_handler->call_on_error(JS_ERR_EXTERNAL, 0, 0);
            goto end;
        }
        ret = player->m_js_media_converter->init_audio_converter(NULL,
                                                                 audio_stream_param->channel_layout,
                                                                 audio_stream_param->channels,
                                                                 DEFAULT_AV_SAMPLE_FMT,
                                                                 audio_stream_param->sample_rate,
                                                                 audio_stream_param->channel_layout,
                                                                 audio_stream_param->channels,
                                                                 (AVSampleFormat) audio_stream_param->format,
                                                                 audio_stream_param->sample_rate);
        if (ret != JS_OK) {
            LOGE("%s unable to init audio converter!", __func__);
            player->m_js_event_handler->call_on_error(JS_ERR_CONVERTER_INIT_AUDIO_FAILED, 0, 0);
            goto end;
        }
    }

    while (player->m_is_playing) {

        avpkt = player->m_audio_cached_que->get();
        if (avpkt == NULL) {
            break;
        }
        if (avpkt->data == player->m_audio_cached_que->m_flush_pkt->data) {
            (player->m_js_media_decoder->*player->m_js_media_decoder->audio_decoder_flush)();
            av_packet_free(&avpkt);
            LOGD("%s audio_decoder_flush", __func__);
            continue;
        }

        while (player->m_is_playing) {

            ret = (player->m_js_media_decoder->*player->
                    m_js_media_decoder->
                    decode_audio_packet)(avpkt, frame);

            if (ret >= 0) {

                //ffplay time_base convert.
//                ﻿AVRational tb = (AVRational){1, frame->sample_rate};
//                if (frame->pts != AV_NOPTS_VALUE)
//                    frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(d->avctx), tb);
//                else if (d->next_pts != AV_NOPTS_VALUE)
//                    frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
//                if (frame->pts != AV_NOPTS_VALUE) {
//                    d->next_pts = frame->pts + frame->nb_samples;
//                    d->next_pts_tb = tb;
//                }
//                LOGD("%s frame->pts=%lld,frame->best_effort_timestamp=%lld", __func__,
//                     frame->pts,
//                     frame->best_effort_timestamp);

                // 设置通道数或channel_layout
                if (frame->channels > 0 && frame->channel_layout == 0) {
                    frame->channel_layout = (uint64_t) av_get_default_channel_layout(
                            frame->channels);
                } else if (frame->channels == 0 && frame->channel_layout > 0) {
                    frame->channels = av_get_channel_layout_nb_channels(
                            frame->channel_layout);
                }

                if (converted_frame) {

                    if (av_frame_copy_props(converted_frame, frame) < 0) {
                        LOGE("%s unable to av_frame_copy_props!", __func__);
                        player->m_js_event_handler->call_on_error(JS_ERR_EXTERNAL, 0, 0);
                        goto end;
                    }

                    converted_frame->format = DEFAULT_AV_SAMPLE_FMT;
                    converted_frame->channels = frame->channels;
                    converted_frame->channel_layout = frame->channel_layout;
                    converted_frame->nb_samples = frame->nb_samples;

                    if (av_frame_get_buffer(converted_frame, 0) != 0) {
                        LOGE("%s unable to av_frame_get_buffer!", __func__);
                        player->m_js_event_handler->call_on_error(JS_ERR_EXTERNAL, 0, 0);
                        goto end;
                    }

                    converted_frame->linesize[0] = player->m_js_media_converter->convert_simple_format_to_S16(
                            converted_frame->data[0],
                            frame);

                    av_frame_unref(frame);

                    if (converted_frame->linesize[0] == 0) {
                        player->m_js_event_handler->call_on_error(
                                JS_ERR_CONVERTER_AUDIO_CONVERT_FAILED, 0, 0);
                        goto end;
                    } else {

                        if (JS_OK !=
                            (player->m_audio_decoded_que->*player->m_audio_decoded_que->put)(
                                    converted_frame)) {
                            player->m_js_event_handler->call_on_error(JS_ERR_CACHE_FRAME_FAILED,
                                                                      0,
                                                                      0);
                            goto end;
                        }
                    }
                } else {
                    if (JS_OK !=
                        (player->m_audio_decoded_que->*player->m_audio_decoded_que->put)(
                                frame)) {
                        player->m_js_event_handler->call_on_error(JS_ERR_CACHE_FRAME_FAILED, 0,
                                                                  0);
                        goto end;
                    }
                }

                if (ret == JS_OK) {
                    av_packet_free(&avpkt);
                    break;
                }

            } else if (ret == JS_ERR_EOF) {
                // fixme invoke once.
                player->m_js_event_handler->call_on_completed();
                goto end;
            } else if (ret == JS_ERR_TRY_TO_USE_SW_DECODER) {
                //fixme data can't serial.
                LOGD("%s JS_ERR_TRY_TO_USE_SW_DECODER", __func__);
                continue;
            } else if (ret == JS_ERR_NEED_SEND_THIS_PACKET_AGAIN) {
                LOGD("%s JS_ERR_NEED_SEND_THIS_PACKET_AGAIN", __func__);
                continue;
            } else if (ret == JS_ERR_NEED_SEND_NEW_PACKET_AGAIN) {
                av_packet_free(&avpkt);
                break;
            } else {
                player->m_js_event_handler->call_on_error(ret, 0, 0);
                goto end;
            }
        }
    }

    end:
    if (avpkt) {
        av_packet_free(&avpkt);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    if (converted_frame) {
        av_frame_free(&converted_frame);
    }
    if (player->m_js_media_converter->is_audio_converter_initialized()) {
        player->m_js_media_converter->release_audio_converter();
    }
    pthread_exit(0);
}


/**
 * return 1 timeout.
 */
int io_interrupt_cb(void *data) {

    JSPlayer *player = (JSPlayer *) data;

    if (av_gettime() - player->m_timing > player->m_io_time_out) {
        LOGD("%s io timeout.", __func__);
        return 1;
    } else {
        return 0;
    }
}


void opensles_buffer_queue_cb(SLAndroidSimpleBufferQueueItf caller, void *data) {
    JSPlayer *player = (JSPlayer *) data;
    JSAudioPlayer *audio_player = player->m_audio_player;
    AVFrame *frame = NULL;

    if (!player->m_is_playing) {
        goto end;
    }
    frame = player->m_audio_decoded_que->get();
    if (frame == NULL) {
        goto end;
    }

    audio_player->enqueue_buffer(frame->data[0], (unsigned int) frame->linesize[0]);
    av_frame_free(&frame);
    return;

    end:
    if (frame) {
        av_frame_free(&frame);
    }
    player->m_is_audio_data_consuming = false;
}

/**
1. 无法察觉：音频和视频的时间戳差值在：-100ms ~ +25ms 之间
2. 能够察觉：音频滞后了 100ms 以上，或者超前了 25ms 以上
3. 无法接受：音频滞后了 185ms 以上，或者超前了 90ms 以上
 */

void egl_buffer_queue_cb(void *data) {

    JSPlayer *player = (JSPlayer *) data;

    if (!player->m_is_playing) {
        return;
    }

    AVFrame *frame = player->m_video_decoded_que->get();
    if (frame == NULL) {
        return;
    }

    int64_t video_position = (int64_t) (frame->pts * av_q2d(player->m_video_stream->time_base) *
                                        1000000);
    if (player->m_first_video_pts == -1) {
        player->m_first_video_pts = video_position;
        player->m_cur_video_pts = 0;
    } else {
        player->m_cur_video_pts = video_position - player->m_first_video_pts;
    }

    LOGI("%s player->m_cur_video_pts=%lld,frame->pts=%lld,player->m_first_video_pts=%lld,m_video_stream->time_base.den=%d",
         __func__, player->m_cur_video_pts, frame->pts, player->m_first_video_pts,
         player->m_video_stream->time_base.den);

    while (player->m_is_playing) {

        player->m_cur_audio_pts = player->m_audio_player->get_position();

        LOGI("%s player->m_cur_audio_pts=%lld",
             __func__, player->m_cur_audio_pts);

        int64_t diff = player->m_cur_video_pts - player->m_cur_audio_pts;

        if (diff <= 0) {
            //video is slow than audio.
            if (diff < -100000) {
                //drop
                LOGD("%s drop video frame diff=%lld", __func__, diff);
                goto end;
            } else {
                //draw immediately.
                LOGD("%s draw immediately diff=%lld", __func__, diff);
                break;
            }
        } else {
            //audio is slow than video.
            int64_t delay = diff;
            if (delay < 40000) {
                if (player->m_last_video_pts != -1) {
                    delay = player->m_cur_video_pts - player->m_last_video_pts;
                } else {
                    delay = player->m_frame_rate_duration == -1 ? delay
                                                                : player->m_frame_rate_duration;
                }
            } else {
                av_usleep(delay);
            }
            LOGD("%s delay=%lld,diff=%lld", __func__, delay, diff);
            break;
        }
    }
    player->m_last_video_pts = player->m_cur_video_pts;

    player->m_egl_renderer->render(frame);
    av_frame_free(&frame);
    return;

    end:
    if (frame) {
        av_frame_free(&frame);
    }
}
//fixme 硬解码 eof signal 不生效。 eof signal 异常

void audio_cached_que_clear_callback(void *data) {
    JSPlayer *player = (JSPlayer *) data;
    LOGD("%s *drop* drop audio frame (clear all)", __func__);
}

void video_cached_que_clear_callback(void *data) {
    JSPlayer *player = (JSPlayer *) data;
    LOGD("%s *drop* drop video frame (clear all)", __func__);
}

void audio_decoded_que_clear_callback(void *data) {
    JSPlayer *player = (JSPlayer *) data;
    LOGD("%s *drop* drop audio frame (clear all)", __func__);
}

void video_decoded_que_clear_callback(void *data) {
    JSPlayer *player = (JSPlayer *) data;
    LOGD("%s *drop* drop video packet (clear all)", __func__);
}

void audio_cached_que_buffering_callback(void *data, bool is_buffering) {
    JSPlayer *player = (JSPlayer *) data;
    if (is_buffering) {
        LOGD("%s start buffering audio packet.", __func__);
    } else {
        LOGD("%s stop buffering audio packet.", __func__);
    }

    player->m_js_event_handler->call_on_buffering(is_buffering);
}

void video_cached_que_buffering_callback(void *data, bool is_buffering) {
    JSPlayer *player = (JSPlayer *) data;
    if (is_buffering) {
        LOGD("%s start buffering video packet.", __func__);
    } else {
        LOGD("%s stop buffering video packet.", __func__);
    }
    player->m_js_event_handler->call_on_buffering(is_buffering);
}


void audio_decoded_que_buffering_callback(void *data, bool is_buffering) {
    JSPlayer *player = (JSPlayer *) data;
    if (is_buffering) {
        LOGD("%s start buffering audio frame.", __func__);
    } else {
        LOGD("%s stop buffering audio frame.", __func__);
    }
    player->m_js_event_handler->call_on_buffering(is_buffering);
}

void video_decoded_que_buffering_callback(void *data, bool is_buffering) {
    JSPlayer *player = (JSPlayer *) data;
    if (is_buffering) {
        LOGD("%s start buffering video frame.", __func__);
    } else {
        LOGD("%s stop buffering video frame.", __func__);
    }
    player->m_js_event_handler->call_on_buffering(is_buffering);
}


void consume_audio_data_by_interceptor(JSPlayer *player) {

    AVFrame *frame = NULL;
    JSEventHandler *js_event_handler = player->m_js_event_handler;

    while (player->m_is_playing) {

        frame = player->m_audio_decoded_que->get();
        if (frame == NULL) {
            break;
        }

        if (player->m_mute || (player->m_left_channel_mute && player->m_right_channel_mute)) {
            memset(frame->data[0], 0, (size_t) frame->linesize[0]);
        } else if (player->m_left_channel_mute) {
            for (int i = 0; i < frame->nb_samples * 2; i += 2) {
                ((short *) frame->data[0])[i] = 0;
            }
        } else if (player->m_right_channel_mute) {
            for (int i = 1; i < frame->nb_samples * 2; i += 2) {
                ((short *) frame->data[0])[i] = 0;
            }
        }

        js_event_handler->call_on_intercepted_pcm_data((short *) frame->data[0],
                                                       frame->linesize[0],
                                                       frame->channels);

        av_frame_free(&frame);
    }

    player->m_is_audio_data_consuming = false;
}

void consume_audio_data_by_audiotrack(JSPlayer *player) {

}


void consume_audio_data_by_opensles(JSPlayer *player) {
    opensles_buffer_queue_cb(player->m_audio_player->m_player_buffer_queue_itf, player);
}


bool is_live(AVFormatContext *s) {
    if (!strcmp(s->iformat->name, "rtp")
        || !strcmp(s->iformat->name, "rtsp")
        || !strcmp(s->iformat->name, "sdp")
        || !strcmp(s->iformat->name, "flv")) {
        LOGD("%s s->iformat->name=%s", __func__, s->iformat->name);
        return true;
    }

    if (s->pb && (!strncmp(s->filename, "rtp:", 4)
                  || !strncmp(s->filename, "udp:", 4))) {
        LOGD("%s s->filename=%s", __func__, s->filename);
        return true;
    }
    return false;
}