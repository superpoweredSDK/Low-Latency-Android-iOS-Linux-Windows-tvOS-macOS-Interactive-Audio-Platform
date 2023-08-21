# Superpowered Player with Effect React Native Sample App

This Sample App demonstrates how to integrate Superpowered SDK with React Native.

## How to run:
* Android: `npm run android`
* iOS: Before the first install please make sure to run `pod install` from the `ios` folder. After you can call: `npm run ios`. 

## Prerequisites
Ensure that you have `node` and `npm` or `yarn` installed. See [React Native's Getting Started Guide](https://reactnative.dev/docs/environment-setup).

## Create a Native Module
In order to use the Superpowered SDK in a React Native application you need to set up a native module that you can import in your JavaScript code. You need to create a separate native module for all platforms that you support. See React Native's [Native Modules documentation](https://reactnative.dev/docs/native-modules-intro).

### iOS Native Module
Integrate the Superpowered SDK into the React Native generated iOS project by following the [iOS Integration Guide](https://docs.superpowered.com/getting-started/how-to-integrate/ios?lang=cpp).

You can put together your module in a [Swift file](ios/SuperpoweredModule.swift) and only expose the methods that you will need to call from JavaScript. To export the Swift methods you need to create an [Objective-C file](ios/SuperpoweredModule.m) that marks the methods external.

### Android Native Module
Integrate the Superpowered SDK into the React Native generated Android project by following the [Android Integration Guide](https://docs.superpowered.com/getting-started/how-to-integrate/android?lang=cpp).

You can put together your module in a [Java/Kotlin file](android/app/src/main/java/com/superpoweredplayerwitheffects/SuperpoweredModule.java) and only expose the methods that you will need to call from JavaScript. The methods you want to export are marked with `@ReactMethod.`

### Using the Native Module from JavaScript
After the native module is set up you can import it and call it in your [JavaScript code](./App.tsx).