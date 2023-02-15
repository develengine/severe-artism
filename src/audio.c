#include "audio.h"

#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


float audio_volume = 1.0f;


#define NO_NEXT -1

static int available_head;

static volatile sound_t sounds[MAX_SOUND_COUNT];

static int available_tail;


bool play_sound(sound_t sound)
{
    int next = sounds[available_head].next;
    if (next == NO_NEXT)
        return false;

    sounds[available_head] = sound;

    available_head = next;

    return true;
}


void audio_callback(int16_t *buffer, unsigned size)
{
    size *= 2;

    for (size_t i = 0; i < size; ++i)
        buffer[i] = 0;

    int stopped_count = 0;

    for (int sound_id = 0; sound_id < MAX_SOUND_COUNT; ++sound_id) {
        sound_t sound = sounds[sound_id];

        if (sound.stopped) {
            ++stopped_count;
            continue;
        }

        size_t written = 0;

        while (written < size) {
            size_t left    = sound.end - sound.pos;
            size_t to_write = left < size ? left : size;

            for (size_t i = 0; i < to_write; i += 2) {
                buffer[i + 0] += (int16_t)(sound.data[sound.pos + i + 0] * sound.vol_l * audio_volume);
                buffer[i + 1] += (int16_t)(sound.data[sound.pos + i + 1] * sound.vol_r * audio_volume);
            }

            sound.pos += to_write;
            written   += to_write;

            if (left < size) {
                sound.pos = sound.start;

                if (sound.times == TIMES_INF)
                    continue;

                --sound.times;

                if (sound.times == 0)
                    written = size;
            }
        }

        if (sound.times == 0) {
            if (sound.handle)
                *(sound.handle) = NULL;

            sounds[sound_id].stopped = true;
            sounds[sound_id].next = NO_NEXT;
            sounds[available_tail].next = sound_id;
            available_tail = sound_id;
        } else {
            sounds[sound_id].pos   = sound.pos;
            sounds[sound_id].times = sound.times;
        }
    }
}


void empty_out_sounds(void)
{
    available_head = 0;
    available_tail = MAX_SOUND_COUNT - 1;

    for (int i = 0; i < MAX_SOUND_COUNT; ++i) {
        sounds[i].next = i + 1;
        sounds[i].stopped = true;
    }

    sounds[MAX_SOUND_COUNT - 1].next = NO_NEXT;
}


void init_audio(void)
{
    audio_info_t info = {
        .write_callback = audio_callback,
    };

    empty_out_sounds();

    init_audio_engine(info);
}


void exit_audio(void)
{
    exit_audio_engine();
}


int16_t *load_wav(const char *path, uint64_t *length)
{
    char buffer[4];
    uint16_t type_of_format, channel_count, frame_size, bits_per_sample;
    uint32_t sample_rate, data_size;
    FILE *file;

    if (!(file = fopen(path, "rb"))) {
        fprintf(stderr, "Can't find file \"%s\"\n", path);
        exit(666);
    }

    // FIXME: add a file length check

    safe_read(buffer, 1, 4, file);
    if (strncmp(buffer, "RIFF", 4)) {
        fprintf(stderr, "Can't parse file \"%s\". Wrong format.\n", path);
        fprintf(stderr, "here\n");
        exit(666);
    }

    fseek(file, 4, SEEK_CUR);
    safe_read(buffer, 1, 4, file);
    if (strncmp(buffer, "WAVE", 4)) {
        fprintf(stderr, "Can't parse file \"%s\". Wrong format.\n", path);
        fprintf(stderr, "there\n");
        exit(666);
    }

    fseek(file, 4, SEEK_CUR);

    int32_t lof;
    safe_read(&lof, 1, 4, file);

    safe_read(&type_of_format, 1, 2, file);
    safe_read(&channel_count, 1, 2, file);
    safe_read(&sample_rate, 1, 4, file);
    fseek(file, 4, SEEK_CUR);
    safe_read(&frame_size, 1, 2, file);
    safe_read(&bits_per_sample, 1, 2, file);

    // NOTE: not sure why it can have some extra garbage in it
    // FIXME: just horrible, but add a file size check
    char state = ' ';
    while (state != 'f') {
        char c;
        safe_read(&c, 1, 1, file);
        switch (c) {
            case 'd':
                state = 'd';
                continue;
            case 'a':
                if (state == 't') {
                    state = 'f';
                    continue;
                }
                if (state == 'd') {
                    state = 'a';
                    continue;
                }
                break;
            case 't':
                if (state == 'a') {
                    state = 't';
                    continue;
                }
                break;
            default:
                break;
        }

        state = ' ';
    }

    safe_read(&data_size, 1, 4, file);
    
#if 0
    printf("WAV file: %s\n"
           "    PCM: %d\n"
           "    Channels: %d\n"
           "    Sample Rate: %d\n"
           "    Frame Size: %d\n"
           "    Bits per Sample: %d\n"
           "    Data Size: %d\n",
           path,
           type_of_format,
           channel_count,
           sample_rate,
           frame_size,
           bits_per_sample,
           data_size);
#endif

    int16_t *data = malloc(data_size);
    if (!data) {
        fprintf(stderr, "failed to allocate memory for audio data \"%s\"\n", path);
        exit(666);
    }

    safe_read(data, 1, data_size, file);
    *length = data_size / (bits_per_sample / 8);

    fclose(file);

    return data;
}
