/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * api.h
 * API main header
 */
#include "api.h"

int pngparts_api_info(void){
  int out = 0;
#ifdef PNGPARTS_API_SHARED
  out |= PNGPARTS_API_EXPORTS;
#endif /*PNGPARTS_API_SHARED*/
  return out;
}
