package com.av.samples;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.util.Stack;

public class ReverseEffect {

    private static final long MEDIA_CODED_TIMEOUT_US = 10000L;
    private Stack<Long> mSyncSampleTimes;
    private MediaCodec mDecoder;
    private MediaExtractor mMediaExtractor;
    private int mVideoTrackIndex;
    private long mLastPresentationTimeUs;
    private MediaFormat mVideoFormat;
    private MediaCodec.BufferInfo mBufferInfo = new MediaCodec.BufferInfo();

    public ReverseEffect() {
        mSyncSampleTimes = new Stack<>();
    }

    public void start(String path, Surface surface) throws Exception {
        mVideoTrackIndex = -1;
        mSyncSampleTimes.clear();
        mMediaExtractor = new MediaExtractor();
        mMediaExtractor.setDataSource(path);
        for (int i = 0; i < mMediaExtractor.getTrackCount(); i++) {
            MediaFormat mediaFormat = mMediaExtractor.getTrackFormat(i);
            if (mediaFormat.getString(MediaFormat.KEY_MIME).startsWith("video/")) {
                mVideoFormat = mediaFormat;
                mVideoTrackIndex = i;
                break;
            }
        }
        if (mVideoTrackIndex < 0) {
            throw new IllegalStateException("no video track" + path);
        }
        mMediaExtractor.selectTrack(mVideoTrackIndex);
        while(true) {
            if (mMediaExtractor.getSampleFlags() == MediaExtractor.SAMPLE_FLAG_SYNC) {
                mSyncSampleTimes.push(mMediaExtractor.getSampleTime());
            }
            if (!mMediaExtractor.advance())
                break;
        }

        long startPts =  mSyncSampleTimes.pop();
        long endPts = mVideoFormat.getLong(MediaFormat.KEY_DURATION);
        mLastPresentationTimeUs = endPts;
        Log.i("ReverseEffect", "startPts:" + startPts + ",end pts:" + endPts);
        mMediaExtractor.seekTo(startPts, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
        Log.i("ReverseEffect", mSyncSampleTimes.toString());
        String mimeType = mVideoFormat.getString(MediaFormat.KEY_MIME);
        mDecoder = MediaCodec.createDecoderByType(mimeType);
        mDecoder.configure( mVideoFormat, surface,null, 0);
        mDecoder.start();
        boolean allInputDecoded = false;
        while (!allInputDecoded) {
            int bufferIndex = mDecoder.dequeueInputBuffer(MEDIA_CODED_TIMEOUT_US);
            if (bufferIndex >= 0) {
                ByteBuffer buffer = mDecoder.getInputBuffer(bufferIndex);
                int sampleSize = mMediaExtractor.readSampleData(buffer, 0);
                if (sampleSize > 0) {
                    Log.i("ReverseEffect", "queue a frame pts:" + (mLastPresentationTimeUs - mMediaExtractor.getSampleTime()));
                    mDecoder.queueInputBuffer(
                            bufferIndex, 0, sampleSize,
                            mLastPresentationTimeUs - mMediaExtractor.getSampleTime(),
                            mMediaExtractor.getSampleFlags());
                }
                Log.i("ReverseEffect", "current  pts:" + mMediaExtractor.getSampleTime() + ", end pts"+ endPts);
                if (!mSyncSampleTimes.isEmpty() && mMediaExtractor.getSampleTime() >= endPts) {
                    endPts = startPts;
                    startPts = mSyncSampleTimes.pop();
                    mMediaExtractor.seekTo(startPts, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                } else {
                    boolean canadvance = mMediaExtractor.advance();
                    if (!canadvance) {
                        if (!mSyncSampleTimes.isEmpty()) {
                            endPts = startPts;
                            startPts = mSyncSampleTimes.pop();
                            mMediaExtractor.seekTo(startPts, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                        } else {
                            mDecoder.queueInputBuffer(bufferIndex, 0, 0,
                                    0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        }
                    }
                }
                int outBufferIndex = mDecoder.dequeueOutputBuffer(mBufferInfo, MEDIA_CODED_TIMEOUT_US);
                if (outBufferIndex >= 0) {
                    Log.i("ReverseEffect", "decode a frame pts:" + mBufferInfo.presentationTimeUs);
                    boolean render = mBufferInfo.size > 0;
                    // Get the decoded frame
                    mDecoder.releaseOutputBuffer(outBufferIndex, render);
                    Thread.sleep(40);
                    // Did we get all output from decoder?
                    if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) !=0){
                        allInputDecoded = true;
                    }
                } else if (outBufferIndex == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    continue;
                }

            }
        }
        mDecoder.stop();
        mMediaExtractor.release();
        mDecoder.release();

    }

}
