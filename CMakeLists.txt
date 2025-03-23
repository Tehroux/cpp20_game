cmake_minimum_required(VERSION 3.8)
project(MyProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

find_package(OpenGL REQUIRED)

set(IMGUI_SRC
	external/imgui/imgui.cpp
	external/imgui/imgui_demo.cpp
	external/imgui/imgui_draw.cpp
	external/imgui/imgui_tables.cpp
	external/imgui/imgui_widgets.cpp
	external/imgui/backends/imgui_impl_sdl3.cpp
	external/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(external/benchmark EXCLUDE_FROM_ALL)

add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(external/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(external/entt EXCLUDE_FROM_ALL)

add_executable(my_app src/main.cpp ${IMGUI_SRC})
target_sources(my_app PRIVATE FILE_SET CXX_MODULES FILES src/gui.cpp src/sprite.cpp  src/board.cpp)
target_include_directories(my_app PRIVATE external/imgui)
target_link_libraries(my_app PRIVATE SDL3::SDL3 SDL3_image::SDL3_image OpenGL::GL)

add_executable(my_tests tests/test_main.cpp)
target_include_directories(my_tests PRIVATE external/doctest)

add_executable(my_benchmark benchmarks/benchmark_main.cpp)
target_link_libraries(my_benchmark PRIVATE benchmark::benchmark)
