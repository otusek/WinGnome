#include "paths.h"

#include <windows.h>

#include <fstream>
#include <system_error>

namespace wingnome {

namespace {

constexpr uint64_t kFnvOffset = 14695981039346656037ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

}  // namespace

std::filesystem::path exeDir() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

std::filesystem::path findConfigFile(const wchar_t* filename) {
    const auto root = exeDir();
    const std::filesystem::path candidates[] = {
        root / L"config" / filename,
        root / filename,
        std::filesystem::current_path() / L"config" / filename,
        std::filesystem::current_path() / filename,
        root / L".." / L"config" / filename,
        root / L".." / L".." / L"config" / filename,
    };

    std::filesystem::path best;
    std::filesystem::file_time_type bestTime{};

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (!std::filesystem::exists(candidate, ec)) continue;

        const auto writeTime = std::filesystem::last_write_time(candidate, ec);
        if (ec) continue;

        if (best.empty() || writeTime > bestTime) {
            best = std::filesystem::weakly_canonical(candidate, ec);
            if (ec) best = candidate;
            bestTime = writeTime;
        }
    }

    if (!best.empty()) return best;
    return root / L"config" / filename;
}

std::filesystem::path findAssetFile(const wchar_t* relativePath) {
    const auto root = exeDir();
    const std::filesystem::path candidates[] = {
        root / L"assets" / relativePath,
        root / relativePath,
        std::filesystem::current_path() / L"assets" / relativePath,
        std::filesystem::current_path() / relativePath,
        root / L".." / L"assets" / relativePath,
        root / L".." / L".." / L"assets" / relativePath,
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return std::filesystem::weakly_canonical(candidate, ec);
        }
    }
    return root / L"assets" / relativePath;
}

uint64_t configFingerprint(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return 0;

    std::ifstream in(path, std::ios::binary);
    if (!in) return 0;

    uint64_t hash = kFnvOffset;
    char buffer[4096]{};
    while (in) {
        in.read(buffer, sizeof(buffer));
        const std::streamsize count = in.gcount();
        if (count <= 0) break;
        for (size_t i = 0; i < static_cast<size_t>(count); ++i) {
            hash ^= static_cast<unsigned char>(buffer[i]);
            hash *= kFnvPrime;
        }
    }
    return hash;
}

bool configNeedsReload(ConfigChangeDetect& state, const wchar_t* filename, uint64_t debounceMs) {
    const std::filesystem::path latest = findConfigFile(filename);
    const uint64_t fingerprint = configFingerprint(latest);
    if (fingerprint == 0) return false;
    if (fingerprint == state.fingerprint && latest == state.path) return false;

    const uint64_t now = GetTickCount64();
    if (fingerprint != state.pendingFingerprint) {
        state.pendingFingerprint = fingerprint;
        state.pendingTick = now;
        return false;
    }

    if (now - state.pendingTick < debounceMs) return false;
    return true;
}

void configMarkReloaded(ConfigChangeDetect& state, const std::filesystem::path& path) {
    state.path = path;
    state.fingerprint = configFingerprint(path);
    state.pendingFingerprint = 0;
    state.pendingTick = 0;
}

}  // namespace wingnome
