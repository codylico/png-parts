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
#include <time.h>

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
  int mode = -1;
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
        }
      } else if (strcmp(argv[argi],"-m") == 0){
        /* variable code: manual */
        if (++argi < argc){
          mode = 3;
          text_informator = argv[argi];
        }
      } else if (strcmp(argv[argi],"-g") == 0){
        /* variable code: histogram */
        if (++argi < argc){
          mode = 4;
          text_informator = argv[argi];
        }
      } else if (strcmp(argv[argi],"-z") == 0){
        /* ascii mode */
        mode = 0;
      } else if (strcmp(argv[argi],"-s") == 0){
        /* seed */
        int v = -1;
        if (++argi < argc){
          if (strcmp(argv[argi],"-") == 0)
            v = -1;
          else
            v = atoi(argv[argi]);
        }
        if (v >= 0){
          srand(v);
        } else {
          srand(time(NULL));
        }
      } else if (strcmp(argv[argi],"-?") == 0){
        usage_tf = 1;
      }
    }
  }
  if (usage_tf || mode == -1){
    fprintf(stderr,"usage: test_huff (-v ...|-m ...|-h ...|-f|-z) [-s ...]\n"
      "  -v (number)   variable code, this many numbers\n"
      "  -m (file)     list of code lengths\n"
      "  -g (file)     histogram of code frequencies\n"
      "  -f            fixed codes\n"
      "  -s (seed)     random seed\n"
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
  case 2: /* automatic variable code */
    {
      int i = 0;
      int size = atoi(text_informator);
      int *histogram;
      if (size < 0) size = 0;
      else if (size > 288){
        size = 288;
        fprintf(stderr,"stop at 288 lines\n");
      }
      histogram = calloc(sizeof(int),size);
      if (histogram == NULL){
        fprintf(stderr, "failed to create histogram list\n");
        result = PNGPARTS_API_MEMORY;
        break;
      }
      result = pngparts_flate_huff_resize(&code_table, size);
      if (result != PNGPARTS_API_OK){
        free(histogram);
        fprintf(stderr,"failed to resize table: %s\n",
            pngparts_api_strerror(result));
        break;
      }
      /* fill with random histogram */for (i = 0; i < size; ++i){
        struct pngparts_flate_code cd = pngparts_flate_code_by_literal(i);
        histogram[i] = ((int)(rand()%100));
        pngparts_flate_huff_index_set(&code_table,i,cd);
      }
      pngparts_flate_huff_make_lengths(&code_table, histogram);
      free(histogram);
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
  case 4: /* histogram variable code */
    {
      int i = 0;
      FILE *to_read;
      int *histogram = calloc(sizeof(int),288);
      if (histogram == NULL){
        fprintf(stderr, "failed to create histogram list\n");
        break;
      }
      result = pngparts_flate_huff_resize(&code_table, 288);
      if (result != PNGPARTS_API_OK){
        free(histogram);
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
          histogram[i] = next;
          pngparts_flate_huff_index_set(&code_table,i,cd);
          i += 1;
          if (i >= 288){
            fprintf(stderr,"stop at 288 lines\n");
            break;
          }
        }
        fclose(to_read);
      } else {
        fprintf(stderr,"failed to open file: %s\n",strerror(errno));
        free(histogram);
        break;
      }
      result = pngparts_flate_huff_resize(&code_table, i);
      if (result != PNGPARTS_API_OK){
        fprintf(stderr,"failed to resize table: %s\n",
            pngparts_api_strerror(result));
        free(histogram);
        break;
      }
      pngparts_flate_huff_make_lengths(&code_table, histogram);
      free(histogram);
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
