/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * test-zread.c
 * zlib reader test program
 */

#include "../src/zread.h"
#include "../src/inflate.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char**argv){
  FILE *to_read = NULL, *to_write = NULL;
  char const* in_fname = NULL, *out_fname = NULL,
    *dict_fname = NULL;
  struct pngparts_z reader;
  struct pngparts_flate inflater;
  int help_tf = 0;
  int result = 0;
  {
    int argi;
    for (argi = 1; argi < argc; ++argi){
      if (strcmp(argv[argi],"-?") == 0){
        help_tf = 1;
      } else if (strcmp(argv[argi],"-d") == 0){
        if (++argi < argc){
          dict_fname = argv[argi];
          fprintf(stderr,"Note: dictionary not implemented (%s)\n",
            dict_fname);
        }
      } else if (in_fname == NULL){
        in_fname = argv[argi];
      } else if (out_fname == NULL){
        out_fname = argv[argi];
      }
    }
    if (help_tf){
      fprintf(stderr,"usage: test_zread ... (infile) (outfile)\n"
        "  -                  stdin/stdout\n"
        "  -?                 help message\n"
        "  -d (file)          dictionary\n"
        );
      return 2;
    }
  }
  /* open */
  if (in_fname == NULL){
    fprintf(stderr,"No input file name given.\n");
    return 2;
  } else if (strcmp(in_fname,"-") == 0){
    to_read = stdin;
  } else {
    to_read = fopen(in_fname,"rb");
    if (to_read == NULL){
      int errval = errno;
      fprintf(stderr,"Failed to open '%s'.\n\t%s\n",
        in_fname, strerror(errval));
      return 1;
    }
  }
  if (out_fname == NULL){
    fprintf(stderr,"No output file name given.\n");
    return 2;
  } else if (strcmp(out_fname,"-") == 0){
    to_write = stdout;
  } else {
    to_write = fopen(out_fname,"wb");
    if (to_write == NULL){
      int errval = errno;
      fprintf(stderr,"Failed to open '%s'.\n\t%s\n",
        out_fname, strerror(errval));
      if (to_read != stdin) fclose(to_read);
      return 1;
    }
  }
  /* parse the zlib stream */
  pngparts_zread_init(&reader);
  pngparts_inflate_init(&inflater);
  pngparts_z_set_cb( &reader, &inflater,
    &pngparts_inflate_start, &pngparts_inflate_dict,
    &pngparts_inflate_one, &pngparts_inflate_finish);
  do {
    unsigned char inbuf[256];
    unsigned char outbuf[256];
    size_t readlen;
//     pngparts_z_setup_input(&reader,inbuf,sizeof(inbuf));
//     pngparts_z_setup_output(&reader,outbuf,sizeof(outbuf));
    while ((readlen = fread(inbuf,sizeof(unsigned char),256,to_read)) > 0){
      pngparts_z_setup_input(&reader,inbuf,readlen);
      pngparts_z_setup_output(&reader,outbuf,sizeof(outbuf));
      while (!pngparts_z_input_done(&reader)){
        result = pngparts_zread_parse(&reader,PNGPARTS_ZREAD_NORMAL);
        if (result < 0) break;
        size_t writelen = pngparts_z_output_left(&reader);
        if (writelen > 0){
          size_t writeresult =
            fwrite(outbuf,sizeof(unsigned char),writelen,to_write);
          if (writeresult != writelen){
            result = PNGPARTS_API_IO_ERROR;
            break;
          }
        }
      }
      if (result < 0) break;
    }
    if (result < 0) break;
    result = pngparts_zread_parse(&reader,PNGPARTS_ZREAD_FINISH);
    do {
      if (result < 0) break;
      size_t writelen = pngparts_z_output_left(&reader);
      if (writelen > 0){
        size_t writeresult =
          fwrite(outbuf,sizeof(unsigned char),writelen,to_write);
        if (writeresult != writelen){
          result = PNGPARTS_API_IO_ERROR;
          break;
        }
      }
    }while (result == PNGPARTS_API_OVERFLOW);
  } while (0);
  pngparts_inflate_free(&inflater);
  pngparts_zread_free(&reader);
  /* close */
  if (to_write != stdout) fclose(to_write);
  if (to_read != stdin) fclose(to_read);
  if (result){
    fprintf(stderr,"Result code %i: %s\n",
      result,pngparts_api_strerror(result));
  }
  return result;
}
