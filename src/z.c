
#include "z.h"

int pngparts_z_compute_check(struct pngparts_z_header hdr){
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
