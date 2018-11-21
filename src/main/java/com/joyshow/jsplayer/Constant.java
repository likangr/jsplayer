package com.joyshow.jsplayer;

public class Constant {

    public static final int PLAY_STATUS_CREATED = 1000;
    public static final int PLAY_STATUS_PREPARING = 1001;
    public static final int PLAY_STATUS_PREPARED = 1002;
    public static final int PLAY_STATUS_PLAYING = 1003;
    public static final int PLAY_STATUS_PAUSED = 1004;
    public static final int PLAY_STATUS_STOPPED = 1005;
    public static final int PLAY_STATUS_DESTROYED = 1006;


    public static final String JS_OPTION_DECODER_TYPE = "key_decoder_type";
    public static final String JS_OPTION_DECODER_TYPE_HW = "decoder_type_hw";
    public static final String JS_OPTION_DECODER_TYPE_SW = "decoder_type_sw";
    public static final String JS_OPTION_DECODER_TYPE_AUTO = "decoder_type_auto";


    public static final int JS_PLAYER_PREPARE_FAILED = -10001;
    public static final int JS_PLAYER_PLAY_AUDIO_FAILED = -10002;
    public static final int JS_PLAYER_READ_FRAME_FAILED = -10003;
    public static final int JS_EGL_RENDERER_SETUP_RENDERER_FAILED = -10004;

}
