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
  return;
}
void pngparts_pngread_free(struct pngparts_png* p) {
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
          if (memcmp(chunk_name, "\x49\x45\x4E\x44", 4) == 0) {
            /*end of PNG stream */
            if (p->chunk_size > 0) state = 2;
            else state = 3;
          } else {
            /* dummy stream */
            if (p->chunk_size > 0) state = 5;
            else state = 6;
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
          p->shortbuf[shortpos] = (unsigned char)(ch & 255);
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
    case 6: /* unknown CRC */
      {
        if (shortpos < 4 && ch >= 0) {
          p->shortbuf[shortpos] = (unsigned char)(ch & 255);
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
