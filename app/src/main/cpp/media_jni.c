//
// Created by chenhao on 11/21/20.
//
#include <jni.h>
#include <pthread.h>
#include "log.h"
#include "yuv_player.h"
#include "rplayer.h"
#include "sei_parser.h"
#include "transcode.h"
#include "ff_audio_decoder.h"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define YUV_PLAYER_CLASS "com/av/samples/YUVPlayer"
#define REVERSE_PLAYER_CLASS "com/av/samples/ReversePlayer"
#define SEI_PARSER_CLASS "com/av/samples/demux/SeiParser"
#define TRANSCODE_CLASS "com/av/samples/FFTranscode"
#define FF_AUDIO_DECODER_CLASS "com/av/samples/decoder/FFAudioDecoder"

static JavaVM *g_jvm;
static pthread_key_t g_thread_key;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;
static jclass g_rplayer_class;
static jmethodID g_method_postEventFromNative;


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
    (*env)->CallStaticVoidMethod(env, g_rplayer_class, g_method_postEventFromNative,
            weakThiz, what, arg1, arg2, NULL);
}

static void post_event2(JNIEnv *env, jobject weakThiz, jint what, jint arg1, jint arg2, jobject obj)
{
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
        post_event(env, week_thiz, msg.what, msg.arg1, msg.arg2);
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

static jint JNI_rplayer_seek(JNIEnv *env, jclass class, jlong handler, jlong posUs)
{
    RPlayer *player = (RPlayer *)handler;
    int ret = 0;
    LOGI("JNI_rplayer_seek");
    ret = rplayer_seek(player, posUs);
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
        {"_seek", "(JJ)I", JNI_rplayer_seek},
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

static jint JNI_sei_parser(JNIEnv *env, jclass class, jstring url)
{
    int ret = 0;
    char *url_str = (*env)->GetStringUTFChars(env, url, 0);
    ret = sei_parser(url_str);
    (*env)->ReleaseStringUTFChars(env, url, 0);
    return ret;
}

static JNINativeMethod sei_parser_jni_methods[] = {
        {"_parser", "(Ljava/lang/String;)I", JNI_sei_parser},
};

static jlong JNI_transcode_create(JNIEnv *env, jclass class)
{
    Transcoder *transcoder = create_transcoder();
    return (intptr_t)transcoder;
}


static jint JNI_transcode_start(JNIEnv *env, jclass class, jlong handle)
{
    int ret = 0;
    Transcoder *transcoder = (Transcoder *)(intptr_t)handle;
    ret = transcode_start(transcoder);
    return ret;
}

static jint JNI_transcode_stop(JNIEnv *env, jclass class, jlong handle)
{
    int ret = 0;
    Transcoder *transcoder = (Transcoder *)(intptr_t)handle;
    ret = transcode_stop(transcoder);
    return ret;
}

static jint JNI_transcode_destroy(JNIEnv *env, jclass class, jlong handle)
{
    int ret = 0;
    Transcoder *transcoder = (Transcoder *)(intptr_t)handle;
    transcode_release(transcoder);
    return ret;
}


static jint JNI_transcode_init(JNIEnv *env, jclass class, jlong handle,
                               jstring input_path, jstring output_path,
                               jint width, jint height, jint fps, jint vb,
                               jint sample_rate, jint channel, jint ab
)
{
    int ret = 0;
    Transcoder *transcoder = (Transcoder *)(intptr_t)handle;
    char *input_path_str = (*env)->GetStringUTFChars(env, input_path, 0);
    char *ouput_path_str = (*env)->GetStringUTFChars(env, output_path, 0);
    ret = transcode_init(transcoder, input_path_str, ouput_path_str, width, height, fps, vb, sample_rate, channel, ab);
    (*env)->ReleaseStringUTFChars(env, input_path, input_path_str);
    (*env)->ReleaseStringUTFChars(env, output_path, ouput_path_str);
    return ret;
}

static JNINativeMethod transcode_jni_methods[] = {
        {"_create", "()J", JNI_transcode_create},
        {"_init", "(JLjava/lang/String;Ljava/lang/String;IIIIIII)I", JNI_transcode_init},
        {"_start", "(J)I", JNI_transcode_start},
        {"_stop", "(J)I", JNI_transcode_stop},
        {"_release", "(J)I", JNI_transcode_destroy}
};



static jlong JNI_ff_audio_create(JNIEnv *env, jclass class)
{
    Decoder *decoder = ff_decoder_create();
    return (intptr_t)decoder;
}


static jint JNI_ff_audio_decoder_init(JNIEnv *env, jclass class, jlong handle, jstring input_path, int channel, int sample_rate)
{
    int ret = 0;
    Decoder *decoder = (Decoder *)(intptr_t)handle;
    char *input_path_str = (*env)->GetStringUTFChars(env, input_path, 0);
    ret = ff_decoder_init(decoder, input_path_str, channel, sample_rate);
    (*env)->ReleaseStringUTFChars(env, input_path, input_path_str);
    return ret;
}

static jint JNI_ff_audio_decoder_decode(JNIEnv *env, jclass class, jlong handle)
{
    int ret = 0;
    Decoder *decoder = (Decoder *)(intptr_t)handle;
    uint8_t *buffer;
    int size;
    int64_t pts;
    ret = ff_decoder_decode(decoder, &buffer, &size, &pts);
    return ret;
}


static JNINativeMethod audio_decoder_jni_methods[] = {
        {"_create", "()J", JNI_ff_audio_create},
        {"_init", "(JLjava/lang/String;II)I", JNI_ff_audio_decoder_init},
        {"_start", "(J)I", JNI_ff_audio_decoder_decode},
};


// 动态注册
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    g_jvm = vm;

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

    jclass sei_parser_class = (*env)->FindClass(env, SEI_PARSER_CLASS);
    (*env)->RegisterNatives(env, sei_parser_class, sei_parser_jni_methods, NELEM(sei_parser_jni_methods));
    (*env)->DeleteLocalRef(env, sei_parser_class);


    jclass transcode_class = (*env)->FindClass(env, TRANSCODE_CLASS);
    (*env)->RegisterNatives(env, transcode_class, transcode_jni_methods, NELEM(transcode_jni_methods));
    (*env)->DeleteLocalRef(env, transcode_class);

    jclass audio_decoder_class = (*env)->FindClass(env, FF_AUDIO_DECODER_CLASS);
    (*env)->RegisterNatives(env, audio_decoder_class, audio_decoder_jni_methods, NELEM(audio_decoder_jni_methods));
    (*env)->DeleteLocalRef(env, audio_decoder_class);
    return JNI_VERSION_1_6;
}