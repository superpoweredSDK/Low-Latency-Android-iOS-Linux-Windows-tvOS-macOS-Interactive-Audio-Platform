package com.superpowered.complexusb;

public interface SuperpoweredUSBAudioHandler {
    void onUSBAudioDeviceAttached(int deviceID);
    void onUSBMIDIDeviceAttached(int deviceID);
    void onUSBDeviceDetached(int deviceID);
}


