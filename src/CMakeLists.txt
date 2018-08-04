cmake_minimum_required(VERSION 3.0)

project(pngparts)

option(BUILD_SHARED_LIBS "Build shared libraries")

set(pngparts_src
  api.c api.h
  z.c z.h)

add_library(pngparts ${pngparts_src})
if (BUILD_SHARED_LIBS)
  if (WIN32)
    target_compile_definitions(pngparts PUBLIC "PNGPARTS_API_SHARED")
  endif(WIN32)
endif(BUILD_SHARED_LIBS)
