package com.av.samples.capture.video;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;

import com.av.samples.base.GLTextureOutputRenderer;

import java.lang.ref.WeakReference;

import javax.microedition.khronos.opengles.GL10;

public class CameraCapture extends GLTextureOutputRenderer implements ICameraCapture {

    protected static final String UNIFORM_CAM_MATRIX = "u_Matrix";
    public static final int CAMERA_POS_FRONT_CAMERA = 0;
    public static final int CAMERA_POS_BACK_CAMERA = 1;
    private int mCameraPos = CAMERA_POS_FRONT_CAMERA;

    private WeakReference<Context> mContext;
    private int mOutputOritation;
    private int mOutputWidth;
    private int mOutputHeight;
    private float mFrameRate;
    private boolean mNeedScalingFps;
    private boolean mCameraChanged;
    private boolean mIsCameraOpened;
    private Camera mCamera;
    protected SurfaceTexture mCamTexture;

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
        mNeedScalingFps = true;
    }


    private void createCamera() {
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
                "";
    }

    @Override
    protected void initShaderHandlers() {
        super.initShaderHandlers();
    }


    @Override
    protected void drawFrame() {
        if (mCameraChanged) {
            if (mCamera != null) {
                mCamera.stopPreview();
                mCamera.release();
                mCamera = null;
            }
        } else {

        }
    }

    @Override
    public void stopPreview() {

    }


    @Override
    public void switchCamera(boolean front) {

    }

    @Override
    public float getMaxZoom() {
        return 0;
    }

    @Override
    public void setZoom(float zoom) {

    }

    @Override
    public void enableAutoFocus(boolean enable) {

    }

    @Override
    public void focus(int width, int height, float x, float y) {

    }

    @Override
    public void setOutputSize(int width, int height) {

    }

    @Override
    public void setFrameRate(float rate) {

    }

    @Override
    public void setFlushMode() {

    }

}
