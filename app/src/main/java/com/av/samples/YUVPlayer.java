package com.av.samples;

import android.view.Surface;

public class YUVPlayer {

    private long mNativeHandle;

    public YUVPlayer() {
        mNativeHandle = _create();
    }

    public void init(Surface surface, int windowWidth, int windowHeight, int width, int height, String url) {
        _init(mNativeHandle, surface, windowWidth, windowHeight, width, height, url);
    }

    public void prepare() {
        _prepare(mNativeHandle);
    }


    public void start() {
        _start(mNativeHandle);
    }


    public void stop() {
        _stop(mNativeHandle);
    }

    public void release() {
        _release(mNativeHandle);
    }

    private native long _create();
    private native int _init(long handle, Surface surface, int windowWidth, int windowHeight, int videoWidth, int videoHeight, String url);
    private native int _prepare(long handle);
    private native int _start(long handle);
    private native int _stop(long handle);
    private native int _release(long handle);


}
