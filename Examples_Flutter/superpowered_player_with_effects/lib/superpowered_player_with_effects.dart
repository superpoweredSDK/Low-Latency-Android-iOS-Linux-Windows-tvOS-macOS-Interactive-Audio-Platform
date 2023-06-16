
import 'dart:async';
import 'dart:ffi';
import 'package:ffi/ffi.dart';
import 'dart:io';

import 'superpowered_player_with_effects_bindings_generated.dart';

void initialize(String tempDir) => _bindings.initialize(tempDir.toNativeUtf8() as Pointer<Char>);
void togglePlayback() => _bindings.togglePlayback();
void enableFlanger(bool enable) => _bindings.enableFlanger(enable);
void playerDispose() => _bindings.playerDispose();


const String _libName = 'superpowered_player_with_effects';

/// The dynamic library in which the symbols for [SuperpoweredPlayerWithEffectsBindings] can be found.
final DynamicLibrary _dylib = () {
  if (Platform.isMacOS || Platform.isIOS) {
    return DynamicLibrary.open('$_libName.framework/$_libName');
  }
  if (Platform.isAndroid || Platform.isLinux) {
    return DynamicLibrary.open('lib$_libName.so');
  }
  if (Platform.isWindows) {
    return DynamicLibrary.open('$_libName.dll');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

/// The bindings to the native functions in [_dylib].
final SuperpoweredPlayerWithEffectsBindings _bindings = SuperpoweredPlayerWithEffectsBindings(_dylib);

