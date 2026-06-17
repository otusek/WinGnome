#pragma once

namespace wingnome {

struct NetworkStatus {
    bool connected{false};
};

bool queryNetworkStatus(NetworkStatus& out);

}  // namespace wingnome
