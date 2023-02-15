#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#define AUDIO_SAMPLES_PER_SECOND 48000

typedef void (*audio_write_callback_t)(int16_t *buffer, unsigned size);

typedef struct
{
    audio_write_callback_t write_callback;
} audio_info_t;

#define MAX_SOUND_COUNT 32
#define TIMES_INF       0xffffffff

typedef struct
{
    int16_t *data;
    unsigned times;
    size_t start, end, pos;
    float vol_l, vol_r;

    /* null initialized */
    union {
        int next;
        void *volatile *handle;
    };
    unsigned stopped;   /* true/false */
} sound_t;


void init_audio_engine(audio_info_t info);
void exit_audio_engine(void);

void init_audio(void);
void exit_audio(void);

bool play_sound(sound_t info);

int16_t *load_wav(const char *path, uint64_t *length);

extern float audio_volume;

#endif
