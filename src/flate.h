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

/*
 * base for flater
 */
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

/*
 * Huffman code item
 */
struct pngparts_flate_code {
  /* bit length of the item */
  short length;
  /* bits for the item */
  unsigned int bits;
  /* corresponding length-literal value */
  int value;
};
/*
 * Huffman code table
 */
struct pngparts_flate_huff {
  /* array of items */
  struct pngparts_flate_code *its;
  /* number of items */
  int count;
};

/*
 * Construct a code struct by literal or length value.
 * - value literal or length
 * @return the code
 */
struct pngparts_flate_code pngparts_flate_code_by_literal(int value);

/*
 * Get a fixed Huffman code array.
 * @return the 288-element code array
 */
struct pngparts_flate_code const* pngparts_flate_huff_fixed(void);
/*
 * Initialize a Huffman code table.
 * - hf Huffman code table
 */
void pngparts_flate_huff_init(struct pngparts_flate_huff* hf);
/*
 * Free out a Huffman code table.
 * - hf Huffman code table
 */
void pngparts_flate_huff_free(struct pngparts_flate_huff* hf);
/*
 * Resize the given code table.
 * - hf table to resize
 * - siz number of items
 * @return OK on success, MEMORY on failure
 */
int pngparts_flate_huff_resize(struct pngparts_flate_huff* hf, int siz);
/*
 * Get table size in codes.
 * - hf Huffman code table
 * @return a length
 */
int pngparts_flate_huff_get_size(struct pngparts_flate_huff const* hf);
/*
 * Access a code in the table.
 * - hf table to modify
 * - i array index
 * @return the code at that index
 */
struct pngparts_flate_code pngparts_flate_huff_index_get
  (struct pngparts_flate_huff const* hf, int i);
/*
 * Access a code in the table.
 * - hf table to modify
 * - i array index
 * - c new code
 */
void pngparts_flate_huff_index_set
  (struct pngparts_flate_huff* hf, int i, struct pngparts_flate_code c);
/*
 * Generate from bit lengths the bit strings.
 * - hf table to modify
 * @return OK on success, CODE_EXCESS if code count exceeds constraints,
 *   BAD_PARAM if the lengths too long
 */
int pngparts_flate_huff_generate(struct pngparts_flate_huff* hf);
/*
 * Generate from histograms the bit lengths.
 * - hf table to modify
 * - hist relative histogram, size equal to size of table
 */
void pngparts_flate_huff_make_lengths
  (struct pngparts_flate_huff* hf, int const* hist);
/*
 * Load a pre-created table into a structure.
 * - hf table to modify
 * - i starting index
 * - s number of codes
 * - c code array
 */
void pngparts_flate_huff_copy
  ( struct pngparts_flate_huff* hf, int i, int s,
    struct pngparts_flate_code const* c);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_FLATE_H__*/
