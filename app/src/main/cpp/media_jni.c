//
// Created by chenhao on 11/21/20.
//
#include <jni.h>
#include "log.h"
#include "audio_decode.h"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define MAIN_CLASS  "com/av/samples/MainActivity"
#define AUDIO_DECODER_CLASS "com/av/samples/decoder/AudioDecoder"

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

static jint JNI_decoder_audio(JNIEnv *env, jclass class, jstring src, jstring des) {
    int ret = 0;
    char *src_file_str = (*env)->GetStringUTFChars(env, src, 0);
    char *des_file_str = (*env)->GetStringUTFChars(env, des, 0);
    ret = decode_audio(src_file_str, des_file_str);
    LOGI("decoder audio ret = %d", ret);
    (*env)->ReleaseStringUTFChars(env, src, src_file_str);
    (*env)->ReleaseStringUTFChars(env, des, des_file_str);
    return ret;
}

static JNINativeMethod audio_decoder_methods[] = {
        {"_decodeAudio", "(Ljava/lang/String;Ljava/lang/String;)I", JNI_decoder_audio}
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

    jclass audio_decode_class = (*env)->FindClass(env, AUDIO_DECODER_CLASS);
    (*env)->RegisterNatives(env, audio_decode_class, audio_decoder_methods, NELEM(audio_decoder_methods));
    (*env)->DeleteLocalRef(env, audio_decode_class);
    return JNI_VERSION_1_6;
}