cmake_minimum_required(VERSION 3.20)

add_subdirectory(../libs/glfw ../gui/build EXCLUDE_FROM_ALL)

add_library(imgui
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/libs/imgui
    ${CMAKE_SOURCE_DIR}/libs/imgui/backends
)
target_link_libraries(imgui PUBLIC glfw)

add_executable(laserinterface gui.cpp)
target_include_directories(laserinterface PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/imgui
)
target_link_libraries(laserinterface PRIVATE imgui lasercontrol opengl32)



