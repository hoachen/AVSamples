package com.av.samples.gles;


import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLContext;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Surface;

public class GLRenderThread{

    private static final String TAG = "GLRenderThread";
    private EglCore mEglCore;
    private EglSurfaceBase mEglSurface;

    private HandlerThread mThread;
    private Handler mHandler;
    private boolean mPaused;

    private static GLRenderThread sInstance;

    public static GLRenderThread sharedThread() {
        if (sInstance == null) {
            synchronized (GLRenderThread.class) {
                if (sInstance == null) {
                    sInstance = new GLRenderThread();
                }
            }
        }
        return sInstance;
    }

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

    public void runOnGLThread(Runnable runnable) {
        if (!mPaused) {
            mHandler.post(runnable);
        }
    }

    public void initEGLContext(EGLContext sharedContext) {
        Log.i(TAG, "initEGLContext");
        mEglCore = new EglCore(sharedContext, 0);
        mEglSurface = new OffscreenSurface(mEglCore, 1, 1);
        mEglSurface.makeCurrent();
    }

    public void attachSurface(Surface surface) {
        Log.i(TAG, "attachSurface Surface");
        mEglSurface = new WindowSurface(mEglCore, surface, false);
        mEglSurface.makeCurrent();
    }

    public void attachSurfaceTexture(SurfaceTexture surface) {
        Log.i(TAG, "attachSurfaceTexture SurfaceTexture");
        mEglSurface = new WindowSurface(mEglCore, surface);
        mEglSurface.makeCurrent();
    }

    public void swapBuffers() {
        Log.i(TAG, "swapBuffers");
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
