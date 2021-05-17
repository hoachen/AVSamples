package com.av.samples;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.av.samples.demux.SeiParser;
import com.tbruyelle.rxpermissions2.RxPermissions;

import java.io.File;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, Transcode.TranscodeListener {

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
        findViewById(R.id.preview_player).setOnClickListener(this);
        findViewById(R.id.transcode).setOnClickListener(this);

        final String srcVideoFilePath = getExternalCacheDir() + "/video.mp4";
        Log.i("Transcode", "srcVideoFilePath:" + srcVideoFilePath);

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
            case R.id.preview_player:
                startPreviewPlayer();
                break;

            case R.id.transcode:
                startTranscode();
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



    private void startPreviewPlayer() {
        final String path = getExternalFilesDir("") + "/";
        String video_path = path + "VID_20210220_180401.mp4";
        PreviewPlayerActivity.startActivity(this, video_path);
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

    private void startTranscode() {


//        MediaFormat targetVideoFormat = new MediaFormat();
//        targetVideoFormat.setString(MediaFormat.KEY_MIME, "video/avc");
//        targetVideoFormat.setInteger(MediaFormat.KEY_BIT_RATE, 1500000); // 设置输出视频码率
//        targetVideoFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 3); // 设置gop
//        targetVideoFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 15); // 设置gop
//        targetVideoFormat.setInteger(MediaFormat.KEY_WIDTH, 240); //
//        targetVideoFormat.setInteger(MediaFormat.KEY_HEIGHT, 426); //480x854
//
//        MediaFormat targetAudioFormat = new MediaFormat(); // 设置音频参数
//        targetAudioFormat.setString(MediaFormat.KEY_MIME, "audio/mp4a-latm");
//        targetAudioFormat.setInteger(MediaFormat.KEY_BIT_RATE, 32000);
////        int size = AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT);
//        targetAudioFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 65540);
//        targetAudioFormat.setInteger(MediaFormat.KEY_CHANNEL_COUNT, 2);
//        targetAudioFormat.setInteger(MediaFormat.KEY_SAMPLE_RATE, 44100);
//        targetAudioFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);

        final String srcVideoFilePath = getExternalCacheDir() + "/video.mp4";
        Log.i("Transcode", "srcVideoFilePath:" + srcVideoFilePath);
//        Uri sourceVideoUri = Uri.parse(srcVideoFilePath);
        final String targetVideoFilePath = getExternalCacheDir() + "/new_video.mp4";

        new Thread(new Runnable() {
            @Override
            public void run() {
                Transcode transcode = new Transcode();
                transcode.setTranscodeListener(MainActivity.this);
                transcode.init(srcVideoFilePath, targetVideoFilePath, 240, 426, 15, 500000, 44100, 2, 32000);
                transcode.start();
                transcode.release();
            }
        }).start();
    }


    @Override
    public void onTranscodeStart() {
        Toast.makeText(this, "onTranscodeStart", Toast.LENGTH_SHORT).show();

    }

    @Override
    public void onTranscodeFailed(int error) {
        Toast.makeText(this, "onTranscodeFailed err" + error, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onTranscodeComplete() {
        Toast.makeText(this, "onTranscodeComplete", Toast.LENGTH_SHORT).show();
    }
}