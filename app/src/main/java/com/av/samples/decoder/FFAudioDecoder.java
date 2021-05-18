package com.av.samples.decoder;

public class FFAudioDecoder {

    private long mNativeHandle;

    public FFAudioDecoder() {
        mNativeHandle = _create();
    }

    public void init(String path, int channel, int sampleRate) {
        _init(mNativeHandle, path, channel, sampleRate);
    }

    public void start() {
        _start(mNativeHandle);
    }

    private static native long _create();

    private static native int _init(long handle, String input_file, int channel, int sampleRate);

    private static native int _start(long handle);
}
