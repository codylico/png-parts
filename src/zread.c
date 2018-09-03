/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * zread.c
 * zlib reader source
 */
#include "zread.h"
#include <stdlib.h>
#include <string.h>

static int pngparts_zread_put_cb(int ch, void* prs);
static unsigned long int pngparts_zread_get32(unsigned char const* );

unsigned long int pngparts_zread_get32(unsigned char const* b){
  return (((unsigned long int)(b[0]&255))<<24)
      |  (((unsigned long int)(b[1]&255))<<16)
      |  (((unsigned long int)(b[2]&255))<< 8)
      |  (((unsigned long int)(b[3]&255))<< 0);
}

void pngparts_zread_init(struct pngparts_z *prs){
  prs->state = 0;
  prs->header = pngparts_z_header_new();
  prs->shortpos = 0;
  memset(prs->shortbuf,0,sizeof(prs->shortbuf));
  prs->check = pngparts_z_adler32_new();
  prs->inbuf = NULL;
  prs->inpos = 0;
  prs->insize = 0;
  prs->outbuf = NULL;
  prs->outpos = 0;
  prs->outsize = 0;
  prs->flags_tf = 0;
  prs->cb_data = NULL;
  prs->start_cb = NULL;
  prs->one_cb = NULL;
  prs->finish_cb = NULL;
  prs->last_result = 0;
  return;
}
void pngparts_zread_free(struct pngparts_z *prs){
  return;
}
int pngparts_zread_parse(struct pngparts_z *prs, int mode){
  int result = prs->last_result;
  int state = prs->state;
  int shortpos = prs->shortpos;
  int sticky_finish = ((mode&PNGPARTS_ZREAD_FINISH) != 0);
  if (result == PNGPARTS_API_OVERFLOW){
    if (prs->outpos < prs->outsize)
      result = PNGPARTS_API_OK;
  }
  while (result == PNGPARTS_API_OK
  &&     (sticky_finish || prs->inpos < prs->insize)){
    /* states:
     * 0  - start
     * 1  - dictionary checksum
     * 2  - data
     * 3  - adler32 checksum
     * 4  - done
     */
    int ch;
    if (prs->flags_tf&2) {
      /* put dummy character */
      ch = -1;
    } else if (prs->inpos < prs->insize)
      /* put actual character */ ch = prs->inbuf[prs->inpos]&255;
    else
      /* put dummy character */ ch = -1;
    switch (state){
    case 0:
      {
        if (ch >= 0 && shortpos < 2){
          prs->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 2){
          prs->header = pngparts_z_header_get(prs->shortbuf);
          if (prs->header.fcheck%31 != pngparts_z_header_check(prs->header)%31){
            result = PNGPARTS_API_BAD_CHECK;
            break;
          } else if (prs->start_cb == NULL
          ||  (*prs->start_cb)(prs->header.fdict, prs->header.flevel,
                prs->header.cm, prs->header.cinfo,prs->cb_data) != 0)
          {
            result = PNGPARTS_API_UNSUPPORTED;
          } else if (prs->header.fdict){
            state = 1;
            shortpos = 0;
          } else {
            state = 2;
            shortpos = 0;
          }
        }
      }break;
    case 1: /*dictionary id */
      {
        if (ch >= 0 && shortpos < 4){
          prs->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 4){
          /* check the dictionary */
          unsigned long int dict_chk
            = pngparts_zread_get32(prs->shortbuf);
          if ((prs->flags_tf&1) == 0){
            result = PNGPARTS_API_NEED_DICT;
            break;
          } else if (prs->dict_check != dict_chk){
            result = PNGPARTS_API_NEED_DICT;
            break;
          } else {
            shortpos = 0;
            state = 2;
          }
        }
      }break;
    case 2: /*data processing callback */
      {
        result = (*prs->one_cb)(ch,prs->cb_data,
              &pngparts_zread_put_cb,prs);
        if (result == PNGPARTS_API_DONE){
          state = 3;
          shortpos = 0;
          result = PNGPARTS_API_OK;
        }
      }break;
    case 3: /* checksum */
      {
        if (ch >= 0 && shortpos < 4){
          prs->shortbuf[shortpos] = (unsigned char)ch;
          shortpos += 1;
        }
        if (shortpos >= 4){
          /* check the sum */
          unsigned long int stream_chk
            = pngparts_zread_get32(prs->shortbuf);
          if (pngparts_z_adler32_tol(prs->check) != stream_chk){
            result = PNGPARTS_API_BAD_SUM;
          } else {
            result = (*prs->finish_cb)(prs->cb_data);
            if (result >= PNGPARTS_API_OK){
              shortpos = 0;
              result = PNGPARTS_API_DONE;
              if (ch >= 0) prs->inpos += 1;/*not else */
              state = 4;
            }
          }
        }
      }break;
    case 4:
      {
        result = PNGPARTS_API_DONE;
      }break;
    default:
      result = PNGPARTS_API_BAD_STATE;
      break;
    }
    if (result != PNGPARTS_API_OK){
      break;
    } else if (prs->flags_tf & 2){
      /* reset the flag */
      prs->flags_tf &= ~2;
      prs->inpos += 1;
    } else if (ch >= 0){
      prs->inpos += 1;
    }
  }
  prs->last_result = result;
  prs->state = (short)state;
  prs->shortpos = (short)shortpos;
  if (result){
    prs->flags_tf |= 2;
  }
  return result;
}
int pngparts_zread_put_cb(int ch, void* data){
  struct pngparts_z* prs = (struct pngparts_z*)data;
  if (prs->outpos < prs->outsize){
    unsigned char chc = (unsigned char)(ch&255);
    prs->outbuf[prs->outpos++] = chc;
    prs->check = pngparts_z_adler32_accum(prs->check, chc);
    return PNGPARTS_API_OK;
  } else {
    return PNGPARTS_API_OVERFLOW;
  }
}
