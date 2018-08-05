
#include "z.h"

int pngparts_z_header_check(struct pngparts_z_header hdr){
  int holding = 0;
  holding |= (hdr.fdict&1)<<5;
  holding |= (hdr.flevel&3)<<6;
  holding |= (hdr.cm&15)<<8;
  holding |= (hdr.cinfo&15)<<12;
  return 31-(holding%31);
}
struct pngparts_z_header pngparts_z_header_new(void){
  struct pngparts_z_header out;
  out.fdict = 0;
  out.flevel = 3;
  out.cm = 8;
  out.cinfo = 7;
  out.fcheck = 26;
  return out;
}
void pngparts_z_header_put(void* buf, struct pngparts_z_header  hdr){
  unsigned char* ptr = (unsigned char*)buf;
  ptr[0] = (hdr.cm&15)<<0
         | (hdr.cinfo&15)<<4;
  ptr[1] = (hdr.fcheck&31)<<0
         | (hdr.fdict&1)<<5
         | (hdr.flevel&3)<<6;
  return;
}
struct pngparts_z_header pngparts_z_header_get(void const* buf){
  unsigned char const* ptr = (void const*)buf;
  struct pngparts_z_header hdr;
  hdr.cm = (ptr[0]&15);
  hdr.cinfo = ((ptr[0]>>4)&15);
  hdr.fcheck = (ptr[1]&31);
  hdr.fdict = ((ptr[1]>>5)&1);
  hdr.flevel = ((ptr[1]>>6)&3);
  return hdr;
}
