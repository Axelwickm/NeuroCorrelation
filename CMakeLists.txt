cmake_minimum_required (VERSION 3.10)
project(NeuroCorrelation)


include_directories(${CMAKE_SOURCE_DIR}/imgui)
include_directories(${CMAKE_SOURCE_DIR}/tinyexpr)

find_package(Boost COMPONENTS container REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

find_package(OpenGL)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

include_directories(${GLEW_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(NeuroCorrelation
	tinyexpr/tinyexpr.h tinyexpr/tinyexpr.c
	imgui/imgui.h imgui/imgui.cpp imgui/imgui_internal.h imgui/imgui_draw.cpp
	src/imgui_impl_glfw_gl3.h src/imgui_impl_glfw_gl3.cpp 
	src/NeuCor.h src/NeuCor.cpp src/NeuCor_Renderer.h src/NeuCor_Renderer.cpp
	src/main.cpp)

target_link_libraries(NeuroCorrelation LINK_PUBLIC ${Boost_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} glfw)

add_custom_command(TARGET NeuroCorrelation PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/resources/ $<TARGET_FILE_DIR:NeuroCorrelation>/resources)
