cmake_minimum_required(VERSION 3.10)
project(CyberDublin)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL, GLEW, and GLFW
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

# Include directories
include_directories(${GLEW_INCLUDE_DIRS} ${GLFW3_INCLUDE_DIRS} src)

link_directories(/opt/homebrew/lib)

# Add the executable
add_executable(CyberDublin src/main.cpp)

# Link libraries
target_link_libraries(CyberDublin OpenGL::GL glfw GLEW::GLEW m)

