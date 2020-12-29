package com.av.samples.demux;

public class VideoSplit {

    public VideoSplit() {

    }

    public void splitVideo(String path, String dir) {
        _splitVideo(path, dir);
    }

    private native int _splitVideo(String inputVideo, String outputDir);
}
