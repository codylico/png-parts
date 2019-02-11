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


enum pngparts_deflate_last_block {
  PNGPARTS_DEFLATE_STATE = 31,
  PNGPARTS_DEFLATE_LAST = PNGPARTS_DEFLATE_STATE+1
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
 * Flush the bit channel.
 * - fl deflater structure holding the queue
 * - put_cb callback for putting output bytes
 * - put_data data to pass to put callback
 * @return OK if available, else OVERFLOW
 */
static int pngparts_deflate_flush_bits
  (struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int));

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

int pngparts_deflate_fashion_chunk
  (struct pngparts_flate *fl, void* put_data, int(*put_cb)(void*,int))
{
  int result = PNGPARTS_API_BAD_STATE;
  /* states:
   *  1 - block information structure
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
  default:
    result = PNGPARTS_API_BAD_STATE;
    break;
  }
  fl->state = state|last;
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

void pngparts_deflate_init(struct pngparts_flate *fl){
  fl->bitpos = 0;
  fl->last_input_byte = 0;
  fl->block_length = 0;
  fl->inscription_size = 0;
  fl->inscription_text = NULL;
  fl->history_bytes = NULL;
  fl->history_size = 0;
  fl->history_pos = 0;
  pngparts_flate_huff_init(&fl->code_table);
  pngparts_flate_huff_init(&fl->length_table);
  pngparts_flate_huff_init(&fl->distance_table);
  fl->block_level = PNGPARTS_FLATE_OFF;
  fl->block_type = PNGPARTS_FLATE_PLAIN;
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
    unsigned short* text_ptr;
    unsigned char* ptr = (unsigned char*)malloc(nsize);
    if (ptr == NULL) return PNGPARTS_API_MEMORY;
    text_ptr = (unsigned short*)malloc(nsize*sizeof(unsigned short));
    if (text_ptr == NULL){
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
  unsigned int const trouble_max = fl->inscription_size+36;
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
      result = pngparts_deflate_churn_input(fl, current_ch);
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
  unsigned int const trouble_max = fl->inscription_size;
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
