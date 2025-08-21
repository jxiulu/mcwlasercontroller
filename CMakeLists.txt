cmake_minimum_required(VERSION 3.20.0)
project(MCWLaser LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(lasercontrol
    src/OsTechProtocols.c
    src/SerialPortProtocols.c
)
target_include_directories(lasercontrol PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(lasercontrol PUBLIC kernel32)

add_subdirectory(gui)

set(src
    src/console.c
    src/OsTechProtocols.c
    src/SerialPortProtocols.c
)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(MCWLaser ${src})
