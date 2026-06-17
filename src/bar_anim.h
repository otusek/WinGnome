#pragma once

#include <algorithm>
#include <cmath>

namespace wingnome {

inline float clamp01(float v) { return std::clamp(v, 0.f, 1.f); }

inline float easeOutCubic(float t) {
    t = clamp01(t);
    const float u = 1.f - t;
    return 1.f - u * u * u;
}

// Smooth 0..1 animation with GNOME-style ease-out curve.
class AnimChannel {
public:
    void snap(float value) {
        current_ = target_ = clamp01(value);
        start_ = current_;
        progress_ = 1.f;
    }

    void setTarget(float target) {
        if (std::abs(target_ - target) < 0.001f) return;
        start_ = current_;
        target_ = clamp01(target);
        progress_ = 0.f;
    }

    void update(float deltaSeconds, float durationMs);
    float value() const;
    bool settled() const { return progress_ >= 1.f - 0.001f; }

private:
    float current_{0.f};
    float target_{0.f};
    float start_{0.f};
    float progress_{1.f};
};

}  // namespace wingnome
