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

add_executable( ${PROJECT_NAME}
        src/main.cpp
        src/studio_model.h
        src/studio_render.cpp
        src/studio_utils.cpp
        src/mathlib.cpp
        src/mathlib.h
        src/studio.h
        
        src/GoldSrcModel.cpp
        src/GoldSrcModel.h
)

target_include_directories(${PROJECT_NAME} PRIVATE deps/glm)
target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw)
