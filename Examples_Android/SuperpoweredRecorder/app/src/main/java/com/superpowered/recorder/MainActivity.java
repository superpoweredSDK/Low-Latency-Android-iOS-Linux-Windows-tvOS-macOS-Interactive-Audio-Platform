package com.superpowered.recorder;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import java.io.FileNotFoundException;

public class MainActivity extends AppCompatActivity {
    private boolean recording = false;
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
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
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
        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        if (audioManager != null) {
            samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
            buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        }
        if (samplerateString == null) samplerateString = "48000";
        if (buffersizeString == null) buffersizeString = "480";
        samplerate = Integer.parseInt(samplerateString);
        buffersize = Integer.parseInt(buffersizeString);

        System.loadLibrary("RecorderExample");  // Load native library.
    }

    private void updateButton() {
        Button b = findViewById(R.id.startStop);
        b.setText(recording ? "Stop" : "Start");
    }

    // Handle the return of the save as dialog.
    public void onActivityResult(int requestCode, int resultCode, Intent resultData) {
        super.onActivityResult(requestCode, resultCode, resultData);
        if (resultCode == android.app.Activity.RESULT_OK) {
            if ((requestCode == 0) && (resultData != null)) {
                Uri u = resultData.getData();
                try {
                    ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(u, "w");
                    if (pfd != null) {
                        StartAudio(samplerate, buffersize, pfd.detachFd());
                        recording = true;
                        updateButton();
                    } else Log.d("Recorder", "File descriptor is null.");
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    // Handle Start/Stop button toggle.
    public void ToggleStartStop(View button) {
        if (recording) {
            StopRecording();
            recording = false;
            updateButton();
        } else {
            // Open the file browser to pick a destination.
            android.content.Intent intent = new android.content.Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/octet-stream");
            intent.putExtra(Intent.EXTRA_TITLE, "recording.wav");
            startActivityForResult(intent, 0);
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (recording) onBackground();
    }

    @Override
    public void onResume() {
        super.onResume();
        if (recording) onForeground();
    }

    protected void onDestroy() {
        super.onDestroy();
        if (recording) StopRecording();
    }

    // Functions implemented in the native library.
    private native void StartAudio(int samplerate, int buffersize, int destinationfd);
    private native void onForeground();
    private native void onBackground();
    private native void StopRecording();
}
