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
#  ifdef _WIN32
#    ifdef PNGPARTS_EXPORTS
#      define PNGPARTS_API __declspec(dllexport)
#    else
#      define PNGPARTS_API __declspec(dllimport)
#    endif /*PNGPARTS_EXPORTS*/
#  else
#    define PNGPARTS_API
#  endif /*_WIN32*/
#endif /*PNGPARTS_API*/

/*
 * API information as an integer
 */
PNGPARTS_API
int pngparts_api_info(void);

#endif /*__PNG_PARTS_API_H__*/
