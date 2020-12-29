package com.av.samples;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.av.samples.demux.VideoSplit;
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
        findViewById(R.id.btn_split_video).setOnClickListener(this);
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



    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.btn_split_video:
                Log.i("CHHH", "start startSplitVideo");
                startSplitVideo();
                break;
        }
    }

    private void startSplitVideo() {
        final String path = getExternalFilesDir("") + "/";
        Log.i("CHHH", "path:" + path);
        new Thread(new Runnable() {
            @Override
            public void run() {
                String inputFile = path + "new_video.mp4";
                String outputDir = path + "new_temps";
                File file = new File(outputDir);
                if (!file.exists()) {
                    file.mkdirs();
                }
                VideoSplit split = new VideoSplit();
                split.splitVideo(inputFile, outputDir);
                Log.i("CHHH", "startSplitVideo end");
            }
        }).start();
    }
}