
#include "flate.h"
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

struct pngparts_flate_extra const pngparts_flate_length_table[] = {
  {257,   3,  0}, {258,   4,  0}, {259,   5,  0}, {260,   6,  0},
  {261,   7,  0}, {262,   8,  0}, {263,   9,  0}, {264,  10,  0},
  {265,  11,  1}, {266,  13,  1}, {267,  15,  1}, {268,  17,  1},
  {269,  19,  2}, {270,  23,  2}, {271,  27,  2}, {272,  31,  2},
  {273,  35,  3}, {274,  43,  3}, {275,  51,  3}, {276,  59,  3},
  {277,  67,  4}, {278,  83,  4}, {279,  99,  4}, {280, 115,  4},
  {281, 131,  5}, {282, 163,  5}, {283, 195,  5}, {284, 227,  5},
  {285, 258,  0}
};
struct pngparts_flate_extra const pngparts_flate_distance_table[] = {
  { 0,     1,  0}, { 1,     2,  0}, { 2,     3,  0}, { 3,     4,  0},
  { 4,     5,  1}, { 5,     7,  1}, { 6,     9,  2}, { 7,    13,  2},
  { 8,    17,  3}, { 9,    25,  3}, {10,    33,  4}, {11,    49,  4},
  {12,    65,  5}, {13,    97,  5}, {14,   129,  6}, {15,   193,  6},
  {16,   257,  7}, {17,   385,  7}, {18,   513,  8}, {19,   769,  8},
  {20,  1025,  9}, {21,  1537,  9}, {22,  2049, 10}, {23,  3073, 10},
  {24,  4097, 11}, {25,  6145, 11}, {26,  8193, 12}, {27, 12289, 12},
  {28, 16385, 13}, {29, 24577, 13}
};
struct pngparts_flate_code const pngparts_flate_fixed_d_table[32] = {
  /*   0 */
  { 5,    0,   0}, { 5,   01,   1}, { 5,   02,   2}, { 5,   03,   3},
  { 5,   04,   4}, { 5,   05,   5}, { 5,   06,   6}, { 5,   07,   7},
  { 5,  010,   8}, { 5,  011,   9}, { 5,  012,  10}, { 5,  013,  11},
  { 5,  014,  12}, { 5,  015,  13}, { 5,  016,  14}, { 5,  017,  15},
  /*  16 */
  { 5,  020,  16}, { 5,  021,  17}, { 5,  022,  18}, { 5,  023,  19},
  { 5,  024,  20}, { 5,  025,  21}, { 5,  026,  22}, { 5,  027,  23},
  { 5,  030,  24}, { 5,  031,  25}, { 5,  032,  26}, { 5,  033,  27},
  { 5,  034,  28}, { 5,  035,  29}, { 5,  036,  30}, { 5,  037,  31}
};
struct pngparts_flate_code const pngparts_flate_fixed_l_table[288] = {
  /*   0 */
  { 8,  060,   0}, { 8,  061,   1}, { 8,  062,   2}, { 8,  063,   3},
  { 8,  064,   4}, { 8,  065,   5}, { 8,  066,   6}, { 8,  067,   7},
  { 8,  070,   8}, { 8,  071,   9}, { 8,  072,  10}, { 8,  073,  11},
  { 8,  074,  12}, { 8,  075,  13}, { 8,  076,  14}, { 8,  077,  15},
  /*  16 */
  { 8, 0100,  16}, { 8, 0101,  17}, { 8, 0102,  18}, { 8, 0103,  19},
  { 8, 0104,  20}, { 8, 0105,  21}, { 8, 0106,  22}, { 8, 0107,  23},
  { 8, 0110,  24}, { 8, 0111,  25}, { 8, 0112,  26}, { 8, 0113,  27},
  { 8, 0114,  28}, { 8, 0115,  29}, { 8, 0116,  30}, { 8, 0117,  31},
  /*  32 */
  { 8, 0120,  32}, { 8, 0121,  33}, { 8, 0122,  34}, { 8, 0123,  35},
  { 8, 0124,  36}, { 8, 0125,  37}, { 8, 0126,  38}, { 8, 0127,  39},
  { 8, 0130,  40}, { 8, 0131,  41}, { 8, 0132,  42}, { 8, 0133,  43},
  { 8, 0134,  44}, { 8, 0135,  45}, { 8, 0136,  46}, { 8, 0137,  47},
  /*  48 */
  { 8, 0140,  48}, { 8, 0141,  49}, { 8, 0142,  50}, { 8, 0143,  51},
  { 8, 0144,  52}, { 8, 0145,  53}, { 8, 0146,  54}, { 8, 0147,  55},
  { 8, 0150,  56}, { 8, 0151,  57}, { 8, 0152,  58}, { 8, 0153,  59},
  { 8, 0154,  60}, { 8, 0155,  61}, { 8, 0156,  62}, { 8, 0157,  63},
  /*  64 */
  { 8, 0160,  64}, { 8, 0161,  65}, { 8, 0162,  66}, { 8, 0163,  67},
  { 8, 0164,  68}, { 8, 0165,  69}, { 8, 0166,  70}, { 8, 0167,  71},
  { 8, 0170,  72}, { 8, 0171,  73}, { 8, 0172,  74}, { 8, 0173,  75},
  { 8, 0174,  76}, { 8, 0175,  77}, { 8, 0176,  78}, { 8, 0177,  79},
  /*  80 */
  { 8, 0200,  80}, { 8, 0201,  81}, { 8, 0202,  82}, { 8, 0203,  83},
  { 8, 0204,  84}, { 8, 0205,  85}, { 8, 0206,  86}, { 8, 0207,  87},
  { 8, 0210,  88}, { 8, 0211,  89}, { 8, 0212,  90}, { 8, 0213,  91},
  { 8, 0214,  92}, { 8, 0215,  93}, { 8, 0216,  94}, { 8, 0217,  95},
  /*  96 */
  { 8, 0220,  96}, { 8, 0221,  97}, { 8, 0222,  98}, { 8, 0223,  99},
  { 8, 0224, 100}, { 8, 0225, 101}, { 8, 0226, 102}, { 8, 0227, 103},
  { 8, 0230, 104}, { 8, 0231, 105}, { 8, 0232, 106}, { 8, 0233, 107},
  { 8, 0234, 108}, { 8, 0235, 109}, { 8, 0236, 110}, { 8, 0237, 111},
  /* 112 */
  { 8, 0240, 112}, { 8, 0241, 113}, { 8, 0242, 114}, { 8, 0243, 115},
  { 8, 0244, 116}, { 8, 0245, 117}, { 8, 0246, 118}, { 8, 0247, 119},
  { 8, 0250, 120}, { 8, 0251, 121}, { 8, 0252, 122}, { 8, 0253, 123},
  { 8, 0254, 124}, { 8, 0255, 125}, { 8, 0256, 126}, { 8, 0257, 127},
  /* 128 */
  { 8, 0260, 128}, { 8, 0261, 129}, { 8, 0262, 130}, { 8, 0263, 131},
  { 8, 0264, 132}, { 8, 0265, 133}, { 8, 0266, 134}, { 8, 0267, 135},
  { 8, 0270, 136}, { 8, 0271, 137}, { 8, 0272, 138}, { 8, 0273, 139},
  { 8, 0274, 140}, { 8, 0275, 141}, { 8, 0276, 142}, { 8, 0277, 143},
  /* 144 */
  { 9, 0620, 144}, { 9, 0621, 145}, { 9, 0622, 146}, { 9, 0623, 147},
  { 9, 0624, 148}, { 9, 0625, 149}, { 9, 0626, 150}, { 9, 0627, 151},
  { 9, 0630, 152}, { 9, 0631, 153}, { 9, 0632, 154}, { 9, 0633, 155},
  { 9, 0634, 156}, { 9, 0635, 157}, { 9, 0636, 158}, { 9, 0637, 159},
  /* 160 */
  { 9, 0640, 160}, { 9, 0641, 161}, { 9, 0642, 162}, { 9, 0643, 163},
  { 9, 0644, 164}, { 9, 0645, 165}, { 9, 0646, 166}, { 9, 0647, 167},
  { 9, 0650, 168}, { 9, 0651, 169}, { 9, 0652, 170}, { 9, 0653, 171},
  { 9, 0654, 172}, { 9, 0655, 173}, { 9, 0656, 174}, { 9, 0657, 175},
  /* 176 */
  { 9, 0660, 176}, { 9, 0661, 177}, { 9, 0662, 178}, { 9, 0663, 179},
  { 9, 0664, 180}, { 9, 0665, 181}, { 9, 0666, 182}, { 9, 0667, 183},
  { 9, 0670, 184}, { 9, 0671, 185}, { 9, 0672, 186}, { 9, 0673, 187},
  { 9, 0674, 188}, { 9, 0675, 189}, { 9, 0676, 190}, { 9, 0677, 191},
  /* 192 */
  { 9, 0700, 192}, { 9, 0701, 193}, { 9, 0702, 194}, { 9, 0703, 195},
  { 9, 0704, 196}, { 9, 0705, 197}, { 9, 0706, 198}, { 9, 0707, 199},
  { 9, 0710, 200}, { 9, 0711, 201}, { 9, 0712, 202}, { 9, 0713, 203},
  { 9, 0714, 204}, { 9, 0715, 205}, { 9, 0716, 206}, { 9, 0717, 207},
  /* 208 */
  { 9, 0720, 208}, { 9, 0721, 209}, { 9, 0722, 210}, { 9, 0723, 211},
  { 9, 0724, 212}, { 9, 0725, 213}, { 9, 0726, 214}, { 9, 0727, 215},
  { 9, 0730, 216}, { 9, 0731, 217}, { 9, 0732, 218}, { 9, 0733, 219},
  { 9, 0734, 220}, { 9, 0735, 221}, { 9, 0736, 222}, { 9, 0737, 223},
  /* 224 */
  { 9, 0740, 224}, { 9, 0741, 225}, { 9, 0742, 226}, { 9, 0743, 227},
  { 9, 0744, 228}, { 9, 0745, 229}, { 9, 0746, 230}, { 9, 0747, 231},
  { 9, 0750, 232}, { 9, 0751, 233}, { 9, 0752, 234}, { 9, 0753, 235},
  { 9, 0754, 236}, { 9, 0755, 237}, { 9, 0756, 238}, { 9, 0757, 239},
  /* 240 */
  { 9, 0760, 240}, { 9, 0761, 241}, { 9, 0762, 242}, { 9, 0763, 243},
  { 9, 0764, 244}, { 9, 0765, 245}, { 9, 0766, 246}, { 9, 0767, 247},
  { 9, 0770, 248}, { 9, 0771, 249}, { 9, 0772, 250}, { 9, 0773, 251},
  { 9, 0774, 252}, { 9, 0775, 253}, { 9, 0776, 254}, { 9, 0777, 255},
  /* 256 */
  { 7,    0, 256}, { 7,   01, 257}, { 7,   02, 258}, { 7,   03, 259},
  { 7,   04, 260}, { 7,   05, 261}, { 7,   06, 262}, { 7,   07, 263},
  { 7,  010, 264}, { 7,  011, 265}, { 7,  012, 266}, { 7,  013, 267},
  { 7,  014, 268}, { 7,  015, 269}, { 7,  016, 270}, { 7,  017, 271},
  /* 272 */
  { 7,  020, 272}, { 7,  021, 273}, { 7,  022, 274}, { 7,  023, 275},
  { 7,  024, 276}, { 7,  025, 277}, { 7,  026, 278}, { 7,  027, 279},
  { 8, 0300, 280}, { 8, 0301, 281}, { 8, 0302, 282}, { 8, 0303, 283},
  { 8, 0304, 284}, { 8, 0305, 285}, { 8, 0306, 286}, { 8, 0307, 287},
  /* 288 */
};

static int pngparts_flate_code_valuecmp(void const* va, void const* vb);

/*
 * Fill bits after the most-significant set bit.
 * - x input integer
 * @return x with its bits filled
 */
static unsigned int pngparts_flate_hash_postfill(unsigned int x);

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
  hf->cap = 0;
}
void pngparts_flate_huff_free(struct pngparts_flate_huff* hf){
  free(hf->its);
  hf->its = NULL;
  hf->count = 0;
  hf->cap = 0;
}
int pngparts_flate_huff_resize(struct pngparts_flate_huff* hf, int siz){
  if (siz < 0){
    return PNGPARTS_API_BAD_PARAM;
  } else if (siz >= (INT_MAX/sizeof(struct pngparts_flate_code))){
    return PNGPARTS_API_MEMORY;
  } else if (siz < hf->cap){
    hf->count = siz;
    return PNGPARTS_API_OK;
  } else if (siz != hf->count){
    void* it = realloc(hf->its,sizeof(struct pngparts_flate_code)*siz);
    if (it != NULL){
      hf->its = it;
      hf->count = siz;
      hf->cap = siz;
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
        return PNGPARTS_API_BAD_CODE_LENGTH;
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
    if (count > 0) average /= count;
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

int pngparts_flate_code_bitcmp(void const* va, void const* vb){
  struct pngparts_flate_code const* a =
    (struct pngparts_flate_code const*)va;
  struct pngparts_flate_code const* b =
    (struct pngparts_flate_code const*)vb;
  if (a->length < b->length) return -1;
  else if (a->length > b->length) return +1;
  else if (a->bits < b->bits) return -1;
  else if (a->bits > b->bits) return +1;
  else return 0;
}
int pngparts_flate_code_valuecmp(void const* va, void const* vb){
  struct pngparts_flate_code const* a =
    (struct pngparts_flate_code const*)va;
  struct pngparts_flate_code const* b =
    (struct pngparts_flate_code const*)vb;
  if (a->value < b->value) return -1;
  else if (a->value > b->value) return +1;
  else return 0;
}
void pngparts_flate_huff_bit_sort(struct pngparts_flate_huff* hf){
  /* sort by lengths */
  qsort(hf->its,hf->count,sizeof(struct pngparts_flate_code),
      pngparts_flate_code_bitcmp);
  return;
}
void pngparts_flate_huff_value_sort(struct pngparts_flate_huff* hf){
  /* sort by lengths */
  qsort(hf->its,hf->count,sizeof(struct pngparts_flate_code),
      pngparts_flate_code_valuecmp);
  return;
}
int pngparts_flate_huff_bit_bsearch
  (struct pngparts_flate_huff const* hf, int length, int bits)
{
  if (length <= 0 || length > 15) return PNGPARTS_API_BAD_BITS;
  /* sorted */{
    int start = 0;
    int stop = hf->count;
    /* binary search */
    while (start < stop){
      int mid = ((stop-start)>>1)+start;
      int const midbits = hf->its[mid].bits;
      int const midlength = hf->its[mid].length;
      if (midbits == bits && midlength == length)
        return hf->its[mid].value;
      else if (midlength > length)
        stop = mid;
      else if (midlength < length)
        start = mid+1;
      else if (midbits > bits)
        stop = mid;
      else start = mid+1;
    }
  }
  /* not found */
  return PNGPARTS_API_NOT_FOUND;
}
int pngparts_flate_huff_bit_lsearch
  (struct pngparts_flate_huff const* hf, int length, int bits)
{
  /* unsorted */
  int i;
  if (length <= 0 || length > 15) return PNGPARTS_API_BAD_BITS;
  for (i = 0; i < hf->count; ++i){
    if (hf->its[i].length == length
    &&  hf->its[i].bits == bits)
      return hf->its[i].value;
  }
  /* not found */
  return PNGPARTS_API_NOT_FOUND;
}
void pngparts_flate_fixed_lengths(struct pngparts_flate_huff* hf){
  assert(hf->count>=288);
  memcpy(hf->its,pngparts_flate_fixed_l_table,
      288*sizeof(struct pngparts_flate_code));
}
void pngparts_flate_fixed_distances(struct pngparts_flate_huff* hf){
  assert(hf->count>=32);
  memcpy(hf->its,pngparts_flate_fixed_d_table,
      32*sizeof(struct pngparts_flate_code));
}
void pngparts_flate_dynamic_codes(struct pngparts_flate_huff* hf){
  int i;
  int const static clengths[19] = {
    16, 17, 18,  0,  8,  7,  9,  6, 10,
     5, 11,  4, 12,  3, 13,  2, 14,  1,
    15
  };
  for (i = 0; i < 19 && i < hf->count; ++i){
    hf->its[i].value = clengths[i];
  }
  return;
}

struct pngparts_flate_extra pngparts_flate_length_decode(int literal){
  struct pngparts_flate_extra const out =
    {literal,PNGPARTS_API_NOT_FOUND,PNGPARTS_API_NOT_FOUND};
  if (literal >= 257 && literal <= 285){
    return pngparts_flate_length_table[literal-257];
  }
  return out;
}
struct pngparts_flate_extra pngparts_flate_distance_decode(int dcode){
  struct pngparts_flate_extra const out =
    {dcode,PNGPARTS_API_NOT_FOUND,PNGPARTS_API_NOT_FOUND};
  if (dcode >= 0 && dcode <= 29){
    return pngparts_flate_distance_table[dcode];
  }
  return out;
}

void pngparts_flate_history_add(struct pngparts_flate *fl, int ch){
  fl->history_bytes[fl->history_pos] = (unsigned char)(ch&255);
  fl->history_pos += 1;
  if (fl->history_pos == fl->history_size)
    fl->history_pos = 0;
  return;
}
int pngparts_flate_history_get(struct pngparts_flate *fl, int dist){
  unsigned int repos;
  if (dist > fl->history_pos)
    repos = fl->history_size-dist+fl->history_pos;
  else
    repos = fl->history_pos-dist;
  if (repos >= fl->history_size) return 0;
  return fl->history_bytes[repos]&255;
}

void pngparts_flate_hash_init(struct pngparts_flate_hash *hash){
  hash->first = NULL;
  hash->next = NULL;
  hash->next_size = 0;
  hash->byte_size = 0;
  hash->first_max = 0;
  return;
}

void pngparts_flate_hash_free(struct pngparts_flate_hash *hash){
  free(hash->first);
  free(hash->next);
  hash->first = NULL;
  hash->next = NULL;
  hash->next_size = 0;
  hash->byte_size = 0;
  hash->first_max = 0;
  return;
}

unsigned int pngparts_flate_hash_postfill(unsigned int x){
  if (x == 0u)
    return 1u;
  else {
    unsigned int last_bit_point = 0;
    unsigned int bit_point;
    unsigned int out;
    /* detect the last bit point */{
      unsigned int fill_point = 1u;
      for (bit_point = 0; bit_point < 64; ++bit_point){
        if (fill_point == 0)
          break;
        if (x&fill_point){
          last_bit_point = bit_point;
        }
        fill_point <<= 1;
      }
    }
    /* correct upto the last bit point */{
      out = (1u<<(last_bit_point+1))-1;
    }
    return out;
  }
}

int pngparts_flate_hash_prepare
  (struct pngparts_flate_hash *hash, unsigned int size)
{
  if (size > 32768u){
    /* say no by */return/*ing */ PNGPARTS_API_MEMORY;
  } else {
    unsigned int const first_size =
      pngparts_flate_hash_postfill((size>>7)-1u)+1u;
    unsigned short* new_first = (unsigned short*)malloc
      (sizeof(unsigned short)*first_size);
    unsigned short* new_next = (unsigned short*)malloc
      (sizeof(unsigned short)*size);
    if (new_first == NULL || new_next == NULL){
      free(new_first);
      free(new_next);
      return PNGPARTS_API_MEMORY;
    }
    /* fill the first and next structures */{
      unsigned int i;
      for (i = 0; i < size; ++i){
        new_next[i] = USHRT_MAX;
      }
      for (i = 0; i < first_size; ++i){
        new_first[i] = USHRT_MAX;
      }
    }
    /* configure the structure */
    free(hash->first);
    free(hash->next);
    hash->first = new_first;
    hash->next = new_next;
    hash->next_size = (unsigned short)size;
    hash->pos = 0;
    hash->bytes[0] = 0;
    hash->bytes[1] = 0;
    hash->byte_size = 0;
    hash->first_max = (unsigned char)(first_size-1);
    return PNGPARTS_API_OK;
  }
}

void pngparts_flate_hash_add(struct pngparts_flate_hash *hash, int ch){
  switch (hash->byte_size){
  case 0:
    hash->bytes[0] = (unsigned char)(ch&255);
    hash->byte_size = 1;
    break;
  case 1:
    hash->bytes[1] = (unsigned char)(ch&255);
    hash->byte_size = 2;
    break;
  default:
    {
      unsigned char const ch_value = (unsigned char)(ch&255);
      unsigned char const hash_key =
        (hash->bytes[0]^hash->bytes[1]^ch_value)&(hash->first_max);
      /* add current hash value to the table */{
        hash->next[hash->pos] = hash->first[hash_key];
        hash->first[hash_key] = hash->pos;
        hash->pos = (hash->pos+1)%(hash->next_size);
      }
      /* shift the working space */{
        hash->bytes[0] = hash->bytes[1];
        hash->bytes[1] = ch_value;
      }
    }break;
  }
  return;
}

unsigned int pngparts_flate_hash_check
  ( struct pngparts_flate_hash *hash, unsigned char const* history_bytes,
    unsigned char const* chs, unsigned int start)
{
  unsigned int history_out = 0;
  unsigned int const adjusted_pos = hash->pos + 2;
  /* hash it */
  unsigned char const hash_key =
    (chs[0]^chs[1]^chs[2])&(hash->first_max);
  /* inspect the hash */{
    unsigned int trouble_count = 0;
    unsigned short *prev_point;
    unsigned int const hash_size = hash->next_size;
    if (start == 0){
      /* use the first pointer as the previous pointer */
      prev_point = hash->first+hash_key;
    } else {
      /* compute the location of the previous pointer */
      unsigned int history_in = start;
      if (history_in > adjusted_pos){
        history_in -= hash->next_size;
      }
      history_in = (adjusted_pos) - history_in;
      prev_point = hash->next + history_in;
    }
    /* traverse the list */
    for (trouble_count = 0; trouble_count < hash_size; ++trouble_count){
      unsigned int current_point;
      unsigned int cp1, cp2;
      /* fetch the current value from the previous point */
      current_point = *prev_point;
      if (current_point == USHRT_MAX)
        break;
      cp1 = (current_point+1)%(hash->next_size);
      cp2 = (current_point+2)%(hash->next_size);
      /* inspect the history */
      if (history_bytes[current_point] == chs[0]
      &&  history_bytes[cp1] == chs[1]
      &&  history_bytes[cp2] == chs[2])
      {
        /* compute the historical position */
        history_out = adjusted_pos - current_point;
        if (history_out >= hash->next_size)
          history_out += hash->next_size;
        break;
      } else /* otherwise retrogress through history */{
        /* check the hash key */
        unsigned char const past_hash_key =
          (history_bytes[current_point]^history_bytes[cp1]^history_bytes[cp2])
          & (hash->first_max);
        if (past_hash_key != hash_key){
          /* cut the linked list here */
          *prev_point = USHRT_MAX;
          break;
        } else {
          /* transition to the next point */
          prev_point = hash->next+current_point;
          continue;
        }
      }
    }/* end for trouble_count */
  }
  /* clamp at history size */if (history_out > hash->next_size){
    return 0;
  }
  /* done */
  return history_out;
}
