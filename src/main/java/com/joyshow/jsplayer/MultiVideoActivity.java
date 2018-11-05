package com.joyshow.jsplayer;

import android.app.Activity;
import android.os.Bundle;

import com.joyshow.jsplayer.player.JSPlayer;

/**
 * @author likangren
 * multi video.
 */
public class MultiVideoActivity extends Activity {

    private JSPlayer player1;
    private JSPlayer player2;
    private JSPlayer player3;
    private JSPlayer player4;

    {
        JSPlayer.setLoggable(true);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_multi_video);

        player1 = (JSPlayer) findViewById(R.id.jsplayer1);
        player2 = (JSPlayer) findViewById(R.id.jsplayer2);
        player3 = (JSPlayer) findViewById(R.id.jsplayer3);
        player4 = (JSPlayer) findViewById(R.id.jsplayer4);


        player1.setUrl("rtmp://live.hkstv.hk.lxdns.com/live/hks");
        player2.setUrl("rtmp://live.hkstv.hk.lxdns.com/live/hks");
        player3.setUrl("rtmp://live.hkstv.hk.lxdns.com/live/hks");
        player4.setUrl("rtmp://live.hkstv.hk.lxdns.com/live/hks");

        player1.prepare();
        player2.prepare();
        player3.prepare();
        player4.prepare();
//        player1.prepare("rtmp://live.hkstv.hk.lxdns.com/live/hks");
//        player2.prepare("rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov");
//        player3.prepare("rtmp://v1.one-tv.com/live/mpegts.stream");
//        player4.prepare("rtmp://live.hkstv.hk.lxdns.com/live/hks");
//        player1.prepare("/sdcard/test.mp3");
//        player2.prepare("/sdcard/test.wav");
        player1.setOnPreparedListener(new JSPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(JSPlayer player) {
                player1.play();
            }
        });
        player2.setOnPreparedListener(new JSPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(JSPlayer player) {
                player2.play();
            }
        });
        player3.setOnPreparedListener(new JSPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(JSPlayer player) {
                player3.play();
            }
        });
        player4.setOnPreparedListener(new JSPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(JSPlayer player) {
                player4.play();
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player1.destroy();
        player2.destroy();
        player3.destroy();
        player4.destroy();
    }
}
