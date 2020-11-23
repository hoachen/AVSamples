package com.av.samples.gles;


import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLContext;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.Surface;

public class GLRenderThread {

    private EglCore mEglCore;
    private EglSurfaceBase mEglSurface;

    private HandlerThread mThread;
    private Handler mHandler;
    private boolean mPaused;

    public GLRenderThread() {
        mThread = new HandlerThread("GLThread");
        mThread.start();
        mHandler = new Handler(mThread.getLooper());
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                initEGLContext(EGL14.EGL_NO_CONTEXT);
            }
        });
    }

    public void pause() {
        mPaused = true;
        mHandler.removeCallbacks(null);
    }

    public void resume() {
        mPaused = false;
    }

    public void runTask(Runnable runnable) {
        if (!mPaused) {
            mHandler.post(runnable);
        }
    }

    public void initEGLContext(EGLContext sharedContext) {
        mEglCore = new EglCore(sharedContext, 0);
        mEglSurface = new OffscreenSurface(mEglCore, 1, 1);
        mEglSurface.makeCurrent();
    }

    public void attachSurface(Surface surface) {
        mEglSurface = new WindowSurface(mEglCore, surface, false);
        mEglSurface.makeCurrent();
    }

    public void attachSurfaceTexture(SurfaceTexture surface) {
        mEglSurface = new WindowSurface(mEglCore, surface);
        mEglSurface.makeCurrent();
    }

    public void swapBuffers() {
        if (mEglSurface != null) {
            mEglSurface.swapBuffers();
        }
    }

    public void destroy() {
        mHandler.removeCallbacks(null);
        mThread.quitSafely();
        mThread = null;
        mEglSurface.releaseEglSurface();
    }

}
