#include "js_player.h"
#include "converter/js_media_converter.h"
#include <unistd.h>

extern "C" {
#include "libavutil/time.h"
#include "util/js_jni.h"
#include "util/js_log.h"
}

//todo use func pointer reduce if-else

JSPlayer::JSPlayer(jobject java_js_player) {
    LOGD("new JSPlayer");

    m_java_js_player = (jobject) js_jni_get_env(NULL)->NewGlobalRef(java_js_player);
    m_js_event_handler = new JSEventHandler(m_java_js_player, this);

    m_egl_renderer = new JSEglRenderer(m_js_event_handler);
    m_egl_renderer->set_egl_buffer_queue_callback(egl_buffer_queue_cb, this);
    m_egl_renderer->create_renderer_thread();

    m_audio_player = new JSAudioPlayer();
    m_audio_player->set_audio_buffer_queue_callback(opensles_buffer_queue_cb);

    m_js_media_decoder = new JSMediaDecoder(m_js_event_handler);

    m_cur_play_status = PLAY_STATUS_CREATED;
}

JSPlayer::~JSPlayer() {
    LOGD("destroy JSPlayer");

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
}

void JSPlayer::prepare() {
    LOGD("prepare...");
    reset();
    m_io_time_out = DEFAULT_PREPARE_TIME_OUT_MICROSECONDS;
    if (!m_egl_renderer->m_is_renderer_thread_running) {
        m_egl_renderer->create_renderer_thread();
    }
    pthread_create(&m_prepare_tid, NULL, prepare_thread, this);
    m_cur_play_status = PLAY_STATUS_PREPARING;
}


void JSPlayer::play(bool is_to_resume) {
    LOGD("play...");
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
        LOGD("play_audio_thread...");
    }
    if (m_has_video_stream) {
        m_video_cached_que->clear_abort_get();
        m_video_cached_que->clear_abort_put();
        m_video_decoded_que->clear_abort_get();
        m_video_decoded_que->clear_abort_put();

        start_decode_video();

        AVCodecParameters *codecpar = m_video_stream->codecpar;
        m_egl_renderer->start_render(codecpar->width,
                                     codecpar->height);
        LOGD("start_render...");
    }
    m_cur_play_status = PLAY_STATUS_PLAYING;
    LOGD("playing.");
}

void JSPlayer::pause() {
    LOGD("pause...");
    stop_play(true);
    m_cur_play_status = PLAY_STATUS_PAUSED;
    LOGD("paused.");
}


void JSPlayer::resume() {
    LOGD("resume...");
    play(true);
    LOGD("resumed.");
}


void JSPlayer::stop() {
    LOGD("stop...");
    reset();
    m_cur_play_status = PLAY_STATUS_STOPPED;
    LOGD("stopped.");
}

void JSPlayer::reset() {
    LOGD("reset...");
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
    LOGD("reset.");
}


void JSPlayer::destroy() {
    LOGD("destroy...");
    reset();
    delete this;
    LOGD("destroyed.");
}


JS_RET JSPlayer::find_stream_info() {
    LOGD("find_stream_info...");
    JS_RET ret;
    m_format_ctx = avformat_alloc_context();
    if (!m_format_ctx) {
        LOGE("failed to malloc m_format_ctx");
        return JS_ERR;
    }

    m_format_ctx->interrupt_callback.callback = io_interrupt_cb;
    m_format_ctx->interrupt_callback.opaque = this;

    ret = avformat_open_input(&m_format_ctx, m_url, NULL, NULL);

    if (ret != 0) {
        LOGE("couldn't open input stream!>ret:%d:", ret);
        return JS_ERR;
    } else {
        LOGD("avformat_open_input success");
    }


    ret = avformat_find_stream_info(m_format_ctx, NULL);

    if (LOGGABLE)
        av_dump_format(m_format_ctx, 0, m_url, 0);

    if (ret < 0) {
        LOGE("couldn't find open stream information!>ret:%d:", ret);
        return JS_ERR;
    } else {

        LOGD("avformat_find_stream_info", ret);
    }

    m_video_stream_index = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    m_audio_stream_index = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_AUDIO, -1,
                                               m_video_stream_index, NULL, 0);

    m_has_audio_stream = m_audio_stream_index != AVERROR_STREAM_NOT_FOUND;
    m_has_video_stream = m_video_stream_index != AVERROR_STREAM_NOT_FOUND;


    m_is_live = is_live(m_format_ctx);
    LOGD("m_is_live=%d", m_is_live);

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

    LOGD("duration threshold ： m_min_cached_duration=%"
         PRId64
         ","
         "m_max_cached_duration=%"
         PRId64
         ","
         "m_min_decoded_duration=%"
         PRId64
         ","
         "m_max_decoded_duration=%"
         PRId64
         "",
         m_min_cached_duration,
         m_max_cached_duration,
         m_min_decoded_duration,
         m_max_decoded_duration);

    LOGD("find_stream_info finish.");

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
    LOGD("prepare_audio...");
    JS_RET ret;
    m_audio_stream = m_format_ctx->streams[m_audio_stream_index];

    ret = (m_js_media_decoder->*m_js_media_decoder->m_create_decoder_by_av_stream)(m_audio_stream);
    if (ret != JS_OK) {
        LOGE("prepare_audio failed to m_create_decoder_by_av_stream");
        return JS_ERR;
    }

    AVCodecParameters *codecpar = m_audio_stream->codecpar;
    ret = m_audio_player->create_AudioPlayer(
            codecpar->sample_rate,
            codecpar->channels,
            DST_BITS_PER_SAMPLE,
            this);
    if (ret != JS_OK) {
        LOGE("unable to create_AudioPlayer!");
        return JS_ERR;
    }


    m_audio_cached_que = new JSQueue<AVPacket *>(QUEUE_TYPE_AUDIO,
                                                 m_audio_stream->time_base,
                                                 m_min_cached_duration,
                                                 m_max_cached_duration,
                                                 m_is_live);

    m_audio_cached_que->set_clear_callback(audio_cached_que_clear_callback, this);


    m_audio_decoded_que = new JSQueue<AVFrame *>(QUEUE_TYPE_AUDIO,
                                                 m_audio_stream->time_base,
                                                 m_min_decoded_duration,
                                                 m_max_decoded_duration,
                                                 m_is_live);

    m_audio_decoded_que->set_clear_callback(audio_decoded_que_clear_callback, this);

    if (m_is_live) {

        cache_audio_packet = &JSPlayer::cache_live_audio_packet;

    } else {
        cache_audio_packet = &JSPlayer::cache_record_audio_packet;
    }

    LOGD("prepare_audio finish.");
    return JS_OK;
}

JS_RET JSPlayer::prepare_video() {
    LOGD("prepare_video...");
    JS_RET ret;
    m_video_stream = m_format_ctx->streams[m_video_stream_index];

    ret = (m_js_media_decoder->*m_js_media_decoder->m_create_decoder_by_av_stream)(m_video_stream);
    if (ret != JS_OK) {
        LOGE("prepare_video failed to m_create_decoder_by_av_stream");
        return JS_ERR;
    }

    m_video_cached_que = new JSQueue<AVPacket *>(QUEUE_TYPE_VIDEO,
                                                 m_video_stream->time_base,
                                                 m_min_cached_duration,
                                                 m_max_cached_duration,
                                                 m_is_live);

    m_video_cached_que->set_clear_callback(video_cached_que_clear_callback, this);

    m_video_decoded_que = new JSQueue<AVFrame *>(QUEUE_TYPE_VIDEO,
                                                 m_video_stream->time_base,
                                                 m_min_decoded_duration,
                                                 m_max_decoded_duration,
                                                 m_is_live);

    m_video_decoded_que->set_clear_callback(video_decoded_que_clear_callback, this);

    if (m_is_live) {
        if (m_egl_renderer->m_is_hold_surface) {
            cache_video_packet = &JSPlayer::cache_live_video_packet_hold_surface;
        } else {
            cache_video_packet = &JSPlayer::cache_live_video_packet_not_hold_surface;
        }

    } else {
        cache_video_packet = &JSPlayer::cache_record_video_packet;
    }

    LOGD("prepare_video finish.");
    return JS_OK;
}


void JSPlayer::start_read_frame() {
    LOGD("start_read_frame...");
    m_io_time_out = DEFAULT_READ_PKT_TIME_OUT_MICROSECONDS;
    pthread_create(&m_read_packet_tid, NULL, read_frame_thread, this);
    m_is_reading_frame = true;
}

void JSPlayer::start_decode_audio() {
    LOGD("start_decode_audio...");
    pthread_create(&m_decode_audio_tid, NULL, decode_audio_thread, this);
}

void JSPlayer::start_decode_video() {
    LOGD("start_decode_video...");
    pthread_create(&m_decode_video_tid, NULL, decode_video_thread, this);
}

void JSPlayer::stop_read_frame() {
    LOGD("stop_read_frame...");
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
        m_video_cached_que->clear(m_video_cached_que, false);
        m_video_decoded_que->clear(m_video_decoded_que, false);
    }
    if (m_has_audio_stream) {
        m_audio_cached_que->clear(m_audio_cached_que, false);
        m_audio_decoded_que->clear(m_audio_decoded_que, false);
    }
    LOGD("stop_read_frame finished.");
}


void JSPlayer::stop_play(bool is_to_pause) {
    LOGD("stop_play...");
    m_is_playing = false;

    if (m_has_video_stream) {
        m_video_cached_que->abort_get();
        m_video_decoded_que->abort_get();

        LOGD("stop_play stop decode video step1.");
        pthread_join(m_decode_video_tid, NULL);
        LOGD("stop_play stop decode video step2.");


        LOGD("stop_play stop render step1.");
        m_egl_renderer->stop_render();
        LOGD("stop_play stop render step2.");

    }

    if (m_has_audio_stream) {
        m_audio_cached_que->abort_get();
        m_audio_decoded_que->abort_get();

        LOGD("stop_play stop decode audio step1.");
        pthread_join(m_decode_audio_tid, NULL);
        LOGD("stop_play stop decode audio step2.");

        LOGD("stop_play stop play audio step1.");
        while (m_is_audio_data_consuming);
        LOGD("stop_play stop play audio step2");
    }

    if (!is_to_pause || m_is_live) {

        stop_read_frame();
    }

    LOGD("stop_play finished.");
}


void JSPlayer::stop_prepare() {
    LOGD("stop_prepare...");
    m_io_time_out = NO_TIME_OUT_MICROSECONDS;
    pthread_join(m_prepare_tid, NULL);
    LOGD("stop_prepare finished.");
}


void JSPlayer::free_res() {
    LOGD("free_res...");

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
    LOGD("free_res finished.");
}


void JSPlayer::set_cur_timing() {
    m_timing = av_gettime();
}

void JSPlayer::set_is_intercept_audio(bool is_intercept_audio) {

    m_is_intercept_audio = is_intercept_audio;
    LOGD("%s stop play audio step0. m_is_intercept_audio=%d", __func__, m_is_intercept_audio);
    if (!m_has_audio_stream || !m_is_audio_data_consuming) {
        return;
    }

    //replay audio.
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
            cache_video_packet = &JSPlayer::cache_live_video_packet_not_hold_surface;
        }
    }

}

JS_RET JSPlayer::cache_record_video_packet(AVPacket *src) {
    return m_video_cached_que->put(m_video_cached_que, src);
}

JS_RET JSPlayer::cache_live_video_packet_hold_surface(AVPacket *src) {
    //如果视频流中有自定义实时数据或命令 可以通过这个函数解析,仅限于直播流。
    if (m_is_parse_data_from_video_packet) {
        m_js_event_handler->call_on_parse_data_from_video_packet(src->data,
                                                                 src->size);
    }
    return m_video_cached_que->put(m_video_cached_que, src);
}

JS_RET JSPlayer::cache_live_video_packet_not_hold_surface(AVPacket *src) {
    //如果视频流中有自定义实时数据或命令 可以通过这个函数解析,仅限于直播流。
    if (m_is_parse_data_from_video_packet) {
        m_js_event_handler->call_on_parse_data_from_video_packet(src->data,
                                                                 src->size);
    }
    av_packet_unref(src);

    m_video_cached_que->clear(m_video_cached_que, true);
    LOGD("*drop* clear video packet.");
    m_video_decoded_que->clear(m_video_decoded_que, false);
    LOGD("*drop* clear video frame.");
//
//    if (JS_OK !=
//        m_video_cached_que->put(m_video_cached_que, flush_pkt)) {
//        LOGE("can't put video flush packet.");
//        m_js_event_handler->call_on_error(JS_ERR_EXTERNAL, 0, 0);
//    }

    return JS_OK;
}

JS_RET JSPlayer::cache_record_audio_packet(AVPacket *src) {
    return m_audio_cached_que->put(m_audio_cached_que, src);
}

JS_RET JSPlayer::cache_live_audio_packet(AVPacket *src) {
    return m_audio_cached_que->put(m_audio_cached_que, src);
}


void *prepare_thread(void *data) {

    pthread_setname_np(pthread_self(), __func__);

    JSPlayer *player = (JSPlayer *) data;

    player->set_cur_timing();
    int ret = player->find_stream_info();
    if (ret != JS_OK) {
        LOGE("find_stream_info error");
        goto fail;
    }

    player->init_decoder();

    if (player->m_has_audio_stream) {
        ret = player->prepare_audio();

        if (ret != JS_OK) {
            LOGE("prepare_audio error");
            goto fail;

        }
    } else {
        LOGW("prepare_audio didn't find a audio stream");
    }

    if (player->m_has_video_stream) {

        ret = player->prepare_video();

        if (ret != JS_OK) {
            LOGE("prepare_video error");
            goto fail;

        }
    } else {
        LOGW("prepare_video didn't find a video stream");
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

                        if (JS_OK !=
                            player->m_audio_cached_que->put(player->m_audio_cached_que, eof_pkt)) {
                            LOGE("can't put audio eof packet.");
                            js_event_handler->call_on_error(JS_ERR_READ_FRAME_FAILED, 0, 0);
                            break;
                        }
                    }

                    if (player->m_has_video_stream) {
                        if (JS_OK !=
                            player->m_video_cached_que->put(player->m_video_cached_que, eof_pkt)) {
                            LOGE("can't put video eof packet.");
                            js_event_handler->call_on_error(JS_ERR_READ_FRAME_FAILED, 0, 0);
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
                LOGE("can't cache audio packet.");
                av_packet_unref(packet);
                js_event_handler->call_on_error(JS_ERR_READ_FRAME_FAILED, 0, 0);
                break;
            }
        } else if (packet->stream_index == player->m_video_stream_index) {
            if (JS_OK != (player->*player->cache_video_packet)(packet)) {
                LOGE("can't cache video packet.");
                av_packet_unref(packet);
                js_event_handler->call_on_error(JS_ERR_READ_FRAME_FAILED, 0, 0);
                break;
            }
        } else {

            LOGD("*drop* drop a packet unsupported.");
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
        ret = JS_ERR_EXTERNAL;
        goto err;
    }

    while (player->m_is_playing) {

        avpkt = player->m_video_cached_que->get();
        if (avpkt == NULL) {
            break;
        }
        if (avpkt->data == flush_pkt->data) {
            (player->m_js_media_decoder->*player->m_js_media_decoder->video_decoder_flush)();
            av_packet_free(&avpkt);
            continue;
        }
        if (avpkt->data == NULL && avpkt->size == 0) {
            LOGD("avpkt->data == NULL && avpkt->size == 0  111");
        }

        while (player->m_is_playing) {

            ret = (player->m_js_media_decoder->*player->
                    m_js_media_decoder->
                    decode_video_packet)(avpkt, frame);

            if (ret >= 0) {

                if (JS_OK != player->m_video_decoded_que->put(player->m_video_decoded_que, frame)) {
                    ret = JS_ERR_EXTERNAL;
                    goto err;
                }

                if (ret == JS_OK) {
                    av_packet_free(&avpkt);
                    break;
                }
            } else if (ret == JS_ERR_EOF) {
                LOGD("avpkt->data == NULL && avpkt->size == 0222");
                av_packet_free(&avpkt);
                // fixme invoke once.
                player->m_js_event_handler->call_on_completed();
                pthread_exit(0);
            } else if (ret == JS_ERR_TRY_TO_USE_SW_DECODER) {

                //fixme data can't serial.
                continue;
            } else if (ret == JS_ERR_NEED_SEND_THIS_PACKET_AGAIN) {
                LOGD("%s JS_ERR_NEED_SEND_THIS_PACKET_AGAIN", __func__);
                usleep(8 * 1000);
                //fixme sleep for lower cpu use rate.
                continue;
            } else if (ret == JS_ERR_NEED_SEND_NEW_PACKET_AGAIN) {
                av_packet_free(&avpkt);
                break;
            } else {
                goto err;
            }
        }
    }

    pthread_exit(0);

    err:
    if (avpkt) {
        av_packet_free(&avpkt);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    player->m_js_event_handler->call_on_error(ret, 0, 0);
    pthread_exit(0);
}

void *decode_audio_thread(void *data) {

    pthread_setname_np(pthread_self(), __func__);
    JSPlayer *player = (JSPlayer *) data;

    JS_RET ret;
    AVPacket *avpkt = NULL;
    AVFrame *frame = av_frame_alloc();

    if (frame == NULL) {
        LOGE("%s unable to allocate an AVFrame!", __func__);
        ret = JS_ERR_EXTERNAL;
        goto err;
    }

    while (player->m_is_playing) {

        avpkt = player->m_audio_cached_que->get();
        if (avpkt == NULL) {
            break;
        }
        if (avpkt->data == flush_pkt->data) {
            (player->m_js_media_decoder->*player->m_js_media_decoder->audio_decoder_flush)();
            av_packet_free(&avpkt);
            continue;
        }

        while (player->m_is_playing) {

            ret = (player->m_js_media_decoder->*player->
                    m_js_media_decoder->
                    decode_audio_packet)(avpkt, frame);

            if (ret >= 0) {

//                // 设置通道数或channel_layout
//                if (frame->channels > 0 && frame->channel_layout == 0) {
//                    frame->channel_layout = (uint64_t) av_get_default_channel_layout(
//                            frame->channels);
//                } else if (frame->channels == 0 && frame->channel_layout > 0) {
//                    frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);
//                }

                if (JS_OK != player->m_audio_decoded_que->put(player->m_audio_decoded_que, frame)) {
                    ret = JS_ERR_EXTERNAL;
                    goto err;
                }

                if (ret == JS_OK) {
                    av_packet_free(&avpkt);
                    break;
                }

            } else if (ret == JS_ERR_EOF) {
                av_packet_free(&avpkt);

                // fixme invoke once.
                player->m_js_event_handler->call_on_completed();
                pthread_exit(0);
            } else if (ret == JS_ERR_TRY_TO_USE_SW_DECODER) {
                //fixme data can't serial.

                continue;
            } else if (ret == JS_ERR_NEED_SEND_THIS_PACKET_AGAIN) {
                LOGD("%s JS_ERR_NEED_SEND_THIS_PACKET_AGAIN", __func__);
                usleep(10 * 1000);
                //fixme sleep for lower cpu use rate.
                continue;
            } else if (ret == JS_ERR_NEED_SEND_NEW_PACKET_AGAIN) {
                av_packet_free(&avpkt);
                break;
            } else {
                goto err;
            }
        }
    }

    pthread_exit(0);

    err:
    if (avpkt) {
        av_packet_free(&avpkt);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    player->m_js_event_handler->call_on_error(ret, 0, 0);
    pthread_exit(0);
}


/**
 * return 1 timeout.
 */
int io_interrupt_cb(void *data) {

    JSPlayer *player = (JSPlayer *) data;

    if (av_gettime() - player->m_timing > player->m_io_time_out) {
        LOGD("io timeout.");
        return 1;
    } else {
        return 0;
    }
}


void opensles_buffer_queue_cb(SLAndroidSimpleBufferQueueItf caller, void *data) {
    JSPlayer *player = (JSPlayer *) data;
    unsigned int audio_size;
    AVFrame *frame;
    JSEventHandler *js_event_handler = player->m_js_event_handler;
    JSAudioPlayer *audio_player = player->m_audio_player;

    if (!player->m_is_playing) {
        goto end;
    }
    frame = player->m_audio_decoded_que->get();
    if (frame == NULL) {
        goto end;
    }

    if (frame->format == DEFAULT_AV_SAMPLE_FMT) {
        audio_player->enqueue(frame->data[0], (unsigned int) frame->linesize[0]);
        av_frame_free(&frame);
    } else {
        audio_size = convert_simple_format_to_S16(audio_player->m_buffer, frame);
        av_frame_free(&frame);
        if (audio_size == 0) {
            js_event_handler->call_on_error(JS_ERR_PLAY_AUDIO_FAILED, 0, 0);
            LOGD("audio_size=0");
            goto end;
        } else {
            audio_player->enqueue(audio_player->m_buffer, audio_size);
        }
    }

    return;

    end:
    player->m_is_audio_data_consuming = false;
}


void egl_buffer_queue_cb(void *data) {

    JSPlayer *player = (JSPlayer *) data;
    if (!player->m_is_playing) {
        return;
    }

    AVFrame *frame = player->m_video_decoded_que->get();
    if (frame == NULL) {
        return;
    }

//    usleep(32000);//todo sync video audio.
    player->m_egl_renderer->render(frame);
    av_frame_free(&frame);
}


void audio_cached_que_clear_callback(void *data) {
    JSPlayer *player = (JSPlayer *) data;

}

void video_cached_que_clear_callback(void *data) {
    JSPlayer *player = (JSPlayer *) data;

}

void audio_decoded_que_clear_callback(void *data) {
    JSPlayer *player = (JSPlayer *) data;
}

void video_decoded_que_clear_callback(void *data) {
    JSPlayer *player = (JSPlayer *) data;
}

void consume_audio_data_by_interceptor(JSPlayer *player) {

    unsigned int audio_size;
    AVFrame *frame;
    JSEventHandler *js_event_handler = player->m_js_event_handler;
    JSAudioPlayer *audio_player = player->m_audio_player;

    if (!player->m_is_playing) {
        goto end;
    }

    frame = player->m_audio_decoded_que->get();
    if (frame == NULL) {
        goto end;
    }


    if (frame->format == DEFAULT_AV_SAMPLE_FMT) {//todo func pointer
        js_event_handler->call_on_intercepted_pcm_data(frame->data[0],
                                                       frame->linesize[0],
                                                       frame->channels);
        av_frame_free(&frame);
        consume_audio_data_by_interceptor(player);
    } else {
        audio_size = convert_simple_format_to_S16(audio_player->m_buffer, frame);

        if (audio_size == 0) {
            js_event_handler->call_on_error(JS_ERR_INTERCEPT_AUDIO_FAILED, 0, 0);
            LOGE("audio_size=0");
            av_frame_free(&frame);
            goto end;
        } else {
            js_event_handler->call_on_intercepted_pcm_data(
                    audio_player->m_buffer, audio_size,
                    frame->channels);
            av_frame_free(&frame);
            consume_audio_data_by_interceptor(player);
        }
    }

    return;

    end:
    player->m_is_audio_data_consuming = false;
}

void consume_audio_data_by_audiotrack(JSPlayer *player) {

}


void consume_audio_data_by_opensles(JSPlayer *player) {
    opensles_buffer_queue_cb(player->m_audio_player->m_bq_player_buffer_queue, player);
}


bool is_live(AVFormatContext *s) {
    if (!strcmp(s->iformat->name, "rtp")
        || !strcmp(s->iformat->name, "rtsp")
        || !strcmp(s->iformat->name, "sdp")
        || !strcmp(s->iformat->name, "flv")) {
        LOGD("s->iformat->name=%s", s->iformat->name);
        return true;
    }

    if (s->pb && (!strncmp(s->filename, "rtp:", 4)
                  || !strncmp(s->filename, "udp:", 4))) {
        LOGD("s->filename=%s", s->filename);
        return true;
    }
    return false;
}

