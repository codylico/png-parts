
#include "pngread.h"
#include <string.h>

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
       * 4  - done
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
