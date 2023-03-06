#include "audio.h"

#include "utils.h"

#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <processthreadsapi.h>
// #include <mmreg.h>
// #include <ks.h>
// #include <ksmedia.h>

#include <stdio.h>
#include <math.h>
#include <limits.h>


#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

/* I want to die */
DEFINE_GUID(based_CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(based_IID_IMMDeviceEnumerator,  0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(based_IID_IAudioClient,         0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(based_IID_IAudioRenderClient,   0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

#define kill_on_fail(hr, msg)                                               \
    do {                                                                    \
        if (FAILED(hr)) {                                                   \
            fprintf(stderr, "%s:%d Audio: %s\n", __FILE__, __LINE__, msg);  \
            exit(666);                                                      \
        }                                                                   \
    } while(0)

#define log_on_fail(hr, msg)                                                        \
    do {                                                                            \
        if (FAILED(hr)) {                                                           \
            fprintf(stderr, "%s:%d Log (audio): %s\n", __FILE__, __LINE__, msg);    \
        }                                                                           \
    } while(0)

#define log(msg)                                                                \
    do {                                                                        \
        fprintf(stderr, "%s:%d Log (audio): %s\n", __FILE__, __LINE__, msg);    \
    } while (0)


static IMMDeviceEnumerator *enumerator = NULL;
static IMMDevice *device = NULL;
static IAudioClient *audio_client = NULL;
static IAudioRenderClient *render_client = NULL;

static HANDLE audio_thread;
static DWORD audio_thread_id;

static HANDLE callback_event;

static HANDLE task;
static DWORD task_id;


DWORD WINAPI audio_thread_function(void *param)
{
    audio_write_callback_t write_callback = ((audio_info_t*)param)->write_callback;

    HRESULT hr;

    if (!SetThreadPriority(audio_thread, THREAD_PRIORITY_HIGHEST))
        log("Failed to raise thread priority!\n");

    hr = CoInitializeEx(0, COINIT_SPEED_OVER_MEMORY);
    kill_on_fail(hr, "Failed to initialize COM!");

    hr = CoCreateInstance(
           &based_CLSID_MMDeviceEnumerator,
           NULL,
           CLSCTX_ALL,
           &based_IID_IMMDeviceEnumerator,
           (void**)&enumerator
    );
    kill_on_fail(hr, "Failed to instantiate DeviceEnumerator!");

    hr = enumerator->lpVtbl->GetDefaultAudioEndpoint(
            enumerator,
            eRender,
            eConsole,
            &device
    );
    kill_on_fail(hr, "Failed to retrieve DefaultAudioEndpoint!");

    hr = device->lpVtbl->Activate(
            device,
            &based_IID_IAudioClient,
            CLSCTX_ALL,
            NULL,
            (void**)&audio_client
    );
    kill_on_fail(hr, "Failed to activate device and create audio_client!");

    WAVEFORMATEXTENSIBLE format = {
        .Format = {
            .cbSize = sizeof(WAVEFORMATEXTENSIBLE),
            .wFormatTag = WAVE_FORMAT_EXTENSIBLE,
            .wBitsPerSample = 16,
            .nChannels = 2,
            .nSamplesPerSec = AUDIO_SAMPLES_PER_SECOND,
            .nBlockAlign = (WORD)(2 * 16 / 8),
            .nAvgBytesPerSec = AUDIO_SAMPLES_PER_SECOND * 2 * 16 / 8,
        },
        .Samples.wValidBitsPerSample = 16,
        .dwChannelMask = KSAUDIO_SPEAKER_STEREO,
        .SubFormat = KSDATAFORMAT_SUBTYPE_PCM,
    };

    WAVEFORMATEX *closest;

    REFERENCE_TIME requested_duration = REFTIMES_PER_SEC / 8;
    hr = audio_client->lpVtbl->IsFormatSupported(
            audio_client,
            AUDCLNT_SHAREMODE_SHARED,
            &format.Format,
            &closest
    );
    if (hr != S_OK) {
        if (hr == S_FALSE) {
            fprintf(stderr, "Requested wave format is not supported!\n");
        } else {
            fprintf(stderr, "Call to is format supported failed!\n");
        }
    }

    hr = audio_client->lpVtbl->Initialize(
            audio_client,
            AUDCLNT_SHAREMODE_SHARED,
            // AUDCLNT_STREAMFLAGS_NOPERSIST  // not sure if this
            // |
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK
            // |
            // AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM  // may or may not be desirable
            ,
            requested_duration,
            0,
            &format.Format,
            NULL
    );
    kill_on_fail(hr, "Failed to initialize audio_client!");

    unsigned buffer_frame_count;
    hr = audio_client->lpVtbl->GetBufferSize(audio_client, &buffer_frame_count);
    kill_on_fail(hr, "Failed to get buffer size ?!?!!");

    printf("obfc(ns): %lld, nbfc(smp): %d\n", requested_duration, buffer_frame_count);

    callback_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (callback_event == NULL) {
        fprintf(stderr, "Failed to create callback_event!\n");
        exit(1);
    }

    hr = audio_client->lpVtbl->SetEventHandle(audio_client, callback_event);
    kill_on_fail(hr, "Failed to set callback_event!\n");

    hr = audio_client->lpVtbl->GetService(
            audio_client,
            &based_IID_IAudioRenderClient,
            (void**)&render_client
    );
    kill_on_fail(hr, "Failed to retrieve render_client!");

#if 0
    int16_t *sampleBuffer = malloc(buffer_frame_count * 2 * sizeof(int16_t));
    malloc_check(sampleBuffer);
    bufferData(sampleBuffer, buffer_frame_count);
#endif

    int16_t *dest_buffer;

    hr = render_client->lpVtbl->GetBuffer(render_client, buffer_frame_count, (BYTE**)&dest_buffer);
    log_on_fail(hr, "Failed to get buffer!");

    if (SUCCEEDED(hr)) {
        write_callback(dest_buffer, buffer_frame_count);

        hr = render_client->lpVtbl->ReleaseBuffer(render_client, buffer_frame_count, 0);
        log_on_fail(hr, "Failed to release buffer!");
    }

#if 0
    task = AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &task_id);
    if (task == NULL) {
        fprintf(stderr, "Failed to set mm thread characteristics!\n");
        exit(1);
    }
#endif

    hr = audio_client->lpVtbl->Start(audio_client);
    kill_on_fail(hr, "Failed to audio client!\n");

    for (;;) {
        if (WaitForSingleObject(callback_event, 2000) != WAIT_OBJECT_0)
            log("Possible timeout!\n");

        unsigned paddingFrames;
        hr = audio_client->lpVtbl->GetCurrentPadding(audio_client, &paddingFrames);
        log_on_fail(hr, "Failed to retrieve padding!\n");

        unsigned framesToRender = buffer_frame_count - paddingFrames;

        if (SUCCEEDED(render_client->lpVtbl->GetBuffer(render_client, framesToRender, (BYTE**)&dest_buffer))) {
            write_callback(dest_buffer, framesToRender);

            hr = render_client->lpVtbl->ReleaseBuffer(render_client, framesToRender, 0);
            log_on_fail(hr, "Failed to release buffer!");
        }
    }
}


static audio_info_t audio_info;

void init_audio_engine(audio_info_t info)
{
    audio_info = info;
    audio_thread = CreateThread(NULL, 0, audio_thread_function, &audio_info, 0, &audio_thread_id);
    if (!audio_thread)
        log("Failed to create audio thread!\n");
}


void exit_audio_engine(void)
{
    audio_client->lpVtbl->Stop(audio_client);

    render_client->lpVtbl->Release(render_client);
    audio_client->lpVtbl->Release(audio_client);
    device->lpVtbl->Release(device);
    enumerator->lpVtbl->Release(enumerator);

    CloseHandle(callback_event);
    CloseHandle(audio_thread);

    CoUninitialize();
}
