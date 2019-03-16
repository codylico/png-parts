/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * test-pngwrite.c
 * png writer test program
 */

#include "../src/pngwrite.h"
#include "../src/zwrite.h"
#include "../src/deflate.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

struct test_image {
  int width;
  int height;
  unsigned char* bytes;
  FILE* fptr;
  char interlace_tf;
  char color_type;
  char bit_depth;
};
static void test_image_describe
  ( void* img, long int *width, long int *height, short *bit_depth,
    short *color_type, short *compression, short *filter, short *interlace);
static void test_image_send_pixel
  ( void* img, long int x, long int y, unsigned int *red,
    unsigned int *green, unsigned int *blue, unsigned int *alpha);
static int test_image_resize
  ( void* img_ptr, long int width, long int height);
static int test_image_get_text_word
  (struct test_image* img, char* buf, int count);
static int test_image_get_ppm(struct test_image* img);

void test_image_describe
  ( void* img_ptr, long int *width, long int *height, short *bit_depth,
    short *color_type, short *compression, short *filter, short *interlace)
{
  struct test_image *img = (struct test_image*)img_ptr;
  *width = img->width;
  *height = img->height;
  *bit_depth = img->bit_depth;
  *color_type = img->color_type;
  *compression = 0;
  *filter = 0;
  *interlace = img->interlace_tf?1:0;
  fprintf(stderr, "{\"image info\":{\n"
    "  \"width\": %li,\n"
    "  \"height\": %li,\n"
    "  \"bit depth\": %i\n"
    "  \"color type\": %i\n"
    "  \"compression\": %i\n"
    "  \"filter\": %i\n"
    "  \"interlace\": %i\n}\n",
    *width, *height, *bit_depth, *color_type, *compression, *filter, *interlace
  );
}

int test_image_resize
  (void* img_ptr, long int width, long int height)
{
  struct test_image *img = (struct test_image*)img_ptr;
  if (width > 2000 || height > 2000) return PNGPARTS_API_UNSUPPORTED;
  void* bytes = malloc(width*height * 4);
  if (bytes == NULL) return PNGPARTS_API_UNSUPPORTED;
  img->width = (int)width;
  img->height = (int)height;
  img->bytes = (unsigned char*)bytes;
  memset(bytes, 55, width*height * 4);
  return PNGPARTS_API_OK;
}

void test_image_send_pixel
  ( void* img_ptr, long int x, long int y, unsigned int *red,
    unsigned int *green, unsigned int *blue, unsigned int *alpha)
{
  struct test_image *img = (struct test_image*)img_ptr;
  unsigned char *const pixel = (&img->bytes[(y*img->width + x)*4]);
  /*fprintf(stderr, "pixel for %li %li..\n", x, y);*/
  *red = pixel[0]*257;
  *green = pixel[1]*257;
  *blue = pixel[2]*257;
  *alpha = pixel[3]*257;
  return;
}

int test_image_get_text_word(struct test_image* img, char* buf, int count){
  int ch;
  int i = 0;
  int const countm1 = count-1;
  /* scan past spaces */
  ch = fgetc(img->fptr);
  while (ch != EOF && isspace(ch)){
    ch = fgetc(img->fptr);
  }
  /* read the word */
  if (ch == EOF || countm1 <= 0){
    return 0;
  } else while (ch != EOF && (!isspace(ch))){
    buf[i] = (char)ch;
    i += 1;
    if (i >= countm1)
      break;
    ch = fgetc(img->fptr);
  }
  /* NUL-terminate it */
  if (i >= countm1)
    buf[countm1] = 0;
  else
    buf[i] = 0;
  return i;
}

int test_image_get_ppm(struct test_image* img) {
  int x, y;
  char word_buf[32];
  if (test_image_get_text_word(img, word_buf, sizeof(word_buf)) <= 0){
    return PNGPARTS_API_IO_ERROR;
  } else if (strcmp("P3", word_buf) == 0){
    int width, height, max_value;
    /* read width */
    if (test_image_get_text_word(img, word_buf, sizeof(word_buf)) <= 0){
      return PNGPARTS_API_IO_ERROR;
    }
    width = atoi(word_buf);
    /* read height */
    if (test_image_get_text_word(img, word_buf, sizeof(word_buf)) <= 0){
      return PNGPARTS_API_IO_ERROR;
    }
    height = atoi(word_buf);
    /* read value maximum */
    if (test_image_get_text_word(img, word_buf, sizeof(word_buf)) <= 0){
      return PNGPARTS_API_IO_ERROR;
    }
    max_value = atoi(word_buf);
    /* allocate the space */{
      int const resize_result = test_image_resize(img, width, height);
      if (resize_result != PNGPARTS_API_OK){
        return resize_result;
      }
    }
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x) {
        unsigned char *const pixel = (&img->bytes[(y*img->width + x) * 4]);
        int red, green, blue, alpha = max_value;
        /* read */
        if (test_image_get_text_word(img, word_buf, sizeof(word_buf)) <= 0){
          return PNGPARTS_API_IO_ERROR;
        }
        red = atoi(word_buf);
        if (test_image_get_text_word(img, word_buf, sizeof(word_buf)) <= 0){
          return PNGPARTS_API_IO_ERROR;
        }
        green = atoi(word_buf);
        if (test_image_get_text_word(img, word_buf, sizeof(word_buf)) <= 0){
          return PNGPARTS_API_IO_ERROR;
        }
        blue = atoi(word_buf);
        /* put */
        pixel[0] = red*255/max_value;
        pixel[1] = green*255/max_value;
        pixel[2] = blue*255/max_value;
        pixel[3] = alpha*255/max_value;
      }
    }
  } else {
    return PNGPARTS_API_UNSUPPORTED;
  }
  return PNGPARTS_API_OK;
}

int main(int argc, char**argv) {
  FILE *to_read = NULL, *to_write = NULL;
  char const* in_fname = NULL, *out_fname = NULL;
  struct pngparts_png writer;
  struct pngparts_z zwriter;
  struct pngparts_flate deflater;
  int help_tf = 0;
  int result = 0;
  struct test_image img = { 0,0,NULL,NULL,0,2,8 };
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
      fprintf(stderr, "usage: test_pngwrite ... (infile) (outfile)\n"
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
  /* generate the PNG stream */
  pngparts_pngwrite_init(&writer);
  /* set image callback */{
    struct pngparts_api_image img_api;
    img_api.cb_data = &img;
    img_api.describe_cb = &test_image_describe;
    img_api.get_cb = &test_image_send_pixel;
    pngparts_png_set_image_cb(&writer, &img_api);
  }
  /* set IDAT callback */ {
    struct pngparts_api_z z_api;
    struct pngparts_api_flate flate_api;
    struct pngparts_png_chunk_cb idat_api;
    pngparts_zwrite_init(&zwriter);
    pngparts_deflate_init(&deflater);
    pngparts_deflate_assign_api(&flate_api, &deflater);
    pngparts_zwrite_assign_api(&z_api, &zwriter);
    pngparts_z_set_cb(&zwriter, &flate_api);
#if 0
    pngparts_pngread_assign_idat_api(&idat_api, &z_api);
    pngparts_png_add_chunk_cb(&writer, &idat_api);
#endif /*0*/
  }
  /* set PLTE callback */ {
    struct pngparts_png_chunk_cb plte_api;
#if 0
    pngparts_pngread_assign_plte_api(&plte_api);
    pngparts_png_add_chunk_cb(&writer, &plte_api);
#endif /*0*/
  }
  img.fptr = to_read;
  /* input from PPM */ {
    int const in_result = test_image_get_ppm(&img);
    if (in_result != PNGPARTS_API_OK){
      /* quit early */
      pngparts_pngwrite_free(&writer);
      pngparts_zwrite_free(&zwriter);
      /* close */
      free(img.bytes);
      if (to_write != stdout) fclose(to_write);
      if (to_read != stdin) fclose(to_read);
      fflush(NULL);
      if (in_result) {
        fprintf(stderr, "\nResult code from PPM %i: %s\n",
          in_result, pngparts_api_strerror(in_result));
      }
      return in_result;
    }
  }
  do {
    unsigned char outbuf[256];
    size_t writelen;
    while (result == PNGPARTS_API_OK){
      pngparts_png_buffer_setup(&writer, outbuf, (int)sizeof(outbuf));
      result = pngparts_pngwrite_generate(&writer);
      if (result < 0) break;
      writelen = pngparts_png_buffer_used(&writer);
      if (writelen !=
          fwrite(outbuf, sizeof(unsigned char), writelen, to_write))
      {
        result = PNGPARTS_API_IO_ERROR;
        break;
      }
    }
    if (result < 0) break;
  } while (0);
  pngparts_zwrite_free(&zwriter);
  pngparts_pngwrite_free(&writer);
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
