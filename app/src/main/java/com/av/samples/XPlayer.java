package com.av.samples;

public class XPlayer {

    private long mNativeHandler;

    public XPlayer() {
        mNativeHandler = _create();
    }

    public void setDataSource(String path) {
        _setDataSource(mNativeHandler, path);
    }

    public void prepare() {
        _prepare(mNativeHandler);
    }
//
//    public void start() {
//        _start(mNativeHandler);
//    }
//
//    public void stop() {
//        _stop(mNativeHandler);
//    }

    private native long _create();

    private native int _setDataSource(long handler, String path);

    private native int _prepare(long handler);

//    private native int _start(long handler);
//
//    private native int _stop(long handler);


}
