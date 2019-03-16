/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * pngwrite.c
 * PNG writer source
 */
#include "pngwrite.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>



enum pngparts_pngwrite_flags {
  PNGPARTS_PNGWRITE_IHDR_DONE = 8
};

static void pngparts_pngwrite_put32(unsigned char* b, unsigned long int w);
static int pngparts_pngwrite_signal_finish(struct pngparts_png* p);

void pngparts_pngwrite_put32(unsigned char* b, unsigned long int w) {
  b[0] = (w>>24)&255;
  b[1] = (w>>16)&255;
  b[2] = (w>> 8)&255;
  b[3] = (w>> 0)&255;
  return;
}

int pngparts_pngwrite_signal_finish(struct pngparts_png* w){
  struct pngparts_png_message message;
  message.type = PNGPARTS_PNG_M_FINISH;
  message.byte = PNGPARTS_API_OK;
  memcpy(message.name, w->active_chunk_cb->name, 4);
  message.ptr = NULL;
  return pngparts_png_send_chunk_msg(w, w->active_chunk_cb, &message);
}

void pngparts_pngwrite_init(struct pngparts_png* w){
  w->state = 0;
  w->check = pngparts_png_crc32_new();
  w->shortpos = 0;
  w->last_result = 0;
  w->flags_tf = 0;
  w->chunk_cbs = NULL;
  w->active_chunk_cb = NULL;
  w->palette_count = 0;
  w->palette = NULL;
  return;
}

void pngparts_pngwrite_free(struct pngparts_png* w){
  pngparts_png_drop_chunk_cbs(w);
  free(w->palette);
  w->palette = NULL;
  return;
}

int pngparts_pngwrite_generate(struct pngparts_png* w){
  int result = w->last_result;
  int state = w->state;
  int shortpos = w->shortpos;
  while (result == PNGPARTS_API_OK
    &&  (w->pos < w->size))
  {
      /* states:
       * 0  - start
       * 1  - length, header
       * 2  - IEND contents
       * 3  - IEND CRC
       * 4  - done
       * 5  - unknown chunk contents
       * 6  - unknown chunk CRC
       * 7  - IHDR handling
       */
    int ch;
    switch (state){
    case 0: /* start */
      if (shortpos < 8){
        ch = pngparts_png_signature()[shortpos];
        shortpos += 1;
      }
      if (shortpos >= 8){
        state = 1;
        shortpos = 0;
      }
      break;
    case 1: /* length, header */
      if (shortpos == 0){
        unsigned char* const chunk_name = w->shortbuf+4;
        w->check = pngparts_png_crc32_new();
        if (w->flags_tf & PNGPARTS_PNGWRITE_IHDR_DONE){
          /* look for the next chunk to do */
          int i;
          struct pngparts_png_chunk_cb const* next_cb =
            pngparts_png_find_ready_cb(w);
          w->active_chunk_cb = NULL;
          if (next_cb != NULL){
            /* prepare for the next chunk */
            memcpy(chunk_name, next_cb->name, 4);
            /* query the chunk size */{
              unsigned long int next_size = 0;
              struct pngparts_png_message message;
              int size_result;
              message.type = PNGPARTS_PNG_M_START;
              message.byte = 1;/*write mode */
              memcpy(message.name, chunk_name, 4);
              message.ptr = &next_size;
              size_result = pngparts_png_send_chunk_msg(w, next_cb, &message);
              if (size_result < 0){
                result = size_result;
                break;
              } else if (size_result == PNGPARTS_API_DONE){
                result = PNGPARTS_API_EOF;
                break;
              } else if (next_size > 0x7fFFffFF){
                result = PNGPARTS_API_CHUNK_TOO_LONG;
                break;
              } else {
                w->chunk_size = next_size;
                w->active_chunk_cb = next_cb;
              }
            }
          } else {
            /* prepare for the IEND */
            memcpy(chunk_name, "\x49\x45\x4E\x44", 4);
            w->chunk_size = 0;
          }
          /* perform common chunk operations */
          for (i = 0; i < 4; ++i){
            w->check = pngparts_png_crc32_accum(w->check, chunk_name[i]);
          }
          pngparts_pngwrite_put32(w->shortbuf, w->chunk_size);
        } else {
          int i;
          /* do the header */
          pngparts_pngwrite_put32(w->shortbuf, 13);
          memcpy(chunk_name, "\x49\x48\x44\x52", 4);
          for (i = 0; i < 4; ++i){
            w->check = pngparts_png_crc32_accum(w->check, chunk_name[i]);
          }
        }
      }
      if (shortpos < 8){
        ch = w->shortbuf[shortpos];
        shortpos += 1;
        if (shortpos >= 8){
          if (w->flags_tf & PNGPARTS_PNGWRITE_IHDR_DONE){
            if (w->active_chunk_cb != NULL){
              /* do the next chunk callback */
              if (w->chunk_size > 0){
                state = 5;
              } else {
                /* signal end of chunk */
                int const finish_result = pngparts_pngwrite_signal_finish(w);
                if (finish_result < 0){
                  result = finish_result;
                  break;
                }
                state = 6;
              }
            } else {
              /* do the IEND */
              if (w->chunk_size > 0){
                state = 2;
              } else state = 3;
            }
            shortpos = 0;
          } else {
            /* do the header */
            shortpos = 0;
            state = 7;
            w->chunk_size = 13;
          }
        }
      }
      break;
    case 2: /* IEND contents */
    case 5: /* unknown contents */
      if (w->chunk_size > 0){
        if (w->active_chunk_cb != NULL){
          /* put the message */
          int put_result;
          struct pngparts_png_message message;
          message.type = PNGPARTS_PNG_M_PUT;
          message.byte = -1;/* byte to receive */
          memcpy(message.name, w->active_chunk_cb->name, 4);
          message.ptr = NULL;
          put_result =
            pngparts_png_send_chunk_msg(w, w->active_chunk_cb, &message);
          if (put_result < 0){
            result = put_result;
            break;
          } else if (put_result == PNGPARTS_API_DONE){
            result = PNGPARTS_API_EOF;
            break;
          } else if (message.byte < 0 || message.byte > 255){
            result = PNGPARTS_API_MISSING_PUT;
            break;
          } else {
            ch = message.byte;
          }
        } else {
          ch = 0;
        }
        w->check = pngparts_png_crc32_accum(w->check, ch);
        w->chunk_size -= 1;
      }
      if (w->chunk_size == 0){
        if (w->active_chunk_cb != NULL){
          int const finish_result = pngparts_pngwrite_signal_finish(w);
          if (finish_result < 0){
            result = finish_result;
            break;
          }
        }
        state += 1;
        shortpos = 0;
      }
      break;
    case 3: /* IEND CRC */
    case 6: /* unknown CRC */
      if (shortpos == 0){
        unsigned long int const final_check =
          pngparts_png_crc32_tol(w->check);
        pngparts_pngwrite_put32(w->shortbuf+0, final_check);
      }
      if (shortpos < 4){
        ch = w->shortbuf[shortpos];
        shortpos += 1;
      }
      if (shortpos >= 4){
        shortpos = 0;
        if (state == 3){
          /* notify end of file */ {
            struct pngparts_png_message message;
            int broadcast_feedback;
            message.byte = 0;
            message.ptr = NULL;
            message.type = PNGPARTS_PNG_M_ALL_DONE;
            broadcast_feedback =
              pngparts_png_broadcast_chunk_msg(w, &message);
            if (broadcast_feedback < 0)
              result = broadcast_feedback;
          }
          state = 4; /* done */
        } else
          state = 1; /* continue */
      }
      break;
    case 4:
      result = PNGPARTS_API_DONE;
      break;
    case 7: /* IHDR */
      if (w->chunk_size == 13){
        struct pngparts_png_header header;
        int i;
        /* default-fill the header */{
          header.width = 0;
          header.height = 0;
          header.bit_depth = 1;
          header.color_type = 0;
          header.compression = 0;
          header.filter = 0;
          header.interlace = 0;
        }
        /* get a better description */if (w->img_cb.describe_cb != NULL){
          (*w->img_cb.describe_cb)(
              w->img_cb.cb_data,
              &header.width, &header.height,
              &header.bit_depth, &header.color_type, &header.compression,
              &header.filter, &header.interlace
            );
        }
        /* verify the header */
        if (header.width < 0
        ||  header.height < 0
        ||  !pngparts_png_header_is_valid(header))
        {
          result = PNGPARTS_API_BAD_HDR;
          break;
        }
        /* record the header */
        w->header = header;
        /* compose the header */
        pngparts_pngwrite_put32(w->shortbuf+0, (unsigned long)header.width);
        pngparts_pngwrite_put32(w->shortbuf+4, (unsigned long)header.height);
        w->shortbuf[8] = (unsigned char)header.bit_depth;
        w->shortbuf[9] = (unsigned char)header.color_type;
        w->shortbuf[10]= (unsigned char)header.compression;
        w->shortbuf[11]= (unsigned char)header.filter;
        w->shortbuf[12]= (unsigned char)header.interlace;
        /* advance compute the CRC */
        for (i = 0; i < 13; ++i){
          w->check = pngparts_png_crc32_accum(w->check, w->shortbuf[i]);
        }
      }
      if (w->chunk_size > 13){
        ch = 255;
        w->chunk_size -= 1;
      } else if (w->chunk_size > 0){
        ch = w->shortbuf[13-w->chunk_size];
        w->chunk_size -= 1;
      }
      if (w->chunk_size == 0){
        state = 6;
        shortpos = 0;
        w->flags_tf |= PNGPARTS_PNGWRITE_IHDR_DONE;
      }
      break;
    }
    if (result != PNGPARTS_API_OK)
      break;
    else /* put actual character */ {
      w->buf[w->pos] = (unsigned char)(ch & 255);
      w->pos += 1;
    }
  }
  w->last_result = result;
  w->state = (short)state;
  w->shortpos = (short)shortpos;
  if (result) {
    w->flags_tf |= 2;
  }
  return result;
}
