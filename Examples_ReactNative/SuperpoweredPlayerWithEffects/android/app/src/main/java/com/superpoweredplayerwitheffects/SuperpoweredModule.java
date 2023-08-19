package com.superpoweredplayerwitheffects;

import androidx.annotation.NonNull;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;

public class SuperpoweredModule extends ReactContextBaseJavaModule {

    private static ReactApplicationContext _reactContext;
    SuperpoweredModule(ReactApplicationContext context) {
        super(context);
        _reactContext = context;
        System.loadLibrary("SuperpoweredPlayerWithEffects");
    }

    @NonNull
    @Override
    public String getName() {
        return "SuperpoweredModule";
    }

    @ReactMethod
    public void initSuperpowered() {
        nativeInit(_reactContext.getApplicationContext().getCacheDir().getAbsolutePath());
    }

    @ReactMethod
    public void togglePlayback() {
        nativeTogglePlayback();
    }

    @ReactMethod
    public void enableFlanger(boolean enable) {
        nativeEnableFlanger(enable);
    }

    private native void nativeInit(String tempdir);
    private native void nativeTogglePlayback();
    private native void nativeEnableFlanger(boolean enable);
}