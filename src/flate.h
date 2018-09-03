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
 * Length conversion structure.
 */
struct pngparts_flate_extra {
  /* stream value */
  int literal;
  /* first length in bytes */
  int length_value;
  /* extra bits for encoding length */
  int extra_bits;
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
  /* capacity of items */
  int cap;
  /* number of items */
  int count;
};

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
 * Construct a code struct by literal or length value.
 * - value literal or length
 * @return the code
 */
struct pngparts_flate_code pngparts_flate_code_by_literal(int value);
/*
 * Compare two codes by bit strings.
 * - a the first code struct
 * - b the second code struct
 * @return 0 if equal, -1 if a < b, +1 if a > b
 */
int pngparts_flate_code_bitcmp(void const* a, void const* b);

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
 * Sort by code bits.
 * - hf table to sort
 */
void pngparts_flate_huff_bit_sort(struct pngparts_flate_huff* hf);
/*
 * Sort by values.
 * - hf table to sort
 */
void pngparts_flate_huff_value_sort(struct pngparts_flate_huff* hf);
/*
 * Search by code bits, binary search.
 * - hf table to sort
 * - length length of bit string
 * - bits bit string; last bit is lsb,
 * @return the value corresponding to the bit string, NOT_FOUND if
 *   the value was not found, or BAD_BITS if the length is improper
 */
int pngparts_flate_huff_bit_bsearch
  (struct pngparts_flate_huff const* hf, int length, int bits);
/*
 * Search by code bits, linear search.
 * - hf table to sort
 * - length length of bit string
 * - bits bit string; last bit is lsb,
 * @return the value corresponding to the bit string, NOT_FOUND if
 *   the value was not found, or BAD_BITS if the length is improper
 */
int pngparts_flate_huff_bit_lsearch
  (struct pngparts_flate_huff const* hf, int length, int bits);

/*
 * Load the fixed Huffman literal table into a structure.
 * - hf table to modify, should have been already resized to
 *   at least 288 entries
 */
void pngparts_flate_fixed_lengths(struct pngparts_flate_huff* hf);
/*
 * Load the fixed Huffman distance table into a structure.
 * - hf table to modify, should have been already resized to
 *   at least 32 entries
 */
void pngparts_flate_fixed_distances(struct pngparts_flate_huff* hf);
/*
 * Load code length values.
 * - hf table to modify, should have at least four entries
 */
void pngparts_flate_dynamic_codes(struct pngparts_flate_huff* hf);

/*
 * Literal to length conversion function.
 * - literal stream length code
 * @return a structure holding the minimum repeat length for
 *   the given length code, along with the number of extra bits
 *   needed for encoding the number, or a structure with negative
 *   repeat length on error
 */
struct pngparts_flate_extra pngparts_flate_length_decode(int literal);
/*
 * Distance code to distance conversion function.
 * - dcode stream distance code
 * @return a structure holding the minimum repeat distance for
 *   the given distance code, along with the number of extra bits
 *   needed for encoding the number, or a structure with negative
 *   repeat distance on error
 */
struct pngparts_flate_extra pngparts_flate_distance_decode(int dcode);

#ifdef __cplusplus
};
#endif /*__cplusplus*/

#endif /*__PNG_PARTS_FLATE_H__*/
