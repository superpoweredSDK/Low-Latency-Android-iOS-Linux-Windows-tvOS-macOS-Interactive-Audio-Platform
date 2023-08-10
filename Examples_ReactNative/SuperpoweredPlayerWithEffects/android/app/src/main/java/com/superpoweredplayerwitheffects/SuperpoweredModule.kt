package com.superpoweredplayerwitheffects

import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod

class SuperpoweredModule(private var reactContext: ReactApplicationContext) : ReactContextBaseJavaModule(reactContext) {
    init {
        System.loadLibrary("SuperpoweredPlayerWithEffects")
    }
    override fun getName() = "SuperpoweredModule"

    @ReactMethod
    fun init() {
        nativeInit(reactContext.applicationContext.cacheDir.absolutePath)
    }

    @ReactMethod
    fun togglePlayback() {
        nativeTogglePlayback()
    }

    @ReactMethod
    fun enableFlanger(enable: Boolean) {
        nativeEnableFlanger(enable)
    }

    private external fun nativeInit(tempDir:String)
    private external fun nativeTogglePlayback()
    private external fun nativeEnableFlanger(enable: Boolean)
}