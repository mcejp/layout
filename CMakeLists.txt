# CMakeList.txt : CMake project for liblayout, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("liblayout")

# Add source to this project's executable.
add_executable (liblayout "liblayout.cpp" "liblayout.h")

target_include_directories(liblayout PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

# TODO: Add tests and install targets if needed.
