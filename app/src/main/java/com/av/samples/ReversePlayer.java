package com.av.samples;

import android.util.Log;
import android.view.Surface;

import java.lang.ref.WeakReference;


public class ReversePlayer {

    private static final String TAG = "ReversePlayer";

    private long mNativeHandler;

    public ReversePlayer() {
        mNativeHandler = _create(new WeakReference<>(this));
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

    public void start() {
        _start(mNativeHandler);
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

    private native int _stop(long handler);

    private native int _release(long handler);

    private static void postEventFromNative(Object weakThiz,
                                            int what, int arg1, int arg2, Object obj) {
        Log.i(TAG, "postEventFromNative what=" + what);
        if (weakThiz == null) {
            return;
        }
        ReversePlayer rp = (ReversePlayer) ((WeakReference) weakThiz).get();
        if (rp != null) {

        }
    }
}
