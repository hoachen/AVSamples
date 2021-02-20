package com.av.samples;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.av.samples.demux.SeiParser;
import com.tbruyelle.rxpermissions2.RxPermissions;

import java.io.File;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("media");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.btn_reverse_video).setOnClickListener(this);
        findViewById(R.id.btn_sei_parser).setOnClickListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        final RxPermissions permissions = new RxPermissions(this);
        permissions.request(Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.CAMERA,
                Manifest.permission.RECORD_AUDIO)
                .subscribe(granted -> {
                }, throwable -> {

                });
    }


    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.btn_reverse_video:
                reversePlayVideo();
                break;
            case R.id.btn_sei_parser:
                startSeiParser();
                break;
        }
    }

    private void reversePlayVideo() {
        final String path = getExternalFilesDir("") + "/";
        String video_path = path + "VID_20210220_180401.mp4";
        String tempDir = path + "reverse_video";
        File file = new File(tempDir);
        file.deleteOnExit();
        if (!file.exists()) {
            file.mkdirs();
        }
        ReversePlayerActivity.startActivity(this, video_path, tempDir);
    }


    private void startSeiParser() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                String url = "http://video-live.watch-it.video/slivee/2uQ_nsVh-Aq_hdshowl.flv";
                SeiParser._parser(url);
            }
        }).start();
    }
}