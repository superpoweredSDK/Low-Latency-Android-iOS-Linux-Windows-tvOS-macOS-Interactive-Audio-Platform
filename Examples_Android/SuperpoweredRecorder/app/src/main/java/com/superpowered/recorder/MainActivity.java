package com.superpowered.recorder;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
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
    private ActivityResultLauncher<Intent>fileBrowserLauncher = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        fileBrowserLauncher = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
            // Handle the return of the save as dialog.
            if (result.getResultCode() == android.app.Activity.RESULT_OK) {
                Intent resultData = result.getData();
                if (resultData != null) {
                    Uri u = resultData.getData();
                    try {
                        ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(u, "w");
                        if (pfd != null) {
                            Intent serviceIntent = new Intent(this, RecorderService.class);
                            serviceIntent.putExtra("fileDescriptor", pfd.detachFd());
                            ContextCompat.startForegroundService(this, serviceIntent);
                            recording = true;
                            updateButton();
                        } else Log.d("Recorder", "File descriptor is null.");
                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                    }
                }
            }
        });
        setContentView(R.layout.activity_main);
        Button b = findViewById(R.id.startStop);
        b.setVisibility(View.GONE);

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

        // Got all permissions, show button.
        showButton();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        // Called when the user answers to the permission dialogs.
        if ((requestCode != 0) || (grantResults.length < 1) || (grantResults.length != permissions.length)) return;
        boolean hasAllPermissions = true;

        for (int grantResult:grantResults) if (grantResult != PackageManager.PERMISSION_GRANTED) {
            hasAllPermissions = false;
            Toast.makeText(getApplicationContext(), "Please allow all permissions for the app.", Toast.LENGTH_LONG).show();
        }

        if (hasAllPermissions) showButton();
    }

    private void showButton() {
        Button b = findViewById(R.id.startStop);
        b.setVisibility(View.VISIBLE);
    }

    private void updateButton() {
        Button b = findViewById(R.id.startStop);
        b.setText(recording ? "Stop" : "Start");
    }

    // Handle Start/Stop button toggle.
    public void ToggleStartStop(View button) {
        if (recording) {
            Intent serviceIntent = new Intent(this, RecorderService.class);
            serviceIntent.setAction("stop");
            startService(serviceIntent);
            recording = false;
            updateButton();
        } else {
            // Open the file browser to pick a destination.
            android.content.Intent intent = new android.content.Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/octet-stream");
            intent.putExtra(Intent.EXTRA_TITLE, "recording.wav");
            fileBrowserLauncher.launch(intent);
        }
    }
}
