package com.av.samples;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

public class ReversePlayerActivity extends AppCompatActivity implements SurfaceHolder.Callback, ReversePlayer.Listener {


    private static final String TAG = "ReversePlayerActivity";

    public static final String KEY_VIDEO_PATH = "video_path";
    public static final String KEY_TEMP_DIR = "video_temp_dir";

    private ReversePlayer mPlayer;
    private SurfaceView mSurfaceView;
    private String mVideoPath;
    private String mVideoTempDir;
    private Handler mHandler;
    private Button mStartBtn;

    public static void startActivity(Context context, String videoPath, String videoTempDir) {
        Intent intent = new Intent(context, ReversePlayerActivity.class);
        intent.putExtra(KEY_VIDEO_PATH, videoPath);
        intent.putExtra(KEY_TEMP_DIR, videoTempDir);
        context.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_player);
        mSurfaceView = findViewById(R.id.surface_view);
        mSurfaceView.getHolder().addCallback(this);
        findViewById(R.id.btn_start).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startPlayerYuv();
            }
        });
        Intent intent = getIntent();
        mVideoPath = intent.getStringExtra(KEY_VIDEO_PATH);
        mVideoTempDir = intent.getStringExtra(KEY_TEMP_DIR);
        mPlayer = new ReversePlayer();
        mPlayer.setListener(this);
        mHandler = new Handler();
    }

    public void startPlayerYuv() {
        Toast.makeText(this, "开始倒序播放", Toast.LENGTH_SHORT).show();
        Log.i(TAG, " start Reverse video");
        mPlayer.init(mSurfaceView.getHolder().getSurface(), mSurfaceView.getWidth(), mSurfaceView.getHeight());
        mPlayer.setDataSource(mVideoPath, mVideoTempDir);
        mPlayer.prepare();
        mPlayer.start();
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mPlayer.seekTo(20000);
            }
        }, 3000);
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mPlayer.seekTo(0);
            }
        }, 4500);
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mPlayer.seekTo(30000);
            }
        }, 5000);
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mPlayer.seekTo(10000);
            }
        }, 6000);
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mPlayer.seekTo(100000);
            }
        }, 7000);
    }

    @Override
    protected void onStop() {
        super.onStop();
        mPlayer.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mPlayer.release();
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {

    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int format, int width, int height) {
//        Log.i(TAG, " player.prepare() url: " + mVideoPath);

    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }

    @Override
    public void onPlayerStateChanged(int state) {

    }

    @Override
    public void onVideoSizeChanged(int width, int height) {
        Log.i(TAG, "onVideoSizeChanged width=" + width + ",height=" + height);
    }

    @Override
    public void onSeekComplete() {

    }

    @Override
    public void onError(int errorCode) {

    }

    @Override
    public void onProgressUpdated(long pos, long duration) {
        Log.i(TAG, "onProgressUpdated pos=" + pos + ",duration=" + duration);
    }
}
