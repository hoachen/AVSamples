//
// Created by chenhao on 11/21/20.
//
#include <jni.h>
#include <pthread.h>
#include "log.h"
#include "video_split.h"
#include "video_convert_yuv.h"
#include "yuv_player.h"
#include "rplayer.h"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define VIDEO_SPLIT_CLASS "com/av/samples/demux/VideoSplit"
#define YUV_PLAYER_CLASS "com/av/samples/YUVPlayer"
#define REVERSE_PLAYER_CLASS "com/av/samples/ReversePlayer"

static JavaVM *g_jvm;
static pthread_key_t g_thread_key;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;
static jclass g_rplayer_class;
static jmethodID g_method_postEventFromNative;


static jint JNI_split_video(JNIEnv *env, jclass class, jstring input_file, jstring output_dir)
{
    int ret = 0;
    char *input_file_str = (*env)->GetStringUTFChars(env, input_file, 0);
    char *output_dir_str = (*env)->GetStringUTFChars(env, output_dir, 0);
    ret = split_video_by_gop(input_file_str, output_dir_str);
    (*env)->ReleaseStringUTFChars(env, input_file, input_file_str);
    (*env)->ReleaseStringUTFChars(env, output_dir, output_dir_str);
    return ret;
}


static jint JNI_split_video_to_yuv(JNIEnv *env, jclass class, jstring input_file, jstring output_dir)
{
    int ret = 0;
    char *input_file_str = (*env)->GetStringUTFChars(env, input_file, 0);
    char *output_dir_str = (*env)->GetStringUTFChars(env, output_dir, 0);
    ret = decode_to_yuv420(input_file_str, output_dir_str);
    (*env)->ReleaseStringUTFChars(env, input_file, input_file_str);
    (*env)->ReleaseStringUTFChars(env, output_dir, output_dir_str);
    return ret;
}

static JNINativeMethod video_split_methods[] = {
        {"_splitVideo", "(Ljava/lang/String;Ljava/lang/String;)I", JNI_split_video},
        {"_splitVideoToYuv", "(Ljava/lang/String;Ljava/lang/String;)I", JNI_split_video_to_yuv}
};

static void JNI_thread_destroyed(void* value)
{
    JNIEnv *env = (JNIEnv*) value;
    if (env != NULL) {
//        LOGE("%s: [%d] didn't call SDL_JNI_DetachThreadEnv() explicity\n", __func__, (int)gettid());
        (*g_jvm)->DetachCurrentThread(g_jvm);
        pthread_setspecific(g_thread_key, NULL);
    }
}


static void make_thread_key()
{
    pthread_key_create(&g_thread_key, JNI_thread_destroyed);
}

jint JNI_setup_thread_env(JNIEnv **p_env)
{
    JavaVM *jvm = g_jvm;
    if (!jvm) {
        LOGE("jni_setup_thread_env: AttachCurrentThread: NULL jvm");
        return -1;
    }

    pthread_once(&g_key_once, make_thread_key);

    JNIEnv *env = (JNIEnv*) pthread_getspecific(g_thread_key);
    if (env) {
        *p_env = env;
        return 0;
    }
    if ((*jvm)->AttachCurrentThread(jvm, &env, NULL) == JNI_OK) {
        pthread_setspecific(g_thread_key, env);
        *p_env = env;
        return 0;
    }
    return -1;
}


static void post_event(JNIEnv *env, jobject weakThiz, jint what, jint arg1, jint arg2)
{
    LOGI("call postEventFromNative");
    (*env)->CallStaticVoidMethod(env, g_rplayer_class, g_method_postEventFromNative,
            weakThiz, what, arg1, arg2, NULL);
}

static void post_event2(JNIEnv *env, jobject weakThiz, jint what, jint arg1, jint arg2, jobject obj)
{
    LOGI("call postEventFromNative");
    (*env)->CallStaticVoidMethod(env, g_rplayer_class, g_method_postEventFromNative,
                                 weakThiz, what, arg1, arg2, obj);
}



static int message_loop_n(JNIEnv *env, RPlayer *rp)
{
    jobject week_thiz = rplayer_get_week_thiz(rp);
    int ret;
    while (1) {
        AVMessage msg;
        ret = rplayer_get_msg(rp, &msg, 1);
        if (ret < 0) {
            break;
        }
        if (ret > 0) {
            switch (msg.what) {
                case MSG_PREPARED:
                case MSG_VIDEO_SIZE_CHANGED:
                case MSG_COMPLETE:
                case MSG_ERROR:
                case MSG_RENDER_FIRST_FRAME:
                case MSG_PLAYER_STATE_CHANGED:
                    post_event(env, week_thiz, msg.what, msg.arg1, msg.arg2);
                    break;
            }
        }
    }
}



static int message_loop(void *arg)
{
    JNIEnv *env = NULL;
    if (JNI_OK != JNI_setup_thread_env(&env)) {
        LOGE("%s: JNI_setup_thread_env failed\n", __func__);
        return -1;
    }
    RPlayer *rp = (RPlayer *) arg;
    message_loop_n(env, rp);
    return 0;
}


static jlong JNI_rplayer_create(JNIEnv *env, jclass class, jobject week_thiz)
{
    jobject week_obj = (*env)->NewGlobalRef(env, week_thiz);
    RPlayer *rp = rplayer_create(message_loop);
    rplayer_set_week_thiz(rp, week_obj);
    return (intptr_t)rp;
}


static jint JNI_rplayer_init(JNIEnv *env, jclass class, jlong handle, jobject surface,
        jint window_width, jint window_height)
{
    int ret = 0;
    RPlayer *player = (RPlayer *)handle;
    ANativeWindow  *window = ANativeWindow_fromSurface(env, surface);
    ret = rplayer_init(player, window, window_width, window_height);
    return ret;
}


static jint JNI_rplayer_setDataSource(JNIEnv *env, jclass class, jlong handler, jstring path, jstring temp_dir)
{
    int ret = 0;
    RPlayer *player = (RPlayer *)handler;
    char *path_str = (*env)->GetStringUTFChars(env, path, 0);
    char *temp_dir_str = (*env)->GetStringUTFChars(env, temp_dir, 0);
    ret = rplayer_set_data_source(player, path_str, temp_dir_str);
    (*env)->ReleaseStringUTFChars(env, path, path_str);
    (*env)->ReleaseStringUTFChars(env, temp_dir, temp_dir_str);
    return ret;
}


static jint JNI_rplayer_prepare(JNIEnv *env, jclass class, jlong handler)
{
    RPlayer *player = (RPlayer *)handler;
    int ret = 0;
    ret = rplayer_prepare(player);
    return ret;
}


static jint JNI_rplayer_start(JNIEnv *env, jclass class, jlong handler)
{
    RPlayer *player = (RPlayer *)handler;
    int ret = 0;
    ret = rplayer_start(player);
    return ret;
}


static jint JNI_rplayer_pause(JNIEnv *env, jclass class, jlong handler)
{
    RPlayer *player = (RPlayer *)handler;
    int ret = 0;
    ret = rplayer_pause(player);
    return ret;
}

static jint JNI_rplayer_stop(JNIEnv *env, jclass class, jlong handler)
{
    RPlayer *player = (RPlayer *)handler;
    int ret = 0;
    ret = rplayer_stop(player);
    return ret;
}


static jint JNI_rplayer_release(JNIEnv *env, jclass class, jlong handler)
{
    int ret = 0;
    RPlayer *player = (RPlayer *)handler;
    ret = rplayer_release(player);
    jobject weak_thiz = (jobject) rplayer_get_week_thiz(player);
    (*env)->DeleteGlobalRef(env, weak_thiz);
    return ret;
}

static JNINativeMethod rplayer_jni_methods[] = {
        {"_create", "(Ljava/lang/ref/WeakReference;)J", JNI_rplayer_create},
        {"_init", "(JLandroid/view/Surface;II)I", JNI_rplayer_init},
        {"_prepare", "(J)I", JNI_rplayer_prepare},
        {"_setDataSource", "(JLjava/lang/String;Ljava/lang/String;)I", JNI_rplayer_setDataSource},
        {"_start", "(J)I", JNI_rplayer_start},
        {"_pause", "(J)I", JNI_rplayer_pause},
        {"_stop", "(J)I", JNI_rplayer_stop},
        {"_release", "(J)I", JNI_rplayer_release}
};


static jlong JNI_yuv_player_create(JNIEnv *env, jclass class)
{
    YUVPlayer *player = av_mallocz(sizeof(YUVPlayer));
    return (intptr_t)player;
}

static jint JNI_yuv_player_init(JNIEnv *env, jclass class, jlong handle,
         jobject surface, jint window_w, jint window_h, jint video_w, jint video_h, jstring url)
{
    int ret;
    YUVPlayer *player = (YUVPlayer *)handle;
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    char *url_str = (*env)->GetStringUTFChars(env, url, 0);
    ret = yuv_player_init(player, window, window_w, window_h, video_w, video_h, url_str);
    return ret;
}

static jint JNI_yuv_player_prepare(JNIEnv *env, jclass class, jlong handle)
{
    int ret;
    YUVPlayer *player = (YUVPlayer *)handle;
    ret = yuv_player_prepare(player);
    return ret;
}

static jint JNI_yuv_player_start(JNIEnv *env, jclass class, jlong handle)
{
    int ret;
    YUVPlayer *player = (YUVPlayer *)handle;
    ret = yuv_player_start(player);
    return ret;
}

static jint JNI_yuv_player_stop(JNIEnv *env, jclass class, jlong handle)
{
    int ret;
    YUVPlayer *player = (YUVPlayer *)handle;
    ret = yuv_player_stop(player);
    return ret;
}

static jint JNI_yuv_player_release(JNIEnv *env, jclass class, jlong handle)
{
    int ret;
    YUVPlayer *player = (YUVPlayer *)handle;
    ret = yuv_player_release(player);
    return ret;
}


static JNINativeMethod yuv_player_jni_methods[] = {
        {"_create", "()J", JNI_yuv_player_create},
        {"_init", "(JLandroid/view/Surface;IIIILjava/lang/String;)I", JNI_yuv_player_init},
        {"_prepare", "(J)I", JNI_yuv_player_prepare},
        {"_start", "(J)I", JNI_yuv_player_start},
        {"_stop", "(J)I", JNI_yuv_player_stop},
        {"_release", "(J)I", JNI_yuv_player_release}
};

// 动态注册
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    g_jvm = vm;

    jclass video_split_class = (*env)->FindClass(env, VIDEO_SPLIT_CLASS);
    (*env)->RegisterNatives(env, video_split_class, video_split_methods, NELEM(video_split_methods));
    (*env)->DeleteLocalRef(env, video_split_class);

    jclass yuv_player_class = (*env)->FindClass(env, YUV_PLAYER_CLASS);
    (*env)->RegisterNatives(env, yuv_player_class, yuv_player_jni_methods, NELEM(yuv_player_jni_methods));
    (*env)->DeleteLocalRef(env, yuv_player_class);

    jclass rplayer_class = (*env)->FindClass(env, REVERSE_PLAYER_CLASS);
    g_rplayer_class = (*env)->NewGlobalRef(env, rplayer_class);
    const char *method_name = "postEventFromNative";
    const char *sign = "(Ljava/lang/Object;IIILjava/lang/Object;)V";
    g_method_postEventFromNative = (*env)->GetStaticMethodID(env, rplayer_class, method_name, sign);
    if (g_method_postEventFromNative == NULL) {
        LOGE("g_method_postEventFromNative is null");
    }
    (*env)->RegisterNatives(env, rplayer_class, rplayer_jni_methods, NELEM(rplayer_jni_methods));
    (*env)->DeleteLocalRef(env, rplayer_class);
    return JNI_VERSION_1_6;
}