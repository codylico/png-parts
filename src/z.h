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


struct pngparts_z;

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
  /* bad Adler32 checksum */
  PNGPARTS_Z_BAD_SUM = -9,
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
 * Start callback.
 * - base the base
 * - hdr header information
 * @return OK if the callback supports the stream,
 *   or UNSUPPORTED otherwise
 */
typedef int (*pngparts_z_start_cb)
  (struct pngparts_z* base, struct pngparts_z_header hdr, void* data);
/*
 * Byte callback.
 * - reader the reader
 * - ch byte, or -1 for repeat bytes
 * - data user data
 * @return zero, or OVERFLOW if the output buffer is too full,
 *   or DONE at the end of the bit stream
 */
typedef int (*pngparts_z_one_cb)
  (struct pngparts_z* base, int ch, void *data);
/*
 * Finish callback.
 * - reader the reader
 * - data user data
 * @return zero, or EOF if the callback expects more data
 */
typedef int (*pngparts_z_finish_cb)(struct pngparts_z* base, void* data);
/*
 * Base structure for zlib processors.
 */
struct pngparts_z {
  /* callback data */
  void* cb_data;
  /* start callback */
  pngparts_z_start_cb start_cb;
  /* bit callback */
  pngparts_z_one_cb one_cb;
  /* finish callback */
  pngparts_z_finish_cb finish_cb;
  /* reader state */
  short state;
  /* the stream header */
  struct pngparts_z_header header;
  /* short position */
  unsigned short shortpos;
  /* buffer for short byte chunks */
  unsigned char shortbuf[15];
  /*
   * 1: if a dictionary has been set
   * 2: skip reading a byte
   */
  unsigned char flags_tf;
  /* dictionary checksum */
  unsigned long int dict_check;
  /* the current checksum */
  struct pngparts_z_adler32 check;
  /* the input buffer for deflated data */
  unsigned char* inbuf;
  /* the output buffer for inflated data */
  unsigned char* outbuf;
  /* the size of the input buffer in bytes */
  int insize;
  /* the size of the output buffer in bytes */
  int outsize;
  /* the position of bytes read from the buffer */
  int inpos;
  /* the position of bytes written to the buffer */
  int outpos;
  /* next error found while reading */
  int last_result;
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

/*
 * Setup an input buffer for next use.
 * - reader reader
 * - inbuf input buffer
 * - insize amount of data to input
 */
PNGPARTS_API
void pngparts_z_setup_input
  (struct pngparts_z *reader, void* inbuf, int insize);
/*
 * Setup an output buffer for next use.
 * - reader reader
 * - outbuf output buffer
 * - outsize amount of space available for output
 */
PNGPARTS_API
void pngparts_z_setup_output
  (struct pngparts_z *reader, void* outbuf, int outsize);
/*
 * Check if the reader has used up all the latest input.
 * - reader reader
 * @return nonzero if the input is used up
 */
PNGPARTS_API
int pngparts_z_input_done(struct pngparts_z const* reader);
/*
 * Check how much output bytes wait for you.
 * - reader reader
 * @return byte count for the output bytes
 */
PNGPARTS_API
int pngparts_z_output_left(struct pngparts_z const* reader);
/*
 * Set the compression callbacks.
 * - base the reader
 * - cb_data static user data
 * - start_cb constructor for callback data
 * - bit_cb bit fiddler
 * - finish_cb destructor for callback data
 */
PNGPARTS_API
void pngparts_z_set_cb
  ( struct pngparts_z *base, void *cb_data,
    pngparts_z_start_cb start_cb, pngparts_z_one_cb one_cb,
    pngparts_z_finish_cb finish_cb);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_Z_H__*/
