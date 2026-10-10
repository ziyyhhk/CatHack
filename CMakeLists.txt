cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # iOS device builds do not use the macOS universal arm64/x86_64 target.
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(CatHackMenu VERSION 0.1.0)

# Keep top-level hacks and the organized display-hack implementations in the
# build, while avoiding an unbounded recursive source search.
file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp src/DisplayHack/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Geode-compatible ImGui backend (pinned for reproducible CI builds).
# Update only after checking the backend against the target Geode/GD bindings.
CPMAddPackage("gh:matcool/gd-imgui-cocos#b93f08ccef778a53ebba09b20c347f6a63980119")

# GD 2.2081 routes iOS keyboard input through Geode events; its bindings do not
# provide the CCKeyboardDispatcher modify header. The backend only uses that
# hook on older GD builds and macOS, so keep its include out of iOS builds.
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(_imgui_cocos_hooks "${gd-imgui-cocos_SOURCE_DIR}/src/hooks.cpp")
    file(READ "${_imgui_cocos_hooks}" _imgui_cocos_hooks_source)
    string(REPLACE
        "#include <Geode/modify/CCKeyboardDispatcher.hpp>"
        "#ifndef GEODE_IS_IOS\n#include <Geode/modify/CCKeyboardDispatcher.hpp>\n#endif"
        _imgui_cocos_hooks_source
        "${_imgui_cocos_hooks_source}"
    )
    file(WRITE "${_imgui_cocos_hooks}" "${_imgui_cocos_hooks_source}")
endif()

target_link_libraries(${PROJECT_NAME} imgui-cocos)

setup_geode_mod(${PROJECT_NAME})
