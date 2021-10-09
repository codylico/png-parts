
#include "trns.h"
#include "png.h"
#include <stdlib.h>
#include <string.h>

enum pngparts_trns_mode {
  /* single color alpha */
  PNGPARTS_tRNS_GRAY = 0x4000,
  /* tri color alpha */
  PNGPARTS_tRNS_COLOR = 0xC000,
  /* palette alpha assignment */
  PNGPARTS_tRNS_PALETTE = 0x8000,
  /* mask for array index */
  PNGPARTS_tRNS_INDEX_MASK = 0x3fff,
  /* mask for parse mode */
  PNGPARTS_tRNS_MODE_MASK = 0xc000
};

struct pngparts_trns {
  unsigned short color[3];
  unsigned short pos;
};

static
int pngparts_trns_read_msg
  (struct pngparts_png*, void* cb_data, struct pngparts_png_message* msg);
static
int pngparts_trns_value_msg
  (struct pngparts_trns* trns, struct pngparts_png_message* msg);

//BEGIN tRNS / static
int pngparts_trns_value_msg
  (struct pngparts_trns* trns, struct pngparts_png_message* msg)
{
  if (msg->byte) {
    /* set */
    memcpy(trns->color, msg->ptr, 3*sizeof(unsigned short));
  } else {
    /* get */
    if (trns->pos != 0x4002 && trns->pos != 0xC006)
      return PNGPARTS_API_NOT_READY;
    else memcpy(msg->ptr, trns->color, 3*sizeof(unsigned short));
  }
  return PNGPARTS_API_OK;
}
//END   tRNS / static

//BEGIN tRNS / read
int pngparts_trns_read_msg
  (struct pngparts_png* p, void* cb_data, struct pngparts_png_message* msg)
{
  struct pngparts_trns* const trns = (struct pngparts_trns*)cb_data;
  switch (msg->type) {
  case PNGPARTS_PNG_M_READY:
    if (!(p->flags_tf & PNGPARTS_PNG_IHDR_DONE)){
      return PNGPARTS_API_NOT_READY;
    } else if (p->header.color_type == 3
    &&  p->palette_count <= 0)
    {
      return PNGPARTS_API_NOT_READY;
    } else return PNGPARTS_API_OK;
  case PNGPARTS_PNG_M_START:
    if (msg->byte == 0) {
      trns->color[0] = 0u;
      trns->color[1] = 0u;
      trns->color[2] = 0u;
      if (p->header.color_type & 1) {
        trns->pos = PNGPARTS_tRNS_PALETTE;
      } else if (p->header.color_type & 2) {
        trns->pos = PNGPARTS_tRNS_COLOR;
      } else trns->pos = PNGPARTS_tRNS_GRAY;
      return PNGPARTS_API_OK;
    } else return PNGPARTS_API_BAD_STATE;
  case PNGPARTS_PNG_M_GET:
    /* */{
      int const index = (int)(trns->pos&PNGPARTS_tRNS_INDEX_MASK);
      switch (trns->pos & PNGPARTS_tRNS_MODE_MASK) {
      case PNGPARTS_tRNS_GRAY:
        if (index < 2) {
          trns->color[0] = (trns->color[0]<<8)
            | (((unsigned char)msg->byte)&255u);
          trns->pos += 1;
        }
        if (index == 2) {
          trns->color[1] = trns->color[0];
          trns->color[2] = trns->color[0];
        } break;
      case PNGPARTS_tRNS_COLOR:
        if (index < 6) {
          unsigned int const v = index/2;
          trns->color[v] = (trns->color[v]<<8)
            | (((unsigned char)msg->byte)&255u);
          trns->pos += 1;
        } break;
      case PNGPARTS_tRNS_PALETTE:
        if (index < p->palette_count) {
          p->palette[index].alpha = (msg->byte&255u);
          trns->pos += 1;
        } break;
      default:
        return PNGPARTS_API_BAD_STATE;
      }
    } return PNGPARTS_API_OK;
  case PNGPARTS_PNG_M_FINISH:
    return PNGPARTS_API_OK;
  case PNGPARTS_PNG_M_ALL_DONE:
    return PNGPARTS_API_OK;
  case PNGPARTS_PNG_M_DESTROY:
    free(trns);
    return PNGPARTS_API_OK;
  case PNGPARTS_PNG_M_tRNS_VALUE:
    return pngparts_trns_value_msg(trns, msg);
  default:
    return PNGPARTS_API_BAD_STATE;
  }
}

int pngparts_trns_assign_read_api(struct pngparts_png_chunk_cb* cb) {
  static unsigned char const trns_name[4] = {0x74, 0x52, 0x4E, 0x53};
  struct pngparts_trns* cb_data = (struct pngparts_trns*)calloc
    (1, sizeof(struct pngparts_trns));
  if (cb_data == NULL)
    return PNGPARTS_API_MEMORY;
  cb->cb_data = cb_data;
  memcpy(cb->name, trns_name, 4*sizeof(unsigned char));
  cb->message_cb = pngparts_trns_read_msg;
  return PNGPARTS_API_OK;
}
//END   tRNS / read

//BEGIN tRNS / write
int pngparts_trns_assign_write_api(struct pngparts_png_chunk_cb* cb) {
  return PNGPARTS_API_UNSUPPORTED;
}
//END   tRNS / write
