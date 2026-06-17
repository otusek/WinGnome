#include "bar_anim.h"

namespace wingnome {

void AnimChannel::update(float deltaSeconds, float durationMs) {
    if (durationMs <= 0.f) {
        current_ = target_;
        progress_ = 1.f;
        return;
    }
    progress_ = std::min(1.f, progress_ + deltaSeconds / (durationMs * 0.001f));
    current_ = start_ + (target_ - start_) * easeOutCubic(progress_);
}

float AnimChannel::value() const { return current_; }

}  // namespace wingnome
