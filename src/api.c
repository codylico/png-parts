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
#ifdef PNGPARTS_EXPORTS
  return 0;
#else
  return -1;
#endif /*PNGPARTS_EXPORTS*/
}
