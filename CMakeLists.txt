cmake_minimum_required(VERSION 3.12)
project(SkinTest VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ======== GLFW ========
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)

set_target_properties(glfw PROPERTIES FOLDER "Libraries/GLFW")
set_target_properties(uninstall PROPERTIES FOLDER "Libraries/GLFW")
set_target_properties(update_mappings PROPERTIES FOLDER "Libraries/GLFW")

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
	dependencies/imgui/backends/imgui_impl_vulkan.cpp
	dependencies/imgui/backends/imgui_impl_glfw.cpp
)
target_include_directories(imgui PRIVATE dependencies/imgui ${Vulkan_INCLUDE_DIRS})
target_include_directories(imgui INTERFACE dependencies/imgui dependencies)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan glfw)

set_target_properties(imgui PROPERTIES FOLDER "Libraries")

# ======== Assimp ========
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/assimp)

set_target_properties(assimp PROPERTIES FOLDER "Libraries/Assimp")
if (WIN32)
	set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Libraries/Assimp")
	set_target_properties(zlibstatic PROPERTIES FOLDER "Libraries/Assimp")
endif (WIN32)

# ======== SkinTest ========
file(GLOB_RECURSE VULKAN_WRAPPER
    "VulkanWrapper/*.cpp"
    "VulkanWrapper/*.h"
)
add_executable(
	skin_test
	Source.cpp
	VulkanInstance.h
	VulkanInstance.cpp
	Vertex.h
	${VULKAN_WRAPPER}
	stb_image_impl.cpp
	Shaders/shader.frag
	Shaders/shader.vert
	ModelLoader.h
	ModelLoader.cpp
	ImguiImpl.h
	ImguiImpl.cpp
)

target_include_directories(skin_test PRIVATE . dependencies ${Vulkan_INCLUDE_DIRS} dependencies/assimp/include build/dependencies/assimp/include)
target_link_libraries(skin_test PRIVATE Vulkan::Vulkan glfw glm assimp imgui)

source_group("VulkanWrapper" FILES ${VULKAN_WRAPPER})

