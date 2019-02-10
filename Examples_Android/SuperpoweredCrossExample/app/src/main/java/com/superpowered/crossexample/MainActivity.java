package com.superpowered.crossexample;

import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.media.AudioManager;
import android.content.Context;
import android.content.pm.PackageManager;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.os.Build;
import android.support.annotation.NonNull;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Button;
import android.widget.Toast;
import android.Manifest;

public class MainActivity extends AppCompatActivity {
    private boolean playing = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Checking permissions.
        String[] permissions = {
                Manifest.permission.MODIFY_AUDIO_SETTINGS
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
        int samplerate = Integer.parseInt(samplerateString);
        int buffersize = Integer.parseInt(buffersizeString);

        // Files under res/raw are not zipped, just copied into the APK.
		// Get the offset and length to know where our files are located.
        try {
            InputStream inputStream = getResources().openRawResource(R.raw.lycka);
            FileOutputStream outputFile = new FileOutputStream(getExternalFilesDir(null) + "lycka.mp3");
            copyStream(inputStream, outputFile);
        } catch (IOException e) {
            Log.e("PlayerExample", "Copy error.");
        }
        try {
            InputStream inputStream = getResources().openRawResource(R.raw.nuyorica);
            FileOutputStream outputFile = new FileOutputStream(getExternalFilesDir(null) + "nuyorica.m4a");
            copyStream(inputStream, outputFile);
        } catch (IOException e) {
            Log.e("PlayerExample", "Copy error.");
        }

        // Initialize the players and effects, and start the audio engine.
        System.loadLibrary("CrossExample");
        CrossExample(
                samplerate,     // sampling rate
                buffersize,     // buffer size
                getExternalFilesDir(null) + "lycka.mp3",
                getExternalFilesDir(null) + "nuyorica.m4a"
        );

        // Setup crossfader events
        final SeekBar crossfader = findViewById(R.id.crossFader);
        if (crossfader != null) crossfader.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                onCrossfader(progress);
            }
            public void onStartTrackingTouch(SeekBar seekBar) {}
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        // Setup FX fader events
        final SeekBar fxfader = findViewById(R.id.fxFader);
        if (fxfader != null) fxfader.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                onFxValue(progress);
            }
            public void onStartTrackingTouch(SeekBar seekBar) {
                onFxValue(seekBar.getProgress());
            }
            public void onStopTrackingTouch(SeekBar seekBar) {
                onFxOff();
            }
        });

        // Setup FX select event
        final RadioGroup group = findViewById(R.id.radioGroup1);
        if (group != null) group.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                RadioButton checkedRadioButton = radioGroup.findViewById(checkedId);
                onFxSelect(radioGroup.indexOfChild(checkedRadioButton));
            }
        });
    }

/*
        try {
        InputStream inputStream = getResources().openRawResource(R.raw.track);
        FileOutputStream outputFile = new FileOutputStream(getExternalFilesDir(null) + "track.mp3");
        copyStream(inputStream, outputFile);
    } catch (IOException e) {
        Log.e("PlayerExample", "Copy error.");
    }
    OpenFile(getExternalFilesDir(null) + "track.mp3");         // open audio file from APK
*/

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

    // PlayPause - Toggle playback state of the player.
    public void CrossExample_PlayPause(View button) {
        playing = !playing;
        onPlayPause(playing);
        Button b = findViewById(R.id.playPause);
        if (b != null) b.setText(playing ? "Pause" : "Play");
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    // Functions implemented in the native library.
    private native void CrossExample(int samplerate, int buffersize, String pathA, String pathB);
    private native void onPlayPause(boolean play);
    private native void onCrossfader(int value);
    private native void onFxSelect(int value);
    private native void onFxOff();
    private native void onFxValue(int value);
}
