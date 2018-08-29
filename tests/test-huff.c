/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * test-huff.c
 * Huffman code table test program
 */

#include "../src/flate.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

static void code_to_string
  (struct pngparts_flate_code cd,char* bitstring);

void code_to_string
  (struct pngparts_flate_code cd,char* bitstring)
{
  int i;
  int len = cd.length;
  if (len > 16) len = 16;
  bitstring[len] = 0;
  for (i = len; i > 0; --i){
    bitstring[i-1] = (cd.bits&1)+'0';
    cd.bits >>= 1;
  }
  return;
}

int main(int argc, char **argv){
  int mode = 1;
  int usage_tf = 0;
  int result = 0;
  struct pngparts_flate_huff code_table;
  char const* text_informator = NULL;
  /**/{
    int argi;
    for (argi = 1; argi < argc; ++argi){
      if (strcmp(argv[argi],"-f") == 0){
        /* fixed mode */
        mode = 1;
      } else if (strcmp(argv[argi],"-v") == 0){
        /* variable code: auto */
        if (++argi < argc){
          mode = 2;
          text_informator = argv[argi];
        } else {
          usage_tf = 1;
        }
      } else if (strcmp(argv[argi],"-m") == 0){
        /* variable code: manual */
        if (++argi < argc){
          mode = 3;
          text_informator = argv[argi];
        } else {
          usage_tf = 1;
        }
      } else if (strcmp(argv[argi],"-z") == 0){
        /* ascii mode */
        mode = 0;
      } else if (strcmp(argv[argi],"-?") == 0){
        usage_tf = 1;
      }
    }
  }
  if (usage_tf){
    fprintf(stderr,"usage: test_huff [-v ...|-m ...|-f|-z]\n"
      "  -v (number)   variable code, this many numbers\n"
      "  -m (file)     list of code lengths\n"
      "  -f            fixed codes\n"
      "  -z            ascii table\n"
      "  -?            help text\n");
    return 1;
  }
  pngparts_flate_huff_init(&code_table);
  /* acquire codes */switch (mode){
  case 0: /* ascii */
    {
      int i;
      result = pngparts_flate_huff_resize(&code_table, 256);
      if (result != PNGPARTS_API_OK){
        fprintf(stderr,"failed to resize table: %s\n",
            pngparts_api_strerror(result));
        break;
      }
      for (i = 0; i < 256; ++i){
        struct pngparts_flate_code cd =
          pngparts_flate_code_by_literal(i);
        cd.length = 8;
        pngparts_flate_huff_index_set(&code_table,i,cd);
      }
    }break;
  case 1: /* fixed mode */
    {
      int i;
      result = pngparts_flate_huff_resize(&code_table, 288);
      if (result != PNGPARTS_API_OK){
        fprintf(stderr,"failed to resize table: %s\n",
            pngparts_api_strerror(result));
        break;
      }
      for (i = 0; i < 144; ++i){
        struct pngparts_flate_code cd =
          pngparts_flate_code_by_literal(i);
        cd.length = 8;
        pngparts_flate_huff_index_set(&code_table,i,cd);
      }
      for (; i < 256; ++i){
        struct pngparts_flate_code cd =
          pngparts_flate_code_by_literal(i);
        cd.length = 9;
        pngparts_flate_huff_index_set(&code_table,i,cd);
      }
      for (; i < 280; ++i){
        struct pngparts_flate_code cd =
          pngparts_flate_code_by_literal(i);
        cd.length = 7;
        pngparts_flate_huff_index_set(&code_table,i,cd);
      }
      for (; i < 288; ++i){
        struct pngparts_flate_code cd =
          pngparts_flate_code_by_literal(i);
        cd.length = 8;
        pngparts_flate_huff_index_set(&code_table,i,cd);
      }
    }break;
  case 3: /* manual variable code */
    {
      int i = 0;
      FILE *to_read;
      result = pngparts_flate_huff_resize(&code_table, 288);
      if (result != PNGPARTS_API_OK){
        fprintf(stderr,"failed to resize table: %s\n",
            pngparts_api_strerror(result));
        break;
      }
      to_read = fopen(text_informator,"rt");
      if (to_read != NULL){
        char buf[80];
        char *read_buffer;
        while ((read_buffer = fgets(buf,sizeof(buf),to_read)) != NULL){
          int next = atoi(read_buffer);
          struct pngparts_flate_code cd = pngparts_flate_code_by_literal(i);
          cd.length = next;
          pngparts_flate_huff_index_set(&code_table,i,cd);
          i += 1;
          if (i >= 288) break;
        }
        fclose(to_read);
      } else {
        fprintf(stderr,"failed to open file: %s\n",strerror(errno));
        break;
      }
      result = pngparts_flate_huff_resize(&code_table, i);
      if (result != PNGPARTS_API_OK){
        fprintf(stderr,"failed to resize table: %s\n",
            pngparts_api_strerror(result));
        break;
      }
    }break;
  }
  /* generate bits */if (result == PNGPARTS_API_OK){
    result = pngparts_flate_huff_generate(&code_table);
    if (result != PNGPARTS_API_OK){
      fprintf(stderr,"failed to generate table: %s\n",
          pngparts_api_strerror(result));
    }
  }
  /* text output of bits */if (result == PNGPARTS_API_OK){
    int i;
    int const l = pngparts_flate_huff_get_size(&code_table);
    if (l == 0){
      fprintf(stderr,"(no entries)\n");
    } else for (i = 0; i < l; ++i){
      char bitstring[17];
      struct pngparts_flate_code const cd
        = pngparts_flate_huff_index_get(&code_table,i);
      code_to_string(cd,bitstring);
      fprintf(stdout,"%3i : %s\n",cd.value,bitstring);
    }
  }
  pngparts_flate_huff_free(&code_table);
  return result?1:0;
}
