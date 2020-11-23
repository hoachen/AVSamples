package com.av.samples.decoder;

import android.util.Log;

public class AudioDecoder {

   private static final String TAG = "AudioDecoder";

   public AudioDecoder() {

   }

   public void decodeAudio(String srcFile, String desFile) {
      Log.i(TAG, "start decode");
      int result = _decodeAudio(srcFile, desFile);
      if (result == 0) {
         Log.i(TAG, "decodeAudio Success save to " + desFile);
      } else {
         Log.i(TAG, "decodeAudio failed");
      }
   }

   private native int _decodeAudio(String src, String des);
}
