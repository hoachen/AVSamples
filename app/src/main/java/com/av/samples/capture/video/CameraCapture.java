package com.av.samples.capture.video;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.util.Log;

import com.av.samples.base.GLTextureOutputRenderer;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.List;

import javax.microedition.khronos.opengles.GL10;

public class CameraCapture extends GLTextureOutputRenderer implements ICameraCapture, SurfaceTexture.OnFrameAvailableListener {

    private static final String TAG = "CameraCapture";
    protected static final String UNIFORM_CAM_MATRIX = "u_Matrix";
    public static final int CAMERA_POS_FRONT_CAMERA = 0;
    public static final int CAMERA_POS_BACK_CAMERA = 1;
    private int mCameraPos = CAMERA_POS_FRONT_CAMERA;
    private int mMatrixHandle;
    private float mMatrix[] = new float[16];
    private WeakReference<Context> mContext;
    private int mOutputOritation;
    private int mOutputWidth;
    private int mOutputHeight;
    private boolean mAutoFocus = false;
    private float mFrameRate;
    private int mZoom = -1;
    private int mCurrentZoom = -1;
    private boolean mCameraChanged;
    private boolean mIsCameraOpened;
    private Camera mCamera;
    protected SurfaceTexture mCamTexture;
    private CameraListener mCameraListener;
    protected volatile boolean mISCapturing = false;

    public CameraCapture(Context context) {
        this(context, CAMERA_POS_FRONT_CAMERA, 0);
    }

    public CameraCapture(Context context, int cameraPos, int outputOritation) {
        mContext = new WeakReference<>(context);
        mCameraPos = cameraPos;
        mOutputOritation = outputOritation;
        mCurRotation = mOutputOritation % 2;
        mOutputWidth = 1280;
        mOutputHeight = 720;
        mFrameRate = 30.0f;
    }


    private Camera createCamera() {
        Camera camera = null;
        int numberOfCameras = Camera.getNumberOfCameras();
        if (numberOfCameras > 1) {
            Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
            for (int i = 0; i < numberOfCameras; i++) {
                Camera.getCameraInfo(i, cameraInfo);
                if ((mCameraPos == CAMERA_POS_FRONT_CAMERA && cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT)
                        || (mCameraPos == CAMERA_POS_BACK_CAMERA && cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_BACK)) {
                    camera = Camera.open(i);
                    break;
                }
            }
        } else {
            camera = Camera.open();
        }
        if (camera != null) {
            Camera.Parameters parameters = camera.getParameters();
            // 设置白平衡
            List<String> whiteBalances =  parameters.getSupportedWhiteBalance();
            if (whiteBalances != null && whiteBalances.contains(Camera.Parameters.WHITE_BALANCE_AUTO)) {
                parameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
            }
            List<String> supportedSceneModes = parameters.getSupportedSceneModes();
            if (supportedSceneModes != null && supportedSceneModes.contains(Camera.Parameters.SCENE_MODE_AUTO)) {
                parameters.setSceneMode(Camera.Parameters.SCENE_MODE_AUTO);
            }
            // 设置自动对焦
            if (mAutoFocus) {
                List<String> supportFocusModes = parameters.getSupportedFocusModes();
                if (supportFocusModes != null && supportFocusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
                    parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
                } else if (supportFocusModes != null && supportFocusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
                    parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
                }
            } else {
                parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_FIXED);
            }
            // 设置预览大小
            List<Camera.Size> supportPreviewSizes = parameters.getSupportedPreviewSizes();
            int targetWidth = (mCurRotation % 2 == 0) ? mOutputWidth : mOutputHeight;
            int targetHeight = (mCurRotation % 2 == 0) ? mOutputHeight : mOutputWidth;
            float targetRadio = targetWidth * 1.0f / targetHeight;
            float targetArea = targetWidth * targetHeight;
            int index = -1;
            float minRatioDiff = 100.0f;
            float minAreaDiff = 1920 * 1080 * 4;
            for (int i = 0; i < supportPreviewSizes.size(); i++) {
                Camera.Size size = supportPreviewSizes.get(i);
                float tempRadio = size.width * 1.0f / size.height;
                float tempArea = size.width * size.height;
                float radioDiff = Math.abs(tempRadio - targetRadio);
                float areaDiff = Math.abs(tempArea - targetArea);
                if (radioDiff + 0.01 < minAreaDiff) {
                    minRatioDiff = radioDiff;
                    minAreaDiff = areaDiff ;
                    index = i;
                } else if (Math.abs(minRatioDiff - radioDiff) < 0.01 && areaDiff < minAreaDiff) {
                    minRatioDiff = radioDiff;
                    minAreaDiff = areaDiff ;
                    index = i;
                }
                Log.i(TAG,"Size w="+ size.width + "h=" + size.height + "index :" + index);
            }
            if (index < 0) {
                camera.release();
                camera = null;
                return null;
            }
            Camera.Size size = supportPreviewSizes.get(index);
            parameters.setPreviewSize(size.width, size.height);
            camera.setParameters(parameters);
        }
        return camera;
    }

    @Override
    public void starPreview() {
        if (mCameraChanged) {
            return;
        }
        mIsCameraOpened = false;
        mCameraChanged = true;
        onDrawFrame();
    }

    @Override
    protected void drawFrame() {
        if (mCameraChanged) {
            if (mCamera != null) {
                mCamera.stopPreview();
                mCamera.release();
                mCamera = null;
            }
            mCamera = createCamera();
            if(mCamera == null){
                if(mCameraListener != null){
                    mCameraListener.onCameraStatus(CAMERA_OPEN_FAILED);
                }
                return;
            }
            mCamTexture.setOnFrameAvailableListener(this);
            try {
                mCamera.setPreviewTexture(mCamTexture);
            } catch (IOException e) {
                e.printStackTrace();
            }
            mCamera.startPreview();
            mISCapturing = true;
            mCameraChanged = false;
        } else {
            if (mCurrentZoom != mZoom) {
                mCurrentZoom = mZoom;
                try {
                    Camera.Parameters parameters = mCamera.getParameters();
                    if (parameters.isZoomSupported()) {
                        int maxZoom = parameters.getMaxZoom();
                        parameters.setZoom((int) (mZoom / 100.f * maxZoom));
                    }
                    mCamera.setParameters(parameters);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            super.drawFrame();
        }
    }

    @Override
    public void stopPreview() {
        if (mCamera != null) {
            mCamera.stopPreview();
            mCamTexture.setOnFrameAvailableListener(null);
            mCamera.release();
            mCamera = null;
        }
        mISCapturing = false;
        mIsCameraOpened = false;
    }


    @Override
    public void switchCamera(boolean front) {
        if (mCameraChanged) {
            return;
        }
        if (mCamera == null) {
            return;
        }
        if (mCameraPos == CAMERA_POS_FRONT_CAMERA) {
            mCameraPos = CAMERA_POS_BACK_CAMERA;
        } else {
            mCameraPos = CAMERA_POS_FRONT_CAMERA;
        }
        mCameraChanged = true;
        mIsCameraOpened = false;
    }

    @Override
    public int getMaxZoom() {
        if (mCamera != null) {
            Camera.Parameters parameters = mCamera.getParameters();
            return parameters.getMaxZoom();
        }
        return 0;
    }

    @Override
    public void setZoom(int zoom) {
        mZoom = zoom;
    }

    @Override
    public void enableAutoFocus(boolean enable) {
        mAutoFocus = enable;
        if (mCamera != null) {
            Camera.Parameters parameters = mCamera.getParameters();
            List<String> supportFocusModes = parameters.getSupportedFocusModes();
            if (mAutoFocus && supportFocusModes != null) {
                if (supportFocusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
                    parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
                } else if (supportFocusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
                    parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
                }
            } else {
                parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_FIXED);
            }
            mCamera.setParameters(parameters);
        }
    }

    @Override
    public void focus(int width, int height, float x, float y) {

    }

    @Override
    public void setOutputSize(int width, int height) {
        mOutputWidth = width;
        mOutputHeight = height;
    }

    @Override
    public void setFrameRate(float rate) {
        mFrameRate = rate;
    }

    @Override
    public void setFlushMode(int mode) {
        if (mCamera != null) {
            Camera.Parameters parameters = mCamera.getParameters();
            List<String> modes = parameters.getSupportedFlashModes();
            if (mode == FLASH_MODE_AUTO && modes.contains(Camera.Parameters.FLASH_MODE_AUTO)) {
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_AUTO);
            } else if (mode == FLASH_MODE_OFF && modes.contains(Camera.Parameters.FLASH_MODE_OFF)) {
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
            } else if (mode == FLASH_MODE_ON && modes.contains(Camera.Parameters.FLASH_MODE_ON)) {
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_ON);
            }
            mCamera.setParameters(parameters);
        }
    }


    @Override
    protected void initGLContext() {
        super.initGLContext();
        int[] textures = new int[1];
        if (mCamTexture != null) {
            mCamTexture.setOnFrameAvailableListener(null);
            mCamTexture.release();
            mCamTexture = null;
        }
        if (mInputTextureId > 0) {
            textures[0] = mInputTextureId;
            GLES20.glDeleteTextures(1, textures, 0);
        }
        GLES20.glGenTextures(1, textures, 0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textures[0]);
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
        mInputTextureId = textures[0];
        mCamTexture = new SurfaceTexture(mInputTextureId);
    }


    @Override
    protected String getVertexShader() {
        return "uniform mat4 " + UNIFORM_CAM_MATRIX + ";\n" +
                "attribute vec4 " + ATTRIBUTE_POSITION + ";\n" +
                "attribute vec2 " + ATTRIBUTE_TEXCOORD + ";\n" +
                "varying vec2 " + VARYING_TEXCOORD + ";\n" +
                "void main() {\n" +
                "    vec4 texPos = " + UNIFORM_CAM_MATRIX + "* vec4(" + ATTRIBUTE_TEXCOORD + ",1,1);\n" +
                "    " + VARYING_TEXCOORD + " = texPos.xy;\n" +
                "    gl_Position = " + ATTRIBUTE_POSITION + ";\n" +
                "}\n";
    }

    @Override
    protected String getFragmentShader() {
        return "#extension GL_OES_EGL_image_external : require\n" +
                "precision mediump float;\n" +
                "uniform samplerExternalOES " + UNIFORM_TEXTURE0 + ";\n" +
                "varying vec2 " + VARYING_TEXCOORD + ";\n" +
                "void main() {\n" +
                "    gl_FrameColor = texture2D(" + UNIFORM_TEXTURE0 + "," +  VARYING_TEXCOORD+ ");\n" +
                "}\n";
    }

    @Override
    protected void initShaderHandlers() {
        super.initShaderHandlers();
        mMatrixHandle = GLES20.glGetUniformLocation(mProgram, UNIFORM_CAM_MATRIX);
    }

    protected void bindTexture() {
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mInputTextureId);
    }

    @Override
    protected void passShaderValues() {
        // 传入顶点坐标
        mVertexBuffer.position(0);
        GLES20.glVertexAttribPointer(mPositionHandle, 2, GLES20.GL_FLOAT, false, 2 * 4, mVertexBuffer);
        GLES20.glEnableVertexAttribArray(mPositionHandle);
        // 纹理坐标
        mTextureVertices[mCurRotation].position(0);
        GLES20.glVertexAttribPointer(mTexCordHandle, 2, GLES20.GL_FLOAT, false, 2 * 4, mTextureVertices[mCurRotation]);
        GLES20.glEnableVertexAttribArray(mTexCordHandle);
        bindTexture();
        GLES20.glUniform1i(mTextureHandle, 0);
        GLES20.glUniformMatrix4fv(mMatrixHandle, 1, false, mMatrix, 0);
    }


    @Override
    public void setCameraListener(CameraListener listener) {
        mCameraListener = listener;
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        if (!mIsCameraOpened) {
            if (mCameraListener != null) {
                mCameraListener.onCameraStatus(CAMERA_OPEN_SUCCESS);
            }
            mIsCameraOpened = true;
        }
        if (mISCapturing) {
            try {
                mCamTexture.updateTexImage();
                mCamTexture.getTransformMatrix(mMatrix);
            } catch (Exception e) {
            }
            // 处理丢帧
        }
    }
}
