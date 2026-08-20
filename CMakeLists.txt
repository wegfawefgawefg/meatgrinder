cmake_minimum_required(VERSION 3.21)

project(meatgrinder VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MG_STRICT "Enable strict compiler warnings" ON)
option(MG_WARN_AS_ERROR "Treat compiler warnings as errors" ON)
option(MG_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

function(mg_target_defaults target)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    if(MG_STRICT)
        target_compile_options(${target} PRIVATE -Wconversion -Wsign-conversion -Wshadow)
    endif()
    if(MG_WARN_AS_ERROR)
        target_compile_options(${target} PRIVATE -Werror)
    endif()
    if(MG_SANITIZERS)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.22)
    FetchContent_MakeAvailable(SDL3)
endif()

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

set(MG_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Splonks/gubsy/imgui"
    CACHE PATH "ImGui source tree")
if(NOT EXISTS "${MG_IMGUI_DIR}/imgui.cpp")
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.9b)
    FetchContent_MakeAvailable(imgui)
    set(MG_IMGUI_DIR "${imgui_SOURCE_DIR}")
endif()

add_library(meatgrinder_core
    src/camera.cpp
    src/game.cpp
    src/level.cpp)
target_include_directories(meatgrinder_core PUBLIC src)
target_link_libraries(meatgrinder_core PUBLIC nlohmann_json::nlohmann_json)
target_compile_definitions(meatgrinder_core PUBLIC MG_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets")
mg_target_defaults(meatgrinder_core)

add_library(meatgrinder_imgui STATIC
    ${MG_IMGUI_DIR}/imgui.cpp
    ${MG_IMGUI_DIR}/imgui_draw.cpp
    ${MG_IMGUI_DIR}/imgui_tables.cpp
    ${MG_IMGUI_DIR}/imgui_widgets.cpp
    ${MG_IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${MG_IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(meatgrinder_imgui SYSTEM PUBLIC ${MG_IMGUI_DIR} ${MG_IMGUI_DIR}/backends)
target_link_libraries(meatgrinder_imgui PUBLIC SDL3::SDL3)

add_executable(meatgrinder
    src/debug.cpp
    src/draw.cpp
    src/input.cpp
    src/main.cpp)
target_include_directories(meatgrinder PRIVATE src)
target_link_libraries(meatgrinder PRIVATE meatgrinder_core meatgrinder_imgui SDL3::SDL3)
mg_target_defaults(meatgrinder)

enable_testing()
add_executable(meatgrinder_tests tests/game_tests.cpp)
target_link_libraries(meatgrinder_tests PRIVATE meatgrinder_core)
mg_target_defaults(meatgrinder_tests)
add_test(NAME meatgrinder_tests COMMAND meatgrinder_tests)
