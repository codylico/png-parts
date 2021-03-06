
#include "png.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

struct pngparts_png_chunk_link {
  struct pngparts_png_chunk_cb cb;
  struct pngparts_png_chunk_link *next;
};
static unsigned long int const pngparts_png_crc32_pre[256] = {
  /*   0 */
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
  0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
  0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
  0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
  /*  16 */
  0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
  0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
  0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
  /*  32 */
  0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
  0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
  0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
  0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  /*  48 */
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
  0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
  0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
  0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
  /*  64 */
  0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
  0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
  0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
  /*  80 */
  0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
  0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
  0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
  0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  /*  96 */
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
  0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
  0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
  0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
  /* 112 */
  0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
  0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
  0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
  /* 128 */
  0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
  0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
  0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
  0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  /* 144 */
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
  0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
  0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
  0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
  /* 160 */
  0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
  0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
  0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
  /* 176 */
  0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
  0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
  0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
  0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  /* 192 */
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
  0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
  0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
  0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
  /* 208 */
  0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
  0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
  0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
  /* 224 */
  0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
  0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
  0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
  0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  /* 240 */
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
  0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
  0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
  0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};


unsigned char const* pngparts_png_signature(void){
  static const unsigned char sig[9] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0
  };
  return sig;
}


void pngparts_png_adam7_reverse_xy
  (int level, long int *dx, long int *dy, long int sx, long int sy)
{
  switch (level) {
  case 1:
    *dx = sx * 8;
    *dy = sy * 8;
    break;
  case 2:
    *dx = sx * 8 + 4;
    *dy = sy * 8;
    break;
  case 3:
    *dx = sx * 4;
    *dy = sy * 8 + 4;
    break;
  case 4:
    *dx = sx * 4 + 2;
    *dy = sy * 4;
    break;
  case 5:
    *dx = sx * 2;
    *dy = sy * 4 + 2;
    break;
  case 6:
    *dx = sx * 2 + 1;
    *dy = sy * 2;
    break;
  case 7:
    *dx = sx * 1;
    *dy = sy * 2 + 1;
    break;
  default:
    *dx = sx;
    *dy = sy;
    break;
  }
}

struct pngparts_png_size pngparts_png_adam7_pass_size
  (unsigned long int width, unsigned long int height, int level)
{
  struct pngparts_png_size out;
  switch (level) {
  case 7:
    out.width = width;
    out.height = height/2;
    break;
  case 6:
    out.width = (width / 2);
    out.height = (height+1) / 2;
    break;
  case 5:
    out.width = ((width + 1) / 2);
    out.height = (height +1)/ 4;
    break;
  case 4:
    out.width = ((width + 1) / 4);
    out.height = (height + 3) / 4;
    break;
  case 3:
    out.width = ((width + 3) / 4);
    out.height = (height + 3) / 8;
    break;
  case 2:
    out.width = ((width + 3) / 8);
    out.height = (height + 7) / 8;
    break;
  case 1:
    out.width = ((width + 7) / 8);
    out.height = (height + 7) / 8;
    break;
  default:
    out.width = width;
    out.height = height;
    break;
  }
  return out;
}

int pngparts_png_paeth_predict(int left, int up, int corner) {
  int const p = left + up - corner;
  int const pa = abs(p - left);
  int const pb = abs(p - up);
  int const pc = abs(p - corner);
  if (pa <= pb && pa <= pc) return left;
  else if (pb <= pc) return up;
  else return corner;
}

int pngparts_png_header_is_valid(struct pngparts_png_header hdr) {
  if (hdr.compression != 0) return 0;
  if (hdr.filter != 0) return 0;
  if (hdr.interlace != 0
    && hdr.interlace != 1)
    return 0;
  if (hdr.width > 0x7fffFFFF) return 0;
  if (hdr.height > 0x7fffFFFF) return 0;
  switch (hdr.color_type) {
  case 0: /* L */
    {
      if (hdr.bit_depth != 1
      &&  hdr.bit_depth != 2
      &&  hdr.bit_depth != 4
      &&  hdr.bit_depth != 8
      &&  hdr.bit_depth != 16)
        return 0;
    }break;
  case 2: /* RGB */
  case 4: /* LA */
  case 6: /* RGBA */
    {
      if (hdr.bit_depth != 8
      &&  hdr.bit_depth != 16)
        return 0;
    }break;
  case 3: /* index */
    {
      if (hdr.bit_depth != 1
      &&  hdr.bit_depth != 2
      &&  hdr.bit_depth != 4
      &&  hdr.bit_depth != 8)
        return 0;
    }break;
  default:
    return 0;
    break;
  }
  return 1;
}


struct pngparts_png_crc32 pngparts_png_crc32_new(void){
  struct pngparts_png_crc32 out = { 0xFFFFFFFF };
  return out;
}
unsigned long int pngparts_png_crc32_tol(struct pngparts_png_crc32 chk){
  return chk.accum^0xFFFFFFFF;
}
struct pngparts_png_crc32 pngparts_png_crc32_accum
  (struct pngparts_png_crc32 chk, int ch)
{
#if (__STDC_VERSION__ >= 199901L)
  struct pngparts_png_crc32 const out =
    {(chk.accum>>8)^(pngparts_png_crc32_pre[(chk.accum^ch)&255])};
#else
  struct pngparts_png_crc32 out;
  out.accum =
    (chk.accum>>8)^(pngparts_png_crc32_pre[(chk.accum^ch)&255]);
#endif /*__STDC_VERSION__*/
  return out;
}
void pngparts_png_buffer_setup
  (struct pngparts_png *p, void* buf, int size)
{
  p->buf = buf;
  p->size = size;
  p->pos = 0;
  return;
}

int pngparts_png_buffer_done(struct pngparts_png const * p){
  return p->pos == p->size || p->state == 4;
}

int pngparts_png_buffer_used(struct pngparts_png const* p){
  return p->pos;
}

void pngparts_png_set_image_cb
  (struct pngparts_png* p, struct pngparts_api_image const* img_cb)
{
  memcpy(&p->img_cb, img_cb, sizeof(*img_cb));
  return;
}
void pngparts_png_get_image_cb
  (struct pngparts_png const* p, struct pngparts_api_image* img_cb)
{
  memcpy(img_cb, &p->img_cb, sizeof(*img_cb));
  return;
}
int pngparts_png_add_chunk_cb
  (struct pngparts_png* p, struct pngparts_png_chunk_cb const* cb)
{
  struct pngparts_png_chunk_link *new_link =
    (struct pngparts_png_chunk_link *)malloc
      (sizeof(struct pngparts_png_chunk_link));
  if (new_link != NULL) {
    memcpy(&new_link->cb, cb, sizeof(struct pngparts_png_chunk_cb));
    new_link->next = p->chunk_cbs;
    p->chunk_cbs = new_link;
    return PNGPARTS_API_OK;
  } else return PNGPARTS_API_MEMORY;
}
int pngparts_png_send_chunk_msg
  ( struct pngparts_png *p, struct pngparts_png_chunk_cb const* cb,
    struct pngparts_png_message* msg)
{
  int result;
  result = (*cb->message_cb)(p, cb->cb_data, msg);
  return result;
}
struct pngparts_png_chunk_cb const* pngparts_png_find_chunk_cb
  ( struct pngparts_png *p, unsigned char const* name)
{
  struct pngparts_png_chunk_link const* link_ptr;
  link_ptr = p->chunk_cbs;
  while (link_ptr != NULL) {
    if (memcmp(link_ptr->cb.name, name, 4 * sizeof(unsigned char)) == 0)
      break;
    else
      link_ptr = link_ptr->next;
  }
  return link_ptr != NULL ? &link_ptr->cb : NULL;
}

struct pngparts_png_chunk_cb const* pngparts_png_find_ready_cb
  (struct pngparts_png *p)
{
  struct pngparts_png_chunk_link const* link_ptr;
  link_ptr = p->chunk_cbs;
  while (link_ptr != NULL) {
    struct pngparts_png_message message;
    int result;
    message.byte = 0;
    memcpy(message.name, link_ptr->cb.name, 4 * sizeof(unsigned char));
    message.ptr = NULL;
    message.type = PNGPARTS_PNG_M_READY;
    result = pngparts_png_send_chunk_msg(p, &link_ptr->cb, &message);
    if (result == PNGPARTS_API_OK)
      break;
    else
      link_ptr = link_ptr->next;
  }
  return link_ptr != NULL ? &link_ptr->cb : NULL;
}

void pngparts_png_remove_chunk_cb
  (struct pngparts_png* p, unsigned char const* name)
{
  struct pngparts_png_chunk_link * link_ptr;
  struct pngparts_png_chunk_link ** link_2ptr;
  link_ptr = p->chunk_cbs;
  link_2ptr = &p->chunk_cbs;
  while (link_ptr != NULL) {
    if (memcmp(link_ptr->cb.name, name, 4 * sizeof(unsigned char)) == 0) {
      struct pngparts_png_message message;
      *link_2ptr = link_ptr->next;
      message.byte = 0;
      memcpy(message.name, name, 4 * sizeof(unsigned char));
      message.ptr = NULL;
      message.type = PNGPARTS_PNG_M_DESTROY;
      pngparts_png_send_chunk_msg(p, &link_ptr->cb, &message);
      free(link_ptr);
      return;
    } else {
      /* update write-back pointer */
      link_2ptr = &link_ptr->next;
      /* update current link */
      link_ptr = link_ptr->next;
    }
  }
  return;
}
void pngparts_png_drop_chunk_cbs(struct pngparts_png* p) {
  struct pngparts_png_chunk_link* link_ptr;
  link_ptr = p->chunk_cbs;
  while (link_ptr != NULL) {
    struct pngparts_png_chunk_link* hold_ptr = link_ptr;
    link_ptr = link_ptr->next;
    {
      struct pngparts_png_message message;
      message.byte = 0;
      memcpy(message.name, hold_ptr->cb.name, 4 * sizeof(unsigned char));
      message.ptr = NULL;
      message.type = PNGPARTS_PNG_M_DESTROY;
      pngparts_png_send_chunk_msg(p, &hold_ptr->cb, &message);
      free(hold_ptr);
    }
  }
  return;
}
int pngparts_png_broadcast_chunk_msg
  (struct pngparts_png *p, struct pngparts_png_message const* msg)
{
  struct pngparts_png_chunk_link* link_ptr;
  int first_result = PNGPARTS_API_OK;
  link_ptr = p->chunk_cbs;
  while (link_ptr != NULL) {
    struct pngparts_png_chunk_link* hold_ptr = link_ptr;
    link_ptr = link_ptr->next;
    {
      struct pngparts_png_message message;
      int result;
      memcpy(&message, msg, sizeof(message));
      memcpy(message.name, hold_ptr->cb.name, 4 * sizeof(unsigned char));
      result = pngparts_png_send_chunk_msg(p, &hold_ptr->cb, &message);
      if (result < 0
      &&  first_result == PNGPARTS_API_OK)
        first_result = result;
    }
  }
  return first_result;
}

long int pngparts_png_chunk_remaining(struct pngparts_png const* p) {
  return p->chunk_size;
}

int pngparts_png_set_chunk_size(struct pngparts_png* p, long int size){
  if (p->flags_tf & PNGPARTS_PNG_CHUNK_RW){
    if (size >= 0 && size < 0x7fffFFFF){
      p->chunk_size = (unsigned long int)size;
      return PNGPARTS_API_OK;
    } else return PNGPARTS_API_CHUNK_TOO_LONG;
  } else return PNGPARTS_API_BAD_STATE;
}

void pngparts_png_set_plte_item
  (struct pngparts_png* p, int i, struct pngparts_png_plte_item v)
{
  p->palette[i] = v;
  return;
}
struct pngparts_png_plte_item pngparts_png_get_plte_item
  (struct pngparts_png const* p, int i)
{
  return p->palette[i];
}

int pngparts_png_nearest_plte_item
  (struct pngparts_png const* p, struct pngparts_png_plte_item color)
{
  int out = -1;
  int out_diff = INT_MAX;
  int const color_red = color.red;
  int const color_green = color.green;
  int const color_blue = color.blue;
  int const color_alpha = color.alpha;
  int i;
  for (i = 0; i < p->palette_count; ++i){
    struct pngparts_png_plte_item const item = p->palette[i];
    int const red_diff   = abs(item.red-color_red);
    int const green_diff = abs(item.green-color_green);
    int const blue_diff  = abs(item.blue-color_blue);
    int const alpha_diff = abs(item.alpha-color_alpha);
    int const total_diff = red_diff + green_diff + blue_diff + alpha_diff;
    if (total_diff < out_diff){
      out_diff = total_diff;
      out = i;
    }
  }
  return out;
}

int pngparts_png_get_plte_size(struct pngparts_png const* p) {
  return p->palette_count;
}
int pngparts_png_set_plte_size(struct pngparts_png* p, int siz) {
  if (p->palette_count != siz) {
    if (siz == 0) {
      free(p->palette);
      p->palette = NULL;
      return PNGPARTS_API_OK;
    } else if (siz < 0 || siz > 256) {
      return PNGPARTS_API_BAD_PARAM;
    } else {
      struct pngparts_png_plte_item* new_palette =
        realloc(p->palette, siz * sizeof(struct pngparts_png_plte_item));
      if (new_palette != NULL) {
        p->palette = new_palette;
        p->palette_count = siz;
        return PNGPARTS_API_OK;
      } else {
        return PNGPARTS_API_MEMORY;
      }
    }
  } else return PNGPARTS_API_OK;
}
