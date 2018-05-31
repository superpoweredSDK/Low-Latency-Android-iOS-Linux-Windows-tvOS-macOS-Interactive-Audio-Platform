package com.superpowered.effect;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {
    private boolean playing = false;
    private int samplerate;
    private int buffersize;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Checking permissions.
        String[] permissions = {
                Manifest.permission.RECORD_AUDIO
        };
        for (String s:permissions) {
            if (ContextCompat.checkSelfPermission(this, s) != PackageManager.PERMISSION_GRANTED) {
                // Some permissions are not granted, ask the user.
                ActivityCompat.requestPermissions(this, permissions, 0);
                return;
            }
        }

        // Got all permissions, initialize.
        initialize();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
        // Called when the user answers to the permission dialogs.
        if ((requestCode != 0) || (grantResults.length < 1) || (grantResults.length != permissions.length)) return;
        boolean hasAllPermissions = true;

        for (int grantResult:grantResults) if (grantResult != PackageManager.PERMISSION_GRANTED) {
            hasAllPermissions = false;
            Toast.makeText(getApplicationContext(), "Please allow all permissions for the app.", Toast.LENGTH_LONG).show();
        }

        if (hasAllPermissions) initialize();
    }

    private void initialize() {
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
        samplerate = Integer.parseInt(samplerateString);
        buffersize = Integer.parseInt(buffersizeString);
        System.loadLibrary("EffectExample");    // load native library
    }

    // Handle Start/Stop button toggle.
    public void ToggleStartStop(View button) {
        if (playing) {
            StopAudio();
            playing = false;
        } else {
            StartAudio(samplerate, buffersize);
            playing = true;
        }
        Button b = findViewById(R.id.startStop);
        b.setText(playing ? "Stop" : "Start");
    }

    @Override
    public void onPause() {
        super.onPause();
        if (playing) onBackground();
    }

    @Override
    public void onResume() {
        super.onResume();
        if (playing) onForeground();
    }

    protected void onDestroy() {
        super.onDestroy();
        if (playing) StopAudio();
    }

    // Functions implemented in the native library.
    private native void StartAudio(int samplerate, int buffersize);
    private native void StopAudio();
    private native void onForeground();
    private native void onBackground();
}
