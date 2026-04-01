cmake_minimum_required(VERSION 3.16)
project(NeuroCorrelation LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_library(neurocorrelation_core
    src/NeuCor.cpp
)

target_include_directories(neurocorrelation_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_options(neurocorrelation_core
    PRIVATE
        -O3
)

add_executable(NeuroCorrelation
    tinyexpr/tinyexpr.c
    tinyexpr/tinyexpr.h
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_draw.cpp
    imgui/imgui_internal.h
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_glfw.h
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/backends/imgui_impl_opengl3.h
    src/NeuCor_Renderer.cpp
    src/NeuCor_Renderer.h
    src/main.cpp
)

target_include_directories(NeuroCorrelation
    PRIVATE
        ${CMAKE_SOURCE_DIR}/imgui
        ${CMAKE_SOURCE_DIR}/imgui/backends
        ${CMAKE_SOURCE_DIR}/tinyexpr
        ${GLEW_INCLUDE_DIRS}
)

target_compile_options(NeuroCorrelation
    PRIVATE
        -O3
)

target_link_libraries(NeuroCorrelation
    PRIVATE
        neurocorrelation_core
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        glfw
)

add_custom_command(TARGET NeuroCorrelation PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:NeuroCorrelation>/resources
)
