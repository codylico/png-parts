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

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

struct pngparts_flate {
  /* bit position for last input byte */
  signed char bitpos;
  /* last input byte */
  unsigned char last_input_byte;
};


#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_FLATE_H__*/
