//
// Created by chenhao on 11/21/20.
//
#include <jni.h>
#include "log.h"
#include "video_split.h"
#include "video_split_yuv.h"
#include "xplayer.h"
#include "yuv_player.h"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define MAIN_CLASS  "com/av/samples/MainActivity"
#define VIDEO_SPLIT_CLASS "com/av/samples/demux/VideoSplit"
#define XPLAYER_CLASS "com/av/samples/XPlayer"
#define YUV_PLAYER_CLASS "com/av/samples/YUVPlayer"

// 静态注册
//JNIEXPORT jstring JNICALL
//Java_com_av_samples_MainActivity_stringFromJNI(JNIEnv *env, jobject thiz) {
//    // TODO: implement stringFromJNI()
//    char *hello = "Hello World From C";
//    LOGI("%s", hello);
//    return (*env)->NewStringUTF(env, hello);
//}


static jstring JNI_string_helloworld(JNIEnv *env, jclass class)
{
    char *hello = "hello world";
    LOGI("%s", hello);
    return (*env)->NewStringUTF(env, hello);
}

static JNINativeMethod main_methods[] = {
        {"stringFromJNI", "()Ljava/lang/String;", JNI_string_helloworld}
};

//
static jint JNI_split_video(JNIEnv *env, jclass class, jstring input_file, jstring output_dir)
{
    int ret = 0;
    char *input_file_str = (*env)->GetStringUTFChars(env, input_file, 0);
    char *output_dir_str = (*env)->GetStringUTFChars(env, output_dir, 0);
    ret = split_video(input_file_str, output_dir_str);
    (*env)->ReleaseStringUTFChars(env, input_file, input_file_str);
    (*env)->ReleaseStringUTFChars(env, output_dir, output_dir_str);
    return ret;
}


static jint JNI_split_video_to_yuv(JNIEnv *env, jclass class, jstring input_file, jstring output_dir)
{
    int ret = 0;
    char *input_file_str = (*env)->GetStringUTFChars(env, input_file, 0);
    char *output_dir_str = (*env)->GetStringUTFChars(env, output_dir, 0);
    ret = split_yuv(input_file_str, output_dir_str);
    (*env)->ReleaseStringUTFChars(env, input_file, input_file_str);
    (*env)->ReleaseStringUTFChars(env, output_dir, output_dir_str);
    return ret;
}

static JNINativeMethod video_split_methods[] = {
        {"_splitVideo", "(Ljava/lang/String;Ljava/lang/String;)I", JNI_split_video},
        {"_splitVideoToYuv", "(Ljava/lang/String;Ljava/lang/String;)I", JNI_split_video_to_yuv}
};


static long JNI_player_create(JNIEnv *env, jclass class)
{
    XPlayer *player = av_mallocz(sizeof(XPlayer));
    player_init(player);
    return (intptr_t)player;
}


static int JNI_player_setDataSource(JNIEnv *env, jclass class, jlong handler, jstring path)
{
    XPlayer *player = (XPlayer *)handler;
    int ret = 0;
    if (!player)
        return -1;
    char *path_str = (*env)->GetStringUTFChars(env, path, 0);
    ret = player_set_data_source(player, path_str);
    (*env)->ReleaseStringUTFChars(env, path, path_str);
    return ret;
}


static int JNI_player_prepare(JNIEnv *env, jclass class, jlong handler)
{
    XPlayer *player = (XPlayer *)handler;
    int ret = 0;
    if (!player)
        return -1;
    ret = player_prepare(player);
    return ret;
}



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
    jclass main_class = (*env)->FindClass(env, MAIN_CLASS);
    (*env)->RegisterNatives(env, main_class, main_methods, NELEM(main_methods));
    (*env)->DeleteLocalRef(env, main_class);

    jclass video_split_class = (*env)->FindClass(env, VIDEO_SPLIT_CLASS);
    (*env)->RegisterNatives(env, video_split_class, video_split_methods, NELEM(video_split_methods));
    (*env)->DeleteLocalRef(env, video_split_class);

    jclass yuv_player_class = (*env)->FindClass(env, YUV_PLAYER_CLASS);
    (*env)->RegisterNatives(env, yuv_player_class, yuv_player_jni_methods, NELEM(yuv_player_jni_methods));
    (*env)->DeleteLocalRef(env, yuv_player_class);

    return JNI_VERSION_1_6;
}