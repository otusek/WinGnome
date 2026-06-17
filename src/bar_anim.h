#pragma once

#include <algorithm>
#include <cmath>

namespace wingnome {

inline float clamp01(float v) { return std::clamp(v, 0.f, 1.f); }

inline float approach(float current, float target, float maxDelta) {
    if (current < target) return std::min(current + maxDelta, target);
    return std::max(current - maxDelta, target);
}

// Smooth 0..1 animation driven by duration in milliseconds.
class AnimChannel {
public:
    void snap(float value) { current_ = target_ = value; }
    void setTarget(float target) { target_ = clamp01(target); }
    void update(float deltaSeconds, float durationMs);
    float value() const { return current_; }
    bool settled() const { return std::abs(current_ - target_) < 0.001f; }

private:
    float current_{0.f};
    float target_{0.f};
};

// Optional pulse when muted (0..1 oscillation).
class PulseAnim {
public:
    void setEnabled(bool enabled) { enabled_ = enabled; }
    void update(float deltaSeconds, float speedHz);
    float value() const { return enabled_ ? value_ : 0.f; }

private:
    bool enabled_{false};
    float phase_{0.f};
    float value_{0.f};
};

}  // namespace wingnome
