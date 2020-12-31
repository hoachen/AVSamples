//
// Created by chenhao on 12/31/20.
//

#ifndef AVSAMPLES_GL_RENDERER_H
#define AVSAMPLES_GL_RENDERER_H

#include <jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/native_window_jni.h>
#include "log.h"

//顶点着色器，每个顶点执行一次，可以并行执行
#define GET_STR(x) #x

static const char *VERTEX_SHADER = GET_STR(
        attribute vec4 aPosition;
        attribute vec2 aTextCoord;
        varying vec2 vTextCoord;
        void main() {
            vTextCoord = vec2(aTextCoord.x, 1.0 - aTextCoord.y);
            gl_Position = aPosition;
        }
);

//图元被光栅化为多少片段，就被调用多少次
static const char *FRAGMENT_SHADER = GET_STR(
        precision mediump float;
        varying vec2 vTextCoord;
        //输入的yuv三个纹理
        uniform sampler2D yTexture;
        uniform sampler2D uTexture;
        uniform sampler2D vTexture;
        void main() {
            vec3 yuv;
            vec3 rgb;
            yuv.x = texture2D(yTexture, vTextCoord).g;
            yuv.y = texture2D(uTexture, vTextCoord).g - 0.5;
            yuv.z = texture2D(vTexture, vTextCoord).g - 0.5;
            rgb = mat3(
                    1.0, 1.0, 1.0,
                    0.0, -0.39465, 2.03211,
                    1.13983, -0.5806, 0.0
            ) * yuv;
            //gl_FragColor是OpenGL内置的
            gl_FragColor = vec4(rgb, 1.0);
        }
);

typedef struct GLRenderer {
    int window_width;
    int window_height;
    EGLContext eglContext;
    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    GLuint program;
    GLint position_handle;
    GLint textCoord_handle;
    GLuint textures[3]; // y_texture, u_texture, v_texture
} GLRenderer;

int gl_renderer_init(GLRenderer *renderer, ANativeWindow *window, int window_width, int window_height);

int gl_renderer_render(GLRenderer *renderer, unsigned char *buffer[], int video_width, int height);

void gl_renderer_destroy(GLRenderer *renderer);

#endif //AVSAMPLES_GL_RENDERER_H
