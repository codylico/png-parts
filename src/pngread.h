/*
* PNG-parts
* parts of a Portable Network Graphics implementation
* Copyright 2018 Cody Licorish
*
* Licensed under the MIT License.
*
* pngread.h
* png reader header
*/
#ifndef __PNG_PARTS_PNGREAD_H__
#define __PNG_PARTS_PNGREAD_H__

#include "api.h"
#include "png.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*
 * Initialize a reader for PNG.
 * - p the reader to initialize
 */
PNGPARTS_API
void pngparts_pngread_init(struct pngparts_png* p);
/*
 * Free out a reader for PNG.
 * - p the reader to free
 */
PNGPARTS_API
void pngparts_pngread_free(struct pngparts_png* p);
/*
 * Process the active buffer.
 * - p the reader to use
 * - mode reading mode
 */
PNGPARTS_API
int pngparts_pngread_parse(struct pngparts_png* p);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_PNGREAD_H__*/