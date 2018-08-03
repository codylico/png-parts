/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * test-api.c
 * API information test program
 */

#include "../src/api.h"
#include <stdio.h>

int main(int argc, char **argv){
  fprintf(stdout,"API info: %i\n", pngparts_api_info());
  return 0;
}
