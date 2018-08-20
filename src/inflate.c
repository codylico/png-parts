
#include "inflate.h"

void pngparts_inflate_init(struct pngparts_flate *fl){
  fl->bitpos = 0;
  fl->last_input_byte = 0;
  return;
}
void pngparts_inflate_free(struct pngparts_flate *fl){
  return;
}
int pngparts_inflate_start
  ( short int fdict, short int flevel, short int cm, short int cinfo,
    void* data)
{
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  return PNGPARTS_API_UNSUPPORTED;
}
int pngparts_inflate_dict(int ch, void* data){
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  return PNGPARTS_API_UNSUPPORTED;
}
int pngparts_inflate_one
  (int ch, void *data, int(*put_cb)(int,void*), void* put_data)
{
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  return PNGPARTS_API_UNSUPPORTED;
}
int pngparts_inflate_finish(void* data){
  struct pngparts_flate *fl = (struct pngparts_flate *)data;
  return PNGPARTS_API_UNSUPPORTED;
}
