package com.av.samples.output;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import com.av.samples.base.GLTextureOutputRenderer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class MediaVideoView extends FrameLayout {


    public MediaVideoView(@NonNull Context context) {
        super(context);
    }

    public MediaVideoView(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public MediaVideoView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void bindToOutput(GLTextureOutputRenderer outputRenderer) {
    }
}
