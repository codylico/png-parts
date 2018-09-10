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
 * Header structure
 */
struct pngparts_png_header {
  /* width of the image */
  int width;
  /* height of the image */
  int height;
  /* bit depth per sample (8 with RGBA = 32 bpp) */
  short int bit_depth;
  /* color type bits (6 = RGBA) */
  short int color_type;
  /* compression method (0 = deflate) */
  short int compression;
  /* filter type */
  short int filter;
  /* interlace method (0 = no interlace, 1 = Adam7) */
  short int interlace;
};

/*
 * Callback for starting image processing.
 * - width image width
 * - height image height
 * - bit_depth sample depth
 * - color_type PNG color bit field
 * - compression method of compression (should be zero)
 * - filter filter method (should be zero)
 * - interlace interlace method (should be zero or one)
 * - data callback data
 * @return OK if the header good for the image, UNSUPPORTED
 *   otherwise
 */
typedef int (*pngparts_png_start_cb)
  ( int width, int height, short bit_depth, short color_type, short compression,
    short filter, short interlace, void* data);

/*
 * PNG stream base
 */
struct pngparts_png {
  /* callback data */
  void* cb_data;
  /* start callback */
  pngparts_png_start_cb start_cb;
  /* stream state */
  short state;
  /* short position */
  unsigned short shortpos;
  /* buffer for short byte chunks */
  unsigned char shortbuf[15];
  /*
   */
  unsigned char flags_tf;
  /* the current checksum */
  struct pngparts_png_crc32 check;
  /* the buffer for PNG stream data */
  unsigned char* buf;
  /* the size of the buffer in bytes */
  int size;
  /* the position of bytes in the buffer */
  int pos;
  /* next error found while reading */
  int last_result;
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

/*
 * Setup a read-write buffer for next use.
 * - p the PNG structure to configure
 * - buf read-write buffer
 * - size amount of data that can fit in buffer, in bytes
 */
PNGPARTS_API
void pngparts_png_buffer_setup
  (struct pngparts_png *p, void* buf, int size);
/*
 * Check if the reader has used up the buffer.
 * - p PNG structure
 * @return nonzero if the buffer is used up
 */
PNGPARTS_API
int pngparts_png_buffer_done(struct pngparts_png const* p);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_PNG_H__*/
