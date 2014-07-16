#import <UIKit/UIKit.h>

@interface ViewController: UIViewController
    @property (nonatomic, retain) IBOutlet UIProgressView *progressView;
@end

// Some WAVE file writing helpers.

typedef struct RIFFWAVE {
    unsigned char RIFF[4];
    unsigned int chunkSize;
    unsigned char WAVE[4];
    unsigned char FMT[4];
    unsigned int sixteen;
    unsigned short int audioFormat;
    unsigned short int numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    unsigned short int blockAlign;
    unsigned short int bitsPerSample;
    unsigned char DATA[4];
    unsigned int dataSize;
} RIFFWAVE; // This is how a simple 16-bit PCM stereo WAV file header looks like.

static void writeWAVEHeader(FILE *fd, unsigned int samplerate) {
    RIFFWAVE header;
    memcpy(header.RIFF, "RIFF", 4);
    memcpy(header.WAVE, "WAVE", 4);
    memcpy(header.FMT, "fmt ", 4);
    header.sixteen = 16;
    header.audioFormat = 1;
    header.numChannels = 2;
    header.bitsPerSample = 16;
    header.sampleRate = samplerate;
    header.byteRate = samplerate * header.numChannels * (header.bitsPerSample >> 3);
    header.blockAlign = header.numChannels * (header.bitsPerSample >> 3);
    memcpy(header.DATA, "data", 4);
    fwrite(&header, 1, sizeof(header), fd);
}

static void writeWAVESize(FILE *fd, unsigned int audioSampleCount) {
    unsigned int bytes = audioSampleCount * 4;
    fseek(fd, 40, SEEK_SET);
    fwrite(&bytes, 1, 4, fd);
    bytes += 36;
    fseek(fd, 4, SEEK_SET);
    fwrite(&bytes, 1, 4, fd);
}
