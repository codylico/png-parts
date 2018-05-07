cmake_minimum_required(VERSION 3.0)

project(pngparts)

set(pngparts_src
  api.c api.h)

add_library(pngparts ${pngparts_src})
