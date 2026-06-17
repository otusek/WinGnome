#pragma once

namespace wingnome {

class AudioVolume {
public:
    bool init();
    void shutdown();

    float level() const;
    bool muted() const;
    void setLevel(float level01);
    void adjustLevel(float delta01);
    void toggleMute();

private:
    void* enumerator_{nullptr};
    void* endpoint_{nullptr};
    void* volume_{nullptr};
};

}  // namespace wingnome
