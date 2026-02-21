cmake_minimum_required(VERSION 3.16)
project(geometry_drawer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(GLFW REQUIRED glfw3)

add_executable(geometry_drawer
  src/main.cpp
  third_party/nanovg/src/nanovg.c
)

target_include_directories(geometry_drawer PRIVATE
  third_party/nanovg/src
  ${GLEW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
)

target_link_directories(geometry_drawer PRIVATE
  ${GLEW_LIBRARY_DIRS}
  ${GLFW_LIBRARY_DIRS}
)

target_compile_options(geometry_drawer PRIVATE
  ${GLEW_CFLAGS_OTHER}
  ${GLFW_CFLAGS_OTHER}
)

if(APPLE)
  target_compile_definitions(geometry_drawer PRIVATE GL_SILENCE_DEPRECATION)
endif()

target_link_libraries(geometry_drawer PRIVATE
  ${GLEW_LIBRARIES}
  ${GLFW_LIBRARIES}
  "-framework OpenGL"
)
