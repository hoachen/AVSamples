package com.av.samples.capture.video;

public interface ICameraCapture {

    void switchCamera(boolean front);

    float getMaxZoom();

    void setZoom(float zoom);

    void enableAutoFocus(boolean enable);

    void focus(int width, int height, float x, float y);

    void starPreview();

    void stopPreview();

    void setOutputSize(int width, int height);

    void setFrameRate(float rate);

    void setFlushMode();

    public interface CameraListener {
        void onCameraStatus(int status);
    }
}
