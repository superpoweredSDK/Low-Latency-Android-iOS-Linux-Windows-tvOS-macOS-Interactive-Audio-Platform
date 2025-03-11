package com.superpoweredplayerwitheffects

import com.facebook.react.bridge.ReactApplicationContext

class NativeSuperpoweredEngineModule(reactContext: ReactApplicationContext) : NativeSuperpoweredEngineSpec(reactContext) {

    init {
        System.loadLibrary("SuperpoweredPlayerWithEffects")
    }

    override fun getName() = NAME

    override fun initSuperpowered() {
        nativeInit(reactApplicationContext.cacheDir.absolutePath)
    }

    override fun togglePlayback() {
        nativeTogglePlayback()
    }

    override fun enableFlanger(enable: Boolean) {
        nativeEnableFlanger(enable)
    }

    private external fun nativeInit(tempdir: String)
    private external fun nativeTogglePlayback()
    private external fun nativeEnableFlanger(enable: Boolean)

    companion object {
        const val NAME = "NativeSuperpoweredEngine"
    }
}