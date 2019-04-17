/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018-2019 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * test-pngread.c
 * png reader test program
 */

#include "../src/pngread.h"
#include "../src/zread.h"
#include "../src/inflate.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

struct test_image {
  int width;
  int height;
  unsigned char* bytes;
  FILE* outfile;
};
static int test_image_header
  ( void* img, long int width, long int height, short bit_depth,
    short color_type, short compression, short filter, short interlace);
static void test_image_recv_pixel
  ( void* img, long int x, long int y, unsigned int red,
    unsigned int green, unsigned int blue, unsigned int alpha);

int test_image_header
  ( void* img_ptr, long int width, long int height, short bit_depth,
    short color_type, short compression, short filter, short interlace)
{
  struct test_image *img = (struct test_image*)img_ptr;
  fprintf(stderr, "{\"image info\":{\n"
    "  \"width\": %li,\n"
    "  \"height\": %li,\n"
    "  \"bit depth\": %i,\n"
    "  \"color type\": %i,\n"
    "  \"compression\": %i,\n"
    "  \"filter\": %i,\n"
    "  \"interlace\": %i\n}}\n",
    width, height, bit_depth, color_type, compression, filter, interlace
  );
  if (width > 2000 || height > 2000) return PNGPARTS_API_UNSUPPORTED;
  void* bytes = malloc(width*height * 4);
  if (bytes == NULL) return PNGPARTS_API_UNSUPPORTED;
  img->width = (int)width;
  img->height = (int)height;
  img->bytes = (unsigned char*)bytes;
  memset(bytes, 55, width*height * 4);
  return PNGPARTS_API_OK;
}
void test_image_recv_pixel
  ( void* img_ptr, long int x, long int y, unsigned int red,
    unsigned int green, unsigned int blue, unsigned int alpha)
{
  struct test_image *img = (struct test_image*)img_ptr;
  unsigned char *const pixel = (&img->bytes[(y*img->width + x)*4]);
  /*fprintf(stderr, "pixel for %li %li..\n", x, y);*/
  pixel[0] = red / 257;
  pixel[1] = green / 257;
  pixel[2] = blue / 257;
  pixel[3] = alpha / 257;
  return;
}
void test_image_put_ppm(struct test_image* img) {
  int x, y;
  fprintf(img->outfile, "P3\n%i %i\n255\n",
    img->width, img->height);
  for (y = 0; y < img->height; ++y) {
    for (x = 0; x < img->width; ++x) {
      unsigned char *const pixel = (&img->bytes[(y*img->width + x) * 4]);
      fprintf(img->outfile, "%i %i %i\n",
        pixel[0], pixel[1], pixel[2]);
    }
  }
}

int main(int argc, char**argv) {
  FILE *to_read = NULL, *to_write = NULL;
  char const* in_fname = NULL, *out_fname = NULL;
  struct pngparts_png parser;
  struct pngparts_z zreader;
  struct pngparts_flate inflater;
  int help_tf = 0;
  int result = 0;
  struct test_image img = { 0,0,NULL };
  {
    int argi;
    for (argi = 1; argi < argc; ++argi) {
      if (strcmp(argv[argi], "-?") == 0) {
        help_tf = 1;
      } else if (in_fname == NULL) {
        in_fname = argv[argi];
      } else if (out_fname == NULL) {
        out_fname = argv[argi];
      }
    }
    if (help_tf) {
      fprintf(stderr, "usage: test_pngread ... (infile) (outfile)\n"
        "  -                  stdin/stdout\n"
        "  -?                 help message\n"
      );
      return 2;
    }
  }
  /* open */
  if (in_fname == NULL) {
    fprintf(stderr, "No input file name given.\n");
    return 2;
  } else if (strcmp(in_fname, "-") == 0) {
    to_read = stdin;
  } else {
    to_read = fopen(in_fname, "rb");
    if (to_read == NULL) {
      int errval = errno;
      fprintf(stderr, "Failed to open '%s'.\n\t%s\n",
        in_fname, strerror(errval));
      return 1;
    }
  }
  if (out_fname == NULL) {
    fprintf(stderr, "No output file name given.\n");
    return 2;
  } else if (strcmp(out_fname, "-") == 0) {
    to_write = stdout;
  } else {
    to_write = fopen(out_fname, "wb");
    if (to_write == NULL) {
      int errval = errno;
      fprintf(stderr, "Failed to open '%s'.\n\t%s\n",
        out_fname, strerror(errval));
      if (to_read != stdin) fclose(to_read);
      return 1;
    }
  }
  /* parse the zlib stream */
  pngparts_pngread_init(&parser);
  /* set image callback */{
    struct pngparts_api_image img_api;
    img_api.cb_data = &img;
    img_api.start_cb = &test_image_header;
    img_api.put_cb = &test_image_recv_pixel;
    pngparts_png_set_image_cb(&parser, &img_api);
  }
  img.outfile = to_write;
  /* set IDAT callback */ {
    struct pngparts_api_z z_api;
    struct pngparts_api_flate flate_api;
    struct pngparts_png_chunk_cb idat_api;
    pngparts_zread_init(&zreader);
    pngparts_inflate_init(&inflater);
    pngparts_inflate_assign_api(&flate_api, &inflater);
    pngparts_zread_assign_api(&z_api, &zreader);
    pngparts_z_set_cb(&zreader, &flate_api);
    pngparts_pngread_assign_idat_api(&idat_api, &z_api);
    pngparts_png_add_chunk_cb(&parser, &idat_api);
  }
  /* set PLTE callback */ {
    struct pngparts_png_chunk_cb plte_api;
    pngparts_pngread_assign_plte_api(&plte_api);
    pngparts_png_add_chunk_cb(&parser, &plte_api);
  }
  /*pngparts_png_set_z_cb(&parser, &reader,
    &pngparts_z_touch_input, &pngparts_z_touch_output,
    &pngparts_z_churn);*/
  do {
    unsigned char inbuf[256];
    size_t readlen;
    while ((readlen = fread(inbuf, sizeof(unsigned char), 256, to_read)) > 0) {
      pngparts_png_buffer_setup(&parser, inbuf, (int)readlen);
      while (!pngparts_png_buffer_done(&parser)) {
        result = pngparts_pngread_parse(&parser);
        if (result < 0) break;
      }
      if (result < 0) break;
    }
    if (result < 0) break;
  } while (0);
  pngparts_pngread_free(&parser);
  pngparts_zread_free(&zreader);
  pngparts_inflate_free(&inflater);
  /* output to PPM */ {
    test_image_put_ppm(&img);
  }
  /* close */
  free(img.bytes);
  if (to_write != stdout) fclose(to_write);
  if (to_read != stdin) fclose(to_read);
  fflush(NULL);
  if (result) {
    fprintf(stderr, "\nResult code %i: %s\n",
      result, pngparts_api_strerror(result));
  }
  return result;
}
