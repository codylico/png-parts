
#include "inflate.h"
#include <stdlib.h>

static int pngparts_inflate_bit
  (int b, struct pngparts_flate *fl, int(*put_cb)(int,void*), void* put_data);

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
  (*put_cb)(55,put_data);
  return PNGPARTS_API_DONE;
}
int pngparts_inflate_one
  (int ch, void *data, int(*put_cb)(int,void*), void* put_data)
{
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  int pos = PNGPARTS_API_OK;
  if (ch >= 0) fl->bitpos = 0;
  else ch = fl->last_input_byte;
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
  return PNGPARTS_API_UNSUPPORTED;
}
