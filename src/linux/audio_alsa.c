#include "audio.h"

#define alloca(x)  __builtin_alloca(x)

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <xmmintrin.h>


static pthread_t thread;
static int running = 1;


static int xrun_recover(snd_pcm_t *handle, int err)
{
    if (err == -EPIPE) {
        err = snd_pcm_prepare(handle);
        if (err < 0)
            fprintf(stderr, "Can't recover from underrun, prepare failed: %s\n",
                    snd_strerror(err));
        return 0;
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(handle)) == -EAGAIN)
            sleep(1);

        if (err < 0) {
            err = snd_pcm_prepare(handle);
            if (err < 0)
                fprintf(stderr, "Can't recover from suspend, prepare failed: %s\n",
                        snd_strerror(err));
        }
        return 0;
    }
    return err;
}


void *start_alsa(void *param)
{
    audio_info_t *info = (audio_info_t*)param;
    audio_write_callback_t write_callback = info->write_callback;

    // Open device

    const char *device_name = "default";
    snd_pcm_t *device;

    int ret = snd_pcm_open(&device, device_name, SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0)
        fprintf(stderr, "failed to open pcm device! error: %s\n", snd_strerror(ret));


    // Hardware parameters

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);

    ret = snd_pcm_hw_params_any(device, hw_params);
    if (ret < 0)
        fprintf(stderr, "failed to fill hw_params! error: %s\n", snd_strerror(ret));

    ret = snd_pcm_hw_params_test_access(device, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) // NOTE(engine): Maybe handle later
        fprintf(stderr, "requested acces not available! error: %s\n", snd_strerror(ret));
    ret = snd_pcm_hw_params_set_access(device, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0)
        fprintf(stderr, "failed to set access! error: %s\n", snd_strerror(ret));
    // TODO: Add support for interupt processing

    ret = snd_pcm_hw_params_test_format(device, hw_params, SND_PCM_FORMAT_S16_LE);
    if (ret < 0) // NOTE(engine): Maybe handle later
        fprintf(stderr, "requested format not available! error: %s\n", snd_strerror(ret));
    ret = snd_pcm_hw_params_set_format(device, hw_params, SND_PCM_FORMAT_S16_LE);
    if (ret < 0)
        fprintf(stderr, "failed to set format! error: %s\n", snd_strerror(ret));

    const unsigned int CHANNEL_COUNT = 2;
    unsigned int ch_min, ch_max;
    snd_pcm_hw_params_get_channels_min(hw_params, &ch_min);
    snd_pcm_hw_params_get_channels_max(hw_params, &ch_max);
    printf("audio: min channel n: %d, max channel n: %d\n", ch_min, ch_max);

    ret = snd_pcm_hw_params_test_channels(device, hw_params, CHANNEL_COUNT);
    if (ret < 0) // NOTE(engine): Maybe handle later
        fprintf(stderr, "requested channel count not available! error: %s\n", snd_strerror(ret));
    ret = snd_pcm_hw_params_set_channels(device, hw_params, CHANNEL_COUNT);
    if (ret < 0)
        fprintf(stderr, "failed to set channel count! error: %s\n", snd_strerror(ret));

    const unsigned int RESAMPLE = 1;
    ret = snd_pcm_hw_params_set_rate_resample(device, hw_params, RESAMPLE);
    if (ret < 0)
        fprintf(stderr, "failed to set resample! error: %s\n", snd_strerror(ret));

    unsigned int rate = AUDIO_SAMPLES_PER_SECOND;
    int dir = 0;
    ret = snd_pcm_hw_params_set_rate_near(device, hw_params, &rate, &dir);
    if (ret < 0)
        fprintf(stderr, "failed to set rate near! error: %s\n", snd_strerror(ret));
    if (dir != 0) // NOTE(engine): Maybe handle later
        printf("wrong rate direction!");
    if (rate != AUDIO_SAMPLES_PER_SECOND) // NOTE(engine): Maybe handle later
        printf("wrong sample rate!");
    printf("returned sample rate: %d\n", rate);

    const unsigned int BUFFER_SIZE = AUDIO_SAMPLES_PER_SECOND / 10;
    snd_pcm_uframes_t buffer_size = BUFFER_SIZE;
    ret = snd_pcm_hw_params_set_buffer_size_near(device, hw_params, &buffer_size);
    if (ret < 0)
        fprintf(stderr, "failed to set buffer size near! error: %s\n", snd_strerror(ret));
    if (buffer_size != BUFFER_SIZE) // NOTE(engine): Maybe handle later
        printf("wring buffer size");
    printf("returned buffer size: %ld\n", buffer_size);

    const unsigned int PERIOD_SIZE = AUDIO_SAMPLES_PER_SECOND / 40;
    snd_pcm_uframes_t period_size = PERIOD_SIZE;
    dir = 0;
    ret = snd_pcm_hw_params_set_period_size_near(device, hw_params, &period_size, &dir);
    if (ret < 0)
        fprintf(stderr, "failed to set period size near! error: %s\n", snd_strerror(ret));
    if (dir != 0) // NOTE(engine): Maybe handle later
        printf("wrong period direction!");
    if (period_size != PERIOD_SIZE) // NOTE(engine): Maybe handle later
        printf("wrong period size!");
    printf("returned period size: %ld\n", period_size);

    ret = snd_pcm_hw_params(device, hw_params);
    if (ret < 0) // NOTE(engine): Maybe handle later
        fprintf(stderr, "failed to set hardware parameters! error: %s\n", snd_strerror(ret));


    // Software parameters

    snd_pcm_sw_params_t *sw_params;
    snd_pcm_sw_params_alloca(&sw_params);

    ret = snd_pcm_sw_params_current(device, sw_params);
    if (ret < 0)
        fprintf(stderr, "failed to set current software params! error: %s\n", snd_strerror(ret));

    ret = snd_pcm_sw_params_set_start_threshold(device, sw_params, (buffer_size / period_size) * period_size);
    if (ret < 0)
        fprintf(stderr, "failed to set start threshold! error: %s\n", snd_strerror(ret));

    ret = snd_pcm_sw_params_set_avail_min(device, sw_params, period_size);
    if (ret < 0)
        fprintf(stderr, "failed to set avail min! error: %s\n", snd_strerror(ret));
    // TODO(engine): Add support for interupt processing

    ret = snd_pcm_sw_params(device, sw_params);
    if (ret < 0)
        fprintf(stderr, "failed to set software parameters! error: %s\n", snd_strerror(ret));


    // Write loop

    int16_t *write_buffer = (int16_t*)calloc(period_size * CHANNEL_COUNT, sizeof(int16_t));
    if (!write_buffer) {
        fprintf(stderr, "failed to allocate memory for write buffer!");
        exit(EXIT_FAILURE);
    }

    while (running) {
        write_callback(write_buffer, period_size);

        int16_t *frame_pointer = write_buffer;
        uint32_t frames_left = period_size;

        while (frames_left > 0) {
            ret = snd_pcm_writei(device, frame_pointer, frames_left);

            if (ret == -EAGAIN)
                continue;

            if (ret < 0) {
                if (xrun_recover(device, ret) < 0) {
                    fprintf(stderr, "failed write! error: %s\n", snd_strerror(ret));
                    exit(EXIT_FAILURE);
                }
                break;
            }

            frame_pointer += ret * CHANNEL_COUNT;
            frames_left -= ret;
        }
    }

    snd_pcm_close(device);

#ifdef _DEBUG
    /* necessary for debugging memory leaks */
    snd_config_update_free_global();
#endif

    free(write_buffer);

    pthread_exit(0);
}

/* global so the thread doesn't outlive it */
static audio_info_t audio_info;

void init_audio_engine(audio_info_t info)
{
    audio_info = info;

    _mm_mfence();

    int ret = pthread_create(&thread, NULL, start_alsa, &audio_info);

    if (ret) {
        fprintf(stderr, "failed to create a separate audio thread!\n");
        exit(222);
    }
}


void exit_audio_engine(void)
{
    running = 0;
    pthread_join(thread, NULL);
}

