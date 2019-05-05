//
// Created by yummy on 18-7-29.
//

#include <jni.h>
#include <string.h>
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include "RtspClientCallback.h"
#include "log.h"
#include "RtspClientProxy.h"
#include "NativeCodecProxy.h"

RtspClientProxy::RtspClientProxy(JNIEnv *env, jobject thiz) {
    mRtspClient = NULL;

    jclass clazz = env->GetObjectClass(thiz);

    env->GetJavaVM(&jvm);

    this->codecProxy = new NativeCodecProxy(jvm, "video/avc", nullptr, this);
    this->javaObj = env->NewGlobalRef(thiz);
    this->methodOnNewFrame = env->GetMethodID(clazz, "onNewFrame", "(JIII)V");
    this->methodOnError = env->GetMethodID(clazz, "onError", "(ILjava/lang/String;)V");

    if (!this->methodOnNewFrame) {
        LOGE("method onNewFrame not found");
    }
    if (!this->methodOnError) {
        LOGE("method onError not found");
    }
}

void RtspClientProxy::onError(JNIEnv* env, int code, const char *msg) {
    LOGE("ERROR:%ld %s", pthread_self(), msg);
//    if (this->methodOnError) {
//        jstring message = NULL;
//        if (msg && strlen(msg) > 0) {
//            message = env->NewStringUTF(msg);
//        }
//        env->CallVoidMethod(javaObj, methodOnError, code, message);
//    }
}

void RtspClientProxy::onNewFrame
        (JNIEnv* env, long matAddr, int length, int format, int width, int height) {
    if (this->methodOnNewFrame) {
        env->CallVoidMethod(javaObj, methodOnNewFrame, matAddr, format, width, height);
    }
}

void RtspClientProxy::start(const char *uri, ANativeWindow *window) {
    codecProxy->setWindow(window);
    startRtsp(uri);
}

void RtspClientProxy::startRtsp(const char *uri) {
    // Begin by setting up our usage environment:
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* usageEnv = BasicUsageEnvironment::createNew(*scheduler);

    // We need at least one "rtsp://" URL argument:


    eventLoopWatchVariable = 0;

    // There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
    openURL(*usageEnv, NULL, uri);

    // All subsequent activity takes place within the event loop:
    usageEnv->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

//  return 0;

    // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
    // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
    // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:

    usageEnv->reclaim();
    usageEnv = NULL;
    delete scheduler;
    scheduler = NULL;
}

void RtspClientProxy::openURL(UsageEnvironment &env, char const *progName, char const *rtspURL) {
    // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
    // to receive (even if more than stream uses the same "rtsp://" URL).

    JNIEnv *jniEnv;
//    jvm->AttachCurrentThread(&jniEnv, NULL);

    mRtspClient = ourRTSPClient::createNew(env, rtspURL,
                                                      RTSP_CLIENT_VERBOSITY_LEVEL, progName);

    if (mRtspClient == NULL) {
//		LOGE("Failed to create a RTSP client for URL \"%s\": %s", rtspURL,
//				env.getResultMsg());
        string tmp = "Failed to create a RTSP client for URL \"" + string(rtspURL)
              + "\":" + string(env.getResultMsg());
        onError(jniEnv, 0, tmp.c_str());
        return;
    }

    StreamClientState& scs = ((ourRTSPClient*) mRtspClient)->scs;
    scs.jniEnv = jniEnv;
    scs.isConnected = false;
    scs.callback = this;
    scs.codecProxy = this->codecProxy;
    scs.eventLoopWatchVariable = &this->eventLoopWatchVariable;

//  ++rtspClientCount;

    // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
    // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
    // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
    mRtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}

void RtspClientProxy::continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

        if (resultCode != 0) {
            string tmp = "Failed to get a SDP description: " + string(resultString);

            scs.callback->onError(scs.jniEnv, 0, tmp.c_str());
            delete[] resultString;
            break;
        }

        char* const sdpDescription = resultString;

		LOGD("Got a SDP description:\n%s", sdpDescription);

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdpDescription);
        delete[] sdpDescription; // because we don't need it anymore
        if (scs.session == NULL) {

            string tmp =
                    "Failed to create a MediaSession object from the SDP description: "
                    + string(env.getResultMsg());
            scs.callback->onError(scs.jniEnv, 0, tmp.c_str());
            break;
        } else if (!scs.session->hasSubsessions()) {
            string tmp =
                    "This session has no media subsessions (i.e., no \"m=\" lines)";
            scs.callback->onError(scs.jniEnv, 0, tmp.c_str());
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);

    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
}

void RtspClientProxy::continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

        if (resultCode != 0) {
            string tmp = "Failed to set up the subsession: " + string(resultString);
            scs.callback->onError(scs.jniEnv, 0, tmp.c_str());
            break;
        }

		LOGD("Set up the subsession (");
        ostringstream oss;
        if (scs.subsession->rtcpIsMuxed()) {
			LOGD("client port %d", scs.subsession->clientPortNum());
        } else {
			LOGD("client port %d-%d", scs.subsession->clientPortNum(),
					scs.subsession->clientPortNum() + 1);
        }


        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)

        scs.subsession->sink = RtspSink::createNew(env, *scs.subsession, scs.callback, scs.codecProxy,
                                                    rtspClient->url());
        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL) {
//			LOGE("Failed to create a data sink for the subsession: %s",
//					env.getResultMsg());
            string tmp = "Failed to create a data sink for the subsession: "
                  + string(env.getResultMsg());
            scs.callback->onError(scs.jniEnv, 0, tmp.c_str());
            break;
        }

//		env << *rtspClient << "Created a data sink for the \""
//				<< *scs.subsession << "\" subsession\n";
		LOGD("Created a data sink for the subsession");

        scs.subsession->miscPtr = rtspClient;// a hack to let subsession handler functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                                           subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) {
            scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler,
                                                          scs.subsession);
        }
    } while (0);
    delete[] resultString;

// Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}

void RtspClientProxy::continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString) {
    Boolean success = False;
    ostringstream oss;

    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

        if (resultCode != 0) {
//			LOGE("Failed to start playing session: %s", resultString);
            string tmp = "Failed to start playing session: " + string(resultString);

            scs.callback->onError(scs.jniEnv, 0, tmp.c_str());
            break;
        }

        // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
        // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
        // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
        // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
        if (scs.duration > 0) {
            unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
            scs.duration += delaySlop;
            unsigned uSecsToDelay = (unsigned) (scs.duration * 1000000);
            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(
                    uSecsToDelay, (TaskFunc*) streamTimerHandler, rtspClient);
        }

		LOGD("Started playing session");
        if (scs.duration > 0) {
			LOGD("(for up to %f seconds)...", scs.duration);
        }

        success = True;
    } while (0);
    delete[] resultString;

    if (!success) {
        // An unrecoverable error occurred with this stream.
        shutdownStream(rtspClient);
    }
}

void RtspClientProxy::subsessionAfterPlaying(void *clientData) {
    MediaSubsession* subsession = (MediaSubsession*) clientData;
    RTSPClient* rtspClient = (RTSPClient*) (subsession->miscPtr);

// Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

// Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->sink != NULL)
            return; // this subsession is still active
    }

// All subsessions' streams have now been closed, so shutdown the client:
    shutdownStream(rtspClient);
}

void RtspClientProxy::subsessionByeHandler(void *clientData) {
    MediaSubsession* subsession = (MediaSubsession*) clientData;
    RTSPClient* rtspClient = (RTSPClient*) subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

	LOGD("Received RTCP \"BYE\" on subsession");

// Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void RtspClientProxy::streamTimerHandler(void *clientData) {
    ourRTSPClient* rtspClient = (ourRTSPClient*) clientData;
    StreamClientState& scs = rtspClient->scs; // alias

    scs.streamTimerTask = NULL;

// Shut down the stream:
    shutdownStream(rtspClient);
}

void RtspClientProxy::setupNextSubsession(RTSPClient *rtspClient) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*) rtspClient)->scs; // alias

    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL) {
        if (!scs.subsession->initiate()) {
//			LOGE("Failed to initiate the subsession: %s", env.getResultMsg());
            string tmp = "Failed to initiate the subsession: "
                  + string(env.getResultMsg());
            scs.callback->onError(scs.jniEnv, 0, tmp.c_str());
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        } else {
			LOGD("Initiated the subsession (");
            ostringstream oss;
            if (scs.subsession->rtcpIsMuxed()) {
				LOGD("client port %d", scs.subsession->clientPortNum());
            } else {
				LOGD("client port %d-%d", scs.subsession->clientPortNum(),
						scs.subsession->clientPortNum() + 1);
            }

            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP,
                                         False, REQUEST_STREAMING_OVER_TCP);
        }
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY,
                                    scs.session->absStartTime(), scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime()
                       - scs.session->playStartTime();
        scs.isConnected = true;
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

void RtspClientProxy::shutdownStream(RTSPClient *rtspClient, int exitCode) {
        UsageEnvironment &env = rtspClient->envir(); // alias
        StreamClientState &scs = ((ourRTSPClient *) rtspClient)->scs; // alias
//    LOGE("shutdownStream start %d  %d", rtspClient, scs.isConnected);
        scs.isConnected = false;
// First, check whether any subsessions have still to be closed:
        if (scs.session != NULL) {
            Boolean someSubsessionsWereActive = False;
            MediaSubsessionIterator iter(*scs.session);
            MediaSubsession *subsession;

            while ((subsession = iter.next()) != NULL) {
                if (subsession->sink != NULL) {
                    ((RtspSink *) subsession->sink)->setEnabled(false);
                    Medium::close(subsession->sink);
                    subsession->sink = NULL;

                    if (subsession->rtcpInstance() != NULL) {
                        subsession->rtcpInstance()->setByeHandler(NULL,
                                                                  NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                    }

                    someSubsessionsWereActive = True;
                }
            }

            if (someSubsessionsWereActive) {
                // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
                // Don't bother handling the response to the "TEARDOWN".
                rtspClient->sendTeardownCommand(*scs.session, NULL);
            }
            scs.session = NULL;
        }

        if (scs.eventLoopWatchVariable != NULL) {
            *scs.eventLoopWatchVariable = 1;
            scs.eventLoopWatchVariable = NULL;
        }

        LOGD("Closing the stream.");

        Medium::close(rtspClient);
        RtspClientProxy *clientProxy = (RtspClientProxy *) (scs.callback);
//    clientProxy->jvm->DetachCurrentThread();
        clientProxy->mRtspClient = NULL;
// Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

}

RtspClientProxy::~RtspClientProxy() {

    methodOnNewFrame = nullptr;
    methodOnError = nullptr;
    if (codecProxy) {
        delete codecProxy;
        codecProxy = nullptr;
    }
//    JNIEnv *env;
//    jvm->AttachCurrentThread(&env, NULL);
//    env->DeleteGlobalRef(javaObj);
//    jvm->DetachCurrentThread();
}

void RtspClientProxy::shutdown() {
    if (mRtspClient != NULL) {
        if ((((ourRTSPClient*) mRtspClient)->scs).isConnected) {
            shutdownStream(mRtspClient, 0);
        }
        mRtspClient = NULL;
    }
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState() :
        iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(
        0.0) {
}

StreamClientState::~StreamClientState() {
    delete iter;
    if (session != NULL) {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment& env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env,
                                        char const* rtspURL, int verbosityLevel, char const* applicationName,
                                        portNumBits tunnelOverHTTPPortNum) {
    return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName,
                             tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
                             int verbosityLevel, char const* applicationName,
                             portNumBits tunnelOverHTTPPortNum) :
        RTSPClient(env, rtspURL, verbosityLevel, applicationName,
                   tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient::~ourRTSPClient() {
}



// Implementation of "RtspSink":
RtspSink* RtspSink::createNew(UsageEnvironment& env, MediaSubsession& subsession,
                              RtspClientCallback *callback, NativeCodecProxy* codec, char const* streamId) {
    return new RtspSink(env, subsession, callback, codec, streamId);
}

RtspSink::RtspSink(UsageEnvironment& env, MediaSubsession& subsession,RtspClientCallback *callback,
                   NativeCodecProxy* codec, char const* streamId) :
        MediaSink(env), fSubsession(subsession) {
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
    this->callback = callback;
    this->codecProxy = codec;
}

RtspSink::~RtspSink() {
    delete[] fReceiveBuffer;
    delete[] fStreamId;
    if (pps) free(pps);
    if (sps) free(sps);

    fReceiveBuffer = NULL;
    fStreamId = NULL;
    pps = NULL;
    sps = NULL;
}

void RtspSink::afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes, struct timeval presentationTime,
                                  unsigned durationInMicroseconds) {
    RtspSink* sink = (RtspSink*) clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime,
                            durationInMicroseconds);
}

void RtspSink::afterGettingFrame(unsigned frameSize,
                                  unsigned numTruncatedBytes, struct timeval presentationTime,
                                  unsigned /*durationInMicroseconds*/) {
    // video data only, audio data excluded.
    if (!isRunning) {
        if (codecProxy) codecProxy->stop();
        return;
    }

    if (strcmp(fSubsession.mediumName(), "video") == 0 && fReceiveBuffer != NULL) {

        if (!fHaveWrittenFirstFrame) {
            const char *base64 = fSubsession.fmtp_spropparametersets();
            unsigned int numSPropRecords = 0;
            size_t firstFrameLength = frameSize + 4;

            SPropRecord *sPropRecords = parseSPropParameterSets(base64, numSPropRecords);
            AMediaFormat *videoFormat = AMediaFormat_new();
            AMediaFormat_setString(videoFormat, AMEDIAFORMAT_KEY_MIME, "video/avc");
            AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_WIDTH, 1920);
            AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_HEIGHT, 1080);

            for (unsigned i = 0; i < numSPropRecords; ++i) {
                if (sPropRecords[i].sPropLength == 0) continue; // bad data
                u_int8_t nal_unit_type = (u_int8_t) ((sPropRecords[i].sPropBytes[0]) & 0x1F);
                if (nal_unit_type == 7/*SPS*/) {
                    LOGV("SPS FOUND");
                } else if (nal_unit_type == 8/*PPS*/) {
                    LOGV("PPS FOUND");
                }

                AMediaFormat_setBuffer(videoFormat, "csd-" + i, sPropRecords[i].sPropBytes,
                                       sPropRecords[i].sPropLength);
                firstFrameLength += sPropRecords[i].sPropLength + 4;
            }

            codecProxy->start(videoFormat);
            AMediaFormat_delete(videoFormat);

            FrameBuffer *frameBuffer = codecProxy->getInputBuffer(500 * 1000);
            if (frameBuffer && frameBuffer->buf && frameBuffer->size >= firstFrameLength) {
                uint8_t *buf = frameBuffer->buf;
                for (int i = 0; i < numSPropRecords; i++) {
                    memcpy(buf, start_code, 4);
                    buf += 4;
                    memcpy(buf, sPropRecords[i].sPropBytes, sPropRecords[i].sPropLength);
                    buf += sPropRecords[i].sPropLength;


                }
                memcpy(buf, start_code, 4);
                buf += 4;
                memcpy(buf, fReceiveBuffer, frameSize);

                frameBuffer->inputSize = firstFrameLength;
                frameBuffer->presentationTimeUs = presentationTime.tv_usec;
                codecProxy->submitInputBuffer(frameBuffer);

                fHaveWrittenFirstFrame = true;
                LOGV("first frame written");
            }

        } else {

            bool input = true;
            if (fReceiveBuffer[0] == 0x67) {
                if (!updateSps(fReceiveBuffer, frameSize)) {
                    input = false;
                } else {
                    LOGI("update sps");
                }
            }

            if (fReceiveBuffer[0] == 0x68) {
                if (!updatePps(fReceiveBuffer, frameSize)) {
                    input = false;
                } else {
                    LOGI("update pps");
                }
            }

            if (input) {
                FrameBuffer *frameBuffer = codecProxy->getInputBuffer();
                size_t size = frameSize + 4;
                if (frameBuffer && frameBuffer->buf && frameBuffer->size >= size) {
                    memcpy(frameBuffer->buf, start_code, 4);
                    memcpy(frameBuffer->buf + 4, fReceiveBuffer, frameSize);
                    frameBuffer->inputSize = size;
                    codecProxy->submitInputBuffer(frameBuffer);
                }
            }
        }

    }

// Then continue, to request the next frame of data:
    if (isRunning) {
//        usleep(30*1000);
        continuePlaying();
    }
}

Boolean RtspSink::continuePlaying() {
    if (fSource == NULL)
        return False; // sanity check (should not happen)

// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this, onSourceClosure, this);
    return True;
}

void RtspSink::setEnabled(bool enabled) {
    isRunning = enabled;
    if (codecProxy) codecProxy->stop();
}

bool RtspSink::updateSps(unsigned char *sps, int length) {
    if (length != this->spsLength || memcmp(this->sps, sps, length)) {
        if (this->spsLength < length) {
            if (this->sps) free(this->sps);
            this->sps = (unsigned char *)(malloc(sizeof(char) * length));
        }

        memcpy(this->sps, sps, length);
        this->spsLength = length;
        return true;
    }

    return false;
}

bool RtspSink::updatePps(unsigned char *pps, int length) {
    if (length != this->ppsLength || memcmp(this->pps, pps, length)) {
        if (this->ppsLength < length) {
            if (this->pps) free(this->pps);
            this->pps = (unsigned char *)(malloc(sizeof(char) * length));
        }

        memcpy(this->pps, pps, length);
        this->ppsLength = length;
        return true;
    }

    return false;
}
