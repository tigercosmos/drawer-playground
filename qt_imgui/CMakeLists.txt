cmake_minimum_required(VERSION 3.21)
project(qt_imgui LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

set(_imgui_candidates
    "${CMAKE_SOURCE_DIR}/third_party/imgui"
    "${CMAKE_SOURCE_DIR}/../imgui"
    "${CMAKE_SOURCE_DIR}/../imgui/third_party/imgui"
)
set(_imgui_detected "")
foreach(_candidate IN LISTS _imgui_candidates)
    if(EXISTS "${_candidate}/imgui.h")
        set(_imgui_detected "${_candidate}")
        break()
    endif()
endforeach()

set(IMGUI_DIR "${_imgui_detected}" CACHE PATH "Path to Dear ImGui")
if((NOT EXISTS "${IMGUI_DIR}/imgui.h") AND (EXISTS "${_imgui_detected}/imgui.h"))
    set(IMGUI_DIR "${_imgui_detected}" CACHE PATH "Path to Dear ImGui" FORCE)
endif()
if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "Dear ImGui not found. Set IMGUI_DIR to your imgui checkout.")
endif()

add_executable(qt_imgui
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/MetalRenderer.h
    src/MetalRenderer.mm
    src/ImGuiLayer.h
    src/ImGuiLayer.mm
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_metal.mm"
)

target_include_directories(qt_imgui PRIVATE
    src
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

target_link_libraries(qt_imgui PRIVATE
    Qt6::Core
    Qt6::Gui
    "-framework Metal"
    "-framework QuartzCore"
    "-framework Cocoa"
)

set_target_properties(qt_imgui PROPERTIES
    OBJCXX_STANDARD 17
    OBJCXX_STANDARD_REQUIRED YES
)

set_source_files_properties(
    src/MetalRenderer.mm
    src/ImGuiLayer.mm
    "${IMGUI_DIR}/backends/imgui_impl_metal.mm"
    PROPERTIES
    COMPILE_FLAGS "-fobjc-arc"
)
