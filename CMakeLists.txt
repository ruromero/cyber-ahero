cmake_minimum_required(VERSION 3.16)
project(cyber_ahero C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(NOT WIN32)
    message(FATAL_ERROR "Este proyecto es para Windows.")
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

set(APP_SOURCES
    src/main.c
    src/app.c
    src/library.c
    src/video.c
    src/text.c
    src/launcher.c
    src/util.c
    third_party/pl_mpeg_impl.c
)

add_executable(cyber_ahero WIN32 ${APP_SOURCES})
target_include_directories(cyber_ahero PRIVATE src third_party)

target_link_libraries(cyber_ahero PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
)

if(MINGW)
    target_compile_definitions(cyber_ahero PRIVATE _WIN32_WINNT=0x0501 WINVER=0x0501)
    target_link_options(cyber_ahero PRIVATE -static-libgcc)
endif()

if(MSVC)
    target_compile_definitions(cyber_ahero PRIVATE _WIN32_WINNT=0x0501 WINVER=0x0501)
    target_compile_options(cyber_ahero PRIVATE /W3)
endif()
