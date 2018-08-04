cmake_minimum_required(VERSION 3.0)

add_executable(test_api "test-api.c")
target_link_libraries(test_api pngparts)

add_executable(test_z "test-z.c")
target_link_libraries(test_z pngparts)
