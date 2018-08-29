
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
