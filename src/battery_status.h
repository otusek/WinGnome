#pragma once

namespace wingnome {

struct BatteryStatus {
    bool present{false};
    bool charging{false};
    int percent{-1};
};

bool queryBatteryStatus(BatteryStatus& out);

}  // namespace wingnome
