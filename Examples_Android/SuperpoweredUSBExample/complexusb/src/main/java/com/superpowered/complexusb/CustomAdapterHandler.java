package com.superpowered.complexusb;

public interface CustomAdapterHandler {
    float onVolume(float volume, int index);
    boolean onMute(boolean mute, int index);
}
