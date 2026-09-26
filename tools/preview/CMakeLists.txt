# Linux-only UI preview (software rendered). Not part of the Windows build.
#   cmake -S tools/preview -B build-preview && cmake --build build-preview && ./build-preview/preview out/
cmake_minimum_required(VERSION 3.24)
project(OverlayPreview CXX)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.9 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
add_executable(preview
  preview.cpp
  ${ROOT}/src/ui/theme.cpp
  ${ROOT}/src/ui/anim.cpp
  ${ROOT}/src/ui/widgets.cpp
  ${ROOT}/src/ui/watermark.cpp
  ${ROOT}/src/ui/menu.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(preview PRIVATE stubs ${ROOT}/src ${imgui_SOURCE_DIR})
target_compile_definitions(preview PRIVATE IMGUI_DEFINE_MATH_OPERATORS IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  FONT_DIR="${ROOT}/assets/fonts")
