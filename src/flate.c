
#include "flate.h"
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

struct pngparts_flate_code pngparts_flate_code_by_literal(int value){
  struct pngparts_flate_code cd;
  cd.length = 0;
  cd.bits = 0;
  cd.value = value;
  return cd;
}

void pngparts_flate_huff_init(struct pngparts_flate_huff* hf){
  hf->its = NULL;
  hf->count = 0;
}
void pngparts_flate_huff_free(struct pngparts_flate_huff* hf){
  free(hf->its);
  hf->its = NULL;
  hf->count = 0;
}
int pngparts_flate_huff_resize(struct pngparts_flate_huff* hf, int siz){
  if (siz < 0){
    return PNGPARTS_API_BAD_PARAM;
  } else if (siz == 0){
    pngparts_flate_huff_free(hf);
    return PNGPARTS_API_OK;
  } else if (siz >= (INT_MAX/sizeof(struct pngparts_flate_code))){
    return PNGPARTS_API_MEMORY;
  } else if (siz != hf->count){
    void* it = realloc(hf->its,sizeof(struct pngparts_flate_code)*siz);
    if (it != NULL){
      hf->its = it;
      hf->count = siz;
      return PNGPARTS_API_OK;
    } else {
      return PNGPARTS_API_MEMORY;
    }
  } else return PNGPARTS_API_OK;
}
struct pngparts_flate_code pngparts_flate_huff_index_get
  (struct pngparts_flate_huff const* hf, int i)
{
  assert(i >= 0 && i < hf->count);
  return hf->its[i];
}
void pngparts_flate_huff_index_set
  (struct pngparts_flate_huff* hf, int i, struct pngparts_flate_code c)
{
  assert(i >= 0 && i < hf->count);
  hf->its[i] = c;
}
int pngparts_flate_huff_length(struct pngparts_flate_huff const* hf){
  return hf->count;
}
int pngparts_flate_huff_generate(struct pngparts_flate_huff* hf){
  /* each length has string start */
  unsigned int vstring[16] = {
    0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0
  };
  /* pass 1: count */{
    int i;
    struct pngparts_flate_code const* p = hf->its;
    for (i = 0; i < hf->count; ++i, ++p){
      if (p->length < 0 || p->length > 15){
        return PNGPARTS_API_BAD_PARAM;
      } else {
        vstring[p->length] += 1;
      }
    }
  }
  /* pass 2: accumulate */{
    int i;
    unsigned int bitstring = 0, maxxer = 1;
    for (i = 1; i < 16; ++i){
      unsigned int count = vstring[i];
      bitstring <<= 1;
      maxxer <<= 1;
      vstring[i] = bitstring;
      if (count > maxxer
      ||  bitstring > maxxer-count)
      {
        return PNGPARTS_API_CODE_EXCESS;
      } else bitstring += count;
    }
  }
  /* pass 3: bit assign */{
    int i;
    struct pngparts_flate_code* p = hf->its;
    for (i = 0; i < hf->count; ++i, ++p){
      unsigned int bitstring = vstring[p->length];
      p->bits = bitstring;
      vstring[p->length] += 1;
    }
  }
  return PNGPARTS_API_OK;
}
int pngparts_flate_huff_get_size(struct pngparts_flate_huff const* hf){
  return hf->count;
}
void pngparts_flate_huff_make_lengths
  (struct pngparts_flate_huff* hf, int const* hist)
{
  int count = 0;
  int max_level = 0;
  int min_code;
  int code_counts[3];
  int min_level = INT_MAX;
  int code_limits[3];
  long int average = 0;
  /* count nonzero items */if (hf->count > 0){
    int i;
    for (i = 0; i < hf->count; ++i){
      if (hist[i] > 0){
        count += 1;
        average += hist[i];
        if (max_level < hist[i])
          max_level = hist[i];
        if (min_level > hist[i])
          min_level = hist[i];
      }
    }
    average /= count;
  } else return;
  /* compute code lengths needed */{
    code_limits[0] = (int)(average-((average-min_level)>>1));
    code_limits[1] = (int)(average+((max_level-average)>>1));
    code_limits[2] = max_level;
    if (count <= 0){
      code_counts[0] = 0;
      code_counts[1] = 0;
      code_counts[2] = 0;
      min_code = 0;
    } else switch (count){
    case 1:
      min_code = 1;
      code_counts[0] = 1;
      code_counts[1] = 0;
      code_counts[2] = 0;
      break;
    case 2:
      min_code = 1;
      code_counts[0] = 2;
      code_counts[1] = 0;
      code_counts[2] = 0;
      break;
    case 3:
      min_code = 1;
      code_counts[0] = 1;
      code_counts[1] = 2;
      code_counts[2] = 0;
      break;
    default:
      {
        int i;
        min_code = 1;
        code_counts[0] = 1;
        code_counts[1] = 1;
        code_counts[2] = 2;
        for (i = 4; i < count; ++i){
          if (code_counts[0] == 0){
            /* shift across */
            code_counts[0] = code_counts[1];
            code_counts[1] = code_counts[2];
            code_counts[2] = 0;
            min_code += 1;
          }
          if (code_counts[1] > (code_counts[2]/2)){
            code_counts[1] -= 1;
            code_counts[2] += 2;
          } else if (code_counts[0] > 0){
            code_counts[0] -= 1;
            code_counts[1] += 2;
          }
        }
      }break;
    }
  }
  /* assign lengths */{
    int i;
    struct pngparts_flate_code* p = hf->its;
    for (i = 0; i < hf->count; ++i, ++p){
      if (hist[i] > 0){
        int j;
        for (j = 0; j < 3; ++j){
          if (hist[i] > code_limits[j]
          ||  code_counts[j] == 0)
            continue;
          else {
            p->length = min_code+j;
            code_counts[j] -= 1;
            break;
          }
        }
        /* go back if not assigned yet */if (j == 3){
          for (j = 0; j < 3; ++j){
            if (code_counts[j] == 0)
              continue;
            else {
              p->length = min_code+j;
              code_counts[j] -= 1;
              break;
            }
          }
        }
      } else {
        p->length = 0;
      }
    }
  }
  return;
}
