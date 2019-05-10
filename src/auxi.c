
#include "auxi.h"
#include "png.h"
#include "pngwrite.h"
#include "pngread.h"
#include "zwrite.h"
#include "zread.h"
#include "inflate.h"
#include "deflate.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static unsigned int pngparts_aux_block_form(unsigned int f);



static int pngparts_aux_image_start8
  ( void* img, long int width, long int height, short bit_depth,
    short color_type, short compression, short filter, short interlace);

static void pngparts_aux_image_describe8
  ( void* img, long int *width, long int *height, short *bit_depth,
    short *color_type, short *compression, short *filter, short *interlace);

static void pngparts_aux_image_put_to8
  ( void* img, long int x, long int y,
    unsigned int red, unsigned int green, unsigned int blue,
    unsigned int alpha);

static void pngparts_aux_image_get_from8
  ( void* img, long int x, long int y,
    unsigned int *red, unsigned int *green, unsigned int *blue,
    unsigned int *alpha);

static int pngparts_aux_block_start
  ( void* img, long int width, long int height, short bit_depth,
    short color_type, short compression, short filter, short interlace);

static void pngparts_aux_block_put
  ( void* img, long int x, long int y,
    unsigned int red, unsigned int green, unsigned int blue,
    unsigned int alpha);

static void pngparts_aux_block_describe
  ( void* img, long int *width, long int *height, short *bit_depth,
    short *color_type, short *compression, short *filter, short *interlace);

static void pngparts_aux_block_get
  ( void* img, long int x, long int y,
    unsigned int *red, unsigned int *green, unsigned int *blue,
    unsigned int *alpha);

static int pngparts_aux_indirect_read_header
  ( void* img, long int width, long int height, short bit_depth,
    short color_type, short compression, short filter, short interlace);

int pngparts_aux_destroy_png_chunk(struct pngparts_png_chunk_cb const* cb){
  if (cb->message_cb != NULL){
    struct pngparts_png_message dtor_message = {
      /* message type */ PNGPARTS_PNG_M_DESTROY,
      /* input/output data */ 0,
      /* target chunk callback */ {0,0,0,0},
      /* input/output pointer */ NULL
      };
    memcpy(dtor_message.name, cb->name, 4*sizeof(unsigned char));
    return (*cb->message_cb)(NULL, cb->cb_data, &dtor_message);
  } else return PNGPARTS_API_OK;
}

int pngparts_aux_read_png_16
  (struct pngparts_api_image* img, char const* fname)
{
  FILE *f = fopen(fname, "rb");
  if (f != NULL){
    int result = PNGPARTS_API_OK;
    struct pngparts_png parser;
    struct pngparts_z zreader;
    struct pngparts_flate inflater;
    unsigned int start_bits = 0;
    do {
      pngparts_pngread_init(&parser);
      start_bits |= 1;
      /* set image callback */{
        pngparts_png_set_image_cb(&parser, img);
      }
      /* set IDAT callback */ {
        struct pngparts_api_z z_api;
        struct pngparts_api_flate flate_api;
        struct pngparts_png_chunk_cb idat_api;
        pngparts_zread_init(&zreader);
        start_bits |= 2;
        pngparts_inflate_init(&inflater);
        start_bits |= 4;
        pngparts_inflate_assign_api(&flate_api, &inflater);
        pngparts_zread_assign_api(&z_api, &zreader);
        pngparts_z_set_cb(&zreader, &flate_api);
        /* assign IDAT callback */{
          int const idat_result =
            pngparts_pngread_assign_idat_api(&idat_api, &z_api);
          if (idat_result != PNGPARTS_API_OK){
            break;
          }
        }
        /* add IDAT callback */{
          int const add_idat_result =
            pngparts_png_add_chunk_cb(&parser, &idat_api);
          if (add_idat_result != PNGPARTS_API_OK){
            /* destroy the IDAT callback */
            pngparts_aux_destroy_png_chunk(&idat_api);
            break;
          }
        }
      }
      /* set PLTE callback */ {
        struct pngparts_png_chunk_cb plte_api;
        /* assign PLTE callback */{
          int const plte_result =
            pngparts_pngread_assign_plte_api(&plte_api);
          if (plte_result != PNGPARTS_API_OK){
            break;
          }
        }
        /* add PLTE callback */{
          int const add_plte_result =
            pngparts_png_add_chunk_cb(&parser, &plte_api);
          if (add_plte_result != PNGPARTS_API_OK){
            /* destroy the PLTE callback */
            pngparts_aux_destroy_png_chunk(&plte_api);
            break;
          }
        }
      }
      start_bits |= 8;
    } while (0);
    if (start_bits != 15){
      /* destroy the PNG structure first */
      if (start_bits & 1)
        pngparts_pngread_free(&parser);
      /* next destroy the zlib stream writer */
      if (start_bits & 2)
        pngparts_zread_free(&zreader);
      /* then, last destroy the inflater */
      if (start_bits & 4)
        pngparts_inflate_free(&inflater);
    } else {
      /* parse the image */
      do {
        unsigned char inbuf[256];
        size_t readlen;
        while ((readlen = fread(inbuf, sizeof(unsigned char), 256, f)) > 0) {
          pngparts_png_buffer_setup(&parser, inbuf, (int)readlen);
          while (!pngparts_png_buffer_done(&parser)) {
            result = pngparts_pngread_parse(&parser);
            if (result < 0) break;
          }
          if (result < 0) break;
        }
        if (result < 0) break;
      } while (0);
    }
    /* cleanup */
    fclose(f);
    pngparts_pngread_free(&parser);
    pngparts_zread_free(&zreader);
    pngparts_inflate_free(&inflater);
    return result<0?result:PNGPARTS_API_OK;
  } else return PNGPARTS_API_IO_ERROR;
}

int pngparts_aux_write_png_16
  (struct pngparts_api_image* img, char const* fname)
{
  FILE *f = fopen(fname, "wb");
  if (f != NULL){
    int result = PNGPARTS_API_OK;
    struct pngparts_png writer;
    struct pngparts_z zwriter;
    struct pngparts_flate deflater;
    unsigned int start_bits = 0;
    do {
      /* generate the PNG stream */
      pngparts_pngwrite_init(&writer);
      start_bits |= 1;
      /* set image callback */{
        pngparts_png_set_image_cb(&writer, img);
      }
      /* set IDAT callback */ {
        struct pngparts_api_z z_api;
        struct pngparts_api_flate flate_api;
        struct pngparts_png_chunk_cb idat_api;
        pngparts_zwrite_init(&zwriter);
        start_bits |= 2;
        pngparts_deflate_init(&deflater);
        start_bits |= 4;
        pngparts_deflate_assign_api(&flate_api, &deflater);
        pngparts_zwrite_assign_api(&z_api, &zwriter);
        pngparts_z_set_cb(&zwriter, &flate_api);
        /* assign IDAT callback */{
          int const idat_result =
            pngparts_pngwrite_assign_idat_api(&idat_api, &z_api, 0);
          if (idat_result != PNGPARTS_API_OK){
            break;
          }
        }
        /* add IDAT callback */{
          int const add_idat_result =
            pngparts_png_add_chunk_cb(&writer, &idat_api);
          if (add_idat_result != PNGPARTS_API_OK){
            /* destroy the IDAT callback */
            pngparts_aux_destroy_png_chunk(&idat_api);
            break;
          }
        }
      }
      /* set PLTE callback */ {
        struct pngparts_png_chunk_cb plte_api;
        /* assign PLTE callback */{
          int const plte_result =
            pngparts_pngwrite_assign_plte_api(&plte_api);
          if (plte_result != PNGPARTS_API_OK){
            break;
          }
        }
        /* add PLTE callback */{
          int const add_plte_result =
            pngparts_png_add_chunk_cb(&writer, &plte_api);
          if (add_plte_result != PNGPARTS_API_OK){
            /* destroy the PLTE callback */
            pngparts_aux_destroy_png_chunk(&plte_api);
            break;
          }
        }
      }
      start_bits |= 8;
    } while (0);
    if (start_bits != 15){
      /* destroy the PNG structure first */
      if (start_bits & 1)
        pngparts_pngwrite_free(&writer);
      /* next destroy the zlib stream writer */
      if (start_bits & 2)
        pngparts_zwrite_free(&zwriter);
      /* then, last destroy the inflater */
      if (start_bits & 4)
        pngparts_deflate_free(&deflater);
    } else {
      /* generate the image */
      unsigned char outbuf[256];
      size_t writelen;
      while (result == PNGPARTS_API_OK){
        pngparts_png_buffer_setup(&writer, outbuf, (int)sizeof(outbuf));
        result = pngparts_pngwrite_generate(&writer);
        if (result < 0) break;
        writelen = pngparts_png_buffer_used(&writer);
        if (writelen !=
            fwrite(outbuf, sizeof(unsigned char), writelen, f))
        {
          result = PNGPARTS_API_IO_ERROR;
          break;
        }
      }
    }
    /* cleanup */
    fclose(f);
    pngparts_pngwrite_free(&writer);
    pngparts_zwrite_free(&zwriter);
    pngparts_deflate_free(&deflater);
    return result<0?result:PNGPARTS_API_OK;
  } else return PNGPARTS_API_IO_ERROR;
}

int pngparts_aux_image_start8
  ( void* img, long int width, long int height, short bit_depth,
    short color_type, short compression, short filter, short interlace)
{
  struct pngparts_api_image* ximg = (struct pngparts_api_image*)img;
  return (*ximg->start_cb)(
      ximg->cb_data, width, height, bit_depth,
      color_type, compression, filter, interlace
    );
}

void pngparts_aux_image_describe8
  ( void* img, long int *width, long int *height, short *bit_depth,
    short *color_type, short *compression, short *filter, short *interlace)
{
  struct pngparts_api_image* ximg = (struct pngparts_api_image*)img;
  (*ximg->describe_cb)(
      ximg->cb_data, width, height, bit_depth,
      color_type, compression, filter, interlace
    );
  return;
}

void pngparts_aux_image_put_to8
  ( void* img, long int x, long int y,
    unsigned int red, unsigned int green, unsigned int blue,
    unsigned int alpha)
{
  unsigned int xred, xgreen, xblue, xalpha;
  struct pngparts_api_image* ximg = (struct pngparts_api_image*)img;
  (*ximg->put_cb)(ximg->cb_data, x,y, red/257, green/257, blue/257, alpha/257);
  return;
}

void pngparts_aux_image_get_from8
  ( void* img, long int x, long int y,
    unsigned int *red, unsigned int *green, unsigned int *blue,
    unsigned int *alpha)
{
  unsigned int xred, xgreen, xblue, xalpha;
  struct pngparts_api_image* ximg = (struct pngparts_api_image*)img;
  (*ximg->get_cb)(ximg->cb_data, x,y,&xred,&xgreen,&xblue,&xalpha);
  *red = (xred&255)*257;
  *green = (xgreen&255)*257;
  *blue = (xblue&255)*257;
  *alpha = (xalpha&255)*257;
  return;
}

int pngparts_aux_read_png_8
  (struct pngparts_api_image* img, char const* fname)
{
  struct pngparts_api_image aux_img = {
    /* callback data */img,
    /* image start callback (read only)*/pngparts_aux_image_start8,
    /* image color posting callback (read only)*/pngparts_aux_image_put_to8,
    /* image describe callback (write only)*/pngparts_aux_image_describe8,
    /* image color fetch callback (write only)*/pngparts_aux_image_get_from8
    };
  return pngparts_aux_read_png_16(&aux_img, fname);
}

int pngparts_aux_write_png_8
  (struct pngparts_api_image* img, char const* fname)
{
  struct pngparts_api_image aux_img = {
    /* callback data */img,
    /* image start callback (read only)*/pngparts_aux_image_start8,
    /* image color posting callback (read only)*/pngparts_aux_image_put_to8,
    /* image describe callback (write only)*/pngparts_aux_image_describe8,
    /* image color fetch callback (write only)*/pngparts_aux_image_get_from8
    };
  return pngparts_aux_write_png_16(&aux_img, fname);
}

void pngparts_aux_free(void* p){
  free(p);
  return;
}

struct pngparts_aux_block {
  unsigned int width;
  unsigned int height;
  unsigned int stride;
  unsigned int vstride;
  unsigned int format;
  unsigned int bits;
  void* data;
  void const* const_data;
};

unsigned int pngparts_aux_block_form(unsigned int f){
  switch (f&6){
  case 6: return 4;
  case 4: return 2;
  case 2: return 3;
  case 0: return 1;
  }
}

int pngparts_aux_block_start
  ( void* img, long int width, long int height, short bit_depth,
    short color_type, short compression, short filter, short interlace)
{
  return PNGPARTS_API_OK;
}

void pngparts_aux_block_put
  ( void* img, long int x, long int y,
    unsigned int red, unsigned int green, unsigned int blue,
    unsigned int alpha)
{
  struct pngparts_aux_block *block = (struct pngparts_aux_block *)img;
  if (x >= 0 && ((unsigned int)x < block->width)
  &&  y >= 0 && ((unsigned int)y < block->height))
  {
    unsigned char* const start = block->data;
    unsigned int const format_count = pngparts_aux_block_form(block->format);
    unsigned int const short_bytes = (block->bits==16)
      ? sizeof(unsigned short) : sizeof(unsigned char);
    unsigned int const bytes = (short_bytes)*format_count;
    unsigned char* const pixel_start =
      start+(block->vstride+bytes*block->width)*y+(block->stride+bytes)*x;
    switch (block->format&6){
    case 6: /* RGBA */
      if (bytes == 2){
        unsigned short* pixel_short = (unsigned short*)pixel_start;
        pixel_short[0] = red;
        pixel_short[1] = green;
        pixel_short[2] = blue;
        pixel_short[3] = alpha;
      } else {
        pixel_start[0] = red/257;
        pixel_start[1] = green/257;
        pixel_start[2] = blue/257;
        pixel_start[3] = alpha/257;
      }break;
    case 4: /* LA */
      if (bytes == 2){
        unsigned short* pixel_short = (unsigned short*)pixel_start;
        pixel_short[0] = red;
        pixel_short[1] = alpha;
      } else {
        pixel_start[0] = red/257;
        pixel_start[1] = alpha/257;
      }break;
    case 2: /* RGB */
      if (bytes == 2){
        unsigned short* pixel_short = (unsigned short*)pixel_start;
        pixel_short[0] = red;
        pixel_short[1] = green;
        pixel_short[2] = blue;
      } else {
        pixel_start[0] = red/257;
        pixel_start[1] = green/257;
        pixel_start[2] = blue/257;
      }break;
    case 0: /* L */
      if (bytes == 2){
        unsigned short* pixel_short = (unsigned short*)pixel_start;
        pixel_short[0] = red;
      } else {
        pixel_start[0] = red/257;
      }break;
    }
  }
  return;
}

void pngparts_aux_block_describe
  ( void* img, long int *width, long int *height, short *bit_depth,
    short *color_type, short *compression, short *filter, short *interlace)
{
  struct pngparts_aux_block *block = (struct pngparts_aux_block *)img;
  *width = block->width;
  *height = block->height;
  *bit_depth = block->bits;
  *color_type = block->format&6;
  *compression = 0;
  *filter = 0;
  *interlace = (block->format&8)?1:0;
  return;
}

void pngparts_aux_block_get
  ( void* img, long int x, long int y,
    unsigned int *red, unsigned int *green, unsigned int *blue,
    unsigned int *alpha)
{
  struct pngparts_aux_block *block = (struct pngparts_aux_block *)img;
  unsigned char const* const start = block->const_data;
  unsigned int const format_count = pngparts_aux_block_form(block->format);
    unsigned int const short_bytes = (block->bits==16)
      ? sizeof(unsigned short) : sizeof(unsigned char);
    unsigned int const bytes = (short_bytes)*format_count;
  unsigned char const* const pixel_start =
    start+(block->vstride+bytes*block->width)*y+(block->stride+bytes)*x;
  switch (block->format&6){
  case 6: /* RGBA */
    if (bytes == 2){
      unsigned short const* pixel_short = (unsigned short const*)pixel_start;
      *red = pixel_short[0]&65535;
      *green = pixel_short[1]&65535;
      *blue = pixel_short[2]&65535;
      *alpha = pixel_short[3]&65535;
    } else {
      *red = (pixel_start[0]&255)*257;
      *green = (pixel_start[1]&255)*257;
      *blue = (pixel_start[2]&255)*257;
      *alpha = (pixel_start[3]&255)*257;
    }break;
  case 4: /* LA */
    if (bytes == 2){
      unsigned short const* pixel_short = (unsigned short const*)pixel_start;
      *red = pixel_short[0]&65535;
      *alpha = pixel_short[1]&65535;
    } else {
      *red = (pixel_start[0]&255)*257;
      *alpha = (pixel_start[1]&255)*257;
    }break;
  case 2: /* RGB */
    if (bytes == 2){
      unsigned short const* pixel_short = (unsigned short const*)pixel_start;
      *red = pixel_short[0]&65535;
      *green = pixel_short[1]&65535;
      *blue = pixel_short[2]&65535;
    } else {
      *red = (pixel_start[0]&255)*257;
      *green = (pixel_start[1]&255)*257;
      *blue = (pixel_start[2]&255)*257;
    }break;
  case 0: /* L */
    if (bytes == 2){
      unsigned short const* pixel_short = (unsigned short const*)pixel_start;
      *red = pixel_short[0]&65535;
    } else {
      *red = (pixel_start[0]&255)*257;
    }break;
  }
  return;
}

int pngparts_aux_write_block
  ( unsigned int width, unsigned int height,
    unsigned int stride, unsigned int vstride,
    unsigned int format, unsigned int bits, void const* data,
    char const* fname)
{
  if (bits != 8 && bits != 16){
    return PNGPARTS_API_BAD_PARAM;
  }
  if (height > 0x7fFFffFF || width > 0x7fFFffFF){
    return PNGPARTS_API_BAD_PARAM;
  }
  struct pngparts_aux_block aux_img_data = {
    /* width */width,
    /* height */height,
    /* stride */stride,
    /* vstride */vstride,
    /* format */format,
    /* bits */bits,
    /* data */NULL,
    /* const_data */data
    };
  struct pngparts_api_image aux_img = {
    /* callback data */&aux_img_data,
    /* image start callback (read only)*/NULL,
    /* image color posting callback (read only)*/NULL,
    /* image describe callback (write only)*/pngparts_aux_block_describe,
    /* image color fetch callback (write only)*/pngparts_aux_block_get
    };
  return pngparts_aux_write_png_16(&aux_img, fname);
}

int pngparts_aux_read_block
  ( unsigned int width, unsigned int height,
    unsigned int stride, unsigned int vstride,
    unsigned int format, unsigned int bits, void* data,
    char const* fname)
{
  if (bits != 8 && bits != 16){
    return PNGPARTS_API_BAD_PARAM;
  }
  if (height > 0x7fFFffFF || width > 0x7fFFffFF){
    return PNGPARTS_API_BAD_PARAM;
  }
  struct pngparts_aux_block aux_img_data = {
    /* width */width,
    /* height */height,
    /* stride */stride,
    /* vstride */vstride,
    /* format */format,
    /* bits */bits,
    /* data */data,
    /* const_data */data
    };
  struct pngparts_api_image aux_img = {
    /* callback data */&aux_img_data,
    /* image start callback (read only)*/pngparts_aux_block_start,
    /* image color posting callback (read only)*/pngparts_aux_block_put,
    /* image describe callback (write only)*/NULL,
    /* image color fetch callback (write only)*/NULL
    };
  return pngparts_aux_read_png_16(&aux_img, fname);
}

struct pngparts_aux_indirect_data {
  struct pngparts_png_header header;
  int set;
};

int pngparts_aux_indirect_read_header
  ( void* img, long int width, long int height, short bit_depth,
    short color_type, short compression, short filter, short interlace)
{
  struct pngparts_aux_indirect_data* aux_indirect =
    (struct pngparts_aux_indirect_data*)img;
  aux_indirect->header.width = width;
  aux_indirect->header.height = height;
  aux_indirect->header.bit_depth = bit_depth;
  aux_indirect->header.color_type = color_type;
  aux_indirect->header.compression = compression;
  aux_indirect->header.filter = filter;
  aux_indirect->header.interlace = interlace;
  aux_indirect->set = 1;
  return PNGPARTS_API_OK;
}

int pngparts_aux_read_header
  (struct pngparts_png_header* header, char const* fname)
{
  FILE *f = fopen(fname, "rb");
  if (f != NULL){
    int result = PNGPARTS_API_OK;
    struct pngparts_png parser;
    unsigned int start_bits = 0;
    struct pngparts_aux_indirect_data aux_indirect;
    struct pngparts_api_image aux_img = {
      /* callback data */&aux_indirect,
      /* image start callback (read only)*/&pngparts_aux_indirect_read_header,
      /* image color posting callback (read only)*/NULL,
      /* image describe callback (write only)*/NULL,
      /* image color fetch callback (write only)*/NULL
      };
    aux_indirect.set = 0;
    do {
      pngparts_pngread_init(&parser);
      start_bits |= 1;
      /* set image callback */{
        pngparts_png_set_image_cb(&parser, &aux_img);
      }
      start_bits |= 8;
    } while (0);
    if (start_bits != 9){
      /* destroy the PNG structure */
      if (start_bits & 1)
        pngparts_pngread_free(&parser);
    } else {
      /* parse the image */
      do {
        unsigned char inbuf[256];
        size_t readlen;
        while ((readlen = fread(inbuf, sizeof(unsigned char), 256, f)) > 0) {
          pngparts_png_buffer_setup(&parser, inbuf, (int)readlen);
          while (!pngparts_png_buffer_done(&parser)) {
            result = pngparts_pngread_parse(&parser);
            if (result < 0 || aux_indirect.set) break;
          }
          if (result < 0 || aux_indirect.set) break;
        }
        if (result < 0 || aux_indirect.set) break;
      } while (0);
      if (result == PNGPARTS_API_UNCAUGHT_CRITICAL){
        /* ignore this error */
        result = PNGPARTS_API_OK;
      }
      if (result >= 0){
        if (aux_indirect.set){
          *header = aux_indirect.header;
        } else result = PNGPARTS_API_MISSING_HDR;
      }
    }
    /* cleanup */
    fclose(f);
    pngparts_pngread_free(&parser);
    return result<0?result:PNGPARTS_API_OK;
  } else return PNGPARTS_API_IO_ERROR;
}