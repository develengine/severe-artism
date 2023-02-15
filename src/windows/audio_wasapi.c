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
static IAudioClient *audioClient = NULL;
static IAudioRenderClient *renderClient = NULL;

static HANDLE audioThread;
static DWORD audioThreadID;

static HANDLE callbackEvent;

static HANDLE task;
static DWORD taskID;


DWORD WINAPI audioThreadFunction(void *param)
{
    AudioWriteCallback writeCallback = ((AudioInfo*)param)->writeCallback;

    HRESULT hr;

    if (!SetThreadPriority(audioThread, THREAD_PRIORITY_HIGHEST))
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
            (void**)&audioClient
    );
    kill_on_fail(hr, "Failed to activate device and create audioClient!");

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

    REFERENCE_TIME requestedDuration = REFTIMES_PER_SEC / 8;
    hr = audioClient->lpVtbl->IsFormatSupported(
            audioClient,
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

    hr = audioClient->lpVtbl->Initialize(
            audioClient,
            AUDCLNT_SHAREMODE_SHARED,
            // AUDCLNT_STREAMFLAGS_NOPERSIST  // not sure if this
            // |
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK
            // |
            // AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM  // may or may not be desirable
            ,
            requestedDuration,
            0,
            &format.Format,
            NULL
    );
    kill_on_fail(hr, "Failed to initialize audioClient!");

    unsigned bufferFrameCount;
    hr = audioClient->lpVtbl->GetBufferSize(audioClient, &bufferFrameCount);
    kill_on_fail(hr, "Failed to get buffer size ?!?!!");

    printf("obfc(ns): %lld, nbfc(smp): %d\n", requestedDuration, bufferFrameCount);

    callbackEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (callbackEvent == NULL) {
        fprintf(stderr, "Failed to create callbackEvent!\n");
        exit(1);
    }

    hr = audioClient->lpVtbl->SetEventHandle(audioClient, callbackEvent);
    kill_on_fail(hr, "Failed to set callbackEvent!\n");

    hr = audioClient->lpVtbl->GetService(
            audioClient,
            &based_IID_IAudioRenderClient,
            (void**)&renderClient
    );
    kill_on_fail(hr, "Failed to retrieve renderClient!");

#if 0
    int16_t *sampleBuffer = malloc(bufferFrameCount * 2 * sizeof(int16_t));
    malloc_check(sampleBuffer);
    bufferData(sampleBuffer, bufferFrameCount);
#endif

    int16_t *destBuffer;

    hr = renderClient->lpVtbl->GetBuffer(renderClient, bufferFrameCount, (BYTE**)&destBuffer);
    log_on_fail(hr, "Failed to get buffer!");

    if (SUCCEEDED(hr)) {
        writeCallback(destBuffer, bufferFrameCount);

        hr = renderClient->lpVtbl->ReleaseBuffer(renderClient, bufferFrameCount, 0);
        log_on_fail(hr, "Failed to release buffer!");
    }

#if 0
    task = AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &taskID);
    if (task == NULL) {
        fprintf(stderr, "Failed to set mm thread characteristics!\n");
        exit(1);
    }
#endif

    hr = audioClient->lpVtbl->Start(audioClient);
    kill_on_fail(hr, "Failed to audio client!\n");

    for (;;) {
        if (WaitForSingleObject(callbackEvent, 2000) != WAIT_OBJECT_0)
            log("Possible timeout!\n");

        unsigned paddingFrames;
        hr = audioClient->lpVtbl->GetCurrentPadding(audioClient, &paddingFrames);
        log_on_fail(hr, "Failed to retrieve padding!\n");

        unsigned framesToRender = bufferFrameCount - paddingFrames;

        if (SUCCEEDED(renderClient->lpVtbl->GetBuffer(renderClient, framesToRender, (BYTE**)&destBuffer))) {
            writeCallback(destBuffer, framesToRender);

            hr = renderClient->lpVtbl->ReleaseBuffer(renderClient, framesToRender, 0);
            log_on_fail(hr, "Failed to release buffer!");
        }
    }
}


static AudioInfo audioInfo;

void initAudioEngine(AudioInfo info)
{
    audioInfo = info;
    audioThread = CreateThread(NULL, 0, audioThreadFunction, &audioInfo, 0, &audioThreadID);
    if (!audioThread)
        log("Failed to create audio thread!\n");
}


void exitAudioEngine(void)
{
    audioClient->lpVtbl->Stop(audioClient);

    renderClient->lpVtbl->Release(renderClient);
    audioClient->lpVtbl->Release(audioClient);
    device->lpVtbl->Release(device);
    enumerator->lpVtbl->Release(enumerator);

    CloseHandle(callbackEvent);
    CloseHandle(audioThread);

    CoUninitialize();
}