cmake_minimum_required(VERSION 3.13)
project(vpnelapsedtimer C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3 -s")

add_executable(vpnelapsedtimer WIN32 src/main.c src/resource.h src/resource.rc)
add_definitions(-DUNICODE -D_UNICODE)

set(CMAKE_RC_COMPILER_INIT windres)
ENABLE_LANGUAGE(RC)
SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
