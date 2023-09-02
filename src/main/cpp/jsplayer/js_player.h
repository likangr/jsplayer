#ifndef JS_PLAYER_H
#define JS_PLAYER_H

#include "decoder/js_media_decoder.h"
#include "audio/js_audio_player.h"
#include "render/js_egl_renderer.h"
#include "converter/js_media_converter.h"
#include "util/js_queue.h"
#include "js_constant.h"

class JSPlayer;

typedef JS_RET (JSPlayer::*CACHE_PACKET_FUNC)(AVPacket *avpkt);

class JSPlayer {

public:

/***
 * ********************************
 * funcs
 * ********************************
 */
    JSPlayer(jobject java_js_player);

    ~JSPlayer();

    /*basic controller*/

    void prepare();

    void play(bool is_to_resume);

    void pause();

    void resume();

    void stop();

    void reset();

    void destroy();

    /*prepare*/

    JS_RET find_stream_info();

    void init_decoder();

    JS_RET prepare_audio();

    JS_RET prepare_video();


    /*play*/

    void start_read_frame();

    void start_decode_audio();

    void start_decode_video();

    /*reset*/

    void stop_read_frame();

    void pause_play();

    void stop_play();

    void stop_prepare();

    void free_res();

    /*others*/

    void set_cur_timing();

    void set_is_intercept_audio(bool is_intercept_audio);

    void on_surface_hold_state_changed();

    CACHE_PACKET_FUNC cache_audio_packet;

    CACHE_PACKET_FUNC cache_video_packet;

    /*cache video packet way*/

    JS_RET cache_record_video_packet(AVPacket *src);

    JS_RET cache_live_video_packet_hold_surface(AVPacket *src);

    JS_RET cache_live_video_packet_not_hold_surface(AVPacket *src);

    /*cache audio packet way*/

    JS_RET cache_record_audio_packet(AVPacket *src);

    JS_RET cache_live_audio_packet(AVPacket *src);


    /*audio control*/
    void set_mute(bool mute);

    bool get_mute();

    void set_channel_mute(int channel, bool mute);

/***
 * ********************************
 * fields
 * ********************************
 */

    /*basic*/
    bool m_is_eof = false;
    bool m_is_cleared_video_frame = false;

    const char *m_url = NULL;
    bool m_is_live = false;
    volatile int m_cur_play_status = 0;
    AVDictionary *m_options = NULL;

    jobject m_java_js_player = NULL;
    JSAudioPlayer *m_audio_player = NULL;
    JSEglRenderer *m_egl_renderer = NULL;
    JSMediaDecoder *m_js_media_decoder = NULL;
    JSEventHandler *m_js_event_handler = NULL;
    JSMediaCoverter *m_js_media_converter = NULL;


    pthread_t m_prepare_tid,
            m_read_packet_tid,
            m_decode_video_tid,
            m_decode_audio_tid,
            m_play_audio_tid;


    AVFormatContext *m_format_ctx = NULL;
    int m_video_stream_index = -1;
    int m_audio_stream_index = -1;
    AVStream *m_audio_stream = NULL;
    AVStream *m_video_stream = NULL;
    bool m_has_audio_stream = false;
    bool m_has_video_stream = false;
    JSPacketQueue *m_audio_cached_que = NULL, *m_video_cached_que = NULL;
    JSFrameQueue *m_audio_decoded_que = NULL, *m_video_decoded_que = NULL;

    int64_t m_timing = 0;
    int m_io_time_out = 0;
    volatile bool m_is_intercept_audio = false;

    volatile bool m_is_parse_data_from_video_packet = false;

    int64_t m_min_cached_duration = -1;
    //live buffer drop frame threshold.
    int64_t m_max_cached_duration = -1;

    int64_t m_min_decoded_duration = -1;
    int64_t m_max_decoded_duration = -1;


    volatile bool m_is_playing = false;
    volatile bool m_is_buffering = false;
    volatile bool m_is_reading_frame = false;
    volatile bool m_is_audio_data_consuming = false;

    bool m_mute = false;
    bool m_left_channel_mute = false;
    bool m_right_channel_mute = false;

    /*func pointers*/

    void (*native_intercepted_pcm_data_callback)(int64_t native_js_player,
                                                 short *pcm_data,
                                                 int pcm_data_size,
                                                 int channel_num);

    void (*native_parse_data_from_video_packet_callback)(int64_t native_js_player,
                                                         uint8_t *video_packet_data,
                                                         int video_packet_data_size);

    /*sync audio and video control field*/
//    int m_av_sync_type = AV_SYNC_AUDIO_MASTER;
    int64_t m_frame_rate_duration = -1;
    double m_frame_rate = -1.0;
    int64_t m_cur_video_pts = -1;
    volatile int64_t m_cur_audio_pts = -1;
    int64_t m_last_video_pts = -1;
    int64_t m_first_video_pts = -1;

    volatile bool m_is_video_buffering = false;
    volatile bool m_is_audio_buffering = false;

    pthread_mutex_t m_mutex0, *m_mutex = &m_mutex0;
    pthread_cond_t m_cond0, *m_cond = &m_cond0;

};

/***
 * ********************************
 * non-member func
 * ********************************
 */

/* threads */

static void *prepare_thread(void *data);

static void *play_audio_thread(void *data);

static void *read_frame_thread(void *data);

static void *decode_video_thread(void *data);

static void *decode_audio_thread(void *data);


/*callback*/

static int io_interrupt_cb(void *data);

static void opensles_buffer_queue_cb(SLAndroidSimpleBufferQueueItf caller,
                                     void *data);

static void egl_buffer_queue_cb(void *data);

static void audio_cached_que_clear_callback(void *data);

static void video_cached_que_clear_callback(void *data);

static void audio_decoded_que_clear_callback(void *data);

static void video_decoded_que_clear_callback(void *data);

static void audio_cached_que_buffering_callback(void *data, bool is_buffering);

static void video_cached_que_buffering_callback(void *data, bool is_buffering);

static void audio_decoded_que_buffering_callback(void *data, bool is_buffering);

static void video_decoded_que_buffering_callback(void *data, bool is_buffering);

/*consume audio data way*/

static void consume_audio_data_by_interceptor(JSPlayer *player);

static void consume_audio_data_by_audiotrack(JSPlayer *player);

static void consume_audio_data_by_opensles(JSPlayer *player);


/*others*/

static inline bool is_live(AVFormatContext *s);


#endif //JS_PLAYER_H