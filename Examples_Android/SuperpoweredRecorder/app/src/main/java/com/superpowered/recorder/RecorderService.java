package com.superpowered.recorder;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Build;
import android.os.IBinder;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

public class RecorderService extends Service {
    public static final String CHANNELID = "RecorderServiceChannel";

    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String action = intent.getAction();
        if ((action != null) && action.equals("stop")) {
            stopForeground(true);
            stopSelf();
            return START_NOT_STICKY;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel serviceChannel = new NotificationChannel(CHANNELID, "Foreground Service Channel", NotificationManager.IMPORTANCE_DEFAULT);
            NotificationManager manager = getSystemService(NotificationManager.class);
            if (manager != null) manager.createNotificationChannel(serviceChannel);
        }

        Notification notification = new NotificationCompat.Builder(this, CHANNELID).setContentTitle("Recorder Service").build();
        startForeground(1, notification);

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
        int samplerate = Integer.parseInt(samplerateString);
        int buffersize = Integer.parseInt(buffersizeString);

        System.loadLibrary("RecorderExample");  // Load native library.
        StartAudio(samplerate, buffersize, intent.getIntExtra("fileDescriptor", 0));
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        StopRecording();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    // Functions implemented in the native library.
    private native void StartAudio(int samplerate, int buffersize, int destinationfd);
    private native void StopRecording();
}
