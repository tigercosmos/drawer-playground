cmake_minimum_required(VERSION 3.16)
project(imgui_geometry_drawer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(GLFW REQUIRED glfw3)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

add_executable(imgui_geometry_drawer
  src/main.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_geometry_drawer PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${GLEW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
)

target_compile_definitions(imgui_geometry_drawer PRIVATE
  IMGUI_IMPL_OPENGL_LOADER_GLEW
)

if(APPLE)
  target_compile_definitions(imgui_geometry_drawer PRIVATE GL_SILENCE_DEPRECATION)
endif()

target_compile_options(imgui_geometry_drawer PRIVATE
  ${GLEW_CFLAGS_OTHER}
  ${GLFW_CFLAGS_OTHER}
)

target_link_directories(imgui_geometry_drawer PRIVATE
  ${GLEW_LIBRARY_DIRS}
  ${GLFW_LIBRARY_DIRS}
)

target_link_libraries(imgui_geometry_drawer PRIVATE
  ${GLEW_LIBRARIES}
  ${GLFW_LIBRARIES}
  "-framework OpenGL"
)
