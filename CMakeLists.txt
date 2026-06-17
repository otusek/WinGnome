cmake_minimum_required(VERSION 3.16)
project(WinGnome LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SFML)

add_executable(wingnome WIN32
    src/main.cpp
    src/paths.cpp
    src/json.cpp
    src/bar_config.cpp
    src/dock_config.cpp
    src/app_icons.cpp
    src/window_tracker.cpp
    src/dock.cpp
    src/bar_anim.cpp
    src/bar_icons.cpp
    src/bar_popup.cpp
    src/bar_modules.cpp
    src/TextureManager.cpp
    src/svg_raster.cpp
    src/audio_endpoint.cpp
    src/bar.cpp
    src/wallpaper.cpp
    src/desktop.cpp
    src/shell.cpp
)

target_include_directories(wingnome PRIVATE src third_party/nanosvg)

target_compile_definitions(wingnome PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(wingnome PRIVATE
    sfml-graphics
    user32 gdi32 gdiplus dwmapi ole32 shell32 iphlpapi
    oleaut32 uuid wininet ws2_32
)

if(MINGW)
    target_link_libraries(wingnome PRIVATE mmdevapi)
elseif(MSVC)
    target_link_libraries(wingnome PRIVATE mmdevapi)
endif()

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
    COMMENT "Copying config files next to wingnome.exe"
)

add_custom_command(TARGET wingnome POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:wingnome>/assets"
    COMMENT "Copying assets next to wingnome.exe"
)

if(WIN32)
    add_custom_command(TARGET wingnome POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/_deps/sfml-build/lib/$<CONFIG>/sfml-graphics-2.dll"
            "$<TARGET_FILE_DIR:wingnome>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/_deps/sfml-build/lib/$<CONFIG>/sfml-window-2.dll"
            "$<TARGET_FILE_DIR:wingnome>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/_deps/sfml-build/lib/$<CONFIG>/sfml-system-2.dll"
            "$<TARGET_FILE_DIR:wingnome>"
        COMMENT "Copying SFML DLLs next to wingnome.exe"
    )
endif()
