#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint superpowered_player_with_effects.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'superpowered_player_with_effects'
  s.version          = '0.0.1'
  s.summary          = 'A new Flutter FFI plugin project.'
  s.description      = <<-DESC
A new Flutter FFI plugin project.
                       DESC
  s.homepage         = 'http://example.com'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Your Company' => 'email@example.com' }

  # This will ensure the source files in Classes/ are included in the native
  # builds of apps using this FFI plugin. Podspec does not support relative
  # paths, so Classes contains a forwarder C file that relatively imports
  # `../src/*` so that the C sources can be shared among all target platforms.
  s.source           = { :path => '.' }
  s.source_files = 'Classes/**/*.{cpp,mm,h}'
  s.dependency 'Flutter'
  s.platform = :ios, '11.0'
  s.ios.framework = 'Foundation', 'AVFoundation', 'AudioToolbox', 'CoreAudio', 'CoreMedia'

  s.vendored_libraries = 'libSuperpoweredAudio.xcframework/ios-arm64/libSuperpoweredAudioIOS-iphoneos.a'
  # s.vendored_libraries = 'libSuperpoweredAudio.xcframework/ios-arm64_x86_64-simulator/libSuperpoweredAudioIOS-iphonesimulator.a'
  end

  # Flutter.framework does not contain a i386 slice.
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES', 'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386' }
  s.swift_version = '5.0'
end
