
#include "deflate.h"
#include <stdlib.h>

void pngparts_deflate_init(struct pngparts_flate *fl){
  fl->bitpos = 0;
  fl->last_input_byte = 0;
  fl->history_bytes = NULL;
  fl->history_size = 0;
  fl->history_pos = 0;
  pngparts_flate_huff_init(&fl->code_table);
  pngparts_flate_huff_init(&fl->length_table);
  pngparts_flate_huff_init(&fl->distance_table);
  return;
}

void pngparts_deflate_free(struct pngparts_flate *fl){
  pngparts_flate_huff_free(&fl->distance_table);
  pngparts_flate_huff_free(&fl->length_table);
  pngparts_flate_huff_free(&fl->code_table);
  free(fl->history_bytes);
  fl->history_bytes = NULL;
  fl->history_size = 0;
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
  if (cinfo > 7) return PNGPARTS_API_UNSUPPORTED;
  else {
    unsigned int nsize = 1u<<(cinfo+8);
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

int pngparts_deflate_dict(void *data, int ch){
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  pngparts_flate_history_add(fl,ch);
  return PNGPARTS_API_OK;
}

int pngparts_deflate_one
  (void *data, int ch, void* put_data, int(*put_cb)(void*,int))
{
  int result;
  int current_ch;
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  if (ch == -1){
    current_ch = fl->last_input_byte;
  } else current_ch = ch;
  /* TODO actual DEFLATE implementation */
  result = (*put_cb)(put_data, current_ch);
  fl->last_input_byte = ch;
  return result;
}

int pngparts_deflate_finish
  (void* fl, void* put_data, int(*put_cb)(void*,int))
{
  return PNGPARTS_API_OK;
}
