cmake_minimum_required(VERSION 3.24)
project(QOLOverlay LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Static CRT so the exe runs without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(imgui PUBLIC
  IMGUI_DISABLE_DEMO_WINDOWS
  IMGUI_DISABLE_DEBUG_TOOLS
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_DEFINE_MATH_OPERATORS
  UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

add_executable(QOLOverlay WIN32
  src/main.cpp
  src/app.cpp
  src/overlay/window.cpp
  src/overlay/renderer.cpp
  src/input/hotkeys.cpp
  src/providers/etw_session.cpp
  src/providers/fps.cpp
  src/providers/ping.cpp
  src/providers/game_select.cpp
  src/providers/clock.cpp
  src/providers/media.cpp
  src/providers/system_stats.cpp
  src/update/updater.cpp
  src/ui/theme.cpp
  src/ui/fonts.cpp
  src/ui/anim.cpp
  src/ui/widgets.cpp
  src/ui/watermark.cpp
  src/ui/menu.cpp
  src/ui/player.cpp
  src/config/config.cpp
  res/app.rc)
target_include_directories(QOLOverlay PRIVATE src res assets/fonts)
set(QOL_VERSION "0.3.0" CACHE STRING "Version shown in the installer and app")
target_compile_definitions(QOLOverlay PRIVATE QOL_VERSION="${QOL_VERSION}")
target_link_libraries(QOLOverlay PRIVATE imgui
  d3d11 d3dcompiler dxgi dcomp dwmapi tdh pdh winhttp bcrypt iphlpapi ws2_32 advapi32 shell32 user32 gdi32 imm32 ole32)

if(MSVC)
  target_compile_options(QOLOverlay PRIVATE /W4 /WX /permissive- /utf-8)
  target_compile_options(imgui PRIVATE /utf-8)
  # The manifest comes from res/app.rc (admin + per-monitor DPI), so don't generate a second one.
  target_link_options(QOLOverlay PRIVATE /MANIFEST:NO)
  # C++/WinRT (Spotify/media controls) and WIC (cover art decoding)
  target_link_libraries(QOLOverlay PRIVATE windowsapp windowscodecs shcore)
  set_property(TARGET QOLOverlay imgui PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
  target_compile_options(QOLOverlay PRIVATE -Wall -Wextra -Werror -Wno-missing-field-initializers)
  target_link_options(QOLOverlay PRIVATE -static -municode)
endif()
