package com.av.samples;

import android.os.Handler;
import android.os.Looper;

public class FFTranscode {

    private long mNativeHandler = -1L;

    private TranscodeListener mListener;

    private Handler mHandler;

    public FFTranscode() {
        mNativeHandler = _create();
        mHandler = new Handler(Looper.getMainLooper());
    }

    private native long _create();

    public void setTranscodeListener(TranscodeListener listener) {
        this.mListener = listener;
    }

    public void start() {
        notifyStart();
        int ret = _start(mNativeHandler);
        if (ret >= 0) {
            notifySuccess();
        } else {
            notifyFailed(ret);
        }
    }

    private void notifyStart() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mListener != null) {
                    mListener.onTranscodeStart();
                }
            }
        });
    }

    private void notifyFailed(int ret) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mListener != null) {
                    mListener.onTranscodeFailed(ret);
                }
            }
        });
    }

    private void notifySuccess() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mListener != null) {
                    mListener.onTranscodeComplete();
                }
            }
        });
    }


    public void init( String inputPath, String outputPath, int width, int height, int fps, int bitrate,
                     int sampleRate, int channel, int audioBitrate) {
        _init(mNativeHandler, inputPath, outputPath, width, height, fps, bitrate, sampleRate, channel, audioBitrate);
    }

    public void stop() {
        _stop(mNativeHandler);
    }

    public void release() {
        _release(mNativeHandler);
    }

    private native int _init(long handle, String inputPath, String outputPath, int width, int height, int fps, int bitrate,
                             int sampleRate, int channel, int audioBitrate);

    private native int _start(long handle);

    private native int _stop(long handle);

    private native int _release(long handle);

    public interface TranscodeListener {

        void onTranscodeStart();

        void onTranscodeFailed(int error);

        void onTranscodeComplete();
    }
}
