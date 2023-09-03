package com.joyshow.jsplayer;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;

import com.joyshow.jsplayer.player.JSPlayer;
import com.joyshow.jsplayer.utils.Logger;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.TimeZone;

/**
 * @author likangren
 * simple video.
 */
public class SimpleVideoActivity extends Activity implements View.OnClickListener, JSPlayer.OnErrorListener, JSPlayer.OnPreparedListener, JSPlayer.OnInfoListener, JSPlayer.OnCompletedListener, JSPlayer.OnBufferingListener, SeekBar.OnSeekBarChangeListener {
    private final String TAG = "SimpleVideoActivity";

    private static final String SDCARD_PATH = Environment.getExternalStorageDirectory().getAbsolutePath();
    @SuppressLint("SimpleDateFormat")
    private static final SimpleDateFormat sDF = new SimpleDateFormat("HH:mm:ss");

    static {
        JSPlayer.setLoggable(true);
//        JSPlayer.setIsWriteLogToFile(false);
        JSPlayer.setLogFileSavePath(SDCARD_PATH + File.separator + "simple_video_activity_log.txt");
        sDF.setTimeZone(TimeZone.getTimeZone("GMT"));
    }

    private JSPlayer mPlayer;
    private Button mBtnMute;
    private Button mBtnPause;
    private Button mBtnStop;
    private ProgressBar mProgressBar;
    private SeekBar mSBSeek;
    private TextView mCurrentPosition;
    private TextView mDuration;

    private static final Handler sHandler = new Handler(Looper.getMainLooper());

    @SuppressLint("DefaultLocale")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_video);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            requestPermissions(new String[]{Manifest.permission.MANAGE_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE}, 1000);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE}, 1000);
        }

        mPlayer = findViewById(R.id.jsplayer);
//        mPlayer.setUrl(SDCARD_PATH + File.separator + "sintel.mp4");
//        mPlayer.setUrl(SDCARD_PATH + File.separator + "寻梦环游记.mp4");
//        mPlayer.setUrl(SDCARD_PATH + File.separator + "test4k.mp4");
//        mPlayer.setUrl(SDCARD_PATH + File.separator + "3.mp4");
        mPlayer.setUrl(SDCARD_PATH + File.separator + "2.mp4");
//        mPlayer.setUrl("http://hw-m-l.cztv.com/channels/lantian/channel008/1080p.m3u8");


        mPlayer.setOption(Constant.JS_OPTION_DECODER_TYPE, Constant.JS_OPTION_DECODER_TYPE_AUTO);
//        mPlayer.setOption(Constant.JS_OPTION_DECODER_TYPE, Constant.JS_OPTION_DECODER_TYPE_SW);
//        mPlayer.setOption(Constant.JS_OPTION_DECODER_TYPE, Constant.JS_OPTION_DECODER_TYPE_HW);
        mPlayer.prepare();

        mPlayer.setOnPreparedListener(this);
        mPlayer.setOnErrorListener(this);
        mPlayer.setOnInfoListener(this);
        mPlayer.setOnCompletedListener(this);
        mPlayer.setOnBufferingListener(this);

        mBtnMute = findViewById(R.id.btn_mute);
        mBtnPause = findViewById(R.id.btn_pause);
        mBtnStop = findViewById(R.id.btn_stop);
        mBtnMute.setOnClickListener(this);
        mBtnPause.setOnClickListener(this);
        mBtnStop.setOnClickListener(this);

        mProgressBar = findViewById(R.id.progress_bar);
        mSBSeek = findViewById(R.id.sb_seek);
        mSBSeek.setOnSeekBarChangeListener(this);

        mCurrentPosition = findViewById(R.id.tv_current_position);
        mDuration = findViewById(R.id.tv_duration);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
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
        long duration = player.getDuration();
        mSBSeek.setMax((int) duration);
        mDuration.setText(sDF.format(duration));
        if (!player.isWantToPause()) {
            player.play();
            startUpdateSeekbar();
        }
    }

    @Override
    public void onInfo(JSPlayer player, int what, int arg1, int arg2) {
        Logger.d(TAG, "onInfo: what=" + what);
    }


    @Override
    protected void onResume() {
        super.onResume();
        mPlayer.resume();
        startUpdateSeekbar();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mPlayer.pause();
        stopUpdateSeekbar();
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

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        mPlayer.seekTo(seekBar.getProgress());
    }

    private void startUpdateSeekbar() {
        stopUpdateSeekbar();
        sHandler.post(new Runnable() {
            @Override
            public void run() {
                long currentPosition = mPlayer.getCurrentPosition();
                mSBSeek.setProgress((int) currentPosition);
                mCurrentPosition.setText(sDF.format(currentPosition));
                sHandler.postDelayed(this, 1000);
            }
        });
    }


    private void stopUpdateSeekbar() {
        sHandler.removeCallbacksAndMessages(null);
    }
}
