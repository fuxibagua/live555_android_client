//
// Created by yummy on 18-7-29.
//

#ifndef NATIVE_CODEC_RTSPCLIENTCALLBACK_H
#define NATIVE_CODEC_RTSPCLIENTCALLBACK_H


#include <stdint.h>

class RtspClientCallback {
public:
    virtual void onNewFrame(JNIEnv* env, long matAddr, int length, int format, int width, int height){}
    virtual void onError(JNIEnv* env, int code, const char* msg){}
};


#endif //NATIVE_CODEC_RTSPCLIENTCALLBACK_H
