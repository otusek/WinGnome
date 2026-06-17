#include "dock_icons.h"

#include "paths.h"

#include <filesystem>
#include <algorithm>

namespace wingnome {

namespace {

std::wstring exeBaseName(const std::wstring& path) {
    const std::wstring resolved = resolveAppPath(path);
    if (resolved.empty()) return {};
    std::wstring base = resolved;
    const auto pos = base.find_last_of(L"\\/");
    if (pos != std::wstring::npos) base = base.substr(pos + 1);
    const auto dot = base.find_last_of(L'.');
    if (dot != std::wstring::npos) base = base.substr(0, dot);
    std::transform(base.begin(), base.end(), base.begin(), ::towlower);
    return base;
}

std::wstring mapAdwaitaIcon(const std::wstring& exe) {
    static const std::pair<const wchar_t*, const wchar_t*> kMap[] = {
        {L"explorer", L"org.gnome.Nautilus"},
        {L"chrome", L"google-chrome"},
        {L"msedge", L"microsoft-edge"},
        {L"firefox", L"firefox"},
        {L"code", L"code"},
        {L"cursor", L"code"},
        {L"devenv", L"visual-studio"},
        {L"notepad", L"text-editor"},
        {L"notepad++", L"text-editor"},
        {L"calc", L"accessories-calculator"},
        {L"mspaint", L"image-x-generic"},
        {L"steam", L"steam"},
        {L"discord", L"discord"},
        {L"spotify", L"spotify"},
        {L"winword", L"x-office-document"},
        {L"excel", L"x-office-spreadsheet"},
        {L"powerpnt", L"x-office-presentation"},
        {L"outlook", L"evolution"},
        {L"olk", L"evolution"},
        {L"teams", L"teams"},
        {L"telegram", L"telegram"},
        {L"vlc", L"vlc"},
        {L"windowsterminal", L"utilities-terminal"},
        {L"wt", L"utilities-terminal"},
        {L"powershell", L"utilities-terminal"},
        {L"pwsh", L"utilities-terminal"},
        {L"cmd", L"utilities-terminal"},
        {L"opera", L"opera"},
        {L"brave", L"brave-browser"},
        {L"slack", L"slack"},
        {L"zoom", L"zoom"},
        {L"thunderbird", L"thunderbird"},
        {L"gimp", L"gimp"},
        {L"inkscape", L"inkscape"},
        {L"blender", L"blender"},
        {L"obs64", L"obs"},
        {L"obs32", L"obs"},
        {L"7zfm", L"archive-manager"},
        {L"winrar", L"archive-manager"},
        {L"totalcmd", L"system-file-manager"},
        {L"filezilla", L"filezilla"},
        {L"putty", L"utilities-terminal"},
        {L"gitkraken", L"gitkraken"},
        {L"githubdesktop", L"github-desktop"},
    };

    for (const auto& entry : kMap) {
        if (exe == entry.first) return entry.second;
    }
    return {};
}

bool assetExists(const std::wstring& relative) {
    const auto path = findAssetFile(relative.c_str());
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

}  // namespace

std::wstring adwaitaIconAssetForApp(const std::wstring& appPath) {
    const std::wstring exe = exeBaseName(appPath);
    if (exe.empty()) return {};

    auto tryIcon = [](const std::wstring& iconName) -> std::wstring {
        const std::wstring asset = L"icons/adwaita/apps/" + iconName + L".svg";
        if (assetExists(asset)) return asset;
        return {};
    };

    const std::wstring mapped = mapAdwaitaIcon(exe);
    if (!mapped.empty()) {
        const std::wstring asset = tryIcon(mapped);
        if (!asset.empty()) return asset;
    }
    return tryIcon(exe);
}

void DockIconCache::clear() {
    systemIcons_.clear();
    themeIcons_.clear();
}

ID2D1Bitmap* DockIconCache::get(ID2D1RenderTarget* rt, const std::wstring& appPath, int size) {
    if (!rt || appPath.empty()) return nullptr;

    if (themeEnabled_) {
        const std::wstring asset = adwaitaIconAssetForApp(appPath);
        if (!asset.empty()) {
            if (ID2D1Bitmap* bmp = themeIcons_.get(rt, asset, size, GfxColor{}, false)) return bmp;
        }
    }

    return systemIcons_.get(rt, appPath, size);
}

}  // namespace wingnome
