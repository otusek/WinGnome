#include "battery_status.h"

#include "platform.h"

#include <windows.h>

namespace wingnome {

bool queryBatteryStatus(BatteryStatus& out) {
    SYSTEM_POWER_STATUS status{};
    if (!GetSystemPowerStatus(&status)) return false;

    out.present = status.BatteryFlag != 128;
    out.charging =
        status.ACLineStatus == 1 || status.BatteryFlag == 8 || status.BatteryFlag == 9;
    if (status.BatteryLifePercent <= 100) {
        out.percent = static_cast<int>(status.BatteryLifePercent);
    } else {
        out.percent = -1;
    }
    return true;
}

}  // namespace wingnome
