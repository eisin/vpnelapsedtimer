cmake_minimum_required(VERSION 3.13)
project(vpnelapsedtimer C)

set(CMAKE_C_STANDARD 99)

add_executable(vpnelapsedtimer main.c resource.h resource.rc)

set(CMAKE_RC_COMPILER_INIT windres)
ENABLE_LANGUAGE(RC)
SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
