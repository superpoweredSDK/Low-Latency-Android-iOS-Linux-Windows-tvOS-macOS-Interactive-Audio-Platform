//
//  ViewController.m
//  SuperpoweredOfflineAnalyzerExample
//
//  Created by Nathan on 3/31/16.
//  Copyright © 2016 com.imect. All rights reserved.
//

#import "ViewController.h"
#import <AVFoundation/AVFoundation.h>
#import "SuperpoweredAnalyzer.h"
#import "SuperpoweredDecoder.h"
#import "SuperpoweredAudioBuffers.h"
#import "SuperpoweredSimple.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)showResult:(SuperpoweredOfflineAnalyzer *)analyzer duration:(int)durationSeconds
{
    unsigned char **averageWaveForm = (unsigned char **)malloc(150 * sizeof(unsigned char *));
    unsigned char **peakWaveForm = (unsigned char **)malloc(150 * sizeof(unsigned char *));
    unsigned char **lowWaveForm = (unsigned char **)malloc(150 * sizeof(unsigned char *));
    unsigned char **midWaveForm = (unsigned char **)malloc(150 * sizeof(unsigned char *));
    unsigned char **highWaveForm = (unsigned char **)malloc(150 * sizeof(unsigned char *));
    
    unsigned char **notes = (unsigned char **)malloc(150 * sizeof(unsigned char *));
    
    char **overViewWaveForm = (char **)malloc(durationSeconds * sizeof(char *));
    
    int *keyIndex = (int *)malloc(sizeof(int));
    int *waveFormSize = (int *)malloc(sizeof(int));
    
    float *averageDecibel = (float *)malloc(sizeof(float));
    float *loudPartsAverageDecibel = (float *)malloc(sizeof(float));
    float *peakDecibel = (float *)malloc(sizeof(float));
    float *bpm = (float *)malloc(sizeof(float));
    float *beatGridStart = (float *)malloc(sizeof(float));
    
    analyzer->getresults(averageWaveForm, peakWaveForm, lowWaveForm, midWaveForm, highWaveForm, notes, waveFormSize, overViewWaveForm, averageDecibel, loudPartsAverageDecibel, peakDecibel, bpm, beatGridStart, keyIndex);
    
    NSLog(@"bpm:%f", *bpm);
    
    free(averageWaveForm);
    free(peakWaveForm);
    free(lowWaveForm);
    free(midWaveForm);
    free(highWaveForm);
    free(notes);
    free(overViewWaveForm);
    free(keyIndex);
    free(waveFormSize);
    free(averageDecibel);
    free(loudPartsAverageDecibel);
    free(peakDecibel);
    free(bpm);
    free(beatGridStart);
}

- (void)analysisFile
{
    NSString *path = [[NSBundle mainBundle] pathForResource:@"bpm120" ofType:@"wav"];
    SuperpoweredDecoder *decoder = new SuperpoweredDecoder();
    const char *openError = decoder->open([path UTF8String]);
    if(openError)
    {
        NSLog(@"%s", openError);
        delete decoder;
        return;
    }
    
    int samplerate = decoder->samplerate;
    int duration = decoder->durationSeconds;
    NSLog(@"samplerate:%i duration:%i", samplerate, duration);
    
    SuperpoweredAudiobufferPool *bufferPool = new SuperpoweredAudiobufferPool(4, 1024 * 1024);
    SuperpoweredOfflineAnalyzer *analyzer = new SuperpoweredOfflineAnalyzer(samplerate, 0, duration);
    short int *intBuffer = (short int *)malloc(decoder->samplesPerFrame * 2 * sizeof(short int) + 16384);
    int samplesMultiplier = 4;
    
    while (true) {
        unsigned int samplesDecoded = decoder->samplesPerFrame * samplesMultiplier;
        
        if (decoder->decode(intBuffer, &samplesDecoded) != SUPERPOWEREDDECODER_OK) break;
        if(samplesDecoded == 0) break;
        NSLog(@"%i", samplesDecoded);
        SuperpoweredAudiobufferlistElement inputBuffer;
        bufferPool->createSuperpoweredAudiobufferlistElement(&inputBuffer, decoder->samplePosition, samplesDecoded + 8);
        
        SuperpoweredShortIntToFloat(intBuffer, bufferPool->floatAudio(&inputBuffer), samplesDecoded);
     
        inputBuffer.endSample = samplesDecoded;             // <-- Important!
        analyzer->process(bufferPool->floatAudio(&inputBuffer), samplesDecoded);
    }
    
    [self showResult:analyzer duration:duration];
    
    delete decoder;
    delete bufferPool;
    free(intBuffer);
    delete analyzer;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self analysisFile];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
