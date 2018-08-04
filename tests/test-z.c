/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * test-z.c
 * zlib base test program
 */

#include "../src/z.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv){
  int argi;
  if (argc < 2){
    fprintf(stderr,"available commans: \n"
      "  fcheck     check a zlib header\n");
    return 2;
  } else if (strcmp("fcheck",argv[1]) == 0){
    struct pngparts_z_header hdr;
    int check_compute;
    int help_tf = 0;
    hdr = pngparts_z_header_new();
    if (argc == 2){
      help_tf = 1;
    } else for (argi = 2; argi < argc; ++argi){
      if (strcmp("-cinfo",argv[argi]) == 0){
        if (++argi < argc){
          hdr.cinfo = atoi(argv[argi]);
        }
      } else if (strcmp("-cm",argv[argi]) == 0){
        if (++argi < argc){
          hdr.cm = atoi(argv[argi]);
        }
      } else if (strcmp("-fcheck",argv[argi]) == 0){
        if (++argi < argc){
          hdr.fcheck = atoi(argv[argi]);
        }
      } else if (strcmp("-fdict",argv[argi]) == 0){
        if (++argi < argc){
          hdr.fdict = atoi(argv[argi]);
        }
      } else if (strcmp("-flevel",argv[argi]) == 0){
        if (++argi < argc){
          hdr.flevel = atoi(argv[argi]);
        }
      } else if (strcmp("-",argv[argi]) == 0){
        /* ignore */
      } else if (strcmp("-?",argv[argi]) == 0){
        help_tf = 1;
      } else {
        fprintf(stderr,"unrecognized option: %s\n",argv[argi]);
        help_tf = 1;
      }
    }
    /*print help */
    if (help_tf){
      fprintf(stderr,"usage: test_z fcheck ...\n"
        "  -                  (ignored)\n"
        "  -?                 help message\n"
        "  -cm (number)       set compression method\n"
        "  -cinfo (number)    set compression information\n"
        "  -fcheck (number)   set check value\n"
        "  -fdict (number)    set dictionary flag\n"
        "  -flevel (number)   set compression level\n"
        );
      return 2;
    }
    /* compute the real check value */
    check_compute = pngparts_z_compute_check(hdr);
    if (check_compute%31 == hdr.fcheck%31){
      fprintf(stdout,"check ok\n");
      return 0;
    } else {
      fprintf(stdout,"check bad, expected fcheck %i\n",check_compute);
      return 1;
    }
  } else {
    fprintf(stdout,"unknown command: %s\n", argv[1]);
    return 2;
  }
}
