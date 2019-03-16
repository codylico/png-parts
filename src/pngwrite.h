/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * pngwrite.h
 * png writer header
 */
#ifndef __PNG_PARTS_PNGWRITE_H__
#define __PNG_PARTS_PNGWRITE_H__

#include "api.h"
#include "png.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*
 * Initialize a writer for PNG.
 * - p the writer to initialize
 */
PNGPARTS_API
void pngparts_pngwrite_init(struct pngparts_png* w);

/*
 * Free out a writer for PNG.
 * - w the writer to free
 */
PNGPARTS_API
void pngparts_pngwrite_free(struct pngparts_png* w);

/*
 * Process the active buffer.
 * - w the writer to use
 */
PNGPARTS_API
int pngparts_pngwrite_generate(struct pngparts_png* w);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_PNGWRITE_H__*/
