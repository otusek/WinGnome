#include "network_status.h"

#include "platform.h"

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>

#include <vector>

#pragma comment(lib, "iphlpapi.lib")

namespace wingnome {

bool queryNetworkStatus(NetworkStatus& out) {
    ULONG size = 0;
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                              GAA_FLAG_SKIP_DNS_SERVER,
                             nullptr, nullptr, &size) != ERROR_BUFFER_OVERFLOW) {
        return false;
    }

    std::vector<uint8_t> buffer(size);
    auto* addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                              GAA_FLAG_SKIP_DNS_SERVER,
                             nullptr, addrs, &size) != NO_ERROR) {
        return false;
    }

    out.connected = false;
    for (auto* adapter = addrs; adapter; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp) continue;
        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (adapter->PhysicalAddressLength == 0) continue;
        out.connected = true;
        break;
    }
    return true;
}

}  // namespace wingnome
