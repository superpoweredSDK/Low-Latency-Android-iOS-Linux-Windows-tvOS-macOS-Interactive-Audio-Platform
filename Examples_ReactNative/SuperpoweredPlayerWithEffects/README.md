# Superpowered Player with Effect React Native Sample App

This Sample App demonstrates how to integrate Superpowered SDK with React Native.

## Prerequisites
Ensure that you have `node` and `npm` or `yarn` installed.

See [React Native's Getting Started Guide](https://reactnative.dev/docs/environment-setup).

To try out this example project:

* Run `npm install` or `yarn install`.

## How to run:
* Android: `npm run android` or `yarn run android`
* iOS: Before the first install please make sure to run `pod install` from the `ios` folder. After you can call: `npm run ios` or `npm run ios`.

## Create a Turbo Native Module
In order to use the Superpowered SDK in a React Native application you need to set up a Turbo native module that you can import in your JavaScript code. You need to create a separate native module for all platforms that you support. See React Native's [Turbo Native Modules documentation](https://reactnative.dev/docs/next/turbo-native-modules-introduction).

### iOS Native Module
Integrate the Superpowered SDK into the React Native generated iOS project by following the [iOS Integration Guide](https://docs.superpowered.com/getting-started/how-to-integrate/ios?lang=cpp).

You can put together your module in an [Objective-C file](ios/RCTNativeSuperpoweredEngine.mm) and only expose the methods that you will need to call from JavaScript. For further info check out [this guide](https://reactnative.dev/docs/next/turbo-native-modules-introduction?platforms=ios#4-write-your-native-platform-code).

### Android Native Module
Integrate the Superpowered SDK into the React Native generated Android project by following the [Android Integration Guide](https://docs.superpowered.com/getting-started/how-to-integrate/android?lang=cpp).

You can put together your module in a [Java/Kotlin file](android/app/src/main/java/com/superpoweredplayerwitheffects/NativeSuperpoweredEngineModule.java) and only expose the methods that you will need to call from JavaScript. For further info check out [this guide](https://reactnative.dev/docs/next/turbo-native-modules-introduction?platforms=android#4-write-your-native-platform-code).

### Using the Native Module from JavaScript
After the Turbo native module is set up you can import it and call it in your [JavaScript code](./App.tsx).
