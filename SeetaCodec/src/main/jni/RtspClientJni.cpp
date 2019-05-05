//
// Created by yummy on 18-7-29.
//

#include <android/native_window_jni.h>
#include "com_seetatech_sdk_rtsp_RtspClient.h"
#include "RtspClientProxy.h"
#include "RtspClientProxy.h"

/*
 * Class:     com_seetatech_sdk_rtsp_RtspClient
 * Method:    create
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_seetatech_sdk_rtsp_RtspClient_create
        (JNIEnv *env, jobject thiz) {
    RtspClientProxy *client = new RtspClientProxy(env, thiz);
    return (jlong)client;
}

/*
 * Class:     com_seetatech_sdk_rtsp_RtspClient
 * Method:    nStart
 * Signature: (JLjava/lang/String;Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_com_seetatech_sdk_rtsp_RtspClient_nStart
        (JNIEnv *env, jobject thiz, jlong natvieObj, jstring _uri, jobject _surface) {
    RtspClientProxy *client = reinterpret_cast<RtspClientProxy *>(natvieObj);
    const char *uri = env->GetStringUTFChars(_uri, NULL);
    ANativeWindow* window = NULL;
    if (_surface) {
        window = ANativeWindow_fromSurface(env, _surface);
    }
    client->start(uri, window);
}

/*
 * Class:     com_seetatech_sdk_rtsp_RtspClient
 * Method:    nRetry
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_seetatech_sdk_rtsp_RtspClient_nRetry
        (JNIEnv *env, jobject thiz, jlong natvieObj) {

    RtspClientProxy *client = reinterpret_cast<RtspClientProxy *>(natvieObj);
}

/*
 * Class:     com_seetatech_sdk_rtsp_RtspClient
 * Method:    nStop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_seetatech_sdk_rtsp_RtspClient_nStop
        (JNIEnv *env, jobject thiz, jlong natvieObj) {

    RtspClientProxy *client = reinterpret_cast<RtspClientProxy *>(natvieObj);
    client->shutdown();
}

/*
 * Class:     com_seetatech_sdk_rtsp_RtspClient
 * Method:    destroy
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_seetatech_sdk_rtsp_RtspClient_destroy
        (JNIEnv *env, jobject thiz, jlong natvieObj) {
    RtspClientProxy *client = reinterpret_cast<RtspClientProxy *>(natvieObj);
    if (client) {
        delete client;
    }
}