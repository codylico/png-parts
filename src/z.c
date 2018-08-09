/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * z.c
 * zlib main header
 */
#include "z.h"

char const* pngparts_z_strerror(int result){
  switch (result){
  case PNGPARTS_Z_UNSUPPORTED: return "unsupported compression method";
  case PNGPARTS_Z_OVERFLOW: return "output overflow";
  case PNGPARTS_Z_IO_ERROR: return "input/output error";
  case PNGPARTS_Z_BAD_PARAM: return "bad parameter";
  case PNGPARTS_Z_NEED_DICT: return "dictionary requested";
  case PNGPARTS_Z_BAD_CHECK: return "bad header check";
  case PNGPARTS_Z_BAD_STATE: return "bad state machine";
  case PNGPARTS_Z_EOF: return "stream too quick to finish";
  case PNGPARTS_Z_OK: return "all is good";
  case PNGPARTS_Z_DONE: return "the stream is done";
  default: return "?";
  }
}
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

struct pngparts_z_adler32 pngparts_z_adler32_new(void){
  struct pngparts_z_adler32 out;
  out.s1 = 1;
  out.s2 = 0;
  return out;
}
unsigned long int pngparts_z_adler32_tol(struct pngparts_z_adler32 chk){
  return chk.s1 | ((chk.s2)<<16);
}
struct pngparts_z_adler32 pngparts_z_adler32_accum
  (struct pngparts_z_adler32 chk, int ch)
{
  unsigned long int xs1;
  struct pngparts_z_adler32 out;
  xs1 = (chk.s1+ch)%65521;
  out.s1 = xs1;
  out.s2 = (chk.s2+xs1)%65521;
  return out;
}
