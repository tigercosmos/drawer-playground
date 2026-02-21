cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "vcpkg toolchain file")
endif()

project(skia_geometry_drawer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET arm64-osx CACHE STRING "vcpkg target triplet")
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(unofficial-skia CONFIG REQUIRED)

add_executable(skia_geometry_drawer src/main.cpp)

if(APPLE)
  target_compile_definitions(skia_geometry_drawer PRIVATE GL_SILENCE_DEPRECATION)
endif()

target_link_libraries(skia_geometry_drawer PRIVATE
  unofficial::skia::skia
  glfw
  "-framework OpenGL"
)
