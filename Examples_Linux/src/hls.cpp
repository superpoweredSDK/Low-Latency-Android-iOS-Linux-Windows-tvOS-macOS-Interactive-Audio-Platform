#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <math.h>
#include "SuperpoweredAdvancedAudioPlayer.h"

typedef struct {
    unsigned int samplerate;
    unsigned int numChannels;
    unsigned int periodSize;
    int pollDescriptorsCount;
    snd_pcm_t *handle;
    float *buffer;
    struct pollfd *ufds;
} alsaPCMContext;

static bool underrunRecovery(snd_pcm_t *pcmHandle, int error) {
    if (error == -EPIPE) {
        error = snd_pcm_prepare(pcmHandle);
        if (error < 0) printf("underrun recovery 1, snd_pcm_prepare error %s\n", snd_strerror(error));
        return true;
    } else if (error == -ESTRPIPE) {
        while ((error = snd_pcm_resume(pcmHandle)) == -EAGAIN) sleep(1);

        if (error < 0) {
            error = snd_pcm_prepare(pcmHandle);
            if (error < 0) printf("underrun recovery 2, snd_pcm_prepare error %s\n", snd_strerror(error));
        }
        return true;
    }
    return (error >= 0);
}

static bool waitForPoll(alsaPCMContext *context, bool *init) {
    unsigned short revents;
    while (1) {
        poll(context->ufds, context->pollDescriptorsCount, -1);
        snd_pcm_poll_descriptors_revents(context->handle, context->ufds, context->pollDescriptorsCount, &revents);

        if (revents & POLLOUT) return true;

        if (revents & POLLERR) {
            snd_pcm_state_t state = snd_pcm_state(context->handle);

            if ((state == SND_PCM_STATE_XRUN) || (state == SND_PCM_STATE_SUSPENDED)) {
                int error = (state == SND_PCM_STATE_XRUN) ? -EPIPE : -ESTRPIPE;
                if (!underrunRecovery(context->handle, error)) {
                    printf("wait for poll write error: %s\n", snd_strerror(error));
                    return false;
                }
                *init = true;
            } else {
                printf("wait for poll failed\n");
                return false;
            }
        }
    }
    return true;
}

static void destroyContext(alsaPCMContext *context) {
    snd_pcm_drain(context->handle);
    snd_pcm_close(context->handle);
    free(context->buffer);
    free(context->ufds);
}

static bool setupContext(alsaPCMContext *context) {
    context->handle = NULL;
    context->periodSize = 0;

    unsigned int bufferTimeUs = 500000, periodTimeUs = 100000;
    snd_pcm_uframes_t bufferSize = 0;
    int dir;

    int error = snd_pcm_open(&context->handle, "sysdefault", SND_PCM_STREAM_PLAYBACK, 0);
    if (error < 0) {
        printf("snd_pcm_open error %s\n", snd_strerror(error));
        return false;
    }

    snd_pcm_hw_params_t *hwParams;
    snd_pcm_hw_params_alloca(&hwParams);

    error = snd_pcm_hw_params_any(context->handle, hwParams);
    if (error < 0) {
        printf("snd_pcm_hw_params_any error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_hw_params_set_rate_resample(context->handle, hwParams, 0);
    if (error < 0) {
        printf("snd_pcm_hw_params_set_rate_resample error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_hw_params_set_access(context->handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (error < 0) {
        printf("snd_pcm_hw_params_set_access error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_hw_params_set_format(context->handle, hwParams, SND_PCM_FORMAT_FLOAT_LE);
    if (error < 0) {
        printf("snd_pcm_hw_params_set_format error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_hw_params_set_channels(context->handle, hwParams, context->numChannels);
    if (error < 0) {
        printf("snd_pcm_hw_params_set_channels error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_hw_params_set_rate_near(context->handle, hwParams, &context->samplerate, 0);
    if (error < 0) {
        printf("snd_pcm_hw_params_set_rate_near error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_hw_params_set_buffer_time_near(context->handle, hwParams, &bufferTimeUs, &dir);
    if (error < 0) {
        printf("snd_pcm_hw_params_set_buffer_time error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_hw_params_get_buffer_size(hwParams, &bufferSize);
    if (error < 0) {
        printf("snd_pcm_hw_params_get_buffer_size error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_hw_params_set_period_time_near(context->handle, hwParams, &periodTimeUs, &dir);
    if (error < 0) {
        printf("snd_pcm_hw_params_set_period_time_near error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    snd_pcm_uframes_t frames = 0;
    error = snd_pcm_hw_params_get_period_size(hwParams, &frames, &dir);
    if (error < 0) {
        printf("snd_pcm_hw_params_get_period_size error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }
    context->periodSize = (unsigned int)frames;

    error = snd_pcm_hw_params(context->handle, hwParams);
    if (error < 0) {
        printf("snd_pcm_hw_params error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    snd_pcm_sw_params_t *swParams;
    snd_pcm_sw_params_alloca(&swParams);

    error = snd_pcm_sw_params_current(context->handle, swParams);
    if (error < 0) {
        printf("snd_pcm_sw_params_current error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_sw_params_set_start_threshold(context->handle, swParams, (bufferSize / context->periodSize) * context->periodSize);
    if (error < 0) {
        printf("snd_pcm_sw_params_set_start_threshold error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_sw_params_set_avail_min(context->handle, swParams, context->periodSize);
    if (error < 0) {
        printf("snd_pcm_sw_params_set_avail_min error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_sw_params(context->handle, swParams);
    if (error < 0) {
        printf("snd_pcm_sw_params error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    context->pollDescriptorsCount = snd_pcm_poll_descriptors_count(context->handle);
    if (context->pollDescriptorsCount <= 0) {
        printf("invalid poll descriptors count %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        return false;
    }

    context->ufds = (pollfd *)malloc(sizeof(struct pollfd) * context->pollDescriptorsCount);
    if (!context->ufds) {
        printf("out of memory\n");
        snd_pcm_close(context->handle);
        return false;
    }

    error = snd_pcm_poll_descriptors(context->handle, context->ufds, context->pollDescriptorsCount);
    if (error < 0) {
        printf("snd_pcm_poll_descriptors error %s\n", snd_strerror(error));
        snd_pcm_close(context->handle);
        free(context->ufds);
        return false;
    }

    context->buffer = (float *)malloc(context->periodSize * context->numChannels * 8);
    if (!context->buffer) {
        printf("out of memory\n");
        snd_pcm_close(context->handle);
        free(context->ufds);
        return false;
    }

    printf("buffer size: %i, period size: %i, sample rate: %i\n", bufferSize, context->periodSize, context->samplerate);
    return true;
}

static SuperpoweredAdvancedAudioPlayer *player = NULL;

static void playerEventCallback(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    switch (event) {
        case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess:
            printf("Load success.\n");
            player->play(false);
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoadError: printf("Load error: %s", (char *)value); break;
        case SuperpoweredAdvancedAudioPlayerEvent_EOF: player->seek(0); break;
        case SuperpoweredAdvancedAudioPlayerEvent_DurationChanged: break;
        default:;
    };
}

int main(int argc, char *argv[]) {
    {
        char **hints;
        int error = snd_device_name_hint(-1, "pcm", (void ***)&hints);
        if (error != 0) {
            printf("snd_device_name_hint error %i\n", error);
            return 0;
        }

        if (*hints == NULL) {
            printf("\nNo audio devices found.\n");
            snd_device_name_free_hint((void **)hints);
            return 0;
        }

        printf("\nAudio Devices:\n");
        char **hint = hints;
        while (*hint != NULL) {
            char *deviceName = snd_device_name_get_hint(*hint, "NAME"), *deviceIO = snd_device_name_get_hint(*hint, "IOID");
            if ((deviceName != NULL) && (strcmp("null", deviceName) != 0)) {
                printf("%s", deviceName);
                if ((deviceIO != NULL) && (strcmp("null", deviceIO) != 0)) printf(" %s", deviceIO); else printf(" Input/Output");
                printf("\n");
            }
            hint++;
        }
        printf("\n");
        snd_device_name_free_hint((void **)hints);
    }

    alsaPCMContext context;
    context.samplerate = 48000;
    context.numChannels = 2;
    if (!setupContext(&context)) return 0;

    SuperpoweredAdvancedAudioPlayer::setTempFolder("/tmp/");
    player = new SuperpoweredAdvancedAudioPlayer(NULL, playerEventCallback, context.samplerate, 0);
    player->open("http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8");

    bool init = true;

    int numTurns = 1, numSamples = context.periodSize;
    if (context.periodSize > 1024) {
        div_t d = div(context.periodSize, 1024);
        numTurns = d.quot;
        if (d.rem > 0) numTurns++;
        numSamples = context.periodSize / numTurns;
    }
    printf("Superpowered turns: %i, samples per turn: %i\n\nPress ENTER to quit.\n", numTurns, numSamples);

    bool run = true;
    while (run) {
        if (!init && !waitForPoll(&context, &init)) break;

        float *buf = context.buffer;
        for (int n = 0; n < numTurns; n++) {
            if (!player->process(buf, false, numSamples)) memset(buf, 0, numSamples * 8);
            buf += numSamples * 2;
        }

        float *ptr = context.buffer;
        int cptr = context.periodSize;

        while (cptr > 0) {
            int error = snd_pcm_writei(context.handle, ptr, cptr);
            if (error < 0) {
                if (!underrunRecovery(context.handle, error)) {
                    printf("underrun recovery write error: %s\n", snd_strerror(error));
                    run = 0;
                    break;
                }
                init = true;
                printf("skip one period\n");
                break;
            }

            if (snd_pcm_state(context.handle) == SND_PCM_STATE_RUNNING) init = false;

            ptr += error * context.numChannels;
            cptr -= error;
            if (cptr == 0) break;

            if (!waitForPoll(&context, &init)) {
                run = false;
                break;
            }
        }

        struct timeval tv = { 0, 0 };
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        if (select(1, &fds, NULL, NULL, &tv)) break;
    }

    destroyContext(&context);
    if (player) delete(player);
    return 0;
}
