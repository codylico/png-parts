/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * inflate.h
 * inflate reader header
 */
#ifndef __PNG_PARTS_INFLATE_H__
#define __PNG_PARTS_INFLATE_H__

#include "api.h"
#include "flate.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*
 * Initialize an inflater.
 * - fl the flate struct to prepare
 */
PNGPARTS_API
void pngparts_inflate_init(struct pngparts_flate *fl);
/*
 * Free up an inflater.
 * - fl the flate struct to free
 */
PNGPARTS_API
void pngparts_inflate_free(struct pngparts_flate *fl);
/*
 * Start callback.
 * - hdr header information
 * @return zero if the callback supports the stream,
 *   or -1 otherwise
 */
PNGPARTS_API
int pngparts_inflate_start
  ( short int fdict, short int flevel, short int cm, short int cinfo,
    void* data);
/*
 * Dictionary byte callback.
 * - ch byte, or -1 for repeat bytes
 * - data user data
 * @return zero, or -1 if preset dictionaries are not supported
 */
PNGPARTS_API
int pngparts_inflate_dict(int ch, void* data);
/*
 * Byte callback.
 * - ch byte, or -1 for repeat bytes
 * - data user data
 * - put_cb callback for putting output bytes
 * - put_data data to pass to put callback
 * @return zero, or -1 if the output buffer is too full,
 *   or 1 at the end of the bit stream
 */
PNGPARTS_API
int pngparts_inflate_one
  (int ch, void *data, int(*put_cb)(int,void*), void* put_data);
/*
 * Finish callback.
 * - data user data
 * @return zero, or -1 if the callback expects more data
 */
PNGPARTS_API
int pngparts_inflate_finish(void* data);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_INFLATE_H__*/
