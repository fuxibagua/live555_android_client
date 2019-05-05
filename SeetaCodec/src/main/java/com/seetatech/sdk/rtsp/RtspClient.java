package com.seetatech.sdk.rtsp;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Surface;

import org.opencv.core.Mat;

public class RtspClient extends Handler {
    public interface RtspCallback {
        void onError(int errCode, String msg);

        /***
         * Called when new frame decoded.
         * @param data raw frame data, only available when no surface set.
         * @param format frame format
         * @param width width
         * @param height height
         */
        void onNewFrame(byte[] data, int format, int width, int height);
    }
    private static final String TAG = "RtspClientProxy";
    private final int MSG_RESTART = 1;
    private final long TIME_OUT = 5000;

    static {
        System.loadLibrary("rtsp_client");
    }

    private native long create();

    private native void nStart(long nativeObj, String uri, Surface surface);

    private native void nRetry(long nativeObj);

    private native void nStop(long nativeObj);

    private native void destroy(long nativeObj);

    private long nativeObj;
    private volatile boolean isRunning;
    private RtspCallback callback;

    private void onNewFrame(byte[] data, int format, int width, int height) {
        if (callback != null) { callback.onNewFrame(data, format, width, height); }
        removeMessages(MSG_RESTART);
        sendEmptyMessageDelayed(MSG_RESTART, TIME_OUT);
    }

    public void onNewFrame(long matAddr, int format, int width, int height) {
        //if (callback != null) { callback.onNewFrame(data, format, width, height); }
        if (matAddr != 0) {
            Mat mat = new Mat(matAddr);
            Log.d(TAG, "onNewFrame mat=" + mat);
            mat.release();
        }
        removeMessages(MSG_RESTART);
        sendEmptyMessageDelayed(MSG_RESTART, TIME_OUT);
    }

    private void onError(int code, String msg) {
        Log.e(TAG, "onError code=" + code + ",msg=" + msg);
        if (callback != null) { callback.onError(code, msg); }
        //nStop(nativeObj);
    }

    public RtspClient() {
        super(Looper.getMainLooper());
        nativeObj = create();
    }

    public void start(String uri, Surface surface) {
        synchronized (this) {
            if (isRunning) {
                return;
            }
            isRunning = true;
        }

        while (isRunning) {
            nStart(nativeObj, uri, surface);
            if (isRunning) {
                try {
                    Thread.sleep(3000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
        isRunning = false;
    }

    public void stop() {
        isRunning = false;// to break the loop while(isRunning){...}
        removeMessages(MSG_RESTART);
        nStop(nativeObj);
    }

    public void destory() {
        if (isRunning) {
            stop();
        }
        destroy(nativeObj);
        nativeObj = 0;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (isRunning) {
            stop();
        }
        if (nativeObj != 0) {
            destroy(nativeObj);
        }
    }

    public void setCallback(RtspCallback callback) {
        this.callback = callback;
    }

    @Override
    public void handleMessage(Message msg) {
        nStop(nativeObj);//causes retry in the loop while(isRunning){...}
    }
}
