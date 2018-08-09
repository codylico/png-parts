/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * zread.h
 * zlib reader header
 */
#ifndef __PNG_PARTS_ZREAD_H__
#define __PNG_PARTS_ZREAD_H__

#include "api.h"
#include "z.h"
#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct pngparts_zread;

/*
 * Reading modes
 */
enum pngparts_zread_mode {
  /* normal reading */
  PNGPARTS_ZREAD_NORMAL = 0,
  /* treat it like it's the end */
  PNGPARTS_ZREAD_FINISH = 1
};

/*
 * Initialize a stream reader.
 * - prs the reader to initialize
 */
PNGPARTS_API
void pngparts_zread_init(struct pngparts_z *prs);
/*
 * End a stream reader.
 * - prs the reader to end
 */
PNGPARTS_API
void pngparts_zread_free(struct pngparts_z *prs);
/*
 * Parse a part of a stream.
 * - prs reader
 * - mode reader expectation mode
 */
PNGPARTS_API
int pngparts_zread_parse(struct pngparts_z *prs, int mode);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_ZREAD_H__*/
