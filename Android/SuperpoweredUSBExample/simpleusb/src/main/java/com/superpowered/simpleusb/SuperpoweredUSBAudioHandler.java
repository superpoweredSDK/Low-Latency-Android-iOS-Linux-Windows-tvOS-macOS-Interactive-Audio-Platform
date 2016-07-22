package com.superpowered.simpleusb;

public interface SuperpoweredUSBAudioHandler {
    void onUSBAudioDeviceAttached(int deviceID);
    void onUSBMIDIDeviceAttached(int deviceID);
    void onUSBDeviceDetached(int deviceID);
}


