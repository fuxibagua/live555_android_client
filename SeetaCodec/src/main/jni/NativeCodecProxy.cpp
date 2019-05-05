//
// Created by yummy on 18-7-29.
//

#include <unistd.h>
#include <malloc.h>
#include <jni.h>
#include "NativeCodecProxy.h"
#include "log.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

NativeCodecProxy::NativeCodecProxy(JavaVM *jvm, const char *mime,
                                   ANativeWindow *window, RtspClientCallback *callback)
        : looper(jvm) {

    this->codec = AMediaCodec_createDecoderByType(mime);

    this->window = window;
    this->callback = callback;
}

NativeCodecProxy::~NativeCodecProxy() {
    if (codec) {
        AMediaCodec_stop(codec);
        AMediaCodec_delete(codec);
    }

    if (window) {
        ANativeWindow_release(window);
    }

    window = nullptr;
    callback = nullptr;
}

void NativeCodecProxy::stop() {
    isPlaying = false;
    if (codec) {
        AMediaCodec_stop(codec);
    }
    post(kMsgCodecStop, nullptr, true);
}

FrameBuffer* NativeCodecProxy::getInputBuffer(int64_t timeout) {
    ssize_t bufIdx = AMediaCodec_dequeueInputBuffer(codec, timeout);
    if (bufIdx >= 0) {
        FrameBuffer* frameBuffer = new FrameBuffer();
        frameBuffer->buf = AMediaCodec_getInputBuffer(codec, bufIdx, &frameBuffer->size);
        frameBuffer->bufIdx = bufIdx;
        return frameBuffer;
    } else {
        LOGD("get input buffer %zd", bufIdx);
    }

    return nullptr;
}

void NativeCodecProxy::submitInputBuffer(FrameBuffer *buffer) {

    AMediaCodec_queueInputBuffer(codec, buffer->bufIdx, 0, buffer->inputSize, buffer->presentationTimeUs, 0);

    if (!isPlaying) {
        LOGI("start codecProxy");
        isPlaying = true;
        post(kMsgCodecBuffer, nullptr);
    }

    delete buffer;
}

void NativeCodecProxy::setWindow(ANativeWindow *window) {
//    AMediaCodec_setOutputSurface(codecProxy, window);
//    undefined reference to `AMediaCodec_setOutputSurface'
    this->window = window;
}

void NativeCodecProxy::start(AMediaFormat *format) {
    AMediaCodec_configure(this->codec, format, /*window*/ NULL, NULL, 0);
    AMediaCodec_start(this->codec);
    post(kMsgCodecStart, NULL, true);
}

void NativeCodecProxy::doOutput() {
    AMediaCodecBufferInfo info;
    auto status = AMediaCodec_dequeueOutputBuffer(codec, &info, 0);
    while (status >= 0) {
        if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
            LOGV("output EOS");
        }

        if (callback && isPlaying) {
            size_t size;
            auto buf = AMediaCodec_getOutputBuffer(codec, status, &size);
            if (buf && size > 0) {
//在surface上绘制
                ANativeWindow_Buffer buffer;
                if (ANativeWindow_lock(window, &buffer, NULL) == 0) {
                    //在这里将要显示的内容以对应的format填充到buffer.bits
                    cv::Mat mat(height * 3 /2, width, CV_8UC1);
                    mat.data = buf;
                    cv::Mat matBgr(height, width, CV_8UC4);
                    cv::cvtColor(mat, matBgr, CV_YUV2RGBA_NV12);
                    if (height != buffer.height || width != buffer.width) {
                        cv::resize(matBgr, matBgr, cv::Size(buffer.width, buffer.height));
                    }
                    memcpy(buffer.bits, matBgr.data, buffer.height* buffer.width*4);
                    ANativeWindow_unlockAndPost(window);
                }
//
                cv::Mat mat(height * 3 /2, width, CV_8UC1);
                mat.data = buf;

                cv::Mat *matBgr = new cv::Mat(height, width, CV_8UC3);
                cv::cvtColor(mat, *matBgr, CV_YUV2BGR_NV12);

                callback->onNewFrame(env, (long)matBgr, 0, 0, width, height);
            }
        }

        AMediaCodec_releaseOutputBuffer(codec, status, false/*info.size != 0*/);
        status = AMediaCodec_dequeueOutputBuffer(codec, &info, 0);
    }
    if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
        LOGD("output buffers changed");
    } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
        auto format = AMediaCodec_getOutputFormat(codec);
        if (format != NULL) {
            LOGD("format changed to: %s", AMediaFormat_toString(format));
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &color);
            callback->onNewFrame(env, 0, 0, 0, width, height);
        }
        AMediaFormat_delete(format);
    } else if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
//        LOGD("no output buffer right now");
    } else {
        LOGD("unexpected info code: %zd", status);
    }
}

void NativeCodecProxy::handle(int what, void *data) {
    switch (what) {
        case kMsgCodecStop:
            LOGV("kMsgCodecStop");

            break;
        case kMsgCodecStart:
            LOGV("kMsgCodecStart");
            break;
        case kMsgCodecBuffer:
            LOGV("kMsgCodecBuffer");
            doOutput();
            usleep(25 * 1000);
            if (isPlaying) post(kMsgCodecBuffer, nullptr);
            break;

        default:
            break;
    }
}

