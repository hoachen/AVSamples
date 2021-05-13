package com.av.samples;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

public class PreviewPlayer {

    private static final String TAG = "PreviewPlayer";
    private static final boolean VERBOSE = true;

    private MediaExtractor mExtractor;

    private MediaCodec mDecoder;
    private Surface mSurface;

    private ByteBuffer[] mDecoderInputBuffers;

    private MediaCodec.BufferInfo mBufferInfo;

    private FrameCallBack mFrameCallback;
    private MediaFormat mVideoFormat;
    private boolean mLoop;

    private boolean mIsStopRequested;
    private int mTrackIndex;

    public PreviewPlayer(Surface surface) {
        mSurface = surface;
    }


    public void setDataSource(String path) throws IOException {
        mExtractor = new MediaExtractor();
        mExtractor.setDataSource(path);
        for(int i = 0 ; i < mExtractor.getTrackCount(); i++) {
            MediaFormat mediaFormat = mExtractor.getTrackFormat(i);
            String mime = mediaFormat.getString(MediaFormat.KEY_MIME);
            if (mime.startsWith("video/")) {
                mVideoFormat = mediaFormat;
                mTrackIndex = i;
                mExtractor.selectTrack(i);
                break;
            }
        }
    }

    public void start() throws IOException {
        mBufferInfo = new MediaCodec.BufferInfo();
        String mime = mVideoFormat.getString(MediaFormat.KEY_MIME);
        mDecoder = MediaCodec.createDecoderByType(mime);
        mDecoder.configure(mVideoFormat, mSurface, null, 0);
        mDecoder.start();
        mDecoderInputBuffers = mDecoder.getInputBuffers();
        int frameRate = mVideoFormat.getInteger(MediaFormat.KEY_FRAME_RATE);
        float framePtsInterval = 1000 / frameRate;
        Log.i(TAG, "frameRate=" + frameRate + ",framePtsInterval=" + framePtsInterval);
        final long duration = mVideoFormat.getLong(MediaFormat.KEY_DURATION);
        new Thread(new Runnable() {
            @Override
            public void run() {
                long pts = (long) (duration - framePtsInterval);
                while(pts >= 0) {
                    seekBackward(pts);
                    pts -= framePtsInterval;
                }
            }
        }).start();
    }

    public void setFrameCallBack(FrameCallBack frameCallBack) {
        this.mFrameCallback = frameCallBack;
    }

    public void seekBackward(long position){
        final int TIMEOUT_USEC = 10000;
        int inputChunk = 0;
        long firstInputTimeNsec = -1;

        boolean outputDone = false;
        boolean inputDone = false;

        mExtractor.seekTo(position, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
        Log.d(TAG, "sampleTime: " + mExtractor.getSampleTime()/1000 + " -- position: " + position + " ----- BACKWARD");

        while (mExtractor.getSampleTime() < position && position >= 0) {

             Log.d(TAG, "loop");
            if (mIsStopRequested) {
                Log.d(TAG, "Stop requested");
                return;
            }
            // Feed more data to the decoder.
            if (!inputDone) {
                int inputBufIndex = mDecoder.dequeueInputBuffer(TIMEOUT_USEC);
                if (inputBufIndex >= 0) {
                    if (firstInputTimeNsec == -1) {
                        firstInputTimeNsec = System.nanoTime();
                    }
                    ByteBuffer inputBuf = mDecoderInputBuffers[inputBufIndex];
                    // Read the sample data into the ByteBuffer.  This neither respects nor
                    // updates inputBuf's position, limit, etc.
                    int chunkSize = mExtractor.readSampleData(inputBuf, 0);
                    if (chunkSize < 0) {
                        // End of stream -- send empty frame with EOS flag set.
                        mDecoder.queueInputBuffer(inputBufIndex, 0, 0, 0L,
                                MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        inputDone = true;
                        if (VERBOSE) Log.d(TAG, "sent input EOS");
                    } else {
                        if (mExtractor.getSampleTrackIndex() != mTrackIndex) {
                            Log.w(TAG, "WEIRD: got sample from track " +
                                    mExtractor.getSampleTrackIndex() + ", expected " + mTrackIndex);
                        }
                        long presentationTimeUs = mExtractor.getSampleTime();
                        mDecoder.queueInputBuffer(inputBufIndex, 0, chunkSize,
                                presentationTimeUs, 0 /*flags*/);
                        if (VERBOSE) {
                            Log.d(TAG, "submitted frame " + inputChunk + " to dec, size=" + chunkSize);
                        }
                        inputChunk++;
                        mExtractor.advance();
                    }
                } else {
                    if (VERBOSE) Log.d(TAG, "input buffer not available");
                }
            }

            if (!outputDone) {
                int decoderStatus = mDecoder.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);
                if (decoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    // no output available yet
                    if (VERBOSE) Log.d(TAG, "no output from decoder available");
                } else if (decoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                    // not important for us, since we're using Surface
                    if (VERBOSE) Log.d(TAG, "decoder output buffers changed");
                } else if (decoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    MediaFormat newFormat = mDecoder.getOutputFormat();
                    if (VERBOSE) Log.d(TAG, "decoder output format changed: " + newFormat);
                } else if (decoderStatus < 0) {
                    throw new RuntimeException(
                            "unexpected result from decoder.dequeueOutputBuffer: " +
                                    decoderStatus);
                } else { // decoderStatus >= 0
                    if (firstInputTimeNsec != 0) {
                        // Log the delay from the first buffer of input to the first buffer
                        // of output.
                        long nowNsec = System.nanoTime();
                        Log.d(TAG, "startup lag " + ((nowNsec-firstInputTimeNsec) / 1000000.0) + " ms");
                        firstInputTimeNsec = 0;
                    }
                    boolean doLoop = false;
                    if (VERBOSE) Log.d(TAG, "surface decoder given buffer " + decoderStatus +
                            " (size=" + mBufferInfo.size + ")");
                    if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                        if (VERBOSE) Log.d(TAG, "output EOS");
                        if (mLoop) {
                            doLoop = true;
                        } else {
                            outputDone = true;
                        }
                    }

                    boolean doRender = (mBufferInfo.size != 0);

                    // As soon as we call releaseOutputBuffer, the buffer will be forwarded
                    // to SurfaceTexture to convert to a texture.  We can't control when it
                    // appears on-screen, but we can manage the pace at which we release
                    // the buffers.
                    if (doRender && mFrameCallback != null) {
                        mFrameCallback.preRender(mBufferInfo.presentationTimeUs);
                    }
                    mDecoder.releaseOutputBuffer(decoderStatus, doRender);

                    doRender = false;
                    if (doRender && mFrameCallback != null) {
                        mFrameCallback.postRender();
                    }

                    if (doLoop) {
                        Log.d(TAG, "Reached EOS, looping");
                        mExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                        inputDone = false;
                        mDecoder.flush();    // reset decoder state
                        mFrameCallback.loopReset();
                    }
                    break;
                }
            }
        }
    }



    public interface FrameCallBack {

        void preRender(long pts);

        void postRender();

        void loopReset();
    }
}
