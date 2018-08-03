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
#ifndef __PNG_PARTS_API_H__
#define __PNG_PARTS_API_H__

/*
 * export variable
 */
#ifndef PNGPARTS_API
#  if (defined PNGPARTS_API_SHARED)
#    ifdef pngparts_EXPORTS
#      define PNGPARTS_API __declspec(dllexport)
#    else
#      define PNGPARTS_API __declspec(dllimport)
#    endif /*pngparts_EXPORTS*/
#  else
#    define PNGPARTS_API
#  endif /*PNGPARTS_API_SHARED*/
#endif /*PNGPARTS_API*/

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*
 * API information flags
 */
enum pngparts_api_flag {
  /* whether the library uses explicit exporting */
  PNGPARTS_API_EXPORTS = 1
};
/*
 * API information as an integer
 */
PNGPARTS_API
int pngparts_api_info(void);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_API_H__*/
