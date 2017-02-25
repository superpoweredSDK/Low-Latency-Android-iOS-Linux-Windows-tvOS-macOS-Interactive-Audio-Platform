package com.superpowered.complexusb;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;

import java.io.ByteArrayOutputStream;

// This class handles USB device permissions, attaching and detaching a device.
public class SuperpoweredUSBAudio {
    private static final String ACTION_USB_PERMISSION = "com.superpowered.USBAudio.USB_PERMISSION";
    private PendingIntent permissionIntent;
    private Context context;
    private SuperpoweredUSBAudioHandler handler;

    public SuperpoweredUSBAudio(Context c, SuperpoweredUSBAudioHandler h) {
        context = c;
        handler = h;
        permissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);

        BroadcastReceiver usbReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();

                if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) { // USB device attached.
                    UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (device != null) {
                        UsbManager manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
                        if (manager != null) {
                            if (manager.hasPermission(device)) addUSBDevice(device); // Open if we have permission.
                            else manager.requestPermission(device, permissionIntent); // Request permission otherwise.
                        }
                    }
                } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) { // USB device detached.
                    UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (device != null) {
                        int id = device.getDeviceId();
                        onDisconnect(id);
                        if (handler != null) handler.onUSBDeviceDetached(id);
                    }
                } else if (ACTION_USB_PERMISSION.equals(action)) { // Permission granted by the user.
                    UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (device != null) addUSBDevice(device);
                }
            }
        };

        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(ACTION_USB_PERMISSION);
        context.registerReceiver(usbReceiver, filter);
    }

    private void addUSBDevice(UsbDevice device) {
        UsbManager manager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
        if (manager == null) return;
        UsbDeviceConnection connection = manager.openDevice(device);
        if (connection != null) {
            int id = device.getDeviceId();
            switch (onConnect(id, connection.getFileDescriptor(), connection.getRawDescriptors())) {
                case 1: if (handler != null) handler.onUSBAudioDeviceAttached(id); break; // Audio device.
                case 2: if (handler != null) handler.onUSBMIDIDeviceAttached(id); break; // MIDI device.
                case 3: // Audio and MIDI device.
                    if (handler != null) handler.onUSBAudioDeviceAttached(id);
                    if (handler != null) handler.onUSBMIDIDeviceAttached(id);
                    break;
            }
        }
    }

    private native int onConnect(int deviceID, int fd, byte[] rawDescriptor);
    private native void onDisconnect(int deviceID);

    // This can be called after an instance of SuperpoweredUSBAudio is created in order to recognize already connected devices.
    public void check() {
        UsbManager manager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
        if (manager != null) for (UsbDevice device : manager.getDeviceList().values()) addUSBDevice(device);
    }
}
