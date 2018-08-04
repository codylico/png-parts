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
 * Compute a header check value.
 * - hdr the header to check
 * @return the new check value. If this value is not equal to the current
 *   check value, the header may be damaged.
 */
PNGPARTS_API
int pngparts_z_compute_check(struct pngparts_z_header hdr);
/*
 * Make a default header.
 * @return the new header
 */
PNGPARTS_API
struct pngparts_z_header pngparts_z_header_new(void);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_Z_H__*/
