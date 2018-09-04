Pod::Spec.new do |s|
 s.name = 'Superpowered'
 s.version = '0.0.1'
 s.license = { :type => 'Commercial' }
 s.authors = { 'Gabor' => 'gabor@superpowered.com' }
 s.homepage = 'https://superpowered.com'
 s.summary = 'Superpowered audio SDK'
 s.platform = :osx, :ios, :tvos
 s.requires_arc = false
 s.source = { :path => './' }
 s.source_files = '*.{cpp,mm,h}'
 s.static_framework = true
 
# OSX
 s.osx.deployment_target = '10.10'
 s.osx.exclude_files = 'SuperpoweredIOSAudioIO.h', 'SuperpoweredIOSAudioIO.mm', 'SuperpoweredtvOSAudioIO.h', 'SuperpoweredtvOSAudioIO.mm', 'SuperpoweredWindowsAudioIO.cpp', 'SuperpoweredWindowsAudioIO.h'
 s.osx.framework = 'AppKit', 'AVFoundation', 'AudioToolbox', 'AudioUnit', 'CoreAudio', 'CoreMedia'
 s.osx.vendored_libraries = 'libSuperpoweredAudioOSX.a'

#IOS
 s.ios.deployment_target = '9.0'
 s.ios.exclude_files = 'SuperpoweredOSXAudioIO.h', 'SuperpoweredOSXAudioIO.mm', 'SuperpoweredtvOSAudioIO.h', 'SuperpoweredtvOSAudioIO.mm', 'SuperpoweredWindowsAudioIO.cpp', 'SuperpoweredWindowsAudioIO.h'
 s.ios.framework = 'Foundation', 'AVFoundation', 'AudioToolbox', 'CoreAudio', 'CoreMedia', 'UIKit'
 s.ios.vendored_libraries = 'libSuperpoweredAudioIOS.a'
 
#tvOS
 s.tvos.deployment_target = '9.0'
 s.tvos.exclude_files = 'SuperpoweredOSXAudioIO.h', 'SuperpoweredOSXAudioIO.mm', 'SuperpoweredIOSAudioIO.h', 'SuperpoweredIOSAudioIO.mm', 'SuperpoweredWindowsAudioIO.cpp', 'SuperpoweredWindowsAudioIO.h'
 s.tvos.framework = 'Foundation', 'AVFoundation', 'AudioToolbox', 'CoreMedia', 'UIKit', 'MediaPlayer'
 s.tvos.vendored_libraries = 'libSuperpoweredAudiotvOS.a'

end

