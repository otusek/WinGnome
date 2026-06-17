#include "audio_volume.h"

#include "platform.h"

#include <windows.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include <algorithm>

#pragma comment(lib, "ole32.lib")

namespace wingnome {

bool AudioVolume::init() {
    shutdown();

    IMMDeviceEnumerator* enumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(&enumerator)))) {
        return false;
    }
    enumerator_ = enumerator;

    IMMDevice* endpoint = nullptr;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &endpoint))) {
        shutdown();
        return false;
    }
    endpoint_ = endpoint;

    IAudioEndpointVolume* volume = nullptr;
    if (FAILED(endpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                  reinterpret_cast<void**>(&volume)))) {
        shutdown();
        return false;
    }
    volume_ = volume;
    return true;
}

void AudioVolume::shutdown() {
    if (volume_) {
        static_cast<IAudioEndpointVolume*>(volume_)->Release();
        volume_ = nullptr;
    }
    if (endpoint_) {
        static_cast<IMMDevice*>(endpoint_)->Release();
        endpoint_ = nullptr;
    }
    if (enumerator_) {
        static_cast<IMMDeviceEnumerator*>(enumerator_)->Release();
        enumerator_ = nullptr;
    }
}

float AudioVolume::level() const {
    if (!volume_) return 0.f;
    float level = 0.f;
    if (FAILED(static_cast<IAudioEndpointVolume*>(volume_)->GetMasterVolumeLevelScalar(&level))) {
        return 0.f;
    }
    return level;
}

bool AudioVolume::muted() const {
    if (!volume_) return false;
    BOOL muted = FALSE;
    if (FAILED(static_cast<IAudioEndpointVolume*>(volume_)->GetMute(&muted))) return false;
    return muted == TRUE;
}

void AudioVolume::setLevel(float level01) {
    if (!volume_) return;
    level01 = std::clamp(level01, 0.f, 1.f);
    static_cast<IAudioEndpointVolume*>(volume_)->SetMasterVolumeLevelScalar(level01, nullptr);
}

void AudioVolume::adjustLevel(float delta01) {
    setLevel(level() + delta01);
}

void AudioVolume::toggleMute() {
    if (!volume_) return;
    BOOL muted = FALSE;
    if (FAILED(static_cast<IAudioEndpointVolume*>(volume_)->GetMute(&muted))) return;
    static_cast<IAudioEndpointVolume*>(volume_)->SetMute(!muted, nullptr);
}

}  // namespace wingnome
