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
)
target_include_directories(imgui PRIVATE dependencies/imgui)
target_include_directories(imgui INTERFACE dependencies/imgui dependencies)

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
add_executable(
	skin_test
	Source.cpp
	VulkanInstance.h
	VulkanInstance.cpp
	Vertex.h
	VulkanWrapper/Image.h
	VulkanWrapper/Image.cpp
	VulkanWrapper/Buffer.h
	VulkanWrapper/Buffer.cpp
	VulkanWrapper/CommandBuffer.h
	VulkanWrapper/CommandBuffer.cpp
	VulkanWrapper/Log.h
	Shaders/shader.frag
	Shaders/shader.vert
)
target_include_directories(skin_test PRIVATE . dependencies ${Vulkan_INCLUDE_DIRS})
target_link_libraries(skin_test PRIVATE Vulkan::Vulkan glfw glm)
