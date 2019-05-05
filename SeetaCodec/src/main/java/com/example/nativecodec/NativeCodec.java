/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.nativecodec;

import android.app.Activity;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.Spinner;

import com.seetatech.sdk.rtsp.RtspClient;

import java.io.IOException;

public class NativeCodec extends Activity {
    static final String TAG = "NativeCodecProxy";

    String mSourceString = null;

    TextureView textureView1;
    TextureView textureView2;
    TextureView textureView3;
    TextureView textureView4;

    SurfaceHolder mSurfaceHolder1;


    boolean mCreated = false;
    boolean mIsPlaying = false;

    RtspClient client = new RtspClient();
    RtspClient client2 = new RtspClient();
    RtspClient client3 = new RtspClient();
    RtspClient client4 = new RtspClient();


    private void startRtsp() {

//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//
//                SurfaceTexture st = textureView1.getSurfaceTexture();
//                Surface s = new Surface(st);
//
//                client.start("rtsp://admin:1qaz2wsx@192.168.0.94/streaming/channels/2", s);
//
//                Log.d("RTSP", "thread rtsp-1 finished");
//            }
//        }, "rtsp-1").start();
//
        new Thread(new Runnable() {
            @Override
            public void run() {

                SurfaceTexture st = textureView2.getSurfaceTexture();
                Surface s = new Surface(st);

                client2.start("rtsp://admin:1qaz2wsx@192.168.0.66/streaming/channels/2", s);
                Log.d("RTSP", "thread rtsp-2 finished");
            }
        }, "rtsp-2").start();
//
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//
//                SurfaceTexture st = textureView3.getSurfaceTexture();
//                Surface s = new Surface(st);
//
//                client3.start("rtsp://admin:1qaz2wsx@192.168.0.94/streaming/channels/1", s);
//                Log.d("RTSP", "thread rtsp-3 finished");
//            }
//        }, "rtsp-3").start();
//
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//
//                SurfaceTexture st = textureView4.getSurfaceTexture();
//                Surface s = new Surface(st);
//
//                client4.start("rtsp://admin:1qaz2wsx@192.168.0.55/cam/realmonitor?channel=1&subtype=0", s);
//                Log.d("rstp", "rstp-4 finished");
//            }
//        }, "rtsp-4").start();
    }


    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.main);
        client = new RtspClient();
        client2 = new RtspClient();
        client3 = new RtspClient();
        client4 = new RtspClient();
        // set up the Surface 1 video sink
        textureView1 = (TextureView) findViewById(R.id.surfaceview1);
        textureView2 = (TextureView) findViewById(R.id.surfaceview2);
        textureView3 = (TextureView) findViewById(R.id.surfaceview3);
        textureView4 = (TextureView) findViewById(R.id.surfaceview4);
    }

    /** Called when the activity is about to be paused. */
    @Override
    protected void onPause()
    {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    /** Called when the activity is about to be destroyed. */
    @Override
    protected void onDestroy()
    {
        client.destory();
        client2.destory();
        client3.destory();
        client4.destory();
        mCreated = false;
        super.onDestroy();
    }

    public void onStartClick(View view) {
        startRtsp();
    }

    public void onStopClick(View view) {
//        client.stop();
        client2.stop();
//        client3.stop();
//        client4.stop();
    }

    public void onCaptureClick(View view) {
        if (view instanceof TextureView) {
            TextureView  textureView = (TextureView) view;
            Bitmap bmp = textureView.getBitmap();
            ((ImageView) findViewById(R.id.image)).setImageBitmap(bmp);
        }
    }
}
