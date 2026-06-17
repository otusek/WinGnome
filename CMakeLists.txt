cmake_minimum_required(VERSION 3.16)
project(WinGnome LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static MSVC runtime — no VC++ Redistributable required on target machines.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(wingnome WIN32
    src/main.cpp
    src/paths.cpp
    src/json.cpp
    src/bar_config.cpp
    src/bar_anim.cpp
    src/bar.cpp
    src/bar_popup.cpp
    src/battery_status.cpp
    src/network_status.cpp
    src/audio_volume.cpp
    src/shell_actions.cpp
    src/svg_icons.cpp
    src/dock_config.cpp
    src/dock.cpp
    src/dock_icons.cpp
    src/app_icons.cpp
    src/window_tracker.cpp
    src/gfx/gfx_font.cpp
    src/gfx/d2d_window.cpp
    src/wallpaper.cpp
    src/desktop.cpp
    src/shell.cpp
)

target_include_directories(wingnome PRIVATE src third_party/nanosvg)

target_compile_definitions(wingnome PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)

target_link_libraries(wingnome PRIVATE
    d2d1
    dwrite
    user32
    gdi32
    gdiplus
    dwmapi
    ole32
    shell32
    iphlpapi
)

add_custom_command(TARGET wingnome POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:wingnome>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config/bar.json"
        "$<TARGET_FILE_DIR:wingnome>/config/bar.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config/shell.json"
        "$<TARGET_FILE_DIR:wingnome>/config/shell.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config/dock.json"
        "$<TARGET_FILE_DIR:wingnome>/config/dock.json"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:wingnome>/assets"
    COMMENT "Copying config and assets next to wingnome.exe"
)
