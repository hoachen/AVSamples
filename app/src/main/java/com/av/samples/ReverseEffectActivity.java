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

public class ReverseEffectActivity extends AppCompatActivity implements SurfaceHolder.Callback {


    private static final String TAG = "ReverseEffect";

    public static final String KEY_VIDEO_PATH = "video_path";

    private SurfaceView mSurfaceView;
    private String mVideoPath;
    private ReverseEffect mReverseEffect;

    public static void startActivity(Context context, String videoPath) {
        Intent intent = new Intent(context, ReverseEffectActivity.class);
        intent.putExtra(KEY_VIDEO_PATH, videoPath);
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
            }
        });
        Intent intent = getIntent();
        mVideoPath = intent.getStringExtra(KEY_VIDEO_PATH);
        mReverseEffect = new ReverseEffect();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {

            Log.i(TAG, "start reverse effect:" + mVideoPath);
            new Thread(new Runnable() {
                @Override
                public void run() {
                    try {
                        mReverseEffect.start(mVideoPath, surfaceHolder.getSurface());
                    } catch (Exception e) {
                        e.printStackTrace();
                        Log.i(TAG, e.getMessage());
                    }
                }
            }).start();


    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int format, int width, int height) {
//        Log.i(TAG, " player.prepare() url: " + mVideoPath);

    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }
}
