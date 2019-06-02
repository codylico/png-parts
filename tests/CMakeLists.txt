cmake_minimum_required(VERSION 3.0)

add_executable(pngparts_test_api "test-api.c")
target_link_libraries(pngparts_test_api pngparts)

add_executable(pngparts_test_z "test-z.c")
target_link_libraries(pngparts_test_z pngparts)

add_executable(pngparts_test_zread "test-zread.c")
target_link_libraries(pngparts_test_zread pngparts)

add_executable(pngparts_test_huff "test-huff.c")
target_link_libraries(pngparts_test_huff pngparts)

add_executable(pngparts_test_png "test-png.c")
target_link_libraries(pngparts_test_png pngparts)

add_executable(pngparts_test_pngread "test-pngread.c")
target_link_libraries(pngparts_test_pngread pngparts)

add_executable(pngparts_test_zwrite "test-zwrite.c")
target_link_libraries(pngparts_test_zwrite pngparts)

add_executable(pngparts_test_pngwrite "test-pngwrite.c")
target_link_libraries(pngparts_test_pngwrite pngparts)

add_executable(pngparts_test_hash "test-hash.c")
target_link_libraries(pngparts_test_hash pngparts)

add_executable(pngparts_test_flate "test-flate.c")
target_link_libraries(pngparts_test_flate pngparts)

if (PNGPARTS_INCLUDE_AUX)
add_executable(pngparts_test_aux_pngread "test-aux-pngread.c")
target_link_libraries(pngparts_test_aux_pngread pngparts)

add_executable(pngparts_test_aux_pngwrite "test-aux-pngwrite.c")
target_link_libraries(pngparts_test_aux_pngwrite pngparts)

add_executable(pngparts_test_aux8_pngread "test-aux8-pngread.c")
target_link_libraries(pngparts_test_aux8_pngread pngparts)

add_executable(pngparts_test_aux8_pngwrite "test-aux8-pngwrite.c")
target_link_libraries(pngparts_test_aux8_pngwrite pngparts)
endif (PNGPARTS_INCLUDE_AUX)
