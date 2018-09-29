/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * pngread.c
 * PNG reader source
 */

#include "pngread.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum pngparts_pngread_flags {
  PNGPARTS_PNGREAD_IHDR_FOUND = 8
};

static unsigned long int pngparts_pngread_get32(unsigned char const*);

unsigned long int pngparts_pngread_get32(unsigned char const* b) {
  return (((unsigned long int)(b[0] & 255)) << 24)
    | (((unsigned long int)(b[1] & 255)) << 16)
    | (((unsigned long int)(b[2] & 255)) << 8)
    | (((unsigned long int)(b[3] & 255)) << 0);
}


void pngparts_pngread_init(struct pngparts_png* p) {
  p->state = 0;
  p->check = pngparts_png_crc32_new();
  p->shortpos = 0;
  p->last_result = 0;
  p->flags_tf = 0;
  p->chunk_cbs = NULL;
  p->active_chunk_cb = NULL;
  return;
}
void pngparts_pngread_free(struct pngparts_png* p) {
  pngparts_png_drop_chunk_cbs(p);
  return;
}
int pngparts_pngread_parse(struct pngparts_png* p) {
  int result = p->last_result;
  int state = p->state;
  int shortpos = p->shortpos;
  while (result == PNGPARTS_API_OK
    &&  (p->pos < p->size))
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
    if (p->flags_tf & 2) {
      /* put dummy character */
      ch = -1;
    } else if (p->pos < p->size)
      /* put actual character */ ch = p->buf[p->pos] & 255;
    else
      /* put dummy character */ ch = -1;
    switch (state) {
    case 0: /* signature */
      {
        if (ch >= 0 && shortpos < 8) {
          p->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 8) {
          int cmp_result = memcmp
            ( p->shortbuf, pngparts_png_signature(),
              8 * sizeof(unsigned char));
          if (cmp_result != 0){
            result = PNGPARTS_API_BAD_SIGNATURE;
            break;
          } else {
            state = 1;
            shortpos = 0;
          }
        }
      }break;
    case 1: /* length, header */
      {
        if (ch >= 0 && shortpos < 8) {
          p->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 8) {
          unsigned char chunk_name[4];
          /* chunk size */
          p->chunk_size = pngparts_pngread_get32(p->shortbuf);
          memcpy(chunk_name, p->shortbuf + 4, 4 * sizeof(unsigned char));
          /* prepare the crc */
          p->check = pngparts_png_crc32_new();
          {
            int i;
            for (i = 0; i < 4; ++i) {
              p->check = pngparts_png_crc32_accum(p->check, chunk_name[i]);
            }
          }
          if ((p->flags_tf & PNGPARTS_PNGREAD_IHDR_FOUND) == 0) {
            if (memcmp(chunk_name, "\x49\x48\x44\x52", 4) == 0) {
              if (p->chunk_size == 13) {
                state = 7;
                shortpos = 0;
                p->flags_tf |= PNGPARTS_PNGREAD_IHDR_FOUND;
              } else result = PNGPARTS_API_BAD_HDR;
            } else {
              result = PNGPARTS_API_MISSING_HDR;
            }
          } else if (memcmp(chunk_name, "\x49\x45\x4E\x44", 4) == 0) {
            /*end of PNG stream */
            if (p->chunk_size > 0) state = 2;
            else state = 3;
          } else {
            /* try to find the matching chunk callback */
            struct pngparts_png_chunk_cb const*
              link = pngparts_png_find_chunk_cb(p, chunk_name);
            if (link != NULL) {
              struct pngparts_png_message message;
              message.byte = 0;
              memcpy(message.name, chunk_name, 4 * sizeof(unsigned char));
              message.ptr = NULL;
              message.type = PNGPARTS_PNG_M_START;
              result = pngparts_png_send_chunk_msg(p, link, &message);
              if (result == PNGPARTS_API_OK) {
                p->active_chunk_cb = link;
                if (p->chunk_size > 0) state = 8;
                else state = 9;
                shortpos = 0;
              } else /* result is set, */break;
            } else if ((chunk_name[0] & 0x20) == 0) {
              result = PNGPARTS_API_UNCAUGHT_CRITICAL;
            } else {
              /* dummy stream */
              if (p->chunk_size > 0) state = 5;
              else state = 6;
            }
          }
          shortpos = 0;
        }
      }break;
    case 2: /* IEND chunk data */
      {
        if (p->chunk_size > 0 && ch >= 0) {
          p->check = pngparts_png_crc32_accum(p->check, ch);
          p->chunk_size -= 1;
        }
        if (p->chunk_size == 0) {
          state = 3;
        }
      }break;
    case 3: /* IEND CRC */
      {
        if (shortpos < 4 && ch >= 0) {
          p->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 4) {
          /* check the sum */
          unsigned long int stream_chk
            = pngparts_pngread_get32(p->shortbuf);
          if (pngparts_png_crc32_tol(p->check) != stream_chk) {
            result = PNGPARTS_API_BAD_SUM;
          } else {
            /* notify done */
            shortpos = 0;
            result = PNGPARTS_API_DONE;
            if (ch >= 0) p->pos += 1;/*not else */
            state = 4;
          }
        }
      }break;
    case 4:
      {
        result = PNGPARTS_API_DONE;
      }break;
    case 5: /* unknown chunk data */
      {
        if (p->chunk_size > 0 && ch >= 0) {
          p->check = pngparts_png_crc32_accum(p->check, ch);
          p->chunk_size -= 1;
        }
        if (p->chunk_size == 0) {
          state = 6;
        }
      }break;
    case 6: /* other CRC */
      {
        if (shortpos < 4 && ch >= 0) {
          p->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 4) {
          /* check the sum */
          unsigned long int stream_chk
            = pngparts_pngread_get32(p->shortbuf);
          if (pngparts_png_crc32_tol(p->check) != stream_chk) {
            result = PNGPARTS_API_BAD_SUM;
          } else {
            /* notify done */
            shortpos = 0;
            result = PNGPARTS_API_OK;
            state = 1;
          }
        }
      }break;
    case 7: /* IHDR handling */
      {
        if (shortpos < 13 && ch >= 0) {
          p->check = pngparts_png_crc32_accum(p->check, ch);
          p->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 13) {
          /* extract the header */
          p->header.width = pngparts_pngread_get32(p->shortbuf + 0);
          p->header.height = pngparts_pngread_get32(p->shortbuf + 4);
          p->header.bit_depth = (int)*(p->shortbuf + 8);
          p->header.color_type = (int)*(p->shortbuf + 9);
          p->header.compression = (int)*(p->shortbuf + 10);
          p->header.filter = (int)*(p->shortbuf + 11);
          p->header.interlace = (int)*(p->shortbuf + 12);
          /* report the header */
          if (p->img_cb.start_cb != NULL) {
            result = (*p->img_cb.start_cb)(p->img_cb.cb_data,
              p->header.width, p->header.height,
              p->header.bit_depth, p->header.color_type,
              p->header.compression, p->header.filter,
              p->header.interlace);
            if (result != PNGPARTS_API_OK) break;
          }
          /* check the CRC32 */
          state = 6;
          shortpos = 0;
        }
      }break;
    case 8: /* callback chunk data */
      {
        if (p->chunk_size > 0 && ch >= 0) {
          unsigned long int const chunk_size = p->chunk_size;
          struct pngparts_png_crc32 const check = p->check;
          /* notify */ {
            struct pngparts_png_message message;
            message.byte = ch;
            memcpy(message.name, p->active_chunk_cb->name,
              4 * sizeof(unsigned char));
            message.ptr = NULL;
            message.type = PNGPARTS_PNG_M_GET;
            result = pngparts_png_send_chunk_msg
              (p, p->active_chunk_cb, &message);
          }
          p->check = pngparts_png_crc32_accum(check, ch);
          p->chunk_size = chunk_size-1;
        }
        if (p->chunk_size == 0) {
          state = 9;
          shortpos = 0;
        }
      }break;
    case 9: /* callback CRC */
      {
        if (shortpos < 4 && ch >= 0) {
          p->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 4) {
          /* check the sum */
          unsigned long int stream_chk
            = pngparts_pngread_get32(p->shortbuf);
          if (pngparts_png_crc32_tol(p->check) != stream_chk) {
            /* notify */ {
              struct pngparts_png_message message;
              message.byte = PNGPARTS_API_BAD_SUM;
              memcpy(message.name, p->active_chunk_cb->name,
                4 * sizeof(unsigned char));
              message.ptr = NULL;
              message.type = PNGPARTS_PNG_M_FINISH;
              result = pngparts_png_send_chunk_msg
                (p, p->active_chunk_cb, &message);
            }
          } else {
            /* notify */ {
              struct pngparts_png_message message;
              message.byte = PNGPARTS_API_OK;
              memcpy(message.name, p->active_chunk_cb->name,
                4 * sizeof(unsigned char));
              message.ptr = NULL;
              message.type = PNGPARTS_PNG_M_FINISH;
              result = pngparts_png_send_chunk_msg
                (p, p->active_chunk_cb, &message);
            }
            shortpos = 0;
            result = PNGPARTS_API_OK;
            state = 1;
          }
        }
      }break;
    default:
      {
        result = PNGPARTS_API_BAD_STATE;
      }break;
    }
    p->state = state;
    p->shortpos = shortpos;
    if (result != PNGPARTS_API_OK) {
      break;
    } else if (p->flags_tf & 2) {
      /* reset the flag */
      p->flags_tf &= ~2;
      p->pos += 1;
    } else if (ch >= 0) {
      p->pos += 1;
    }
  }
  p->last_result = result;
  p->state = (short)state;
  p->shortpos = (short)shortpos;
  if (result) {
    p->flags_tf |= 2;
  }
  return result;
}
