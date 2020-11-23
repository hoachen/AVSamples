package com.av.samples.gles;

import android.opengl.GLES20;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public abstract class GLRenderer {

    public static final String ATTRIBUTE_POSITION = "a_Positon";
    public static final String ATTRIBUTE_TEXCOORD = "a_TexCoord";
    public static final String VARYING_TEXCOORD = "v_TexCoord";
    protected static final String UNIFORM_TEXTUREBASE = "u_Texture";

    public static final String UNIFORM_TEXTURE0 = UNIFORM_TEXTUREBASE+0;

    private boolean mInitialized;
    protected FloatBuffer mVertexBuffer;
    protected FloatBuffer[] mTextureVertices; // 支持多角度的纹理顶点坐标
    protected int mVertexShader;
    protected int mFragmentShader;
    protected int mProgram;

    protected int mPositionHandle;
    protected int mTextureHandle;
    protected int mTexCordHandle;

    protected int mInputTextureId;
    // 旋转角度
    protected int mCurRotation;
    protected int mRenderWidth;
    protected int mRenderHeight;

    private float mRed;
    private float mGreen;
    private float mBlue;
    private float mAlpha;

    public GLRenderer() {
        setRenderVertices(new float[] {
                -1f, -1f,
                1f, -1f,
                -1f, 1f,
                1f, 1f,
        });
        mTextureVertices = new FloatBuffer[4];
        setTextVertices(0.0f, 0.0f, 1.0f, 1.0f);
        mCurRotation = 0;

    }

    /**
     * 设置顶点坐标
     * @param vertices
     */
    protected void setRenderVertices(float vertices[]) {
        mVertexBuffer = ByteBuffer.allocateDirect(vertices.length * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
        mVertexBuffer.put(vertices).position(0);
    }

    protected void setTextVertices(float minX, float minY, float maxX, float maxY) {
        float[] textVertices0 = new float[] {
                minX, minY,
                maxX, minY,
                minX, maxY,
                maxX, maxY
        };
        mTextureVertices[0] = ByteBuffer.allocateDirect(textVertices0.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        mTextureVertices[0].put(textVertices0).position(0);
        // 旋转90度
        float[] textVertices1 = new float[] {
                maxX, minY,
                maxX, maxY,
                minX, minY,
                minX, maxY
        };
        mTextureVertices[1] = ByteBuffer.allocateDirect(textVertices1.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        mTextureVertices[1].put(textVertices1).position(0);

        float[] textVertices2 = new float[] {
                maxX, maxY,
                minX, maxY,
                minX, minY,
                maxX, minY
        };
        mTextureVertices[2] = ByteBuffer.allocateDirect(textVertices2.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        mTextureVertices[2].put(textVertices2).position(0);

        float[] textVertices3 = new float[] {
                minX, maxY,
                minX, minY,
                maxX, minY,
                maxX, maxY
        };
        mTextureVertices[3] = ByteBuffer.allocateDirect(textVertices3.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        mTextureVertices[3].put(textVertices3).position(0);

    }

    public boolean initialized() {
        return mInitialized;
    }

    protected void onDrawFrame() {
        // 需要执行EGLContext环境
        if (!mInitialized) {
            initGLContext();
            mInitialized = true;
        }
        drawFrame();
    }

    protected void drawFrame() {
        if (mInputTextureId < 0) {
            return;
        }
        GLES20.glClearColor(mRed, mGreen, mBlue, mAlpha);
        GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glViewport(0, 0, getWidth(), getHeight());
        GLES20.glUseProgram(mProgram);
        passShaderValues();
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    }


    protected void initGLContext() {
        mVertexShader = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER);
        if (mVertexShader <= 0) {
            throw new RuntimeException(this + ": Could not create vertex shader. Reason: "+GLES20.glGetError());
        }
        String errorInfo = "none";
        if (mVertexShader > 0) {
            GLES20.glShaderSource(mVertexShader, getVertexShader());
            GLES20.glCompileShader(mVertexShader);
            int compileStatus[] = new int[1];
            GLES20.glGetShaderiv(mVertexShader, GLES20.GL_COMPILE_STATUS, compileStatus, 0);
            if (compileStatus[0] == 0) {
                errorInfo = GLES20.glGetShaderInfoLog(mVertexShader);
                GLES20.glDeleteShader(mVertexShader);
                mVertexShader = 0;
            }
        }
        if (mVertexShader == 0) {
            throw new RuntimeException(this + ": Could not compile vertex shader. Reason: " + errorInfo);
        }
        mFragmentShader = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);
        if (mFragmentShader == 0) {
            throw new RuntimeException(this + ": Could not create fragment shader. Reason: "+GLES20.glGetError());
        }
        if (mFragmentShader > 0) {
            GLES20.glShaderSource(mFragmentShader, getFragmentShader());
            GLES20.glCompileShader(mFragmentShader);
            int compileStatus[] = new int[1];
            GLES20.glGetShaderiv(mFragmentShader, GLES20.GL_COMPILE_STATUS, compileStatus, 0);
            if (compileStatus[0] == 0) {
                errorInfo = GLES20.glGetShaderInfoLog(mFragmentShader);
                GLES20.glDeleteShader(mFragmentShader);
                mFragmentShader = 0;
            }
        }
        if (mFragmentShader == 0) {
            throw new RuntimeException(this + ": Could not compile fragment shader. Reason: "+ errorInfo);
        }
        mProgram = GLES20.glCreateProgram();
        if (mProgram == 0) {
            throw new RuntimeException(this + ": Could create program. Reason" + GLES20.glGetError());
        }
        GLES20.glAttachShader(mProgram, mVertexShader);
        GLES20.glAttachShader(mProgram, mFragmentShader);
        bindShaderAttributes();
        GLES20.glLinkProgram(mProgram);
        int linkStatus[] = new int[1];
        GLES20.glGetShaderiv(mProgram, GLES20.GL_LINK_STATUS, linkStatus, 0);
        if (linkStatus[0] == 0) {
            GLES20.glDeleteProgram(mProgram);
            mProgram = 0;
        }
        if (mProgram == 0) {
            throw new RuntimeException(this + ": Could Link program. Reason" + GLES20.glGetError());
        }
        initShaderHandlers();
    }

    protected void bindShaderAttributes() {
        GLES20.glBindAttribLocation(mProgram, 0, ATTRIBUTE_POSITION);
        GLES20.glBindAttribLocation(mProgram, 1, ATTRIBUTE_TEXCOORD);
    }


    protected void initShaderHandlers() {
        mTextureHandle = GLES20.glGetUniformLocation(mProgram, UNIFORM_TEXTURE0);
        mPositionHandle = GLES20.glGetAttribLocation(mProgram, ATTRIBUTE_POSITION);
        mTexCordHandle = GLES20.glGetAttribLocation(mProgram, ATTRIBUTE_TEXCOORD);
    }

    protected void passShaderValues() {
        // 传入顶点位置
        mVertexBuffer.position(0);
        GLES20.glVertexAttribPointer(mPositionHandle, 2, GLES20.GL_FLOAT, false, 0, mVertexBuffer);
        GLES20.glEnableVertexAttribArray(mPositionHandle);
        // 传入纹理坐标位置
        mTextureVertices[mCurRotation].position(0);
        GLES20.glVertexAttribPointer(mTexCordHandle, 2, GLES20.GL_FLOAT, false, 0, mTextureVertices[mCurRotation]);
        GLES20.glEnableVertexAttribArray(mTexCordHandle);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mInputTextureId);
        GLES20.glUniform1i(mTextureHandle, 0);
    }

    protected String getVertexShader() {
        return  "attribute vec4 " + ATTRIBUTE_POSITION + ";\n" +
                "attribute vec2 " + ATTRIBUTE_TEXCOORD + ";\n" +
                "varying vec2 " + VARYING_TEXCOORD + ";\n" +
                "void main() {\n" +
                "   " + VARYING_TEXCOORD + "= " + ATTRIBUTE_TEXCOORD + ";\n" +
                "   gl_Position = " + ATTRIBUTE_POSITION + ";\n" +
                "}";
    }

    protected String getFragmentShader() {
        return "precision mediump float;\n" +
                "uniform sample2D " + UNIFORM_TEXTURE0 + ";\n" +
                "varying vec2 " + VARYING_TEXCOORD + ";\n" +
                "void main() {\n" +
                "   gl_FragColor = texture2D(" + UNIFORM_TEXTURE0 + "," + VARYING_TEXCOORD + ");\n" +
                "}";
    }

    /**
     * Sets the render size of the renderer to the given mRenderWidth and mRenderHeight.
     * This also prevents the size of the renderer from changing automatically
     * when one of the source(s) of the renderer has a size change.  If the renderer
     * has been rotated an odd number of times, the mRenderWidth and mRenderHeight will be swapped.
     * @param width
     * The mRenderWidth at which the renderer should draw at.
     * @param height
     * The mRenderHeight at which the renderer should draw at.
     */
    public void setRenderSize(int width, int height) {
        if(mCurRotation % 2 == 1) { // 0= 0 ,1=90, 2 = 180. 270 = 3
            this.mRenderWidth = height;
            this.mRenderHeight = width;
        } else {
            this.mRenderWidth = width;
            this.mRenderHeight = height;
        }
    }

    protected int getWidth() {
        return mRenderWidth;
    }

    protected int getHeight() {
        return mRenderHeight;
    }


    /**
     * Sets the background colour for this GLRenderer to the given colour in rgba space.
     * @param red
     * The red component of the colour.
     * @param green
     * The green component of the colour.
     * @param blue
     * The blue component of the colour.
     * @param alpha
     * The alpha component of the colour.
     */
    public void setBackgroundColour(float red, float green, float blue, float alpha) {
        this.setBackgroundRed(red);
        this.setBackgroundGreen(green);
        this.setBackgroundBlue(blue);
        this.setBackgroundAlpha(alpha);
    }

    /**
     * Returns the red component of the background colour currently set for this GLRenderer.
     * @return red
     * The red component of the background colour.
     */
    public float getBackgroundRed() {
        return mRed;
    }

    /**
     * Sets only the red component of the background colour currently set for this GLRenderer.
     * @param red
     * The red component to set as the background colour.
     */
    public void setBackgroundRed(float red) {
        this.mRed = red;
    }

    /**
     * Returns the green component of the background colour currently set for this GLRenderer.
     * @return green
     * The green component of the background colour.
     */
    public float getBackgroundGreen() {
        return mGreen;
    }

    /**
     * Sets only the green component of the background colour currently set for this GLRenderer.
     * @param green
     * The green component to set as the background colour.
     */
    public void setBackgroundGreen(float green) {
        this.mGreen = green;
    }

    /**
     * Returns the blue component of the background colour currently set for this GLRenderer.
     * @return blue
     * The blue component of the background colour.
     */
    public float getBackgroundBlue() {
        return mBlue;
    }

    /**
     * Sets only the blue component of the background colour currently set for this GLRenderer.
     * @param blue
     * The blue component to set as the background colour.
     */
    public void setBackgroundBlue(float blue) {
        this.mBlue = blue;
    }

    /**
     * Returns the alpha component of the background colour currently set for this GLRenderer.
     * @return alpha
     * The alpha component of the background colour.
     */
    public float getBackgroundAlpha() {
        return mAlpha;
    }

    /**
     * Sets only the alpha component of the background colour currently set for this GLRenderer.
     * @param alpha
     * The alpha component to set as the background colour.
     */
    public void setBackgroundAlpha(float alpha) {
        this.mAlpha = alpha;
    }

    public void destroy() {
        Log.e("GLRenderer", "GLRenderer destroyed.");
        mInitialized = false;
        if(mProgram != 0) {
            GLES20.glDeleteProgram(mProgram);
            mProgram = 0;
        }
        if(mVertexShader != 0) {
            GLES20.glDeleteShader(mVertexShader);
            mVertexShader = 0;
        }
        if(mFragmentShader != 0) {
            GLES20.glDeleteShader(mFragmentShader);
            mFragmentShader = 0;
        }
    }

}
