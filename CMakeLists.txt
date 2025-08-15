cmake_minimum_required(VERSION 3.20.0)
project(MCWLaser LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(Sources
    src/main.c
    src/OsTechProtocols.c
    src/SerialPortProtocols.c
)


add_executable(MCWLaser ${Sources})