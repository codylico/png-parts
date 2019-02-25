/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018-2019 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * deflate.c
 * source for deflate writer and compressor
 */
#include "deflate.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>


enum pngparts_deflate_last_block {
  PNGPARTS_DEFLATE_STATE = 31,
  PNGPARTS_DEFLATE_LAST = PNGPARTS_DEFLATE_STATE+1
};

enum pngparts_deflate_alphabet {
  PNGPARTS_DEFLATE_MAXALPHABET = 3+286+30,
};

/*
 * Process a byte of input.
 * - fl deflater structure
 * - ch next byte to put
 * @return OK on success, or OVERFLOW if the next block
 *   needs to go.
 */
static int pngparts_deflate_churn_input
  (struct pngparts_flate *fl, int ch);

/*
 * Queue up a value for the next byte of input.
 * - fl deflater structure
 * - ch next byte to put
 * @return OK on success, or OVERFLOW if the next block
 *   needs to go.
 */
static int pngparts_deflate_queue_value
  (struct pngparts_flate *fl, unsigned short int value);

/*
 * Check whether the queue has a certain number of value slots available.
 * - fl deflater structure holding the queue
 * - count number of slots to check
 * @return OK if available, else OVERFLOW
 */
static int pngparts_deflate_queue_check
  (struct pngparts_flate *fl, unsigned short count);

/*
 * Fashion the next chunk of compressed data.
 * - fl deflater structure holding the queue
 * - put_cb callback for putting output bytes
 * - put_data data to pass to put callback
 * @return OK if available, else OVERFLOW
 */
static int pngparts_deflate_fashion_chunk
  (struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int));

/*
 * Fashion the next alphabet for compressed data.
 * - fl deflater structure holding the queue
 * - put_cb callback for putting output bytes
 * - put_data data to pass to put callback
 * @return OK if available, else OVERFLOW
 */
static int pngparts_deflate_fashion_alphabet(struct pngparts_flate *fl);

/*
 * Send a bit through the bit channel.
 * - fl deflater structure holding the queue
 * - b bit to send
 * - put_cb callback for putting output bytes
 * - put_data data to pass to put callback
 * @return OK if available, else OVERFLOW
 */
static int pngparts_deflate_send_bit
  ( struct pngparts_flate *fl, unsigned char b,
    void* put_data, int(*put_cb)(void*,int));

/*
 * Send an integer through the bit channel.
 * - fl deflater structure holding the queue
 * - put_cb callback for putting output bytes
 * - put_data data to pass to put callback
 * @return OK if available, else OVERFLOW
 */
static int pngparts_deflate_send_integer
  ( struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int));

/*
 * Send a Huffman code through the bit channel.
 * - fl deflater structure holding the queue
 * - put_cb callback for putting output bytes
 * - put_data data to pass to put callback
 * @return OK if available, else OVERFLOW
 */
static int pngparts_deflate_send_code
  ( struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int));

/*
 * Flush the bit channel.
 * - fl deflater structure holding the queue
 * - put_cb callback for putting output bytes
 * - put_data data to pass to put callback
 * @return OK if available, else OVERFLOW
 */
static int pngparts_deflate_flush_bits
  (struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int));

/*
 * Compose inscription-specific tables.
 * - fl deflater structure to configure
 * @return OK on success, other negative value on error
 */
static int pngparts_deflate_compose_tables(struct pngparts_flate *fl);

int pngparts_deflate_churn_input
  (struct pngparts_flate *fl, int ch)
{
  int result = PNGPARTS_API_OK;
  if (ch >= 0){
    /* add next value */
    memmove(fl->shortbuf, fl->shortbuf+1, 3*sizeof(unsigned char));
    fl->shortbuf[3] = (unsigned char)(ch&255);
    fl->short_pos += 1;
  }
  switch (fl->block_level){
  case PNGPARTS_FLATE_OFF:
  default:
    /* plain put */
    result = pngparts_deflate_queue_check(fl, fl->short_pos);
    if (result == PNGPARTS_API_OK){
      unsigned int i;
      unsigned int point = 4-fl->short_pos;
      for (i = 0; i < fl->short_pos; ++i){
        pngparts_deflate_queue_value(fl, fl->shortbuf[point]);
        point += 1;
      }
      fl->short_pos = 0;
    }
    break;
  }
  return result;
}

int pngparts_deflate_queue_check
  (struct pngparts_flate *fl, unsigned short count)
{
  if (fl->state & PNGPARTS_DEFLATE_LAST
  &&  (fl->inscription_size - fl->block_length) >= count)
    return PNGPARTS_API_OK;
  else if ((fl->inscription_size - fl->block_length) >= count+4)
    /* ensure 4 inscriptions of extra space at finish */
    return PNGPARTS_API_OK;
  else return PNGPARTS_API_OVERFLOW;
}

int pngparts_deflate_queue_value
  (struct pngparts_flate *fl, unsigned short int value)
{
  if (fl->block_length < fl->inscription_size){
    fl->inscription_text[fl->block_length] = value;
    fl->block_length += 1;
    return PNGPARTS_API_OK;
  } else return PNGPARTS_API_OVERFLOW;
}

int pngparts_deflate_send_bit
  ( struct pngparts_flate *fl, unsigned char b,
    void* put_data, int(*put_cb)(void*,int))
{
  if (fl->bitpos >= 8){
    /* send the byte */
    int result = (*put_cb)(put_data, fl->next_output_byte);
    if (result != PNGPARTS_API_OK){
      return result;
    }
    fl->bitpos = 0;
    fl->next_output_byte = 0;
  }
  fl->next_output_byte |= (b<<(fl->bitpos));
  fl->bitpos += 1;
  return PNGPARTS_API_OK;
}

int pngparts_deflate_flush_bits
  (struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int))
{
  if (fl->bitpos > 0){
    int result = (*put_cb)(put_data, fl->next_output_byte);
    if (result != PNGPARTS_API_OK){
      return result;
    }
    fl->bitpos = 0;
    fl->next_output_byte = 0;
  }
  return PNGPARTS_API_OK;
}

int pngparts_deflate_compose_tables(struct pngparts_flate *fl){
  int result = PNGPARTS_API_OK;
  int length_hist[288];
  int distance_hist[32];
  /* initialize the histograms */{
    int i;
    for (i = 0; i < 288; ++i){
      pngparts_flate_huff_index_set(
          &fl->length_table, i, pngparts_flate_code_by_literal(i)
        );
      if (i == 256)
        length_hist[i] = 1;
      else length_hist[i] = 0;
    }
    for (i = 0; i < 32; ++i){
      pngparts_flate_huff_index_set(
          &fl->distance_table, i, pngparts_flate_code_by_literal(i)
        );
      distance_hist[i] = 0;
    }
  }
  /* accumulate values */{
    unsigned int j;
    int code_switch = 0;
    for (j = 0; j < fl->block_length; ++j){
      unsigned int const value = fl->inscription_text[j];
      switch (code_switch){
      case 0:
        if (value < 286){
          if (length_hist[value] < INT_MAX){
            length_hist[value] += 1;
          }
          if (value >= 265 && value <= 284){
            code_switch = 1;
          } else if (value > 256){
            code_switch = 2;
          }
        } else result = PNGPARTS_API_BAD_BLOCK;
        break;
      case 1:
        code_switch = 2;
        break;
      case 2:
        if (value < 30){
          if (distance_hist[value] < INT_MAX){
            distance_hist[value] += 1;
          }
          if (value >= 4){
            code_switch = 3;
          } else {
            code_switch = 0;
          }
        } else result = PNGPARTS_API_BAD_BLOCK;
        break;
      case 3:
        code_switch = 0;
        break;
      }
      if (result != PNGPARTS_API_OK)
        break;
    }
  }
  /* make lengths */if (result == PNGPARTS_API_OK){
    pngparts_flate_huff_make_lengths(&fl->length_table, length_hist);
    pngparts_flate_huff_make_lengths(&fl->distance_table, distance_hist);
  }
  /* generate bits */if (result == PNGPARTS_API_OK){
    result = pngparts_flate_huff_generate(&fl->length_table);
    if (result != PNGPARTS_API_OK)
      return result;
    result = pngparts_flate_huff_generate(&fl->distance_table);
    /*
    if (result != PNGPARTS_API_OK)
      return result;
    */
  }
  return result;
}

int pngparts_deflate_fashion_chunk
  (struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int))
{
  int result = PNGPARTS_API_BAD_STATE;
  /* states:
   *  1 - block information structure
   *  2 - compose the block length
   *  3 - compose the reverse block length
   *  4 - plain output
   *  5 - final state
   *  6 - setup and preamble to fixed block
   *  7 - length/literal
   *  8 - length extra
   *  9 - distance
   * 10 - distance extra
   * 11 - setup dynamic block
   * 12 - emit the alphabet preamble
   * 13 - emit the proto-alphabet
   * 14 - encode the alphabet
   * 15 - alphabet extra
   */
  int state = fl->state&PNGPARTS_DEFLATE_STATE;
  int const last = (fl->state&PNGPARTS_DEFLATE_LAST);
  switch (state){
  case 1: /* block information structure */
    if (fl->bitlength == 0){
      unsigned int const last_tf = last ? 1 : 0;
      unsigned int const block_type =
        (fl->block_type > 2) ? 2 : fl->block_type;
      fl->bitline = (block_type<<1) | last_tf;
      fl->bitlength = 3;
    }
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_integer(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        /* switch on type */
        switch (fl->block_type){
        case PNGPARTS_FLATE_DYNAMIC: /* dynamic */
          state = 11;
          fl->bitlength = 0;
          break;
        case PNGPARTS_FLATE_FIXED: /* fixed */
          state = 6;
          fl->bitlength = 0;
          break;
        case PNGPARTS_FLATE_PLAIN: /* plain */
        default:
          state = 2;
          fl->bitlength = 0;
          break;
        }
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 2: /* compose the block length */
    result = pngparts_deflate_flush_bits(fl, put_data, put_cb);
    if (result != PNGPARTS_API_OK)
      break;
    if (fl->bitlength == 0){
      fl->bitline = fl->block_length;
      fl->bitlength = 16;
    }
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_integer(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        state = 3;
        /*fl->bitlength = 0;*/
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 3: /* compose the reverse block length */
    if (fl->bitlength == 0){
      fl->bitline = fl->block_length^65535u;
      fl->bitlength = 16;
    }
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_integer(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        state = 4;
        fl->inscription_pos = 0;
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 4: /* plain output */
    result = pngparts_deflate_flush_bits(fl, put_data, put_cb);
    if (result != PNGPARTS_API_OK)
      break;
    if (fl->inscription_pos < fl->block_length){
      result = (*put_cb)(put_data, fl->inscription_text[fl->inscription_pos]);
      if (result == PNGPARTS_API_OK){
        result = PNGPARTS_API_LOOPED_STATE;
      } else break;
      fl->inscription_pos += 1;
    }
    if (fl->inscription_pos >= fl->block_length){
      /* back to initial state */
      fl->block_length = 0;
      if (last) {
        state = 5;
      } else
        state = 0;
      result = PNGPARTS_API_LOOPED_STATE;
    }
    break;
  case 5: /* final state */
    result = pngparts_deflate_flush_bits(fl, put_data, put_cb);
    if (result != PNGPARTS_API_OK)
      break;
    result = PNGPARTS_API_DONE;
    break;
  case 6: /* setup and preamble to fixed block */
    /* prepare the tables */
    result = pngparts_flate_huff_resize(&fl->length_table, 288);
    if (result != PNGPARTS_API_OK)
      break;
    result = pngparts_flate_huff_resize(&fl->distance_table, 32);
    if (result != PNGPARTS_API_OK)
      break;
    /* fill the tables with fixed information */
    pngparts_flate_fixed_lengths(&fl->length_table);
    pngparts_flate_fixed_distances(&fl->distance_table);
    /* transition to emitting data */
    state = 7;
    fl->bitlength = 0;
    fl->inscription_pos = 0;
    result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 7: /* length/literal */
    if (fl->bitlength == 0){
      unsigned short int const value =
          (fl->inscription_pos < fl->block_length)
          ? fl->inscription_text[fl->inscription_pos]
          : 256;
      struct pngparts_flate_code const bitcode = pngparts_flate_huff_index_get
            (&fl->length_table, value);
      fl->bitline = bitcode.bits;
      fl->bitlength = bitcode.length;
    }
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_code(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        unsigned short int const value =
          (fl->inscription_pos < fl->block_length)
          ? fl->inscription_text[fl->inscription_pos]
          : 256;
        if (value == 256) {
          /* done with this block */
          if (last) {
            state = 5;
          } else
            state = 0;
          /* clear the block */
          fl->inscription_pos = 0;
          fl->block_length = 0;
        } else if (value > 256) {
          struct pngparts_flate_extra const extra =
              pngparts_flate_length_decode(value);
          fl->inscription_pos += 1;
          if (extra.length_value < 0){
            result = PNGPARTS_API_BAD_BITS;
            break;
          } else if (extra.extra_bits > 0){
            /* encode now, process later */
            if (fl->inscription_pos >= fl->block_length){
              result = PNGPARTS_API_CODE_EXCESS;
              break;
            } else {
              unsigned int extended =
                fl->inscription_text[fl->inscription_pos];
              fl->bitlength = extra.extra_bits;
              fl->bitline = extended;
              state = 8;
            }
          } else {
            state = 9;
          }
        } else {
          /* come right back */
          fl->inscription_pos += 1;
          state = 7;
        }
        result = PNGPARTS_API_OK;
        break;
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 8: /* length extra */
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_integer(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        state = 9;
        fl->inscription_pos += 1;
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 9: /* distance */
    if (fl->inscription_pos >= fl->block_length){
      result = PNGPARTS_API_CODE_EXCESS;
      break;
    } else if (fl->bitlength == 0){
      unsigned short int const value =
          fl->inscription_text[fl->inscription_pos];
      struct pngparts_flate_code const bitcode = pngparts_flate_huff_index_get
            (&fl->distance_table, value);
      fl->bitline = bitcode.bits;
      fl->bitlength = bitcode.length;
    }
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_code(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        unsigned short int const value =
          fl->inscription_text[fl->inscription_pos];
        /* check next state */{
          struct pngparts_flate_extra const extra =
              pngparts_flate_distance_decode(value);
          fl->inscription_pos += 1;
          if (extra.length_value < 0){
            result = PNGPARTS_API_BAD_BITS;
            break;
          } else if (extra.extra_bits > 0){
            /* encode now, process later */
            if (fl->inscription_pos >= fl->block_length){
              result = PNGPARTS_API_CODE_EXCESS;
              break;
            } else {
              unsigned int extended =
                fl->inscription_text[fl->inscription_pos];
              fl->bitlength = extra.extra_bits;
              fl->bitline = extended;
              state = 10;
            }
          } else {
            state = 7;
          }
        }
        result = PNGPARTS_API_OK;
        break;
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 10: /* distance extra */
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_integer(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        state = 7;
        fl->inscription_pos += 1;
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 11: /* setup dynamic block */
    /* prepare the tables */
    result = pngparts_flate_huff_resize(&fl->length_table, 288);
    if (result != PNGPARTS_API_OK)
      break;
    result = pngparts_flate_huff_resize(&fl->distance_table, 32);
    if (result != PNGPARTS_API_OK)
      break;
    result = pngparts_flate_huff_resize(&fl->code_table, 19);
    if (result != PNGPARTS_API_OK)
      break;
    /* fill the tables with dynamic information */
    pngparts_flate_dynamic_codes(&fl->code_table);
    result = pngparts_deflate_compose_tables(fl);
    if (result != PNGPARTS_API_OK)
      break;
    result = pngparts_deflate_fashion_alphabet(fl);
    if (result != PNGPARTS_API_OK)
      break;
    /* transition to emitting data */
    state = 12;
    fl->bitlength = 0;
    fl->inscription_pos = 0;
    result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 12: /* emit the alphabet preamble */
    if (fl->bitlength == 0){
      unsigned short int const value =
          fl->alphabet[fl->inscription_pos];
      fl->bitline = value;
      fl->bitlength = (fl->inscription_pos==2)?4:5;
    }
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_integer(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        fl->inscription_pos += 1;
        if (fl->inscription_pos >= 3){
          state = 13;
          fl->inscription_pos = 0;
        } else state = 12;
        result = PNGPARTS_API_OK;
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 13: /* emit the proto-alphabet */
    if (fl->bitlength == 0){
      struct pngparts_flate_code const alpha_code =
        pngparts_flate_huff_index_get(&fl->code_table, fl->inscription_pos);
      fl->bitline = alpha_code.length;
      fl->bitlength = 3;
    }
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_integer(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        fl->inscription_pos += 1;
        result = PNGPARTS_API_OK;
        if (fl->inscription_pos >= fl->alphabet[2]+4){
          state = 14;
          fl->inscription_pos = 3;
          pngparts_flate_huff_value_sort(&fl->code_table);
          result = pngparts_flate_huff_generate(&fl->code_table);
          if (result != PNGPARTS_API_OK)
            break;
        } else {
          state = 13;
        }
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 14: /* encode the alphabet */
    if (fl->inscription_pos >= PNGPARTS_DEFLATE_MAXALPHABET
    ||  fl->alphabet[fl->inscription_pos] == 19)
    {
      state = 7;
      fl->inscription_pos = 0;
      result = PNGPARTS_API_LOOPED_STATE;
      break;
    }
    if (fl->bitlength == 0){
      struct pngparts_flate_code const alpha_code =
        pngparts_flate_huff_index_get(
            &fl->code_table,
            fl->alphabet[fl->inscription_pos]&31
          );
      fl->bitline = alpha_code.bits;
      fl->bitlength = alpha_code.length;
    }
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_code(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        unsigned short const core_value =
          fl->alphabet[fl->inscription_pos]&31;
        unsigned short const residual_value =
          fl->alphabet[fl->inscription_pos]>>5;
        result = PNGPARTS_API_OK;
        if (core_value == 18){
          state = 15;
          fl->bitline = residual_value;
          fl->bitlength = 7;
          break;
        } else if (core_value == 17){
          state = 15;
          fl->bitline = residual_value;
          fl->bitlength = 3;
          break;
        } else if (core_value == 16){
          state = 15;
          fl->bitline = residual_value;
          fl->bitlength = 2;
          break;
        } else {
          state = 14;
          fl->inscription_pos += 1;
        }
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  case 15: /* alphabet extra */
    while (fl->bitlength > 0){
      result = pngparts_deflate_send_integer(fl, put_data, put_cb);
      if (result != PNGPARTS_API_OK)
        break;
      else if (fl->bitlength == 0){
        state = 14;
        fl->inscription_pos += 1;
      }
    }
    if (result == PNGPARTS_API_OK)
      result = PNGPARTS_API_LOOPED_STATE;
    break;
  default:
    result = PNGPARTS_API_BAD_STATE;
    break;
  }
  fl->state = state|last;
  return result;
}

int pngparts_deflate_fashion_alphabet(struct pngparts_flate *fl){
  int result;
  int alpha_hist[19];
  unsigned int literal_maximum = 257, distance_maximum = 1;
  /* calculate HLIT and HDIST */{
    int i;
    for (i = literal_maximum; i < 286; ++i){
      struct pngparts_flate_code const literal_code =
        pngparts_flate_huff_index_get(&fl->length_table, i);
      if (literal_code.length > 0){
        literal_maximum = i+1;
      }
    }
    for (i = distance_maximum; i < 30; ++i){
      struct pngparts_flate_code const distance_code =
        pngparts_flate_huff_index_get(&fl->distance_table, i);
      if (distance_code.length > 0){
        distance_maximum = i+1;
      }
    }
    fl->alphabet[0] = literal_maximum-257;
    fl->alphabet[1] = distance_maximum-1;
  }
  /* initialize the histogram */{
    int i;
    for (i = 0; i < 19; ++i){
      alpha_hist[i] = 0;
    }
  }
  /* write the alphabet */{
    unsigned int j;
    unsigned int write_pos = 3;
    unsigned int last_length = 23;
    unsigned int repeat_count = 0;
    unsigned int total_maximum = literal_maximum+distance_maximum;
    for (j = 0; j <= total_maximum; ++j){
      struct pngparts_flate_code literal_code;
      if (j < literal_maximum){
        literal_code =
            pngparts_flate_huff_index_get(&fl->length_table, j);
      } else if (j < total_maximum){
        literal_code = pngparts_flate_huff_index_get
            (&fl->distance_table, j-literal_maximum);
      } else {
        literal_code.length = 19;
      }
      if (last_length == literal_code.length){
        if (literal_code.length > 0){
          repeat_count += 1;
          if (repeat_count == 6){
            /* emit nonzero */
            fl->alphabet[write_pos] = 16|((repeat_count-3)<<5);
            alpha_hist[16] += 1;
            repeat_count = 0;
            write_pos += 1;
          }
        } else {
          /* emit zero */
          repeat_count += 1;
          if (repeat_count == 138){
            /* emit nonzero */
            fl->alphabet[write_pos] = 18|((repeat_count-11)<<5);
            alpha_hist[18] += 1;
            repeat_count = 0;
            write_pos += 1;
          }
        }
      } else {
        if (repeat_count >= 3){
          if (last_length > 0){
            /* emit nonzero */
            fl->alphabet[write_pos] = 16|((repeat_count-3)<<5);
            alpha_hist[16] += 1;
            repeat_count = 0;
            write_pos += 1;
          } else if (repeat_count >= 11){
            /* emit zero */
            fl->alphabet[write_pos] = 18|((repeat_count-11)<<5);
            alpha_hist[18] += 1;
            repeat_count = 0;
            write_pos += 1;
          } else if (repeat_count >= 3){
            /* emit zero */
            fl->alphabet[write_pos] = 17|((repeat_count-3)<<5);
            alpha_hist[17] += 1;
            repeat_count = 0;
            write_pos += 1;
          }
        } else while (repeat_count > 0){
          repeat_count -= 1;
          fl->alphabet[write_pos] = last_length;
          alpha_hist[last_length] += 1;
          write_pos += 1;
        }
        /* repeat count now zero */
        fl->alphabet[write_pos] = literal_code.length;
        write_pos += 1;
        last_length = literal_code.length;
        if (literal_code.length < 16)
          alpha_hist[literal_code.length] += 1;
      }
    }
  }
  /* make lengths */{
    int swizzled_hist[19];
    int i;
    unsigned int alphabet_maximum = 4;
    for (i = 0; i < 19; ++i){
      struct pngparts_flate_code alphabet_code =
        pngparts_flate_huff_index_get(&fl->code_table, i);
      swizzled_hist[i] = alpha_hist[alphabet_code.value];
      if (swizzled_hist[i] > 0
      &&  i >= 4)
      {
        alphabet_maximum = i + 1;
      }
    }
    pngparts_flate_huff_make_lengths(&fl->code_table, swizzled_hist);
    result = PNGPARTS_API_OK;
    fl->alphabet[2] = alphabet_maximum-4;
  }
  return result;
}

int pngparts_deflate_send_integer
  ( struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int))
{
  int result;
  if (fl->bitlength > 0){
    result = pngparts_deflate_send_bit(fl, fl->bitline&1, put_data, put_cb);
    if (result == PNGPARTS_API_OK){
      fl->bitlength -= 1;
      fl->bitline >>= 1;
    }
  } else result = PNGPARTS_API_OK;
  return result;
}

int pngparts_deflate_send_code
  ( struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int))
{
  int result;
  if (fl->bitlength > 0){
    unsigned int const b = (fl->bitline>>(fl->bitlength-1))&1;
    result = pngparts_deflate_send_bit(fl, b, put_data, put_cb);
    if (result == PNGPARTS_API_OK){
      fl->bitlength -= 1;
    }
  } else result = PNGPARTS_API_OK;
  return result;
}

void pngparts_deflate_init(struct pngparts_flate *fl){
  fl->bitpos = 0;
  fl->last_input_byte = 0;
  fl->block_length = 0;
  fl->inscription_size = 0;
  fl->inscription_text = NULL;
  fl->history_bytes = NULL;
  fl->history_size = 0;
  fl->history_pos = 0;
  fl->alphabet = NULL;
  pngparts_flate_huff_init(&fl->code_table);
  pngparts_flate_huff_init(&fl->length_table);
  pngparts_flate_huff_init(&fl->distance_table);
  fl->block_level = PNGPARTS_FLATE_OFF;
  fl->block_type = PNGPARTS_FLATE_DYNAMIC;
  return;
}

void pngparts_deflate_free(struct pngparts_flate *fl){
  pngparts_flate_huff_free(&fl->distance_table);
  pngparts_flate_huff_free(&fl->length_table);
  pngparts_flate_huff_free(&fl->code_table);
  free(fl->history_bytes);
  fl->history_bytes = NULL;
  fl->history_size = 0;
  free(fl->inscription_text);
  fl->inscription_text = NULL;
  fl->block_length = 0;
  fl->inscription_size = 0;
  free(fl->alphabet);
  fl->alphabet = NULL;
  fl->block_level = 0;
  fl->block_type = 0;
  return;
}

void pngparts_deflate_assign_api
  (struct pngparts_api_flate *fcb, struct pngparts_flate *fl)
{
  fcb->cb_data = fl;
  fcb->start_cb = pngparts_deflate_start;
  fcb->dict_cb = pngparts_deflate_dict;
  fcb->one_cb = pngparts_deflate_one;
  fcb->finish_cb = pngparts_deflate_finish;
  return;
}

int pngparts_deflate_start
  ( void *data,
    short int fdict, short int flevel, short int cm, short int cinfo)
{
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  if (cm != 8) return PNGPARTS_API_UNSUPPORTED;
  if (cinfo > 7 || cinfo < 0) return PNGPARTS_API_UNSUPPORTED;
  else {
    unsigned int nsize = 1u<<(cinfo+8);
    unsigned short* alpha_ptr;
    unsigned short* text_ptr;
    unsigned char* ptr = (unsigned char*)malloc(nsize);
    if (ptr == NULL) return PNGPARTS_API_MEMORY;
    text_ptr = (unsigned short*)malloc(nsize*sizeof(unsigned short));
    if (text_ptr == NULL){
      free(ptr);
      return PNGPARTS_API_MEMORY;
    }
    alpha_ptr = (unsigned short*)malloc
        (PNGPARTS_DEFLATE_MAXALPHABET*sizeof(unsigned short));
    if (alpha_ptr == NULL){
      free(text_ptr);
      free(ptr);
      return PNGPARTS_API_MEMORY;
    }
    free(fl->history_bytes);
    fl->history_bytes = ptr;
    fl->history_size = (unsigned int)nsize;
    fl->history_pos = 0;
    free(fl->inscription_text);
    fl->inscription_text = text_ptr;
    fl->block_length = 0;
    fl->inscription_size = (unsigned short)nsize;
    free(fl->alphabet);
    fl->alphabet = alpha_ptr;
    fl->bitpos = 0;
    fl->last_input_byte = -1;
    fl->bitline = 0;
    fl->bitlength = 0;
    fl->state = 0;
    fl->short_pos = 0;
    fl->next_output_byte = 0;
  }
  return PNGPARTS_API_OK;
}

int pngparts_deflate_dict(void *data, int ch){
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  pngparts_flate_history_add(fl,ch);
  return PNGPARTS_API_OK;
}

int pngparts_deflate_one
  (void *data, int ch, void* put_data, int(*put_cb)(void*,int))
{
  int result = PNGPARTS_API_OK;
  int current_ch;
  struct pngparts_flate *const fl = (struct pngparts_flate *)data;
  unsigned int trouble_counter = 0;
  unsigned int const trouble_max = fl->inscription_size+341;
  int skip_back = 1;
  if (ch == -1){
    current_ch = fl->last_input_byte;
  } else current_ch = ch;
  while (result == PNGPARTS_API_OK
  &&  skip_back)
  {
    if (skip_back){
      trouble_counter += 1;
    }
    if (trouble_counter >= trouble_max){
      result = PNGPARTS_API_LOOPED_STATE;
      break;
    }
    skip_back = 0;
    switch (fl->state&PNGPARTS_DEFLATE_STATE){
    case 0: /* base */
      result = pngparts_deflate_churn_input(fl, ch);
      if (result == PNGPARTS_API_OK)
        break;
      else if (result == PNGPARTS_API_OVERFLOW){
        /* make the block information */
        fl->state = 1;
        result = PNGPARTS_API_OK;
        /* fallthrough */;
      } else break;
    default:
      result = pngparts_deflate_fashion_chunk(fl, put_data, put_cb);
      if (result == PNGPARTS_API_LOOPED_STATE){
        /* interpret looped-state as a request for skip back */
        skip_back = 1;
        result = PNGPARTS_API_OK;
      }
      break;
    }
  }
  fl->last_input_byte = ch;
  return result;
}

int pngparts_deflate_finish
  (void* data, void* put_data, int(*put_cb)(void*,int))
{
  int result = PNGPARTS_API_OK;
  int current_ch;
  struct pngparts_flate *const fl = (struct pngparts_flate *)data;
  unsigned int trouble_counter = 0;
  unsigned int const trouble_max = fl->inscription_size+341;
  int skip_back = 1;
  /* enter the last state */
  fl->state |= PNGPARTS_DEFLATE_LAST;
  /* finish the block */
  while (result == PNGPARTS_API_OK
  &&  skip_back)
  {
    int state = fl->state&PNGPARTS_DEFLATE_STATE;
    if (skip_back){
      trouble_counter += 1;
    }
    if (trouble_counter >= trouble_max){
      result = PNGPARTS_API_LOOPED_STATE;
      break;
    }
    skip_back = 0;
    switch (state){
    case 0: /* base */
      /* make the block information */
      state = 1;
      result = PNGPARTS_API_OK;
      /* fallthrough */;
    default:
      fl->state = state|PNGPARTS_DEFLATE_LAST;
      result = pngparts_deflate_fashion_chunk(fl, put_data, put_cb);
      state = fl->state&PNGPARTS_DEFLATE_STATE;
      if (result == PNGPARTS_API_LOOPED_STATE){
        /* interpret looped-state as a request for skip back */
        skip_back = 1;
        result = PNGPARTS_API_OK;
      }
      break;
    }
  }
  return result;
}
