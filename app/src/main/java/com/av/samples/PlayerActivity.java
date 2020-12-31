package com.av.samples;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

public class PlayerActivity  extends AppCompatActivity implements SurfaceHolder.Callback {

    private static final String TAG = "PlayerActivity";

    public static final String KEY_URL = "url";
    public static final String KEY_VIDEO_WIDTH = "video_width";
    public static final String KEY_VIDEO_HEIGHT = "video_height";

    private YUVPlayer player;

    private SurfaceView mSurfaceView;

    private String url;
    private int videoWidth;
    private int videoHeight;

    public static void startActivity(Context context, String url, int width, int height) {
        Intent intent = new Intent(context, PlayerActivity.class);
        intent.putExtra(KEY_URL, url);
        intent.putExtra(KEY_VIDEO_WIDTH, width);
        intent.putExtra(KEY_VIDEO_HEIGHT, height);
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
        url = intent.getStringExtra(KEY_URL);
        videoWidth = intent.getIntExtra(KEY_VIDEO_WIDTH, 0);
        videoHeight = intent.getIntExtra(KEY_VIDEO_HEIGHT, 0);
        player = new YUVPlayer();
    }

    public void startPlayerYuv() {
        Log.i(TAG, " startPlayerYuv");
        player.start();
    }

    @Override
    protected void onStop() {
        super.onStop();
        player.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.release();
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {

    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int format, int width, int height) {
        Log.i(TAG, " player.prepare() url: " + url);
        player.init(surfaceHolder.getSurface(), width, height, videoWidth, videoHeight, url);
        player.prepare();
        Log.i(TAG, " player.prepare() end");
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }
}
