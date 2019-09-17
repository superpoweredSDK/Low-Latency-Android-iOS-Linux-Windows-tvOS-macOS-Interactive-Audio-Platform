Pod::Spec.new do |s|
 s.name = 'Superpowered'
 s.version = '0.0.1'
 s.license = { :type => 'Commercial' }
 s.authors = { 'Gabor Szanto' => 'gabor@superpowered.com' }
 s.homepage = 'https://superpowered.com'
 s.summary = 'Superpowered Audio, Networking and Cryptographics SDK'
 s.platform = :osx, :ios, :tvos
 s.requires_arc = false
 s.source = { :path => './' }
 s.static_framework = true
 s.source_files = '*.{cpp,mm,h}'
 s.exclude_files = 'SuperpoweredAndroidUSB.h'
 
#macOS
 s.osx.deployment_target = '10.10'
 s.osx.source_files = '/OpenSource/SuperpoweredOSXAudioIO.h', '/OpenSource/SuperpoweredOSXAudioIO.mm'
 s.osx.framework = 'AppKit', 'AVFoundation', 'AudioToolbox', 'AudioUnit', 'CoreAudio', 'CoreMedia'
 s.osx.vendored_libraries = 'libSuperpoweredAudioOSX.a'

#iOS
 s.ios.deployment_target = '9.3'
 s.ios.source_files = '/OpenSource/SuperpoweredIOSAudioIO.h', '/OpenSource/SuperpoweredIOSAudioIO.mm'
 s.ios.framework = 'Foundation', 'AVFoundation', 'AudioToolbox', 'CoreAudio', 'CoreMedia', 'UIKit'
 s.ios.vendored_libraries = 'libSuperpoweredAudioIOS.a'
 
#tvOS
 s.tvos.deployment_target = '9.0'
 s.tvos.source_files = '/OpenSource/SuperpoweredtvOSAudioIO.h', '/OpenSource/SuperpoweredtvOSAudioIO.mm'
 s.tvos.framework = 'Foundation', 'AVFoundation', 'AudioToolbox', 'CoreMedia', 'UIKit', 'MediaPlayer'
 s.tvos.vendored_libraries = 'libSuperpoweredAudiotvOS.a'

end

