//
// Created by chenhao on 11/21/20.
//
#include <jni.h>
#include "log.h"
#include "video_split.h"
#include "video_convert_yuv.h"
#include "yuv_player.h"
#include "rplayer.h"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define MAIN_CLASS  "com/av/samples/MainActivity"
#define VIDEO_SPLIT_CLASS "com/av/samples/demux/VideoSplit"
#define YUV_PLAYER_CLASS "com/av/samples/YUVPlayer"
#define REVERSE_PLAYER_CLASS "com/av/samples/ReversePlayer"

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
    VideoInfo video_info;
    ret = convert_to_yuv420(input_file_str, output_dir_str, &video_info);
    (*env)->ReleaseStringUTFChars(env, input_file, input_file_str);
    (*env)->ReleaseStringUTFChars(env, output_dir, output_dir_str);
    return ret;
}

static JNINativeMethod video_split_methods[] = {
        {"_splitVideo", "(Ljava/lang/String;Ljava/lang/String;)I", JNI_split_video},
        {"_splitVideoToYuv", "(Ljava/lang/String;Ljava/lang/String;)I", JNI_split_video_to_yuv}
};


static jlong JNI_rplayer_create(JNIEnv *env, jclass class)
{
    RPlayer *player = av_mallocz(sizeof(RPlayer));
    return (intptr_t)player;
}

static int msg_loop_n(void *arg)
{

}

static jint JNI_rplayer_init(JNIEnv *env, jclass class, jlong handle, jobject surface,
        jint window_width, jint window_height)
{
    int ret = 0;
    RPlayer *player = (RPlayer *)handle;
    ANativeWindow  *window = ANativeWindow_fromSurface(env, surface);
    ret = rplayer_init(player, window, window_width, window_height, msg_loop_n);
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
    RPlayer *player = (RPlayer *)handler;
    int ret = 0;
    ret = rplayer_release(player);
    return ret;
}

static JNINativeMethod rplayer_jni_methods[] = {
        {"_create", "()J", JNI_rplayer_create},
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
    jclass main_class = (*env)->FindClass(env, MAIN_CLASS);
    (*env)->RegisterNatives(env, main_class, main_methods, NELEM(main_methods));
    (*env)->DeleteLocalRef(env, main_class);

    jclass video_split_class = (*env)->FindClass(env, VIDEO_SPLIT_CLASS);
    (*env)->RegisterNatives(env, video_split_class, video_split_methods, NELEM(video_split_methods));
    (*env)->DeleteLocalRef(env, video_split_class);

    jclass yuv_player_class = (*env)->FindClass(env, YUV_PLAYER_CLASS);
    (*env)->RegisterNatives(env, yuv_player_class, yuv_player_jni_methods, NELEM(yuv_player_jni_methods));
    (*env)->DeleteLocalRef(env, yuv_player_class);

    jclass rplayer_class = (*env)->FindClass(env, REVERSE_PLAYER_CLASS);
    (*env)->RegisterNatives(env, rplayer_class, rplayer_jni_methods, NELEM(rplayer_jni_methods));
    (*env)->DeleteLocalRef(env, rplayer_class);
    return JNI_VERSION_1_6;
}