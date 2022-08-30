#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <math.h>
#include "Superpowered.h"
#include "SuperpoweredAdvancedAudioPlayer.h"

typedef struct ALSAContext {
    snd_pcm_t *handle;
    float *buffer;
    struct pollfd *ufds;
    unsigned int samplerate, numChannels, periodSize;
    int pollDescriptorsCount;

    void destroy() {
        if (this->buffer) {
            snd_pcm_drain(this->handle);
            snd_pcm_close(this->handle);
            free(this->buffer);
            free(this->ufds);
        }
    }

    bool setup() {
        this->handle = NULL;
        this->buffer = NULL;
        this->ufds = NULL;
        this->periodSize = 0;

        unsigned int bufferTimeUs = 500000, periodTimeUs = 100000;
        snd_pcm_uframes_t bufferSize = 0;
        int dir;

        int error = snd_pcm_open(&this->handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
        if (error < 0) {
            printf("snd_pcm_open error %s\n", snd_strerror(error));
            return false;
        }

        snd_pcm_hw_params_t *hwParams;
        snd_pcm_hw_params_alloca(&hwParams);

        error = snd_pcm_hw_params_any(this->handle, hwParams);
        if (error < 0) {
            printf("snd_pcm_hw_params_any error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_hw_params_set_rate_resample(this->handle, hwParams, 0);
        if (error < 0) {
            printf("snd_pcm_hw_params_set_rate_resample error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_hw_params_set_access(this->handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (error < 0) {
            printf("snd_pcm_hw_params_set_access error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_hw_params_set_format(this->handle, hwParams, SND_PCM_FORMAT_FLOAT_LE);
        if (error < 0) {
            printf("snd_pcm_hw_params_set_format error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_hw_params_set_channels(this->handle, hwParams, this->numChannels);
        if (error < 0) {
            printf("snd_pcm_hw_params_set_channels error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_hw_params_set_rate_near(this->handle, hwParams, &this->samplerate, 0);
        if (error < 0) {
            printf("snd_pcm_hw_params_set_rate_near error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_hw_params_set_buffer_time_near(this->handle, hwParams, &bufferTimeUs, &dir);
        if (error < 0) {
            printf("snd_pcm_hw_params_set_buffer_time error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_hw_params_get_buffer_size(hwParams, &bufferSize);
        if (error < 0) {
            printf("snd_pcm_hw_params_get_buffer_size error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_hw_params_set_period_time_near(this->handle, hwParams, &periodTimeUs, &dir);
        if (error < 0) {
            printf("snd_pcm_hw_params_set_period_time_near error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        snd_pcm_uframes_t frames = 0;
        error = snd_pcm_hw_params_get_period_size(hwParams, &frames, &dir);
        if (error < 0) {
            printf("snd_pcm_hw_params_get_period_size error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }
        this->periodSize = (unsigned int)frames;

        error = snd_pcm_hw_params(this->handle, hwParams);
        if (error < 0) {
            printf("snd_pcm_hw_params error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        snd_pcm_sw_params_t *swParams;
        snd_pcm_sw_params_alloca(&swParams);

        error = snd_pcm_sw_params_current(this->handle, swParams);
        if (error < 0) {
            printf("snd_pcm_sw_params_current error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_sw_params_set_start_threshold(this->handle, swParams, (bufferSize / this->periodSize) * this->periodSize);
        if (error < 0) {
            printf("snd_pcm_sw_params_set_start_threshold error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_sw_params_set_avail_min(this->handle, swParams, this->periodSize);
        if (error < 0) {
            printf("snd_pcm_sw_params_set_avail_min error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_sw_params(this->handle, swParams);
        if (error < 0) {
            printf("snd_pcm_sw_params error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        this->pollDescriptorsCount = snd_pcm_poll_descriptors_count(this->handle);
        if (this->pollDescriptorsCount <= 0) {
            printf("invalid poll descriptors count %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            return false;
        }

        this->ufds = (pollfd *)malloc(sizeof(struct pollfd) * this->pollDescriptorsCount);
        if (!this->ufds) {
            printf("out of memory\n");
            snd_pcm_close(this->handle);
            return false;
        }

        error = snd_pcm_poll_descriptors(this->handle, this->ufds, this->pollDescriptorsCount);
        if (error < 0) {
            printf("snd_pcm_poll_descriptors error %s\n", snd_strerror(error));
            snd_pcm_close(this->handle);
            free(this->ufds);
            return false;
        }

        this->buffer = (float *)malloc(this->periodSize * this->numChannels * 8);
        if (!this->buffer) {
            printf("out of memory\n");
            snd_pcm_close(this->handle);
            free(this->ufds);
            return false;
        }

        printf("buffer size: %i, period size: %i, sample rate: %i\n", bufferSize, this->periodSize, this->samplerate);
        return true;
    }

    bool underrunRecovery(int error) {
        if (error == -EPIPE) {
            error = snd_pcm_prepare(this->handle);
            if (error < 0) printf("underrun recovery 1, snd_pcm_prepare error %s\n", snd_strerror(error));
            return true;
        } else if (error == -ESTRPIPE) {
            while ((error = snd_pcm_resume(this->handle)) == -EAGAIN) sleep(1);

            if (error < 0) {
                error = snd_pcm_prepare(this->handle);
                if (error < 0) printf("underrun recovery 2, snd_pcm_prepare error %s\n", snd_strerror(error));
            }
            return true;
        }
        return (error >= 0);
    }

    bool waitForPoll(bool *init) {
        unsigned short revents;
        while (1) {
            poll(this->ufds, this->pollDescriptorsCount, -1);
            snd_pcm_poll_descriptors_revents(this->handle, this->ufds, this->pollDescriptorsCount, &revents);

            if (revents & POLLOUT) return true;

            if (revents & POLLERR) {
                snd_pcm_state_t state = snd_pcm_state(this->handle);

                if ((state == SND_PCM_STATE_XRUN) || (state == SND_PCM_STATE_SUSPENDED)) {
                    int error = (state == SND_PCM_STATE_XRUN) ? -EPIPE : -ESTRPIPE;
                    if (!underrunRecovery(error)) {
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

    bool printAudioDevices() {
        char **hints;
        int error = snd_device_name_hint(-1, "pcm", (void ***)&hints);
        if (error != 0) {
            printf("snd_device_name_hint error %i\n", error);
            return false;
        }

        if (*hints == NULL) {
            printf("\nNo audio devices found.\n");
            snd_device_name_free_hint((void **)hints);
            return false;
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
        return true;
    }
} ALSAContext;

int main(int argc, char *argv[]) {
    ALSAContext context;
    if (!context.printAudioDevices()) return 0;
    context.samplerate = 48000;
    context.numChannels = 2;
    if (!context.setup()) return 0;

    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");

    Superpowered::AdvancedAudioPlayer::setTempFolder("/tmp/");
    Superpowered::AdvancedAudioPlayer *player = new Superpowered::AdvancedAudioPlayer(context.samplerate, 0);
    player->openHLS("http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8");

    bool init = true;

    int numTurns = 1, numFrames = context.periodSize;
    if (context.periodSize > 1024) {
        div_t d = div(context.periodSize, 1024);
        numTurns = d.quot;
        if (d.rem > 0) numTurns++;
        numFrames = context.periodSize / numTurns;
    }
    printf("Superpowered turns: %i, frames per turn: %i\n\nPress ENTER to quit.\n", numTurns, numFrames);

    bool run = true;
    while (run) {
        if (!init && !context.waitForPoll(&init)) break;

        switch (player->getLatestEvent()) {
            case Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened:
                printf("\rLoad success.\n");
                player->play();
                break;
            case Superpowered::AdvancedAudioPlayer::PlayerEvent_ConnectionLost:
                printf("\rConnection Lost\n");
                run = false;
                break;
            case Superpowered::AdvancedAudioPlayer::PlayerEvent_OpenFailed:
            {
                int openError = player->getOpenErrorCode();
                printf("\rOpen error %i: %s\n", openError, Superpowered::AdvancedAudioPlayer::statusCodeToString(openError));
                run = false;
            }
                break;
            default:;
        }

        if (player->eofRecently()) {
            printf("\rEnd of file.\n");
            break;
        }

        float *buf = context.buffer;
        for (int n = 0; n < numTurns; n++) {
            if (!player->processStereo(buf, false, numFrames)) memset(buf, 0, numFrames * 8);
            buf += numFrames * 2;
        }

        float *ptr = context.buffer;
        int cptr = context.periodSize;

        while (cptr > 0) {
            int error = snd_pcm_writei(context.handle, ptr, cptr);
            if (error < 0) {
                if (!context.underrunRecovery(error)) {
                    printf("underrun recovery write error: %s\n", snd_strerror(error));
                    run = false;
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

            if (!context.waitForPoll(&init)) {
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

    context.destroy();
    if (player) delete(player);
    return 0;
}
