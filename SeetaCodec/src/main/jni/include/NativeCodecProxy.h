//
// Created by yummy on 18-7-29.
//

#ifndef NATIVE_CODEC_NATIVECODEC_H
#define NATIVE_CODEC_NATIVECODEC_H

#include <vector>
#include <android/native_window.h>
#include <media/NdkMediaCodec.h>
#include "looper.h"
#include "RtspClientCallback.h"
using namespace std;

class FrameBuffer{
public:
    ssize_t bufIdx;
    size_t size;
    size_t inputSize;
    uint8_t *buf;
    uint64_t presentationTimeUs;
};

enum {
    kMsgJvmAttachThread,
    kMsgJvmDettachThread,
    kMsgCodecBuffer,
    kMsgCodecStop,
    kMsgCodecStart
};

class NativeCodecProxy : public looper {
private:
    ANativeWindow* window = NULL;
    RtspClientCallback *callback;
    AMediaCodec* codec = NULL;
    int width = 0, height = 0, color = 0, i;

    volatile bool isPlaying = false;

    static int sChannelMaxId;
    int channel;

    static int64_t systemnanotime() {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec * 1000000000LL + now.tv_nsec;
    }

    void doOutput();

public:
    NativeCodecProxy(JavaVM *jvm, const char *mime,
                         ANativeWindow *window, RtspClientCallback *callback);
    ~NativeCodecProxy();

    void stop();

    void handle(int what, void *data);

    void setWindow(ANativeWindow* window);

    void start(AMediaFormat *format);

    void submitInputBuffer(FrameBuffer *buffer);

    FrameBuffer* getInputBuffer(int64_t timeout = 30000);

};

#endif //NATIVE_CODEC_NATIVECODEC_H
