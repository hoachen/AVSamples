//
// Created by chenhao on 11/21/20.
//
#include <jni.h>
#include "log.h"
#include "video_split.h"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define MAIN_CLASS  "com/av/samples/MainActivity"
#define VIDEO_SPLIT_CLASS "com/av/samples/demux/VideoSplit"

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

static JNINativeMethod video_split_methods[] = {
        {"_splitVideo", "(Ljava/lang/String;Ljava/lang/String;)I", JNI_split_video}
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
    return JNI_VERSION_1_6;
}