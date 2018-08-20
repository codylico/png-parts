/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * flate.h
 * flate main header
 */
#ifndef __PNG_PARTS_FLATE_H__
#define __PNG_PARTS_FLATE_H__

#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct pngparts_flate {
  /* bit position for last input byte */
  signed char bitpos;
  /* last input byte */
  unsigned char last_input_byte;
  /* state */
  signed char state;
  /* size of history of bits */
  unsigned char bitlength;
  /* history of previous bits */
  unsigned int bitline;
  /* past bytes read */
  unsigned char* history_bytes;
  /* size of history in bytes */
  unsigned int history_size;
  /* position in history */
  unsigned int history_pos;
  /* forward and reverse lengths */
  unsigned char shortbuf[4];
  /* short buffer position */
  unsigned short short_pos;
  /* direct block length */
  unsigned int block_length;
};


#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_FLATE_H__*/
