package com.av.samples;

public class Transcode {

    private long mNativeHandler;

    public Transcode() {
        mNativeHandler = _create();
    }

    private native long _create();

    public void start() {
        _start(mNativeHandler);
    }

    public void init( String inputPath, String outputPath, int width, int height, int fps, int bitrate,
                     int sampleRate, int channel, int audioBitrate) {
        _init(mNativeHandler, inputPath, outputPath, width, height, fps, bitrate, sampleRate, channel, audioBitrate);
    }

    public void stop() {
        _stop(mNativeHandler);
    }

    public void destroy() {
        _destroy(mNativeHandler);
    }
    private native int _init(long handle, String inputPath, String outputPath, int width, int height, int fps, int bitrate,
                             int sampleRate, int channel, int audioBitrate);

    private native int _start(long handle);

    private native int _stop(long handle);

    private native int _destroy(long handle);
}
