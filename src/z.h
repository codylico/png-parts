/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * z.h
 * zlib main header
 */
#ifndef __PNG_PARTS_Z_H__
#define __PNG_PARTS_Z_H__

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*
 * Header structure
 */
struct pngparts_z_header {
  /* check value */
  short int fcheck;
  /* flag: whether the stream has a dictionary */
  short int fdict;
  /* compression level */
  short int flevel;
  /* compression method (8 = deflate) */
  short int cm;
  /* compression information */
  short int cinfo;
};

/*
 * Accumulator for Adlr32 checksums
 */
struct pngparts_z_adler32 {
  /* Flat accumulator */
  unsigned long int s1;
  /* Compound accumulator */
  unsigned long int s2;
};

/*
 * Errors
 */
enum pngparts_z_error {
  /* unsupported stream compression algorithm */
  PNGPARTS_Z_UNSUPPORTED = -8,
  /* output buffer overflow */
  PNGPARTS_Z_OVERFLOW = -7,
  /* i/o error */
  PNGPARTS_Z_IO_ERROR = -6,
  /* parameter not fit the function */
  PNGPARTS_Z_BAD_PARAM = -5,
  /* dictionary requested */
  PNGPARTS_Z_NEED_DICT = -4,
  /* bad check value */
  PNGPARTS_Z_BAD_CHECK = -3,
  /* state machine broke */
  PNGPARTS_Z_BAD_STATE = -2,
  /* premature end of file */
  PNGPARTS_Z_EOF = -1,
  /* all is good */
  PNGPARTS_Z_OK = 0,
  /* the stream is done; quit pushing data */
  PNGPARTS_Z_DONE = 1
};

/*
 * Error message.
 * - result error value
 * @return corresponding error message
 */
PNGPARTS_API
char const* pngparts_z_strerror(int result);
/*
 * Compute a header check value.
 * - hdr the header to check
 * @return the new check value. If this value is not equal to the current
 *   check value, the header may be damaged.
 */
PNGPARTS_API
int pngparts_z_header_check(struct pngparts_z_header hdr);
/*
 * Make a default header.
 * @return the new header
 */
PNGPARTS_API
struct pngparts_z_header pngparts_z_header_new(void);

/*
 * Put the header into a byte stream.
 * - buf buffer of bytes to which to write (2 bytes)
 * - hdr header to write
 */
PNGPARTS_API
void pngparts_z_header_put(void* buf, struct pngparts_z_header  hdr);
/*
 * Get the header from a byte stream.
 * - buf the buffer to read
 * @return the header
 */
PNGPARTS_API
struct pngparts_z_header pngparts_z_header_get(void const* buf);

/*
 * Create a new Adler32 checksum.
 * @return the new checksum
 */
PNGPARTS_API
struct pngparts_z_adler32 pngparts_z_adler32_new(void);
/*
 * Convert an accumulator to a single value.
 * - chk the accumulator
 * @return the single 32-bit checksum
 */
PNGPARTS_API
unsigned long int pngparts_z_adler32_tol(struct pngparts_z_adler32 chk);
/*
 * Accumulate a single byte.
 * - chk the current checksum accumulator state
 * - ch the character to accumulate
 * @return the new accumulator state
 */
PNGPARTS_API
struct pngparts_z_adler32 pngparts_z_adler32_accum
  (struct pngparts_z_adler32 chk, int ch);


#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_Z_H__*/
