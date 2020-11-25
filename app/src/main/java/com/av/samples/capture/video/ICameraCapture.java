package com.av.samples.capture.video;

public interface ICameraCapture {

    int CAMERA_OPEN_FAILED = 0;
    int CAMERA_OPEN_SUCCESS = 1;
    int CAMERA_STATED = 2;
    int CAMERA_STOPED = 3;

    int FLASH_MODE_OFF = 0;
    int FLASH_MODE_ON = 1;
    int FLASH_MODE_AUTO = 2;


    void switchCamera(boolean front);

    int getMaxZoom();

    void setZoom(int zoom);

    void enableAutoFocus(boolean enable);

    void focus(int width, int height, float x, float y);

    void starPreview();

    void stopPreview();

    void setOutputSize(int width, int height);

    void setFrameRate(float rate);

    void setFlushMode(int mode);

    void setCameraListener(CameraListener listener);

    public interface CameraListener {
        void onCameraStatus(int status);
    }
}
