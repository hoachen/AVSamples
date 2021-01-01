package com.av.samples;

import android.view.Surface;

public class ReversePlayer {

    private long mNativeHandler;

    public ReversePlayer() {
        mNativeHandler = _create();
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

    private native long _create();

    private native int _init(long handle, Surface surface, int windowWidth, int windowHeight);

    private native int _setDataSource(long handler, String path, String tempDir);

    private native int _prepare(long handler);

    private native int _start(long handler);

    private native int _pause(long handler);

    private native int _stop(long handler);

    private native int _release(long handler);


}
