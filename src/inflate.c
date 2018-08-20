
#include "inflate.h"
#include <stdlib.h>

static int pngparts_inflate_bit
  (int b, struct pngparts_flate *fl, int(*put_cb)(int,void*), void* put_data);
enum pngparts_inflate_last_block {
  PNGPARTS_INFLATE_STATE = 15,
  PNGPARTS_INFLATE_LAST = PNGPARTS_INFLATE_STATE+1
};

void pngparts_inflate_init(struct pngparts_flate *fl){
  fl->bitpos = 0;
  fl->last_input_byte = 0;
  fl->history_bytes = NULL;
  fl->history_size = 0;
  fl->history_pos = 0;
  return;
}
void pngparts_inflate_free(struct pngparts_flate *fl){
  free(fl->history_bytes);
  fl->history_bytes = NULL;
  fl->history_size = 0;
  return;
}
int pngparts_inflate_start
  ( short int fdict, short int flevel, short int cm, short int cinfo,
    void* data)
{
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  if (cm != 8) return PNGPARTS_API_UNSUPPORTED;
  if (cinfo > 7) return PNGPARTS_API_UNSUPPORTED;
  {
    unsigned int nsize = 1u<<(flevel+8);
    unsigned char* ptr = (unsigned char*)malloc(nsize);
    if (ptr == NULL) return PNGPARTS_API_MEMORY;
    free(fl->history_bytes);
    fl->history_bytes = ptr;
    fl->history_size = (unsigned int)nsize;
    fl->history_pos = 0;
    fl->bitpos = 0;
    fl->last_input_byte = -1;
    fl->bitline = 0;
    fl->bitlength = 0;
    fl->state = 0;
  }
  return PNGPARTS_API_OK;
}
int pngparts_inflate_dict(int ch, void* data){
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  return PNGPARTS_API_UNSUPPORTED;
}
int pngparts_inflate_bit
  (int b, struct pngparts_flate *fl, int(*put_cb)(int,void*), void* put_data)
{
  int result = PNGPARTS_API_OK;
  int bit = (b&255)?1:0;
  int repeat_tf = b&256;
  int state = fl->state&PNGPARTS_INFLATE_STATE;
  int last_block = fl->state&PNGPARTS_INFLATE_LAST;
  switch (state){
  case 0: /*header*/
    {
      if (fl->bitlength < 3){
        fl->bitline |= (bit<<fl->bitlength);
        fl->bitlength += 1;
      }
      if (fl->bitlength >= 3){
        /* final block? */
        if ((fl->bitline&1) == 1){
          last_block = PNGPARTS_INFLATE_LAST;
        }
        switch (fl->bitline&6){
        case 0: /* direct */
          {
            if (fl->bitpos == 7){
              state = 2;
              fl->short_pos = 0;
            } else state = 1;/* wait until end of block */
          }break;
        case 2: /* text Huffman */
          {
            state = 6;
          }break;
        case 4: /* custom codes */
          {
            state = 7;
          }break;
        default:
          state = 5;
          result = PNGPARTS_API_BAD_BLOCK;
          break;
        }
      }
    }break;
  case 1: /* direct patience */
    {
      /* do nothing */
      if (fl->bitpos == 7){
        state = 2;
        fl->short_pos = 0;
      }
    }break;
  case 2: /* direct length */
    break;
  case 3: /* direct characters */
    break;
  case 4:
    result = PNGPARTS_API_DONE;
    break;
  default:
    result = PNGPARTS_API_BAD_STATE;
    break;
  }
  if (state == 0 && last_block != 0){
    result = PNGPARTS_API_DONE;
    state = 4;
  }
  fl->state = state|last_block;
  return result;
}
int pngparts_inflate_one
  (int ch, void *data, int(*put_cb)(int,void*), void* put_data)
{
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  int pos = PNGPARTS_API_OK;
  int state = fl->state&PNGPARTS_INFLATE_STATE;
  if (state >= 2 && state <= 3){
    int last_block = fl->state&PNGPARTS_INFLATE_LAST;
    if (ch < 0){
      ch = fl->last_input_byte;
    }
    /* direct bytes mode */
    switch (state){
    case 2: /* forward and reverse lengths */
      {
        if (fl->short_pos < 2){
          fl->shortbuf[fl->short_pos++] = ((unsigned char)(ch&255));
        } else if (fl->short_pos < 4){
          fl->shortbuf[fl->short_pos++] = ((unsigned char)((~ch)&255));
        }
        if (fl->short_pos >= 4){
          if (fl->shortbuf[0] != fl->shortbuf[2]
          ||  fl->shortbuf[1] != fl->shortbuf[3])
          {
            pos = PNGPARTS_API_CORRUPT_LENGTH;
            break;
          } else {
            fl->block_length =
              fl->shortbuf[0]|((unsigned int)fl->shortbuf[1]<<8);
            if (fl->block_length > 0)
              state = 3;
            else if (last_block) {
              state = 4;
              pos = PNGPARTS_API_DONE;
            } else
              state = 0;
          }
        }
      }break;
    case 3:
      {
        if (fl->block_length > 0){
          pos = (*put_cb)(ch,put_data);
          if (pos != PNGPARTS_API_OK){
            break;
          } else fl->block_length -= 1;
        }
        if (fl->block_length == 0){
          if (last_block) {
            state = 4;
            pos = PNGPARTS_API_DONE;
          } else
            state = 0;
        }
      }break;
    }
    fl->state = state|last_block;
    if (pos != PNGPARTS_API_OK) fl->last_input_byte = ch;
    return pos;
  } else if (ch >= 0)
    fl->bitpos = 0;
  else {
    ch = fl->last_input_byte;
    pos = pngparts_inflate_bit((ch&(1<<fl->bitpos))|256,fl,put_cb,put_data);
    if (pos != 0) return pos;
    else fl->bitpos += 1;
  }
  for (; fl->bitpos < 8; ++fl->bitpos){
    pos = pngparts_inflate_bit(ch&(1<<fl->bitpos),fl,put_cb,put_data);
    if (pos != 0) break;
  }
  if (fl->bitpos < 8){
    fl->last_input_byte = ch;
  }
  return pos;
}
int pngparts_inflate_finish(void* data){
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  if ((fl->state & PNGPARTS_INFLATE_STATE) != 4)
    return PNGPARTS_API_EOF;
  else
    return PNGPARTS_API_DONE;
}
