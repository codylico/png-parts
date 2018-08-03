cmake_minimum_required(VERSION 3.0)

project(pngparts)

option(BUILD_SHARED_LIBS "Build shared libraries")

set(pngparts_src
  api.c api.h)

add_library(pngparts ${pngparts_src})
target_compile_definitions(pngparts PRIVATE "PNGPARTS_EXPORTS")
