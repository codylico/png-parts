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

struct pngparts_png;
struct pngparts_png_chunk_link;

/*
 * CRC32 checksum
 */
struct pngparts_png_crc32 {
  unsigned long int accum;
};
/*
 * Palette color
 */
struct pngparts_png_plte_item {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
};

/*
 * Message types for PNG chunk callbacks.
 */
enum pngparts_png_message_type {
  /* Readiness check */
  PNGPARTS_PNG_M_READY = 1,
  /* Byte getter (send byte to the callback) */
  PNGPARTS_PNG_M_GET = 2,
  /* Byte putter (receive byte from the callback) */
  PNGPARTS_PNG_M_PUT = 3,
  /* Start this chunk
   * - byte nonzero in write mode, zero in read mode
   * - ptr points to an unsigned long int, get or set chunk length
   */
  PNGPARTS_PNG_M_START = 4,
  /* Finish this chunk */
  PNGPARTS_PNG_M_FINISH = 5,
  /* At end of stream */
  PNGPARTS_PNG_M_ALL_DONE = 6,
  /* Destructor */
  PNGPARTS_PNG_M_DESTROY = 7
};
/*
 * Chunk callback message.
 */
struct pngparts_png_message {
  /* message type */
  int type;
  /* input/output data */
  int byte;
  /* target chunk callback */
  unsigned char name[4];
  /* input/output pointer */
  void* ptr;
};

/* chunk callback */
struct pngparts_png_chunk_cb {
  /* callback data */
  void* cb_data;
  /* name */
  unsigned char name[4];
  /* message passing callback */
  int (*message_cb)
    (struct pngparts_png*, void* cb_data, struct pngparts_png_message* msg);
};

/*
 * Header structure
 */
struct pngparts_png_header {
  /* width of the image */
  long int width;
  /* height of the image */
  long int height;
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
 * PNG stream base
 */
struct pngparts_png {
  /* stream state */
  short state;
  /* short position */
  unsigned short shortpos;
  /* buffer for short byte chunks */
  unsigned char shortbuf[15];
  /*
   * 8 - IHDR crossed
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
  /* size of main palette */
  int palette_count;
  /* palette array */
  struct pngparts_png_plte_item *palette;
  /* chunk size */
  unsigned long int chunk_size;
  /* chunk callback linked list */
  struct pngparts_png_chunk_link *chunk_cbs;
  /* active chunk callback */
  struct pngparts_png_chunk_cb const* active_chunk_cb;
  /* header */
  struct pngparts_png_header header;
  /* image callback */
  struct pngparts_api_image img_cb;
};


/*
 * Convert from  Adam7 coordinates to regular coordinates.
 * - level interlace level
 * - dx regular x-coordinate
 * - dy regular y-coordinate
 * - sx Adam7 x-coordinate
 * - sy Adam7 y-coordinate
 */
PNGPARTS_API
void pngparts_png_adam7_reverse_xy
  (int level, long int *dx, long int *dy, long int sx, long int sy);

/*
 * Compute the Paeth prediction.
 * - left the left byte value
 * - up the up byte value
 * - corner the top-left neighbor byte value
 * @return the Paeth prediction
 */
PNGPARTS_API
int pngparts_png_paeth_predict(int left, int up, int corner);

/*
 * @return an 8-byte signature for PNG files
 */
PNGPARTS_API
unsigned char const* pngparts_png_signature(void);

/*
 * Check if a header is valid.
 * - hdr the header to check
 * @return nonzero if the header is valid
 */
PNGPARTS_API
int pngparts_png_header_is_valid(struct pngparts_png_header hdr);

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

/*
 * Get the image callback.
 * - p PNG structure
 * - img_cb image callback structure to write
 */
PNGPARTS_API
void pngparts_png_get_image_cb
  (struct pngparts_png const* p, struct pngparts_api_image* img_cb);
/*
 * Set the image callback.
 * - p PNG structure
 * - img_cb image callback structure
 */
PNGPARTS_API
void pngparts_png_set_image_cb
  (struct pngparts_png* p, struct pngparts_api_image const* img_cb);

/*
 * Set the palette size.
 * - p the PNG structure to modify
 * - siz new desired size (between 0 and 256 accepted)
 * @return OK on success, MEMORY on failure
 */
PNGPARTS_API
int pngparts_png_set_plte_size(struct pngparts_png* p, int siz);
/*
 * Get the palette size.
 * - p the PNG structure to modify
 * @return the current size of the palette
 */
PNGPARTS_API
int pngparts_png_get_plte_size(struct pngparts_png const* p);
/*
 * Set the palette item.
 * - p the PNG structure to modify
 * - i array index
 * - v color value
 */
void pngparts_png_set_plte_item
  (struct pngparts_png* p, int i, struct pngparts_png_plte_item v);
/*
 * Get a palette item.
 * - p the PNG structure to modify
 * - i array index
 * @return the color value at that index
 */
struct pngparts_png_plte_item pngparts_png_get_plte_item
  (struct pngparts_png const* p, int i);

/*
 * Add a chunk callback.
 * - p PNG structure
 * - cb chunk callback structure
 * @return OK on success, MEMORY on allocation failure
 */
PNGPARTS_API
int pngparts_png_add_chunk_cb
  (struct pngparts_png* p, struct pngparts_png_chunk_cb const*cb);
/*
 * Remove a chunk callback.
 * - p PNG structure
 * - name chunk name of callback to remove
 */
PNGPARTS_API
void pngparts_png_remove_chunk_cb
  (struct pngparts_png* p, unsigned char const* name);
/*
 * Drop all chunk callbacks.
 * - p PNG structure
 */
PNGPARTS_API
void pngparts_png_drop_chunk_cbs(struct pngparts_png* p);
/*
 * Find a chunk callback by name.
 * - p PNG structure
 * - name name of chunk callback to find
 * @return a pointer to the chunk callback if found, NULL otherwise
 */
PNGPARTS_API
struct pngparts_png_chunk_cb const* pngparts_png_find_chunk_cb
  ( struct pngparts_png *p, unsigned char const* name);

/*
 * Send a message to a chunk callback.
 * - p PNG structure
 * - cb callback to which to send a message
 * - msg message to send
 * @return the result from the callback
 */
PNGPARTS_API
int pngparts_png_send_chunk_msg
  ( struct pngparts_png *p, struct pngparts_png_chunk_cb const* cb,
    struct pngparts_png_message* msg);
/*
 * Get the number of bytes remaining in a chunk.
 * - p PNG structure
 * @return number of bytes remaining
 */
PNGPARTS_API
long int pngparts_png_chunk_remaining(struct pngparts_png const* p);

/*
 * Boradcast a message to all chunk callbacks.
 * - p PNG structure
 * - msg message to broadcast
 * @return the first negative result from a callback,
 *   or OK on success
 */
PNGPARTS_API
int pngparts_png_broadcast_chunk_msg
  (struct pngparts_png *p, struct pngparts_png_message const* msg);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_PNG_H__*/
