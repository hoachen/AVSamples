//
// Created by chenhao on 12/31/20.
//

#include "gl_renderer.h"

static GLuint load_shader(const char *source, GLint type) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        LOGD("glCreateShader %d failed", type);
        return 0;
    }
    //加载shader
    glShaderSource(shader, 1, &source, 0);
    //编译shader
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        LOGD("glCompileShader %d failed", type);
        LOGD("source %s", source);
        return 0;
    }
    LOGD("glCompileShader %d success", type);
    return shader;
}

static GLuint create_gl_program(const char *vertex_source, const char *fragment_source)
{
    GLuint vertex_shader = load_shader(vertex_source, GL_VERTEX_SHADER);
    GLuint fragment_shader = load_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (vertex_shader == 0 || fragment_shader == 0) {
        return 0;
    }
    GLuint program = glCreateProgram();
    if (program == 0) {
        return 0;
    }
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == 0) {
        LOGD("glLinkProgram failed");
        glDeleteProgram(program);
        return 0;
    }
    LOGD("glLinkProgram success %d", program);
    return program;
}

static int init_egl_context(GLRenderer *renderer, ANativeWindow *window)
{
    renderer->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (renderer->eglDisplay == EGL_NO_DISPLAY) {
        LOGI("egl display failed");
        return -1;
    }
    //2.初始化egl，后两个参数为主次版本号
    if (EGL_TRUE != eglInitialize(renderer->eglDisplay, 0, 0)) {
        LOGD("eglInitialize failed");
        return -1;
    }
    //3.1 surface配置，可以理解为窗口
    EGLConfig eglConfig;
    EGLint configNum;
    EGLint configSpec[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };
    if (EGL_TRUE != eglChooseConfig(renderer->eglDisplay, configSpec, &eglConfig, 1, &configNum)) {
        LOGD("eglChooseConfig failed");
        return -1;
    }

    //3.2创建surface(egl和NativeWindow进行关联。最后一个参数为属性信息，0表示默认版本)
    renderer->eglSurface = eglCreateWindowSurface(renderer->eglDisplay, eglConfig, window, 0);
    if (renderer->eglSurface == EGL_NO_SURFACE) {
        LOGD("eglCreateWindowSurface failed");
        return -1;
    }
    //4 创建关联上下文
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };
    //EGL_NO_CONTEXT表示不需要多个设备共享上下文
    renderer->eglContext = eglCreateContext(renderer->eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttr);
    if (renderer->eglContext == EGL_NO_CONTEXT) {
        LOGD("eglCreateContext failed");
        return -1;
    }
    if (EGL_TRUE != eglMakeCurrent(renderer->eglDisplay, renderer->eglSurface, renderer->eglSurface, renderer->eglContext)) {
        LOGD("eglMakeCurrent failed");
        return -1;
    }
    renderer->program = create_gl_program(VERTEX_SHADER, FRAGMENT_SHADER);
    if (renderer->program == 0) {
        LOGD("create egl program failed");
        return -1;
    }
    return 0;
}

static void update_texture(GLint textureId, int width, int height, const void *pixels)
{
    //绑定纹理
    glBindTexture(GL_TEXTURE_2D, textureId);
    //缩小的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //设置纹理的格式和大小
    glTexImage2D(GL_TEXTURE_2D,
                 0,//细节基本 默认0
                 GL_LUMINANCE,//gpu内部格式 亮度，灰度图（这里就是只取一个颜色通道的意思）
                 width,
                 height,//v数据数量为屏幕的4分之1
                 0,//边框
                 GL_LUMINANCE,//数据的像素格式 亮度，灰度图
                 GL_UNSIGNED_BYTE,//像素点存储的数据类型
                 pixels); //纹理的数据（先不传）
}

static int init_program_handle(GLRenderer *renderer)
{
    LOGI("%s, program is %d", __func__ , renderer->program);
    glUseProgram(renderer->program);

    renderer->position_handle = glGetAttribLocation(renderer->program, "aPosition");

    renderer->textCoord_handle = glGetAttribLocation(renderer->program, "aTextCoord");

    glGenTextures(3, renderer->textures);

}

int gl_renderer_init(GLRenderer *renderer, ANativeWindow *window, int width, int height)
{
    int ret;
    renderer->window_width = width;
    renderer->window_height = height;
    renderer->video_width = 0;
    renderer->video_height = 0;
    ret = init_egl_context(renderer, window);
    if (ret < 0) {
        return -1;
    }
    init_program_handle(renderer);
    return 0;
}

static float ver[8];

static void reset_vertex_data(GLRenderer *renderer)
{
    glUseProgram(renderer->program);
    float scale_width = 1.0f;
    float scale_height = 1.0f;
    if (renderer->video_width > 0 && renderer->video_height > 0) {
        float render_ratio = renderer->window_width * 1.0 / renderer->window_height;
        float video_ratio = renderer->video_width * 1.0 / renderer->video_height;
        if (video_ratio > render_ratio) {
            scale_height = render_ratio / video_ratio;
            scale_width = 1.0f;
        } else {
            scale_width = video_ratio / render_ratio;
            scale_height = 1.0f;
        }
    }
//    static float ver[] = {
//        scale_width, -scale_height,
//        -scale_width, -scale_height,
//        scale_width, scale_height,
//        -scale_width, scale_height
//    };

//    ver[0] = {
//            1.0f, -1.0f,
//            -1.0f, -1.0f,
//            1.0f, 1.0f,
//            -1.0f, 1.0f
//    };
    ver[0] = scale_width;
    ver[1] = -scale_height;
    ver[2] = -scale_width;
    ver[3] = -scale_height;
    ver[4] = scale_width;
    ver[5] = scale_height;
    ver[6] = -scale_width;
    ver[7] = scale_height;

    glEnableVertexAttribArray(renderer->position_handle);
    glVertexAttribPointer(renderer->position_handle, 2, GL_FLOAT, GL_FALSE, 0, ver);
    LOGI("%s, position_handle is %d", __func__ , renderer->position_handle);
    //加入纹理坐标数据
    static float fragment[] = {
            1.0f, 0.0f,
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
    };
    glEnableVertexAttribArray(renderer->textCoord_handle);
    glVertexAttribPointer(renderer->textCoord_handle, 2, GL_FLOAT, GL_FALSE, 0, fragment);
    LOGI("%s, textCoord_handle is %d", __func__ , renderer->textCoord_handle);
    glUniform1i(glGetUniformLocation(renderer->program, "yTexture"), 0);
    glUniform1i(glGetUniformLocation(renderer->program, "uTexture"), 1);
    glUniform1i(glGetUniformLocation(renderer->program, "vTexture"), 2);
}

int gl_renderer_render(GLRenderer *renderer, unsigned char **buffer, int video_width, int video_height)
{
    LOGI("%s frame %dx%d", __func__, video_width, video_height);
    if (EGL_TRUE != eglMakeCurrent(renderer->eglDisplay, renderer->eglSurface, renderer->eglSurface, renderer->eglContext)) {
        LOGE("eglMakeCurrent failed");
        return -1;
    }
    glViewport(0, 0, renderer->window_width, renderer->window_height);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (renderer->video_width != video_width || renderer->video_height != video_height) {
        renderer->video_width = video_width;
        renderer->video_height = video_height;
        reset_vertex_data(renderer);
    }
    glActiveTexture(GL_TEXTURE0);
    update_texture(renderer->textures[0], video_width, video_height, buffer[0]);
    glActiveTexture(GL_TEXTURE1);
    update_texture(renderer->textures[1], video_width / 2, video_height / 2, buffer[1]);
    glActiveTexture(GL_TEXTURE2);
    update_texture(renderer->textures[2], video_width / 2, video_height / 2, buffer[2]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    eglSwapBuffers(renderer->eglDisplay, renderer->eglSurface);
    return 0;
}

void gl_renderer_destroy(GLRenderer *renderer)
{
    glDeleteTextures(3, renderer->textures);
    glDeleteProgram(renderer->program);
}