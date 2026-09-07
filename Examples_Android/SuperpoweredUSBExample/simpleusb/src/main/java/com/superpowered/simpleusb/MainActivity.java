package com.superpowered.simpleusb;

import android.os.Handler;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.view.View;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import android.widget.TextView;

import java.util.Locale;

public class MainActivity extends AppCompatActivity implements SuperpoweredUSBAudioHandler {
    private Handler handler;
    private TextView textView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        applyWindowInsets();
        textView = findViewById(R.id.text);

        SuperpoweredUSBAudio usbAudio = new SuperpoweredUSBAudio(getApplicationContext(), this);
        usbAudio.check();

        // Update UI every 40 ms.
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                int[] midi = getLatestMidiMessage();
                switch (midi[0]) {
                    case 8: textView.setText(String.format(Locale.ENGLISH, "Note Off, CH %d, %d, %d",
                            midi[1] + 1, midi[2], midi[3]));
                            break;
                    case 9: textView.setText(String.format(Locale.ENGLISH, "Note On, CH %d, %d, %d",
                            midi[1] + 1, midi[2], midi[3]));
                            break;
                    case 11: textView.setText(String.format(Locale.ENGLISH, "Control Change, CH %d, %d, %d",
                            midi[1] + 1, midi[2], midi[3]));
                            break;
                }
                handler.postDelayed(this, 40);
            }
        };
        handler = new Handler();
        handler.postDelayed(runnable, 40);
    }

    public void onUSBAudioDeviceAttached(int deviceIdentifier) {
    }

    public void onUSBMIDIDeviceAttached(int deviceIdentifier) {
    }

    public void onUSBDeviceDetached(int deviceIdentifier) {
    }

    // Function implemented in the native library.
    private native int[] getLatestMidiMessage();

    static {
        System.loadLibrary("SuperpoweredExample");
    }

    // Edge-to-edge is enforced from targetSdk 35. AppCompat offsets the action bar by the status
    // bar inset but lays the content out above it, so pad the content by the same amount. The
    // dispatched insets arrive already consumed, hence the root window insets.
    private void applyWindowInsets() {
        final View root = findViewById(R.id.root);
        final int left = root.getPaddingLeft(), top = root.getPaddingTop();
        final int right = root.getPaddingRight(), bottom = root.getPaddingBottom();
        ViewCompat.setOnApplyWindowInsetsListener(root, (view, windowInsets) -> {
            WindowInsetsCompat rootInsets = ViewCompat.getRootWindowInsets(view);
            Insets bars = (rootInsets != null ? rootInsets : windowInsets)
                    .getInsets(WindowInsetsCompat.Type.systemBars());
            view.setPadding(left + bars.left, top + bars.top, right + bars.right, bottom + bars.bottom);
            return windowInsets;
        });
    }
}
