cmake_minimum_required(VERSION 3.21.0)
project(MCWLaser LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)
set(srcfiles
    src/comm/OsTechProtocols.c
    src/comm/serial-win.c
    src/app/console.c
)
add_executable(cli ${srcfiles})


find_package(OpenGL REQUIRED)
add_subdirectory(lib/glfw)

set(gui_srcfiles
    src/app/gui.cpp
    src/comm/lumics.c
    src/comm/serial-win.c
)
list(APPEND gui_srcfiles
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
)
add_executable(gui ${gui_srcfiles})

target_include_directories(gui PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/glfw/include
    ${CMAKE_SOURCE_DIR}/lib/imgui
)
target_link_libraries(gui OpenGL::GL)
target_link_libraries(gui
    glfw
    ${CMAKE_DL_LIBS}
)
