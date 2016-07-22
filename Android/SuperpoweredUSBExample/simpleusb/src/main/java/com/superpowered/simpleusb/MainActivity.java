package com.superpowered.simpleusb;

import android.app.Instrumentation;
import android.os.Handler;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.KeyEvent;
import android.widget.TextView;

import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity implements SuperpoweredUSBAudioHandler {
    private Handler handler;
    private TextView textView;
    private Timer fakeTouchTimer;
    private int numAttachedDevices;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        textView = (TextView)findViewById(R.id.text);
        numAttachedDevices = 0;

        SuperpoweredUSBAudio usbAudio = new SuperpoweredUSBAudio(getApplicationContext(), this);
        usbAudio.check();

        // Update UI every 40 ms.
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                int[] midi = getLatestMidiMessage();
                switch (midi[0]) {
                    case 8: textView.setText(String.format(Locale.ENGLISH, "Note Off, CH %d, %d, %d", midi[1] + 1, midi[2], midi[3])); break;
                    case 9: textView.setText(String.format(Locale.ENGLISH, "Note On, CH %d, %d, %d", midi[1] + 1, midi[2], midi[3])); break;
                    case 11: textView.setText(String.format(Locale.ENGLISH, "Control Change, CH %d, %d, %d", midi[1] + 1, midi[2], midi[3])); break;
                }
                handler.postDelayed(this, 40);
            }
        };
        handler = new Handler();
        handler.postDelayed(runnable, 40);
    }

    public void onUSBAudioDeviceAttached(int deviceIdentifier) {
        if (++numAttachedDevices == 1) {
            TimerTask fakeTouchTask = new TimerTask() {
                public void run() {
                    try {
                        Instrumentation instrumentation = new Instrumentation();
                        instrumentation.sendKeyDownUpSync(KeyEvent.KEYCODE_BACKSLASH);
                    } catch(java.lang.Exception e) {
                        assert true;
                    }
                }
            };
            fakeTouchTimer = new Timer();
            fakeTouchTimer.schedule(fakeTouchTask, 1000, 1000);
        }
    }

    public void onUSBMIDIDeviceAttached(int deviceIdentifier) {

    }

    public void onUSBDeviceDetached(int deviceIdentifier) {
        if (--numAttachedDevices == 0) {
            fakeTouchTimer.cancel();
            fakeTouchTimer.purge();
        }
    }

    private native int[]getLatestMidiMessage();

    static {
        System.loadLibrary("simpleusb");
    }
}
