package com.av.samples.demux;

public class VideoSplit {

    public VideoSplit() {

    }

    public int splitVideo(String path, String dir) {
        return _splitVideo(path, dir);
    }


    public int splitVideoToYuv(String path, String dir) {
        return _splitVideoToYuv(path, dir);
    }

    private native int _splitVideo(String inputVideo, String outputDir);

    private native int _splitVideoToYuv(String inputVideo, String outputDir);


}
