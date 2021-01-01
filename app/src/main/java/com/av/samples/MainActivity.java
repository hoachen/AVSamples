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
        findViewById(R.id.btn_save_yuv).setOnClickListener(this);
        findViewById(R.id.btn_player_yuv).setOnClickListener(this);
        findViewById(R.id.btn_reverse_video).setOnClickListener(this);
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
                startSplitVideo();
                break;
            case R.id.btn_save_yuv:
                saveToYuvFile();
                break;
            case R.id.btn_player_yuv:
                playerYuvFile();
                break;
            case R.id.btn_reverse_video:
                reversePlayVideo();
                break;
        }
    }

    private void startSplitVideo() {
        final String path = getExternalFilesDir("") + "/";
        Log.i("CHHH", "path:" + path);
        new Thread(new Runnable() {
            @Override
            public void run() {
                String inputFile = path + "uaJ9p480p.mp4";
                String outputDir = path + "uaJ9p480p_temp";
                File file = new File(outputDir);
                file.deleteOnExit();
                if (!file.exists()) {
                    file.mkdirs();
                }
                Log.i("CHHH", "split to video start");
                VideoSplit split = new VideoSplit();
                int fileCount = split.splitVideo(inputFile, outputDir);
                Log.i("CHHH", "split to video end file count:" + fileCount);
            }
        }).start();
    }

    private void saveToYuvFile() {
        final String path = getExternalFilesDir("") + "/";
        Log.i("CHHH", "path:" + path);
        new Thread(new Runnable() {
            @Override
            public void run() {
//                String inputFile = path + "new_video_temps/temp_0.mp4";
                String outputDir = path + "uaJ9p480p_temp";
                File file = new File(outputDir);
                file.deleteOnExit();
                if (!file.exists()) {
                    file.mkdirs();
                }
                VideoSplit split = new VideoSplit();
                for (int i = 0; i < 2; i++) {
                    String inputFile = path + "uaJ9p480p_temp/temp_" + i + ".mp4";
                    String outputFile = path + "uaJ9p480p_temp/temp_" + i + ".yuv";
                    Log.i("CHHH", "decoder" + i + " video to yuv start");
                    int fileCount = split.splitVideoToYuv(inputFile, outputFile);
                    Log.i("CHHH", "decoder" + i + " video to yuv end fileCount:" + fileCount);
                }
            }
        }).start();
    }

    private void playerYuvFile() {
        final String path = getExternalFilesDir("") + "/";
        String url = path + "uaJ9p480p_temp/temp_" + 0 + ".yuv";
        YUVPlayerActivity.startActivity(this, url, 480, 854);
    }

    private void reversePlayVideo() {
        final String path = getExternalFilesDir("") + "/";
        String video_path = path + "new_video2.mp4";
        String tempDir = path + "reverse_video/";
        File file = new File(tempDir);
        file.deleteOnExit();
        if (!file.exists()) {
            file.mkdirs();
        }
        ReversePlayerActivity.startActivity(this, video_path, tempDir);
    }
}