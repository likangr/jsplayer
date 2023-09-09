#ifndef JS_CONSTANT_H
#define JS_CONSTANT_H

/*error code*/
typedef int JS_RET;
#define JS_OK                                               0         /* no error */
#define JS_OK_NOT_DECODE_PACKET                             JS_OK+1

#define JS_ERR                                              -1         /* generic error */
#define JS_ERR_HW_DECODER_UNAVAILABLE                       JS_ERR-1
#define JS_ERR_SW_DECODER_UNAVAILABLE                       JS_ERR-2
#define JS_ERR_EOF                                          JS_ERR-4
#define JS_ERR_EXTERNAL                                     JS_ERR-5
#define JS_ERR_PREPARE_FAILED                               JS_ERR-6
#define JS_ERR_PLAY_AUDIO_FAILED                            JS_ERR-7
#define JS_ERR_READ_FRAME_FAILED                            JS_ERR-8
#define JS_ERR_EGL_RENDERER_SETUP_RENDERER_FAILED           JS_ERR-9
#define JS_ERR_INTERCEPT_AUDIO_FAILED                       JS_ERR-10
#define JS_ERR_TRY_AGAIN                                    JS_ERR-11
#define JS_ERR_TRY_TO_USE_SW_DECODER                        JS_ERR-12
#define JS_ERR_NEED_SEND_NEW_PACKET_AGAIN                   JS_ERR-13
#define JS_ERR_NEED_SEND_THIS_PACKET_AGAIN                  JS_ERR-14
#define JS_ERR_EGL_RENDERER_RENDER_FAILED                   JS_ERR-15
#define JS_ERR_CONVERTER_AUDIO_CONVERT_FAILED               JS_ERR-16
#define JS_ERR_CONVERTER_INIT_AUDIO_FAILED                  JS_ERR-17
#define JS_ERR_CACHE_PACKET_FAILED                          JS_ERR-18
#define JS_ERR_CACHE_FRAME_FAILED                           JS_ERR-19

/*io timeout*/
#define DEFAULT_PREPARE_TIME_OUT_MICROSECONDS               10000000*2//20s
#define DEFAULT_READ_PKT_TIME_OUT_MICROSECONDS              10000000*6//1min
#define NO_TIME_OUT_MICROSECONDS                            0

/*buffer duration*/
#define DEFAULT_MIN_CACHED_DURATION_LIVE                    0
#define DEFAULT_MAX_CACHED_DURATION_LIVE                    500000//500ms
#define DEFAULT_MIN_DECODED_DURATION_LIVE                   0
#define DEFAULT_MAX_DECODED_DURATION_LIVE                   500000//500ms

#define DEFAULT_MIN_CACHED_DURATION_RECORD                  1000000*10//10s
#define DEFAULT_MAX_CACHED_DURATION_RECORD                  1000000*60*3/2//1.5min
//#define DEFAULT_MIN_DECODED_DURATION_RECORD               1000000*5//5s
//#define DEFAULT_MAX_DECODED_DURATION_RECORD               1000000*10//10s
#define DEFAULT_MIN_DECODED_DURATION_RECORD                 1000000//1s
#define DEFAULT_MAX_DECODED_DURATION_RECORD                 5000000//5s

//todo map value from Constant.java.
/*play status*/
#define  PLAY_STATUS_CREATED                                1000
#define  PLAY_STATUS_PREPARING                              1001
#define  PLAY_STATUS_PREPARED                               1002
#define  PLAY_STATUS_PLAYING                                1003
#define  PLAY_STATUS_PAUSED                                 1004
#define  PLAY_STATUS_STOPPED                                1005

/*options*/
#define JS_OPTION_DECODER_TYPE                              "key_decoder_type"
#define JS_OPTION_DECODER_TYPE_HW                           "decoder_type_hw"
#define JS_OPTION_DECODER_TYPE_SW                           "decoder_type_sw"
#define JS_OPTION_DECODER_TYPE_AUTO                         "decoder_type_auto"
#define LK_OPTION_LIVE_MEDIA_DELAY_MODE                     "live_media_delay_mode"
#define LK_OPTION_LIVE_MEDIA_DELAY_MODE_REAL_TIME           "live_media_delay_mode_real_time"
#define LK_OPTION_LIVE_MEDIA_DELAY_MODE_FLUENCY             "live_media_delay_mode_fluency"

#define LK_USER_TIME_BASE                                       1000//1ms
#define LK_NO_TIME_VALUE                                        -1

#endif //JS_CONSTANT_H
