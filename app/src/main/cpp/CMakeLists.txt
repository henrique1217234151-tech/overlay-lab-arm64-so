cmake_minimum_required(VERSION 3.22.1)
project(overlay_lab_dlopen LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/imgui")
if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
  message(FATAL_ERROR "Dear ImGui ausente. Execute scripts/setup-deps.ps1 ou scripts/setup-deps.sh")
endif()

add_library(overlay_plugin SHARED
    plugin.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_android.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(overlay_plugin PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_compile_definitions(overlay_plugin PRIVATE IMGUI_IMPL_OPENGL_ES3)
target_link_libraries(overlay_plugin PRIVATE android EGL GLESv3 log)

add_library(overlay_host SHARED
    host.cpp
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)
target_include_directories(overlay_host PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)
target_link_options(overlay_host PRIVATE "-u" "ANativeActivity_onCreate")
target_link_libraries(overlay_host PRIVATE android log dl)
