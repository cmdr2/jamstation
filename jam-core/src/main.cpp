#define _CRT_SECURE_NO_WARNINGS
#include <audioclient.h>
#include <avrt.h>
#include <mmdeviceapi.h>
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "graph.h"

#define LOG(fmt, ...) printf("[LOG] " fmt "\n", __VA_ARGS__)
#define ERR(fmt, ...) printf("[ERROR] " fmt "\n", __VA_ARGS__)
#define HR_FAIL(hr, msg)                   \
    do {                                   \
        ERR("%s failed: 0x%08X", msg, hr); \
        return -1;                         \
    } while (0)

static const REFERENCE_TIME PERIOD = 10 * 10000;  // 10 ms

struct AudioFormat {
    WAVEFORMATEXTENSIBLE fmt;
    const char* name;
    bool is24in32;
};

int main(int argc, char** argv) {
    printf("jam-core v0.1.0\n");

    HRESULT hr;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* inDev = nullptr;
    IMMDevice* outDev = nullptr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void**)&enumerator);
    if (FAILED(hr)) HR_FAIL(hr, "MMDeviceEnumerator");

    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &inDev);
    if (FAILED(hr)) HR_FAIL(hr, "Get capture device");

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &outDev);
    if (FAILED(hr)) HR_FAIL(hr, "Get render device");

    IAudioClient* inClient = nullptr;
    IAudioClient* outClient = nullptr;

    hr = inDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&inClient);
    if (FAILED(hr)) HR_FAIL(hr, "Activate capture client");

    hr = outDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&outClient);
    if (FAILED(hr)) HR_FAIL(hr, "Activate render client");

    LOG("Audio clients activated");

    std::vector<AudioFormat> formats;

    {
        WAVEFORMATEXTENSIBLE f = {};
        f.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        f.Format.nChannels = 2;
        f.Format.nSamplesPerSec = 48000;
        f.Format.wBitsPerSample = 32;
        f.Format.nBlockAlign = 2 * 4;
        f.Format.nAvgBytesPerSec = 48000 * f.Format.nBlockAlign;
        f.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        f.Samples.wValidBitsPerSample = 24;
        f.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
        f.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        formats.push_back({f, "PCM 24-in-32", true});
    }

    {
        WAVEFORMATEXTENSIBLE f = {};
        f.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        f.Format.nChannels = 2;
        f.Format.nSamplesPerSec = 48000;
        f.Format.wBitsPerSample = 16;
        f.Format.nBlockAlign = 2 * 2;
        f.Format.nAvgBytesPerSec = 48000 * f.Format.nBlockAlign;
        f.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        f.Samples.wValidBitsPerSample = 16;
        f.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
        f.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        formats.push_back({f, "PCM 16", false});
    }

    AudioFormat* chosen = nullptr;

    for (auto& f : formats) {
        LOG("Probing %s", f.name);
        if (FAILED(inClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&f.fmt, nullptr))) continue;
        if (FAILED(outClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&f.fmt, nullptr))) continue;
        chosen = &f;
        break;
    }

    if (!chosen) {
        ERR("No compatible exclusive format found");
        return -1;
    }

    LOG("Using format: %s", chosen->name);

    hr = inClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, PERIOD, PERIOD,
                              (WAVEFORMATEX*)&chosen->fmt, nullptr);
    if (FAILED(hr)) HR_FAIL(hr, "Initialize capture");

    hr = outClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, PERIOD, PERIOD,
                               (WAVEFORMATEX*)&chosen->fmt, nullptr);
    if (FAILED(hr)) HR_FAIL(hr, "Initialize render");

    HANDLE inEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    HANDLE outEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    inClient->SetEventHandle(inEvent);
    outClient->SetEventHandle(outEvent);

    IAudioCaptureClient* capture = nullptr;
    IAudioRenderClient* render = nullptr;

    inClient->GetService(__uuidof(IAudioCaptureClient), (void**)&capture);
    outClient->GetService(__uuidof(IAudioRenderClient), (void**)&render);

    DWORD taskIndex = 0;
    HANDLE mmcss = AvSetMmThreadCharacteristicsA("Pro Audio", &taskIndex);
    LOG("MMCSS handle: %p", mmcss);

    inClient->Start();
    outClient->Start();

    LOG("Streaming");

    const UINT32 frameBytes = chosen->fmt.Format.nBlockAlign;
    const uint16_t channels = chosen->fmt.Format.nChannels;
    const SampleFormat sampleFormat = chosen->is24in32 ? FORMAT_PCM24IN32 : FORMAT_PCM16;

    while (true) {
        WaitForSingleObject(inEvent, INFINITE);

        BYTE* inData = nullptr;
        UINT32 frames = 0;
        DWORD flags = 0;

        hr = capture->GetBuffer(&inData, &frames, &flags, nullptr, nullptr);
        if (FAILED(hr)) break;

        if (frames > 0) {
            BYTE* outData = nullptr;
            hr = render->GetBuffer(frames, &outData);
            if (FAILED(hr)) {
                capture->ReleaseBuffer(frames);
                break;
            }

            bool bad = flags & (AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY | AUDCLNT_BUFFERFLAGS_SILENT);

            if (bad) {
                memset(outData, 0, frames * frameBytes);
            } else {
                processAudioBlock(inData, outData, frames, sampleFormat);
            }

            render->ReleaseBuffer(frames, 0);
        }

        capture->ReleaseBuffer(frames);
    }

    inClient->Stop();
    outClient->Stop();
    if (mmcss) AvRevertMmThreadCharacteristics(mmcss);

    return 0;
}
