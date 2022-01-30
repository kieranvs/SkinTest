cmake_minimum_required(VERSION 3.12)
project(SkinTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

# ======== GLFW ========
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)

# ======== GLM ========
add_library(
	glm INTERFACE
)
target_include_directories(glm INTERFACE dependencies/glm)

# ======== Imgui ========
add_library(
	imgui
	dependencies/imgui/imgui.cpp
	dependencies/imgui/imgui_demo.cpp
	dependencies/imgui/imgui_draw.cpp
	dependencies/imgui/imgui_tables.cpp
	dependencies/imgui/imgui_widgets.cpp
	dependencies/imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PRIVATE dependencies/imgui)
target_include_directories(imgui INTERFACE dependencies/imgui dependencies)

# ======== Assimp ========
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/assimp)

# ======== SkinTest ========
add_executable(
	skin_test
	Source.cpp
)
target_include_directories(skin_test PRIVATE . dependencies ${Vulkan_INCLUDE_DIRS})
target_link_libraries(skin_test PRIVATE Vulkan::Vulkan glfw glm)
