/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
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

int main(int argc, char**argv) {
  FILE *to_read = NULL, *to_write = NULL;
  char const* in_fname = NULL, *out_fname = NULL;
  struct pngparts_png parser;
  struct pngparts_z reader;
  struct pngparts_flate inflater;
  int help_tf = 0;
  int result = 0;
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
  pngparts_zread_init(&reader);
  pngparts_inflate_init(&inflater);
  /*pngparts_png_set_z_cb(&parser, &reader,
    &pngparts_z_touch_input, &pngparts_z_touch_output,
    &pngparts_z_churn);*/
  pngparts_z_set_cb(&reader, &inflater,
    &pngparts_inflate_start, &pngparts_inflate_dict,
    &pngparts_inflate_one, &pngparts_inflate_finish);
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
  pngparts_inflate_free(&inflater);
  pngparts_zread_free(&reader);
  pngparts_pngread_free(&parser);
  /* close */
  if (to_write != stdout) fclose(to_write);
  if (to_read != stdin) fclose(to_read);
  fflush(NULL);
  if (result) {
    fprintf(stderr, "\nResult code %i: %s\n",
      result, pngparts_api_strerror(result));
  }
  return result;
}
