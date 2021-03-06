package com.av.samples;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Surface;

import java.lang.ref.WeakReference;

import androidx.annotation.NonNull;


public class ReversePlayer {

    private static final String TAG = "ReversePlayer";

    private static final int WHAT_ERROR = 100;
    private static final int WHAT_PREPARED = 101;
    private static final int WHAT_COMPLETE = 102;
    private static final int WHAT_VIDEO_SIZE_CHANGED = 103;
    private static final int WHAT_RENDER_FIRST_FRAME = 104;
    private static final int WHAT_PLAYER_STATE_CHANGED = 105;
    private static final int WHAT_SEEK_COMPLETE = 106;
    private static final int WHAT_PLAYER_POSITION_CHANGED = 107;

    public static final int STATE_IDLE = 0;
    public static final int STATE_INITIALIZED = 1;
    public static final int STATE_ASYNC_PREPARING = 2;
    public static final int STATE_PREPARED = 3;
    public static final int STATE_STARTED = 4;
    public static final int STATE_PAUSED = 5;
    public static final int STATE_COMPLETED = 6;
    public static final int STATE_STOPPED = 7;
    public static final int STATE_ERROR = 8;

    private long mNativeHandler;
    private EventHandler mEventHandler;
    private Listener mListener;

    public ReversePlayer() {
        mNativeHandler = _create(new WeakReference<>(this));
        Looper looper = (Looper.myLooper() != null) ? Looper.myLooper() : Looper.getMainLooper();
        mEventHandler = new EventHandler(looper);
    }

    public void setListener(Listener listener) {
        this.mListener = listener;
    }

    public void init(Surface surface, int windowWidth, int windowHeight) {
        _init(mNativeHandler, surface, windowWidth, windowHeight);
    }

    public void setDataSource(String path, String tempDir) {
        _setDataSource(mNativeHandler, path, tempDir);
    }

    public void prepare() {
        _prepare(mNativeHandler);
    }

    public void seekTo(long posMs) {
        _seek(mNativeHandler, posMs * 1000);
    }

    public void start() {
        _start(mNativeHandler);
    }

    public void pause() {
        _pause(mNativeHandler);
    }

    public void stop() {
        _stop(mNativeHandler);
    }


    public void release() {
        _release(mNativeHandler);
    }

    private native long _create(WeakReference<ReversePlayer> weekRef);

    private native int _init(long handle, Surface surface, int windowWidth, int windowHeight);

    private native int _setDataSource(long handler, String path, String tempDir);

    private native int _prepare(long handler);

    private native int _start(long handler);

    private native int _pause(long handler);

    private native int _seek(long handler, long posUs);

    private native int _stop(long handler);

    private native int _release(long handler);

    private static void postEventFromNative(Object weakThiz,
                                            int what, int arg1, int arg2, Object obj) {
        Log.i(TAG, "receive a msg from native what=" + what + "thread = " + Thread.currentThread());
        if (weakThiz == null) {
            return;
        }
        ReversePlayer rp = (ReversePlayer) ((WeakReference) weakThiz).get();
        if (rp != null && rp.mEventHandler != null) {
            rp.mEventHandler.obtainMessage(what, arg1, arg2, obj).sendToTarget();
        }
    }

    private class EventHandler extends Handler {

        public EventHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            Log.i(TAG, "EventHandler receive a msg what=" + msg.what + "thread = " + Thread.currentThread());
            switch (msg.what) {
                case WHAT_PREPARED:
                    Log.i(TAG, "on video prepared");
                    break;
                case WHAT_COMPLETE:
                    Log.i(TAG, "on video complete");
                    break;
                case WHAT_ERROR:
                    int errorCode = msg.arg1;
                    Log.i(TAG, "error :" + errorCode);
                    if (mListener != null) {
                        mListener.onError(errorCode);
                    }
                    break;
                case WHAT_VIDEO_SIZE_CHANGED:
                    int width = msg.arg1;
                    int height = msg.arg2;
                    Log.i(TAG, "onVideoSizeChanged video=(" + width + "x" + height + ")");
                    if (mListener != null) {
                        mListener.onVideoSizeChanged(width, height);
                    }
                case WHAT_RENDER_FIRST_FRAME:
                    break;
                case WHAT_PLAYER_STATE_CHANGED:
                    int state = msg.arg1;
                    if (mListener != null) {
                        mListener.onPlayerStateChanged(state);
                    }
                    break;
                case WHAT_SEEK_COMPLETE:
                    if (mListener != null) {
                        Log.i(TAG, "onSeekComplete");
                        mListener.onSeekComplete();
                    }
                    break;
                case WHAT_PLAYER_POSITION_CHANGED:
                    long pos = msg.arg1;
                    long duration = msg.arg2;
                    if (mListener != null) {
                        Log.i(TAG, "onProgressUpdated");
                        mListener.onProgressUpdated(pos, duration);
                    }
                    break;
            }
        }
    }

    interface Listener {

        void onPlayerStateChanged(int state);

        void onVideoSizeChanged(int width, int height);

        void onSeekComplete();

        void onError(int errorCode);

        void onProgressUpdated(long posMs, long durationMs);
    }
}
