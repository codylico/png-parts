/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018-2019 Cody Licorish
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

/*
 * Assign an API for writing IDAT chunks.
 * - cb chunk callback
 * - z zlib stream writer
 * - chunk_size size of chunks to generate (or 0 for default value)
 * @return OK on success
 */
PNGPARTS_API
int pngparts_pngwrite_assign_idat_api
  ( struct pngparts_png_chunk_cb* cb, struct pngparts_api_z const* z,
    int chunk_size);

/*
 * Assign an API for writing a PLTE chunk.
 * - cb chunk callback
 * @return OK on success
 */
PNGPARTS_API
int pngparts_pngwrite_assign_plte_api( struct pngparts_png_chunk_cb* cb);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_PNGWRITE_H__*/
