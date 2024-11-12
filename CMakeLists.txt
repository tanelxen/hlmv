cmake_minimum_required(VERSION 3.28)
project(hlmv)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# Configure GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# Thirdparty libraries
add_subdirectory( deps/glad )
add_subdirectory( deps/glfw )
add_subdirectory( deps/imgui )

add_executable( ${PROJECT_NAME}
        src/main.cpp
        src/studio.h
        
        src/GoldSrcModel.cpp
        src/GoldSrcModel.h
        
        src/Renderer.cpp
        src/Renderer.h

        deps/imgui/backends/imgui_impl_glfw.cpp
        deps/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE deps/glm)

target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw imgui)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/assets
)
