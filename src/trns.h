/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018-2021 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * trns.h
 * support for tRNS chunk
 */
#ifndef hg_PNG_PARTS_tRNS_H_
#define hg_PNG_PARTS_tRNS_H_

#include "api.h"
#include "png.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct pngparts_png_chunk_cb;

/*
 * Assign an API for reading tRNS chunks.
 * - cb chunk callback
 */
PNGPARTS_API
int pngparts_trns_assign_read_api(struct pngparts_png_chunk_cb* cb);

/*
 * Assign an API for writing tRNS chunks.
 * - cb chunk callback
 */
PNGPARTS_API
int pngparts_trns_assign_write_api(struct pngparts_png_chunk_cb* cb);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*hg_PNG_PARTS_tRNS_H_*/
