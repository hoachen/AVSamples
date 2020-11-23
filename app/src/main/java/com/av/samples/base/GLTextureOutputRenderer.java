package com.av.samples.base;

import android.opengl.GLES20;

import com.av.samples.gles.GLRenderer;

import java.util.ArrayList;
import java.util.List;


public abstract class GLTextureOutputRenderer extends GLRenderer {

    protected long mCurTimestampus;

    protected int[] frameBuffer;

    protected int[] depthRenderBuffer;

    protected int[] texture_out;

    protected int mTextureOut;

    protected final Object mLock = new Object();

    protected boolean mDirty;

    private int mFrameWidth;

    private int mFrameHeight;

    protected List<GLTextureInputRenderer> mTargets;

    protected List<Integer> mTextureIds;

    public GLTextureOutputRenderer() {
        frameBuffer = null;
        mTextureIds = new ArrayList<>();
        mTargets = new ArrayList<>();
    }

    public void addTarget(GLTextureInputRenderer target) {
        addTargetAtTextureLocation(target, target.nextAvalibleTextureIndices());
    }

    public void addTargetAtTextureLocation(GLTextureInputRenderer target, int textureLocation){
        synchronized(mLock) {
            if (target != this && !mTargets.contains(target)) {
                mTargets.add(target);
                mTextureIds.add(textureLocation);
            }
            target.registerTextureIndices(textureLocation, this);
        }
    }

    public void removeTarget(GLTextureInputRenderer target) {
        synchronized (mLock) {
            int index = mTargets.indexOf(target);
            if (index >= 0) {
                target.unregisterTextureIndices(mTextureIds.get(index));
                mTargets.remove(index);
                mTextureIds.remove(index);
            }
        }
    }

    public void removeAllTarget() {
        synchronized (mLock) {
            for (GLTextureInputRenderer target : mTargets) {
                int i = mTargets.indexOf(target);
                if (i>= 0) {
                    target.unregisterTextureIndices(mTextureIds.get(i));
                }
            }
            mTargets.clear();
            mTextureIds.clear();
        }
    }

    protected void makeAsDirty() {
        mDirty = true;
    }

    private void initFBO() {
        releaseFBO();
        frameBuffer = new int[1];
        texture_out = new int[1];
        depthRenderBuffer = new int[1];
        GLES20.glGenFramebuffers(1, frameBuffer, 0);
        GLES20.glGenRenderbuffers(1, depthRenderBuffer, 0);
        GLES20.glGenTextures(1, texture_out, 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, frameBuffer[0]);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture_out[0]);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, getWidth(), getHeight(),
                0, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, texture_out[0], 0);
        GLES20.glBindRenderbuffer(GLES20.GL_RENDERBUFFER, depthRenderBuffer[0]);
        GLES20.glRenderbufferStorage(GLES20.GL_RENDERBUFFER, GLES20.GL_DEPTH_COMPONENT16, getWidth(), getHeight());
        GLES20.glFramebufferRenderbuffer(GLES20.GL_FRAMEBUFFER, GLES20.GL_DEPTH_ATTACHMENT, GLES20.GL_RENDERBUFFER, depthRenderBuffer[0]);
        int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
        if (status != GLES20.GL_FRAMEBUFFER_COMPLETE) {
            //throw new RuntimeException(this+": Failed to set up render buffer with status "+status+" and error "+GLES20.glGetError());
        }
    }

    public void releaseFBO() {
        if(frameBuffer != null) {
            GLES20.glDeleteFramebuffers(1, frameBuffer, 0);
            frameBuffer = null;
        }
        if(texture_out != null) {
            GLES20.glDeleteTextures(1, texture_out, 0);
            texture_out = null;
        }
        if(depthRenderBuffer != null) {
            GLES20.glDeleteRenderbuffers(1, depthRenderBuffer, 0);
            depthRenderBuffer = null;
        }
        mFrameWidth = 0;
        mFrameHeight = 0;
    }


    @Override
    protected void drawFrame() {
        long before = System.nanoTime();
        if (frameBuffer == null || getWidth() != mFrameWidth || getHeight() != mFrameHeight) {
            initFBO();
            mFrameWidth = getWidth();
            mFrameHeight = getHeight();
        }
        boolean newData = false;
        if (mDirty) {
            newData = true;
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, frameBuffer[0]);
            super.drawFrame();
            afterDraw();
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
            mDirty = false;
        }
        synchronized (mLock) {
            for(GLTextureInputRenderer target : mTargets) {
                target.newTextureReady(getTextureOut(), this, newData, mCurTimestampus);
            }
        }
    }

    protected void afterDraw(){
    }

    public void setTextureOut(int texture) {
        mTextureOut = texture;
    }

    public int getTextureOut() {
        if (mTextureOut != 0) {
            return mTextureOut;
        }
        if (texture_out != null) {
            return texture_out[0];
        }
        return -1;
    }

    public void destroy() {
        super.destroy();
        releaseFBO();;
    }

}
