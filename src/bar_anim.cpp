#include "bar_anim.h"

namespace wingnome {

void AnimChannel::update(float deltaSeconds, float durationMs) {
    if (durationMs <= 0.f) {
        current_ = target_;
        return;
    }
    const float step = deltaSeconds / (durationMs * 0.001f);
    current_ = approach(current_, target_, step);
}

void PulseAnim::update(float deltaSeconds, float speedHz) {
    if (!enabled_) {
        value_ = 0.f;
        return;
    }
    phase_ += deltaSeconds * speedHz * 6.2831853f;
    value_ = 0.5f + 0.5f * std::sin(phase_);
}

}  // namespace wingnome
