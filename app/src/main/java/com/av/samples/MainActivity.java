package com.av.samples;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.av.samples.decoder.AudioDecoder;
import com.tbruyelle.rxpermissions2.RxPermissions;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("media");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.btn_decoder_audio).setOnClickListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        final RxPermissions permissions = new RxPermissions(this);
        permissions.request(Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.CAMERA,
                Manifest.permission.RECORD_AUDIO,
                Manifest.permission.READ_PHONE_STATE,
                Manifest.permission.MODIFY_AUDIO_SETTINGS)
                .subscribe(granted -> {
                }, throwable -> {

                });

    }


    public void startDecoderAudio() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                String srcFile = "/storage/emulated/0/Android/data/com.av.samples/files/ws_30.mp3";
                String desFile = "/storage/emulated/0/Android/data/com.av.samples/files/ws_30.pcm";
                AudioDecoder decoder = new AudioDecoder();
                decoder.decodeAudio(srcFile, desFile);
            }
        }).start();
    }


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.btn_decoder_audio:
                startDecoderAudio();
                break;
        }
    }
}