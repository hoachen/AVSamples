package com.av.samples.base;

public interface GLTextureInputRenderer {

    void newTextureReady(int textureId, GLTextureOutputRenderer source, boolean newData, long presetTimeUs);

    void registerTextureIndices(int textureId, GLTextureOutputRenderer source);

    void unregisterTextureIndices(int textureId);

    int nextAvalibleTextureIndices();
}
