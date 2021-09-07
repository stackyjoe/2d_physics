# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)

project ( 2d_physics )

add_executable(2d_physics)


# Include sub-projects.
add_subdirectory ("src")
add_subdirectory ("lib")

target_link_libraries(2d_physics PRIVATE src)
