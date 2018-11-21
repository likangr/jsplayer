package com.joyshow.jsplayer.player;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.widget.FrameLayout;

import com.joyshow.jsplayer.Constant;
import com.joyshow.jsplayer.utils.Logger;
import com.joyshow.jsplayer.utils.StackTraceInfo;


/**
 * @author likangren
 */
public class JSPlayer extends FrameLayout {

    private static final String TAG = JSPlayer.class.getSimpleName();

    private JSSurfaceView mJSRenderView;
    private long mNativePlayer;
    private Handler mMainThreadHandler = new Handler();
    private boolean mIsWantToPause = false;

    private OnPreparedListener mOnPreparedListener;
    private OnErrorListener mOnErrorListener;
    private OnInfoListener mOnInfoListener;
    private OnCompletedListener mOnCompletedListener;
    private OnInterceptedPcmDataCallback mOnInterceptedPcmDataCallback;

    public enum ChannelIndex {
        CHANNEL_LEFT, CHANNEL_RIGHT
    }


    public JSPlayer(Context context) {
        super(context);
        init();
    }


    public JSPlayer(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public JSPlayer(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public JSPlayer(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        setKeepScreenOn(true);
        createPlayer();
        mJSRenderView = new JSSurfaceView(getContext());
        addView(mJSRenderView);
    }


    private class JSSurfaceView extends SurfaceView implements SurfaceHolder.Callback {

        public JSSurfaceView(Context context) {
            super(context);
            init();
        }

        public JSSurfaceView(Context context, AttributeSet attrs) {
            super(context, attrs);
            init();
        }

        public JSSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
            super(context, attrs, defStyleAttr);
            init();
        }

        @TargetApi(Build.VERSION_CODES.LOLLIPOP)
        public JSSurfaceView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
            super(context, attrs, defStyleAttr, defStyleRes);
            init();
        }

        private void init() {
            // Install a SurfaceHolder.Callback so we get notified when the
            // underlying surface is created and destroyed
            SurfaceHolder holder = getHolder();
            holder.addCallback(this);
            holder.setFormat(PixelFormat.TRANSPARENT);
            // setFormat is done by SurfaceView in SDK 2.3 and newer. Uncomment
            // this statement if back-porting to 2.2 or older:
            // holder.setFormat(PixelFormat.RGB_565);
            //
            // setType is not needed for SDK 2.0 or newer. Uncomment this
            // statement if back-porting this code to older SDKs.
            // holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
        }


        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            synchronized (JSPlayer.this) {
                Logger.d(TAG, TAG + ":onSurfaceCreated");
                if (isDestroyed()) {
                    return;
                }
                JSPlayer.this.onSurfaceCreated(mNativePlayer, holder.getSurface());
            }
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
//            synchronized (JSPlayer.this) {
//                Logger.d(TAG, TAG + ":onSurfaceChanged:" + "width=" + width + ",height=" + height);
//                if (isDestroyed()) {
//                    return;
//                }
//                JSPlayer.this.onSurfaceChanged(mNativePlayer, width, height);
//            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            synchronized (JSPlayer.this) {
                Logger.d(TAG, TAG + ":onSurfaceDestroyed");
                if (isDestroyed()) {
                    return;
                }
                JSPlayer.this.onSurfaceDestroyed(mNativePlayer);
            }
        }


    }


    private class JSTextureView extends TextureView implements TextureView.SurfaceTextureListener {

        private Surface mSurface;

        public JSTextureView(Context context) {
            super(context);
            init();
        }

        public JSTextureView(Context context, AttributeSet attrs) {
            super(context, attrs);
            init();
        }

        public JSTextureView(Context context, AttributeSet attrs, int defStyleAttr) {
            super(context, attrs, defStyleAttr);
            init();
        }

        @TargetApi(Build.VERSION_CODES.LOLLIPOP)
        public JSTextureView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
            super(context, attrs, defStyleAttr, defStyleRes);
            init();
        }

        private void init() {
            setSurfaceTextureListener(this);
        }

        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            this.mSurface = new Surface(surface);
            synchronized (JSPlayer.this) {
                Logger.d(TAG, TAG + ":onSurfaceCreated");
                if (isDestroyed()) {
                    return;
                }
                JSPlayer.this.onSurfaceCreated(mNativePlayer, mSurface);
            }
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
            synchronized (JSPlayer.this) {
                Logger.d(TAG, TAG + ":onSurfaceChanged:" + "width=" + width + ",height=" + height);
                if (isDestroyed()) {
                    return;
                }
                JSPlayer.this.onSurfaceChanged(mNativePlayer, width, height);
            }
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            synchronized (JSPlayer.this) {
                Logger.d(TAG, TAG + ":onSurfaceDestroyed");
                if (isDestroyed()) {
                    return true;
                }
                JSPlayer.this.onSurfaceDestroyed(mNativePlayer);
            }
            return true;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {

        }
    }


    public long getNativePlayer() {
        return mNativePlayer;
    }

    /**
     * set z order on top.
     *
     * @param onTop
     */
    public void setZOrderOnTop(boolean onTop) {
        if (mJSRenderView == null) {
            return;
        }
        mJSRenderView.setZOrderOnTop(onTop);
    }

    /**
     * set z order media overlay.
     *
     * @param isMediaOverlay
     */
    public void setZOrderMediaOverlay(boolean isMediaOverlay) {
        if (mJSRenderView == null) {
            return;
        }
        mJSRenderView.setZOrderMediaOverlay(isMediaOverlay);
    }

    private void checkDestroyedException(String operation) {
        if (isDestroyed()) {
            throw new IllegalStateException(operation + " is failed,player is destroyed");
        }
    }


    /**
     * create native player.
     *
     * @return
     */
    private synchronized void createPlayer() {
        mNativePlayer = create();
    }


    /**
     * set player option.
     *
     * @param key
     * @param value
     */
    public synchronized void setOption(String key, String value) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        setOption(mNativePlayer, key, value);
    }

    /**
     * get player option.
     *
     * @param key
     * @return
     */
    public synchronized String getOption(String key) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        return getOption(mNativePlayer, key);
    }


    /**
     * prepare.
     */
    public synchronized void prepare() {
        checkDestroyedException(StackTraceInfo.getMethodName());
        if (mIsWantToPause) {
            mIsWantToPause = false;
        }
        prepare(mNativePlayer);
    }


    /**
     * play.
     */
    public synchronized void play() {
        checkDestroyedException(StackTraceInfo.getMethodName());
        if (mIsWantToPause) {
            mIsWantToPause = false;
        }
        if (Constant.PLAY_STATUS_PREPARED != getPlayStatus()) {
            Logger.w(TAG, "play is failed : player's status isn't PLAY_STATUS_PREPARED.");
            return;
        }
        play(mNativePlayer);
    }

    /**
     * pause play.
     *
     * @return
     */
    public synchronized void pause() {
        checkDestroyedException(StackTraceInfo.getMethodName());
        if (Constant.PLAY_STATUS_PLAYING != getPlayStatus()) {
            mIsWantToPause = true;
            Logger.w(TAG, "pause is failed : player's status isn't PLAY_STATUS_PLAYING.");
            return;
        }
        pause(mNativePlayer);
    }

    /**
     * resume play.
     *
     * @return
     */
    public synchronized void resume() {
        checkDestroyedException(StackTraceInfo.getMethodName());
        if (mIsWantToPause) {
            if (Constant.PLAY_STATUS_PREPARED != getPlayStatus()) {
                play();
                return;
            }
            mIsWantToPause = false;
        }
        if (Constant.PLAY_STATUS_PAUSED != getPlayStatus()) {
            Logger.w(TAG, "resume is failed : player's status isn't PLAY_STATUS_PAUSED.");
            return;
        }
        resume(mNativePlayer);
    }

    /**
     * stop play.
     *
     * @return
     */
    public synchronized void stop() {
        checkDestroyedException(StackTraceInfo.getMethodName());
        if (mIsWantToPause) {
            mIsWantToPause = false;
        }
        if (getPlayStatus() > Constant.PLAY_STATUS_CREATED && getPlayStatus() < Constant.PLAY_STATUS_STOPPED) {
            stop(mNativePlayer);
        } else {
            Logger.w(TAG, "stop is failed : player's status is " + getPlayStatus() + ".");
        }

    }

    /**
     * reset player.
     * after invoke this func, play status can be changed to PLAY_STATUS_CREATED;
     *
     * @return
     */
    public synchronized void reset() {
        if (mIsWantToPause) {
            mIsWantToPause = false;
        }
        checkDestroyedException(StackTraceInfo.getMethodName());
        reset(mNativePlayer);

    }

    /**
     * destroy.
     *
     * @return
     */
    public synchronized void destroy() {
        checkDestroyedException(StackTraceInfo.getMethodName());
        destroy(mNativePlayer);
        mNativePlayer = 0;
    }


    public boolean isWantToPause() {
        return mIsWantToPause;
    }

    /**
     * set url.
     *
     * @param url
     */
    public synchronized void setUrl(String url) {
        checkDestroyedException(StackTraceInfo.getMethodName());
//        url="rtmp://v1.one-tv.com/live/mpegts.stream";
//        url = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
//        url="rtmp://pull-g.kktv8.com/livekktv/100987038";
//        url="rtmp://ossrs.net/dkd/kb4yr";
//        url="rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
//        url="http://hc.yinyuetai.com/uploads/videos/common/CE3C0166CE5EB6D5FA9FDB182D51DFA9.mp4";
//        url = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + "sintel.mp4";
        setUrl(mNativePlayer, url);
    }


    /**
     * set mute status.
     *
     * @param mute
     */
    public synchronized void setMute(boolean mute) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        setMute(mNativePlayer, mute);
    }

    /**
     * set channel mute.
     *
     * @param channelIndex
     * @param mute
     */
    public synchronized void setChannelMute(ChannelIndex channelIndex, boolean mute) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        int index = channelIndex.ordinal();
        setChannelMute(mNativePlayer, index, mute);
    }

    /**
     * set volume.
     *
     * @param volume [0-100]
     */
    public synchronized void setVolume(int volume) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        setVolume(mNativePlayer, volume);
    }

    /**
     * judge is mute.
     *
     * @return
     */
    public synchronized boolean getMute() {
        checkDestroyedException(StackTraceInfo.getMethodName());
        return getMute(mNativePlayer);
    }


    /**
     * get current volume[0-100].
     *
     * @return
     */
    public synchronized int getVolume() {
        checkDestroyedException(StackTraceInfo.getMethodName());
        return getVolume(mNativePlayer);
    }


    /**
     * judge is playing.
     *
     * @return
     */
    public synchronized boolean isPlaying() {
        return Constant.PLAY_STATUS_PLAYING == getPlayStatus();
    }

    /**
     * get play status；
     *
     * @return
     */
    public synchronized int getPlayStatus() {
        if (mNativePlayer == 0) {
            return Constant.PLAY_STATUS_DESTROYED;
        }
        return getPlayStatus(mNativePlayer);
    }

    private boolean isDestroyed() {
        return getPlayStatus() == Constant.PLAY_STATUS_DESTROYED;
    }


    /**
     * set is intercept pcm data.
     *
     * @param isInterceptPcmData
     */
    public synchronized void interceptPcmData(boolean isInterceptPcmData) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        interceptPcmData(mNativePlayer, isInterceptPcmData);
    }

    /**
     * set is parse data from video packet.
     *
     * @param isParseDataFromVideoPacket
     */
    public synchronized void parseDataFromVideoPacket(boolean isParseDataFromVideoPacket) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        parseDataFromVideoPacket(mNativePlayer, isParseDataFromVideoPacket);
    }

    /**
     * if invoke this func, will be preferred to use callbackHandle,ignore  {@link #mOnInterceptedPcmDataCallback}
     *
     * @param callbackHandle
     */
    public synchronized void setNativeInterceptedPcmDataCallbackHandle(long callbackHandle) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        setNativeInterceptedPcmDataCallbackHandle(mNativePlayer, callbackHandle);
    }


    /**
     * @param callbackHandle
     */
    public synchronized void setNativeParseDataFromVideoPacketCallbackHandle(long callbackHandle) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        setNativeParseDataFromVideoPacketCallbackHandle(mNativePlayer, callbackHandle);
    }

    /**
     * set surface available
     *
     * @param available
     */
    public void setSurfaceAvailable(boolean available) {
        checkDestroyedException(StackTraceInfo.getMethodName());
        if (available) {
            addView(mJSRenderView);
        } else {
            removeView(mJSRenderView);
        }

    }

    /**
     * intercepted audio listener.
     */
    public interface OnInterceptedPcmDataCallback {

        void onInterceptedPcmData(JSPlayer player, short[] data, int sampleNum, int channelNum);

    }

    /**
     * prepared listener.
     */
    public interface OnPreparedListener {

        void onPrepared(JSPlayer player);

    }

    /**
     * error listener.
     */
    public interface OnErrorListener {

        void onError(JSPlayer player, int what, int arg1, int arg2);

    }

    /**
     * info listener.
     */
    public interface OnInfoListener {

        void onInfo(JSPlayer player, int what, int arg1, int arg2);

    }

    /**
     * prepared listener.
     */
    public interface OnCompletedListener {

        void onCompleted(JSPlayer player);

    }

    /**
     * called by native.
     */
    private void onInterceptedPcmData(short[] data, int sampleNum, int channelNum) {
        if (mOnInterceptedPcmDataCallback != null) {
            mOnInterceptedPcmDataCallback.onInterceptedPcmData(JSPlayer.this, data, sampleNum, channelNum);
        }
    }

    /**
     * called by native.
     */
    private void onPrepared() {
        if (mOnPreparedListener != null) {
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    synchronized (JSPlayer.this) {
                        if (Constant.PLAY_STATUS_PREPARED != getPlayStatus()) {
                            Logger.w(TAG, "call onPrepared failed,player isn't prepared.");
                            return;
                        }
                        mOnPreparedListener.onPrepared(JSPlayer.this);
                    }
                }
            });
        }
    }


    /**
     * called by native.
     */
    private void onError(final int what, final int arg1, final int arg2) {

        if (mOnErrorListener != null) {
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    synchronized (JSPlayer.this) {
                        if (isDestroyed()) {
                            Logger.w(TAG, "call onError failed,player is destroyed.");
                            return;
                        }

                        reset();
                        mOnErrorListener.onError(JSPlayer.this, what, arg1, arg2);
                    }
                }
            });
        }
    }

    /**
     * called by native.
     */
    private void onInfo(final int what, final int arg1, final int arg2) {

        if (mOnInfoListener != null) {
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    synchronized (JSPlayer.this) {
                        if (isDestroyed()) {
                            Logger.w(TAG, "call onInfo failed,player is destroyed.");
                            return;
                        }
                        mOnInfoListener.onInfo(JSPlayer.this, what, arg1, arg2);
                    }
                }
            });
        }
    }

    /**
     * called by native.
     */
    private void onCompleted() {
        if (mOnCompletedListener != null) {
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    synchronized (JSPlayer.this) {
                        if (isDestroyed()) {
                            Logger.w(TAG, "call onCompleted failed,player is destroyed.");
                            return;
                        }
                        reset();
                        mOnCompletedListener.onCompleted(JSPlayer.this);
                    }
                }
            });
        }
    }


    /**
     * set on intercepted pcm data listener.
     *
     * @param callback
     */
    public void setOnInterceptedPcmDataCallback(OnInterceptedPcmDataCallback callback) {
        mOnInterceptedPcmDataCallback = callback;
    }

    /**
     * set on prepared listener.
     *
     * @param listener
     */
    public void setOnPreparedListener(OnPreparedListener listener) {
        mOnPreparedListener = listener;
    }


    /**
     * set on error listener.
     *
     * @param listener
     */
    public void setOnErrorListener(OnErrorListener listener) {
        mOnErrorListener = listener;
    }

    /**
     * set on info listener.
     *
     * @param listener
     */
    public void setOnInfoListener(OnInfoListener listener) {
        mOnInfoListener = listener;
    }

    /**
     * set on completed listener.
     *
     * @param listener
     */
    public void setOnCompletedListener(OnCompletedListener listener) {
        mOnCompletedListener = listener;
    }


    static {
        System.loadLibrary("jsplayer");
        nativeSetupJNI();
    }


    private static native void nativeSetupJNI();

    private native long create();

    private native void setOption(long handle, String key, String value);

    private native String getOption(long handle, String key);

    private native void setUrl(long handle, String url);

    private native void prepare(long handle);

    private native void play(long handle);

    private native void pause(long handle);

    private native void resume(long handle);

    private native void stop(long handle);

    private native void reset(long handle);

    private native void destroy(long handle);

    private native void onSurfaceCreated(long handle, Surface surface);

    private native void onSurfaceChanged(long handle, int width, int height);

    private native void onSurfaceDestroyed(long handle);

    private native void setMute(long handle, boolean mute);

    private native boolean getMute(long handle);

    private native void setVolume(long handle, int volume);

    private native int getVolume(long handle);

    private native void setChannelMute(long handle, int channelIndex, boolean mute);

    private native int getPlayStatus(long handle);

    private native void interceptPcmData(long handle, boolean isInterceptPcmData);

    private native void parseDataFromVideoPacket(long handle, boolean isParseDataFromVideoPacket);

    private native void setNativeInterceptedPcmDataCallbackHandle(long handle, long callbackHandle);

    private native void setNativeParseDataFromVideoPacketCallbackHandle(long handle, long callbackHandle);


    public static native void setLoggable(boolean loggable);

    public static native boolean getLoggable();

    public static native void setLogFileSavePath(String logFileSavePath);
}