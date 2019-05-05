//
// Created by yummy on 18-7-29.
//

#ifndef NATIVE_CODEC_RTSPCLIENTCALLBACKJNI_H
#define NATIVE_CODEC_RTSPCLIENTCALLBACKJNI_H

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

#include <jni.h>
#include <string>
#include <sstream>
#include <android/native_window.h>
#include <RTSPClient.hh>
#include "RtspClientCallback.h"
#include "NativeCodecProxy.h"
#include <semaphore.h>

using namespace std;

class RtspClientProxy : RtspClientCallback {
private:
    volatile jmethodID methodOnNewFrame;
    volatile jmethodID methodOnError;
    volatile jobject javaObj;
    JavaVM* jvm;
    JNIEnv* env;

    char eventLoopWatchVariable = 0;

    RTSPClient* mRtspClient = NULL;
    NativeCodecProxy* codecProxy = NULL;




    void startRtsp(const char* uri);

// RTSP 'response handlers':
    void static continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode,
                               char* resultString);
    void static continueAfterSETUP(RTSPClient* rtspClient, int resultCode,
                            char* resultString);
    void static continueAfterPLAY(RTSPClient* rtspClient, int resultCode,
                           char* resultString);

// Other event handler functions:
    void static subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
    void static subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
    void static streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
    void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
    void static setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
    void static shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

public:
    RtspClientProxy(JNIEnv *env, jobject thiz);
    ~RtspClientProxy();
    void onNewFrame(JNIEnv* env, long matAddr, int length, int format, int width, int height);
    void onError(JNIEnv* env, int code, const char* msg);

    void start(const char* uri, ANativeWindow *window);
    void shutdown();
};



// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
    StreamClientState();
    virtual ~StreamClientState();

public:
    volatile bool isConnected = false;
    JNIEnv* jniEnv;
    MediaSubsessionIterator* iter = NULL;
    MediaSession* session = NULL;
    MediaSubsession* subsession = NULL;
    TaskToken streamTimerTask;
    RtspClientCallback *callback = NULL;
    NativeCodecProxy *codecProxy = NULL;
    MediaSink *sink = NULL;
    double duration;
    char* eventLoopWatchVariable = NULL;
};


// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient {
public:
    static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
                                    int verbosityLevel = 0, char const* applicationName = NULL,
                                    portNumBits tunnelOverHTTPPortNum = 0);

protected:
    ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
                  int verbosityLevel, char const* applicationName,
                  portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
    virtual ~ourRTSPClient();

public:
    StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.


// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000000

class RtspSink: public MediaSink {
public:
    static RtspSink* createNew(UsageEnvironment& env,
                               MediaSubsession& subsession, // identifies the kind of data that's being received
                               RtspClientCallback *callback,
                               NativeCodecProxy* codec,
                               char const* streamId = NULL); // identifies the stream itself (optional)

    void setEnabled(bool enabled);
private:

    unsigned char *sps = NULL;
    int spsLength = 0;
    int ppsLength = 0;
    unsigned char *pps = NULL;

    unsigned char start_code[4] = {0x00, 0x00, 0x00, 0x01};
    volatile Boolean isRunning = true;
    bool fHaveWrittenFirstFrame = false;

    RtspSink(UsageEnvironment& env, MediaSubsession& subsession,RtspClientCallback *callback,
             NativeCodecProxy* codec, char const* streamId);
    // called only by "createNew()"
    virtual ~RtspSink();

    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes, struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                           struct timeval presentationTime, unsigned durationInMicroseconds);

    bool updateSps(unsigned char *sps, int length);
    bool updatePps(unsigned char *pps, int length);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

private:
    u_int8_t* fReceiveBuffer = NULL;
    MediaSubsession& fSubsession;
    char* fStreamId = NULL;
    RtspClientCallback *callback = NULL;
    NativeCodecProxy *codecProxy = NULL;
};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

#endif //NATIVE_CODEC_RTSPCLIENTCALLBACKJNI_H
