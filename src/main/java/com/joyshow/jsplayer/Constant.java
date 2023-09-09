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
    public static final String LK_OPTION_LIVE_MEDIA_DELAY_MODE = "live_media_delay_mode";
    public static final String LK_OPTION_LIVE_MEDIA_DELAY_MODE_REAL_TIME
            = "live_media_delay_mode_real_time";
    public static final String LK_OPTION_LIVE_MEDIA_DELAY_MODE_FLUENCY
            = "live_media_delay_mode_fluency";
}