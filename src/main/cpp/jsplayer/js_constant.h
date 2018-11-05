#ifndef JS_CONSTANT_H
#define JS_CONSTANT_H


/*error code*/
typedef int JS_RET;
#define JS_OK                                                 0         /* no error */
#define JS_ERR                                               -1         /* generic error */
#define JS_ERR_MEDIACODEC_UNAVAILABLE                        JS_ERR-1

#define JS_PLAYER_PREPARE_FAILED                                -10001
#define JS_PLAYER_PLAY_AUDIO_FAILED                             -10002
#define JS_PLAYER_READ_FRAME_FAILED                             -10003
#define JS_EGL_RENDERER_SETUP_RENDERER_FAILED                   -10004
#define JS_PLAYER_INTERCEPT_AUDIO_FAILED                        -10005

//todo map value from Constant.java.
/*play status*/
#define  PLAY_STATUS_CREATED                                    1000
#define  PLAY_STATUS_PREPARING                                  1001
#define  PLAY_STATUS_PREPARED                                   1002
#define  PLAY_STATUS_PLAYING                                    1003
#define  PLAY_STATUS_PAUSED                                     1004
#define  PLAY_STATUS_STOPPED                                    1005

/*options*/
#define JS_OPTION_DECODER_TYPE                     "key_decoder_type"
#define JS_OPTION_DECODER_TYPE_HW                  "decoder_type_hw"
#define JS_OPTION_DECODER_TYPE_SW                  "decoder_type_sw"

#endif //JS_CONSTANT_H
