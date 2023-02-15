cmake_minimum_required(VERSION 3.24)
project(game)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

add_executable(game
        src/main.cpp

        src/window.cpp
        src/renderer.cpp
        src/mesh/gpu_mesh.cpp
        src/mesh/mesh.cpp
        src/logger.cpp

        src/platform/vulkan/vulkan_renderer.cpp
        src/platform/opengl/opengl_renderer.cpp)
target_include_directories(game PRIVATE ${SDL2_INCLUDE_DIRECTORIES} pch src)
target_link_libraries(game PRIVATE ${SDL2_LIBRARIES})
target_precompile_headers(game PRIVATE pch/vulkan.hpp)