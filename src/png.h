/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * png.h
 * png base header
 */
#ifndef __PNG_PARTS_PNG_H__
#define __PNG_PARTS_PNG_H__

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*
 * CRC32 checksum
 */
struct pngparts_png_crc32 {
  unsigned long int accum;
};

/*
 * @return an 8-byte signature for PNG files
 */
PNGPARTS_API
unsigned char const* pngparts_png_signature(void);

/*
 * @return a new CRC-32 instance
 */
PNGPARTS_API
struct pngparts_png_crc32 pngparts_png_crc32_new(void);
/*
 * Convert an accumulator to a single value.
 * - chk the accumulator
 * @return the single 32-bit checksum
 */
PNGPARTS_API
unsigned long int pngparts_png_crc32_tol(struct pngparts_png_crc32 chk);
/*
 * Accumulate a single byte.
 * - chk the current checksum accumulator state
 * - ch the byte (0 - 255) to accumulate
 * @return the new accumulator state
 */
PNGPARTS_API
struct pngparts_png_crc32 pngparts_png_crc32_accum
  (struct pngparts_png_crc32 chk, int ch);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_PNG_H__*/
