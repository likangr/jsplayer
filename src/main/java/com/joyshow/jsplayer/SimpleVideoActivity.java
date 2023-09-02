package com.joyshow.jsplayer;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;

import com.joyshow.jsplayer.player.JSPlayer;
import com.joyshow.jsplayer.utils.Logger;

import java.io.File;

/**
 * @author likangren
 * simple video.
 */
public class SimpleVideoActivity extends Activity implements View.OnClickListener, JSPlayer.OnErrorListener, JSPlayer.OnPreparedListener, JSPlayer.OnInfoListener, JSPlayer.OnCompletedListener, JSPlayer.OnBufferingListener {
    private final String TAG = "SimpleVideoActivity";

    private static final String SDCARD_PATH = Environment.getExternalStorageDirectory().getAbsolutePath();

    static {
        JSPlayer.setLoggable(true);
//        JSPlayer.setIsWriteLogToFile(false);
        JSPlayer.setLogFileSavePath(SDCARD_PATH + File.separator + "simple_video_activity_log.txt");
    }

    private JSPlayer mPlayer;
    private Button mBtnMute;
    private Button mBtnPause;
    private Button mBtnStop;
    private ProgressBar mProgressBar;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_video);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            requestPermissions(new String[]{Manifest.permission.MANAGE_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_MEDIA_AUDIO, Manifest.permission.READ_MEDIA_IMAGES, Manifest.permission.READ_MEDIA_VIDEO}, 1000);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            requestPermissions(new String[]{Manifest.permission.MANAGE_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1000);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1000);
        }

        mPlayer = (JSPlayer) findViewById(R.id.jsplayer);
//        mPlayer.setUrl("http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8");
//        mPlayer.setUrl("http://221.120.177.59/hls/i3d7ragr.m3u8");
//        mPlayer.setUrl("http://58.221.254.33:1935/live/zhongwen.sdp/playlist.m3u8");
//        mPlayer.setUrl("http://live.3gv.ifeng.com/live/hongkong.m3u8");
//        mPlayer.setUrl("http://202.102.79.114:554/live/tvb8.stream/playlist.m3u8");
//        mPlayer.setUrl("http://wzfree.10043.doftp.com/tvtest/182tv.php/live/id/hlc.m3u8");
//        mPlayer.setUrl("http://v.hrtv8.com/vip.hrtv8.com/stream/2506.m3u8");
//        mPlayer.setUrl("http://wzfree.10043.doftp.com/tvtest/182tv.php/live/id/suntv.m3u8");

//        mPlayer.setUrl("rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov");
//        mPlayer.setUrl("rtsp://116.199.127.68/huayu");
//        mPlayer.setUrl("rtsp://116.199.127.68/bengang");
//        mPlayer.setUrl("rtsp://rtsp.vdowowza.tvb.com/tvblive/mobileinews200.stream");
//        mPlayer.setUrl("rtsp://live.3gv.ifeng.com/live/71");

//        mPlayer.setUrl("rtmp://live.hkstv.hk.lxdns.com/live/hks");
//        mPlayer.setUrl("rtmp://pull-g.kktv8.com/livekktv/100987038");
//        mPlayer.setUrl("rtmp://v1.one-tv.com/live/mpegts.stream");

//        mPlayer.setUrl("http://221.228.226.23/11/t/j/v/b/tjvbwspwhqdmgouolposcsfafpedmb/sh.yinyuetai.com/691201536EE4912BF7E4F1E2C67B8119.mp4");
//        mPlayer.setUrl("http://hc.yinyuetai.com/uploads/videos/common/CE3C0166CE5EB6D5FA9FDB182D51DFA9.mp4");
//        mPlayer.setUrl("http://v.ysbang.cn//data/video/2015/rkb/2015rkb01.mp4");
//        mPlayer.setUrl(SDCARD_PATH + File.separator + "sintel.mp4");
//        mPlayer.setUrl(SDCARD_PATH + File.separator + "寻梦环游记.mp4");
//        mPlayer.setUrl(SDCARD_PATH + File.separator + "test4k.mp4");
        mPlayer.setUrl(SDCARD_PATH + File.separator + "1.mp4");


//        mPlayer.setOption(Constant.JS_OPTION_DECODER_TYPE, Constant.JS_OPTION_DECODER_TYPE_AUTO);
//        mPlayer.setOption(Constant.JS_OPTION_DECODER_TYPE, Constant.JS_OPTION_DECODER_TYPE_SW);
        mPlayer.setOption(Constant.JS_OPTION_DECODER_TYPE, Constant.JS_OPTION_DECODER_TYPE_HW);
        mPlayer.prepare();

        mPlayer.setOnPreparedListener(this);
        mPlayer.setOnErrorListener(this);
        mPlayer.setOnInfoListener(this);
        mPlayer.setOnCompletedListener(this);
        mPlayer.setOnBufferingListener(this);

        mBtnMute = (Button) findViewById(R.id.btn_mute);
        mBtnPause = (Button) findViewById(R.id.btn_pause);
        mBtnStop = (Button) findViewById(R.id.btn_stop);
        mProgressBar = (ProgressBar) findViewById(R.id.progress_bar);
        mBtnMute.setOnClickListener(this);
        mBtnPause.setOnClickListener(this);
        mBtnStop.setOnClickListener(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Logger.d("likang", "onDestroy");
        mPlayer.destroy();
    }

    @SuppressLint("SetTextI18n")
    @Override
    public void onClick(View v) {
        if (v == mBtnMute) {
            if (mBtnMute.getText().equals("mute")) {
                mBtnMute.setText("not mute");
                mPlayer.setMute(true);
            } else {
                mBtnMute.setText("mute");
                mPlayer.setMute(false);
            }
        } else if (v == mBtnPause) {
            if (mBtnPause.getText().equals("pause")) {
                mBtnPause.setText("resume");
                mPlayer.pause();
            } else {
                mBtnPause.setText("pause");
                mPlayer.resume();
            }
        } else if (v == mBtnStop) {
            if (mBtnStop.getText().equals("stop")) {
                mBtnStop.setText("play");
                mPlayer.stop();
            } else {
                mBtnStop.setText("stop");
                mPlayer.prepare();
            }
        }
    }

    @Override
    public void onError(JSPlayer player, int what, int arg1, int arg2) {
        Logger.d(TAG, "onError: what=" + what);
    }

    @Override
    public void onPrepared(JSPlayer player) {
        if (!player.isWantToPause())
            player.play();
    }

    @Override
    public void onInfo(JSPlayer player, int what, int arg1, int arg2) {
        Logger.d(TAG, "onInfo: what=" + what);
    }


    @Override
    protected void onResume() {
        super.onResume();
//        mPlayer.resume();
    }

    @Override
    protected void onPause() {
        super.onPause();
//        mPlayer.pause();
    }

    @Override
    public void onCompleted(JSPlayer player) {
        Logger.d(TAG, "onCompleted");
    }

    @Override
    public void onBuffering(JSPlayer player, boolean isBuffering) {
        Logger.d(TAG, "onBuffering isBuffering=" + isBuffering);
        mProgressBar.setVisibility(isBuffering ? View.VISIBLE : View.GONE);
    }
}
