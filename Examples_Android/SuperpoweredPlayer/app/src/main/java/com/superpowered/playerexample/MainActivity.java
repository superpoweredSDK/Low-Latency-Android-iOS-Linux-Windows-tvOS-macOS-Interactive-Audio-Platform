package com.superpowered.playerexample;

import android.support.v7.app.AppCompatActivity;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.content.Context;
import android.widget.Button;
import android.view.View;
import android.util.Log;
import android.os.Build;
import android.os.Bundle;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;

public class MainActivity extends AppCompatActivity {

    // Used to load the native 'PlayerExample' library on application startup.
    static {
        System.loadLibrary("PlayerExample");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Get the device's sample rate and buffer size to enable
        // low-latency Android audio output, if available.
        String samplerateString = null, buffersizeString = null;
        if (Build.VERSION.SDK_INT >= 17) {
            AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
            if (audioManager != null) {
                samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
                buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
            }
        }
        if (samplerateString == null) samplerateString = "48000";
        if (buffersizeString == null) buffersizeString = "480";
        int samplerate = Integer.parseInt(samplerateString);
        int buffersize = Integer.parseInt(buffersizeString);

        // Files under res/raw are not zipped, just copied into the APK.
        // Get the offset and length to know where our file is located.
        AssetFileDescriptor fd = getResources().openRawResourceFd(R.raw.track);
        int fileOffset = (int)fd.getStartOffset();
        int fileLength = (int)fd.getLength();
        try {
            fd.getParcelFileDescriptor().close();
        } catch (IOException e) {
            Log.e("PlayerExample", "Close error.");
        }
        StartAudio(samplerate, buffersize);             // start audio engine
        try {
            InputStream inputStream = getResources().openRawResource(R.raw.track);
            FileOutputStream outputFile = new FileOutputStream(getExternalFilesDir(null) + "track.mp3");
            copyStream(inputStream, outputFile);
        } catch (IOException e) {
            Log.e("PlayerExample", "Copy error.");
        }
        OpenFile(getExternalFilesDir(null) + "track.mp3");         // open audio file from APK
    }

    public static void copyStream(InputStream input, OutputStream output)
            throws IOException
    {
        byte[] buffer = new byte[1024]; // Adjust if you want
        int bytesRead;
        while ((bytesRead = input.read(buffer)) != -1) // test for EOF
        {
            output.write(buffer, 0, bytesRead);
        }
        output.close();
    }

    // Handle Play/Pause button toggle.
    public void PlayerExample_PlayPause (View button) {
        TogglePlayback();
        playing = !playing;
        Button b = findViewById(R.id.playPause);
        b.setText(playing ? "Pause" : "Play");
    }

    @Override
    public void onPause() {
        super.onPause();
        onBackground();
    }

    @Override
    public void onResume() {
        super.onResume();
        onForeground();
    }

    protected void onDestroy() {
        super.onDestroy();
        Cleanup();
    }

    // Functions implemented in the native library.
    private native void StartAudio(int samplerate, int buffersize);
    private native void OpenFile(String path);
    private native void TogglePlayback();
    private native void onForeground();
    private native void onBackground();
    private native void Cleanup();

    private boolean playing = false;
}
