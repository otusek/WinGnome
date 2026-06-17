#include "audio_endpoint.h"

#include "platform.h"

#include <mmdeviceapi.h>
#include <endpointvolume.h>

#include <algorithm>

namespace wingnome {

AudioEndpoint& AudioEndpoint::get() {
    static AudioEndpoint instance;
    return instance;
}

bool AudioEndpoint::read(float& level, bool& muted) {
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioEndpointVolume* volume = nullptr;
    bool ok = false;

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(&enumerator))))
        return false;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
        enumerator->Release();
        return false;
    }
    if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&volume)))) {
        device->Release();
        enumerator->Release();
        return false;
    }

    BOOL mute = FALSE;
    if (SUCCEEDED(volume->GetMute(&mute)) && SUCCEEDED(volume->GetMasterVolumeLevelScalar(&level))) {
        muted = mute != FALSE;
        ok = true;
    }
    volume->Release();
    device->Release();
    enumerator->Release();
    return ok;
}

bool AudioEndpoint::write(float level, bool muted) {
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioEndpointVolume* volume = nullptr;
    bool ok = false;

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(&enumerator))))
        return false;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
        enumerator->Release();
        return false;
    }
    if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&volume)))) {
        device->Release();
        enumerator->Release();
        return false;
    }

    level = std::clamp(level, 0.f, 1.f);
    if (SUCCEEDED(volume->SetMasterVolumeLevelScalar(level, nullptr)) &&
        SUCCEEDED(volume->SetMute(muted ? TRUE : FALSE, nullptr)))
        ok = true;

    volume->Release();
    device->Release();
    enumerator->Release();
    return ok;
}

}  // namespace wingnome
