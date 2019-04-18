/*
 * PNG-parts
 * parts of a Portable Network Graphics implementation
 * Copyright 2018-2019 Cody Licorish
 *
 * Licensed under the MIT License.
 *
 * pngwrite.c
 * PNG writer source
 */
#include "pngwrite.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#if !(defined PNGPARTS_PNGWRITE_CHUNK_SIZE)
#define PNGPARTS_PNGWRITE_CHUNK_SIZE 7000
#endif /*PNGPARTS_PNGWRITE_CHUNK_SIZE*/

struct pngparts_pngwrite_idat;

/*
 * Put a 16-bit integer to byte storage.
 * - b the byte storage into which to write
 * - w the value to put
 */
static void pngparts_pngwrite_put16clamp16(unsigned char* b, unsigned int w);

/*
 * Put a 16-bit integer to single byte storage.
 * - b the byte storage into which to write
 * - w the value to put, converted from [0,65535] to [0,255]
 */
static void pngparts_pngwrite_put8clamp16(unsigned char* b, unsigned int w);

static void pngparts_pngwrite_put32(unsigned char* b, unsigned long int w);
static int pngparts_pngwrite_signal_finish(struct pngparts_png* p);

/*
 * Start a new interlace section.
 * - p the png writer state structure
 * - idat the IDAT chunk generator structure
 * @return OK on success, OVERFLOW if the line is empty, or
 *   MEMORY if memory is not available from allocation
 */
static int pngparts_pngwrite_start_line
  (struct pngparts_png* p, struct pngparts_pngwrite_idat* idat);

/*
 * Generate a single chunk of data.
 * - p the png writer state structure
 * - idat the IDAT chunk generator structure
 * @return an API value
 */
static int pngparts_pngwrite_generate_chunk
  (struct pngparts_png* p, struct pngparts_pngwrite_idat* idat);

/*
 * Fetch color data and compose a string of samples.
 * - p the png writer state structure
 * - idat the IDAT chunk generator structure
 */
static void pngparts_pngwrite_idat_fetch
  (struct pngparts_png* p, struct pngparts_pngwrite_idat* idat);

void pngparts_pngwrite_put16clamp16(unsigned char* b, unsigned int w) {
  b[0] = (w>> 8)&255;
  b[1] = (w>> 0)&255;
  return;
}

void pngparts_pngwrite_put8clamp16(unsigned char* b, unsigned int w) {
  b[0] = (w&65535)/257;
  return;
}

void pngparts_pngwrite_put32(unsigned char* b, unsigned long int w) {
  b[0] = (w>>24)&255;
  b[1] = (w>>16)&255;
  b[2] = (w>> 8)&255;
  b[3] = (w>> 0)&255;
  return;
}

int pngparts_pngwrite_signal_finish(struct pngparts_png* w){
  struct pngparts_png_message message;
  message.type = PNGPARTS_PNG_M_FINISH;
  message.byte = PNGPARTS_API_OK;
  memcpy(message.name, w->active_chunk_cb->name, 4);
  message.ptr = NULL;
  return pngparts_png_send_chunk_msg(w, w->active_chunk_cb, &message);
}

void pngparts_pngwrite_init(struct pngparts_png* w){
  w->state = 0;
  w->check = pngparts_png_crc32_new();
  w->shortpos = 0;
  w->last_result = 0;
  w->flags_tf = 0;
  w->chunk_cbs = NULL;
  w->active_chunk_cb = NULL;
  w->palette_count = 0;
  w->palette = NULL;
  return;
}

void pngparts_pngwrite_free(struct pngparts_png* w){
  pngparts_png_drop_chunk_cbs(w);
  free(w->palette);
  w->palette = NULL;
  return;
}

int pngparts_pngwrite_generate(struct pngparts_png* w){
  int result = w->last_result;
  int state = w->state;
  int shortpos = w->shortpos;
  while (result == PNGPARTS_API_OK
    &&  (w->pos < w->size))
  {
      /* states:
       * 0  - start
       * 1  - length, header
       * 2  - IEND contents
       * 3  - IEND CRC
       * 4  - done
       * 5  - unknown chunk contents
       * 6  - unknown chunk CRC
       * 7  - IHDR handling
       */
    int ch;
    switch (state){
    case 0: /* start */
      if (shortpos < 8){
        ch = pngparts_png_signature()[shortpos];
        shortpos += 1;
      }
      if (shortpos >= 8){
        state = 1;
        shortpos = 0;
      }
      break;
    case 1: /* length, header */
      if (shortpos == 0){
        unsigned char* const chunk_name = w->shortbuf+4;
        w->check = pngparts_png_crc32_new();
        if (w->flags_tf & PNGPARTS_PNG_IHDR_DONE){
          /* look for the next chunk to do */
          int i;
          struct pngparts_png_chunk_cb const* next_cb =
            pngparts_png_find_ready_cb(w);
          w->active_chunk_cb = NULL;
          if (next_cb != NULL){
            /* prepare for the next chunk */
            memcpy(chunk_name, next_cb->name, 4);
            /* query the chunk size */{
              struct pngparts_png_message message;
              int size_result;
              message.type = PNGPARTS_PNG_M_START;
              message.byte = 1;/*write mode */
              memcpy(message.name, chunk_name, 4);
              message.ptr = NULL;
              w->chunk_size = 0;
              w->flags_tf |= PNGPARTS_PNG_CHUNK_RW;
              size_result = pngparts_png_send_chunk_msg(w, next_cb, &message);
              w->flags_tf &= ~PNGPARTS_PNG_CHUNK_RW;
              if (size_result < 0){
                result = size_result;
                break;
              } else if (size_result == PNGPARTS_API_DONE){
                result = PNGPARTS_API_EOF;
                break;
              } else if (w->chunk_size > 0x7fFFffFF){
                result = PNGPARTS_API_CHUNK_TOO_LONG;
                break;
              } else {
                /*w->chunk_size = w->chunk_size;*/
                w->active_chunk_cb = next_cb;
              }
            }
          } else {
            /* prepare for the IEND */
            memcpy(chunk_name, "\x49\x45\x4E\x44", 4);
            w->chunk_size = 0;
          }
          /* perform common chunk operations */
          for (i = 0; i < 4; ++i){
            w->check = pngparts_png_crc32_accum(w->check, chunk_name[i]);
          }
          pngparts_pngwrite_put32(w->shortbuf, w->chunk_size);
        } else {
          int i;
          /* do the header */
          pngparts_pngwrite_put32(w->shortbuf, 13);
          memcpy(chunk_name, "\x49\x48\x44\x52", 4);
          for (i = 0; i < 4; ++i){
            w->check = pngparts_png_crc32_accum(w->check, chunk_name[i]);
          }
        }
      }
      if (shortpos < 8){
        ch = w->shortbuf[shortpos];
        shortpos += 1;
        if (shortpos >= 8){
          if (w->flags_tf & PNGPARTS_PNG_IHDR_DONE){
            if (w->active_chunk_cb != NULL){
              /* do the next chunk callback */
              if (w->chunk_size > 0){
                state = 5;
              } else {
                /* signal end of chunk */
                int const finish_result = pngparts_pngwrite_signal_finish(w);
                if (finish_result < 0){
                  result = finish_result;
                  break;
                }
                state = 6;
              }
            } else {
              /* do the IEND */
              if (w->chunk_size > 0){
                state = 2;
              } else state = 3;
            }
            shortpos = 0;
          } else {
            /* do the header */
            shortpos = 0;
            state = 7;
            w->chunk_size = 13;
          }
        }
      }
      break;
    case 2: /* IEND contents */
    case 5: /* unknown contents */
      if (w->chunk_size > 0){
        if (w->active_chunk_cb != NULL){
          /* put the message */
          int put_result;
          struct pngparts_png_message message;
          message.type = PNGPARTS_PNG_M_PUT;
          message.byte = -1;/* byte to receive */
          memcpy(message.name, w->active_chunk_cb->name, 4);
          message.ptr = NULL;
          put_result =
            pngparts_png_send_chunk_msg(w, w->active_chunk_cb, &message);
          if (put_result < 0){
            result = put_result;
            break;
          } else if (put_result == PNGPARTS_API_DONE){
            result = PNGPARTS_API_EOF;
            break;
          } else if (message.byte < 0 || message.byte > 255){
            result = PNGPARTS_API_MISSING_PUT;
            break;
          } else {
            ch = message.byte;
          }
        } else {
          ch = 0;
        }
        w->check = pngparts_png_crc32_accum(w->check, ch);
        w->chunk_size -= 1;
      }
      if (w->chunk_size == 0){
        if (w->active_chunk_cb != NULL){
          int const finish_result = pngparts_pngwrite_signal_finish(w);
          if (finish_result < 0){
            result = finish_result;
            break;
          }
        }
        state += 1;
        shortpos = 0;
      }
      break;
    case 3: /* IEND CRC */
    case 6: /* unknown CRC */
      if (shortpos == 0){
        unsigned long int const final_check =
          pngparts_png_crc32_tol(w->check);
        pngparts_pngwrite_put32(w->shortbuf+0, final_check);
      }
      if (shortpos < 4){
        ch = w->shortbuf[shortpos];
        shortpos += 1;
      }
      if (shortpos >= 4){
        shortpos = 0;
        if (state == 3){
          /* notify end of file */ {
            struct pngparts_png_message message;
            int broadcast_feedback;
            message.byte = 0;
            message.ptr = NULL;
            message.type = PNGPARTS_PNG_M_ALL_DONE;
            broadcast_feedback =
              pngparts_png_broadcast_chunk_msg(w, &message);
            if (broadcast_feedback < 0)
              result = broadcast_feedback;
          }
          state = 4; /* done */
        } else
          state = 1; /* continue */
      }
      break;
    case 4:
      result = PNGPARTS_API_DONE;
      break;
    case 7: /* IHDR */
      if (w->chunk_size == 13){
        struct pngparts_png_header header;
        int i;
        /* default-fill the header */{
          header.width = 0;
          header.height = 0;
          header.bit_depth = 1;
          header.color_type = 0;
          header.compression = 0;
          header.filter = 0;
          header.interlace = 0;
        }
        /* get a better description */if (w->img_cb.describe_cb != NULL){
          (*w->img_cb.describe_cb)(
              w->img_cb.cb_data,
              &header.width, &header.height,
              &header.bit_depth, &header.color_type, &header.compression,
              &header.filter, &header.interlace
            );
        }
        /* verify the header */
        if (header.width < 0
        ||  header.height < 0
        ||  !pngparts_png_header_is_valid(header))
        {
          result = PNGPARTS_API_BAD_HDR;
          break;
        }
        /* record the header */
        w->header = header;
        /* compose the header */
        pngparts_pngwrite_put32(w->shortbuf+0, (unsigned long)header.width);
        pngparts_pngwrite_put32(w->shortbuf+4, (unsigned long)header.height);
        w->shortbuf[8] = (unsigned char)header.bit_depth;
        w->shortbuf[9] = (unsigned char)header.color_type;
        w->shortbuf[10]= (unsigned char)header.compression;
        w->shortbuf[11]= (unsigned char)header.filter;
        w->shortbuf[12]= (unsigned char)header.interlace;
        /* advance compute the CRC */
        for (i = 0; i < 13; ++i){
          w->check = pngparts_png_crc32_accum(w->check, w->shortbuf[i]);
        }
      }
      if (w->chunk_size > 13){
        ch = 255;
        w->chunk_size -= 1;
      } else if (w->chunk_size > 0){
        ch = w->shortbuf[13-w->chunk_size];
        w->chunk_size -= 1;
      }
      if (w->chunk_size == 0){
        state = 6;
        shortpos = 0;
        w->flags_tf |= PNGPARTS_PNG_IHDR_DONE;
      }
      break;
    }
    if (result != PNGPARTS_API_OK)
      break;
    else /* put actual character */ {
      w->buf[w->pos] = (unsigned char)(ch & 255);
      w->pos += 1;
    }
  }
  w->last_result = result;
  w->state = (short)state;
  w->shortpos = (short)shortpos;
  return result;
}

/*BEGIN IDAT*/
struct pngparts_pngwrite_idat {
  int level;
  int pixel_size;
  unsigned long int line_width;
  unsigned long int line_height;
  long int x;
  long int y;
  struct pngparts_api_z z;
  int filter_mode;
  /* compressed output buffer */
  unsigned char* outbuf;
  /* capacity of output buffer */
  int outsize;
  /* length of content on output buffer */
  int outlen;
  /* read position for output buffer */
  int outpos;
  /* uncompressed scan line */
  unsigned char* inbuf;
  /* size of uncompressed scan line */
  unsigned long int insize;
  /* write position of uncompressed scan line */
  unsigned long int inpos;
  /* amount of next pixel space remaining */
  int next_left;
  /* current and same-line previous pixel data */
  unsigned char nextbuf[16];
  unsigned char filter_byte;
};

int pngparts_pngwrite_start_line
  (struct pngparts_png* p, struct pngparts_pngwrite_idat* idat)
{
  /*compute line length*/
  unsigned long int buffer_length;
  {
    unsigned long int line_length = idat->pixel_size;
    struct pngparts_png_size const line_size =
      pngparts_png_adam7_pass_size(
        p->header.width, p->header.height, idat->level
      );
    idat->line_width = line_size.width;
    idat->line_height = line_size.height;
    if (idat->line_width == 0 || idat->line_height == 0) {
      return PNGPARTS_API_OVERFLOW;
    }
    line_length = idat->line_width*idat->pixel_size;
    buffer_length = (line_length + 7) >> 3;
  }
  if (idat->insize != buffer_length) {
    /* resize the buffer */
    unsigned char* new_buffer;
    free(idat->inbuf);
    idat->inbuf = NULL;
    new_buffer = (unsigned char*)calloc(buffer_length, sizeof(unsigned char));
    if (new_buffer == NULL) {
      return PNGPARTS_API_MEMORY;
    }
    idat->inbuf = new_buffer;
    idat->insize = buffer_length;
    idat->inpos = 0;
  } else {
    memset(idat->inbuf, 0, idat->insize * sizeof(unsigned char));
  }
  memset(idat->nextbuf, 0, 8*sizeof(unsigned char));
  idat->outpos = 0;
  idat->filter_mode = -2;
  return PNGPARTS_API_OK;
}


void pngparts_pngwrite_idat_fetch
  (struct pngparts_png* p, struct pngparts_pngwrite_idat* idat)
{
  int const color_type = p->header.color_type;
  struct pngparts_api_image img;
  pngparts_png_get_image_cb(p, &img);
  static const int multiplier[9] =
    { 0, 0xffff, 0x5555, 0x2492, 0x1111, 0, 0, 0, 0x0101 };
  switch (idat->pixel_size) {
  case 1: /* either L/1 or index/1 */
  case 2: /* either L/2 or index/2 */
  case 4: /* either L/4 or index/4 */
  case 8: /* either L/8 or index/8 */
    {
      int i, count = 8/idat->pixel_size;
      int mask = (1 << idat->pixel_size) - 1;
      long int nx, ny;
      idat->nextbuf[8] = 0;
      for (i = count-1; i >= 0; --i) {
        unsigned int bit_string = 0;
        pngparts_png_adam7_reverse_xy(idat->level, &nx, &ny, idat->x, idat->y);
        if (nx < p->header.width) {
          switch (color_type) {
          case 0: /* L/i */
            {
              unsigned int red;
              unsigned int /*EXTRA*/ green;
              unsigned int /*EXTRA*/ blue;
              unsigned int /*EXTRA*/ alpha;
              (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
              bit_string = red/multiplier[idat->pixel_size];
            }break;
          case 3: /* index/i */
            {
              unsigned int red;
              unsigned int green;
              unsigned int blue;
              unsigned int alpha;
              struct pngparts_png_plte_item color;
              int plte_nearest;
              (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
              pngparts_pngwrite_put8clamp16(&color.red, red);
              pngparts_pngwrite_put8clamp16(&color.green, green);
              pngparts_pngwrite_put8clamp16(&color.blue, blue);
              pngparts_pngwrite_put8clamp16(&color.alpha, alpha);
              plte_nearest = pngparts_png_nearest_plte_item(p, color);
              bit_string = plte_nearest < 0 ? 0 : plte_nearest;
            }break;
          }
          idat->x += 1;
        }
        /* compose and add the bit string */{
          idat->nextbuf[8] |= ((bit_string & mask) << (i*idat->pixel_size));
        }
      }
    }break;
  case 16: /* either L/16, LA/8 */
    {
      long int nx, ny;
      pngparts_png_adam7_reverse_xy(idat->level, &nx, &ny, idat->x, idat->y);
      if (nx < p->header.width) {
        switch (color_type) {
        case 0: /* L/16 */
          {
            unsigned int red;
            unsigned int /*EXTRA*/ green;
            unsigned int /*EXTRA*/ blue;
            unsigned int /*EXTRA*/ alpha;
            (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
            pngparts_pngwrite_put16clamp16(idat->nextbuf+8, red);
          }break;
        case 4: /* LA/8 */
          {
            unsigned int red;
            unsigned int /*EXTRA*/ green;
            unsigned int /*EXTRA*/ blue;
            unsigned int alpha;
            (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
            pngparts_pngwrite_put8clamp16(idat->nextbuf+8, red);
            pngparts_pngwrite_put8clamp16(idat->nextbuf+9, alpha);
          }break;
        }
      }
      idat->x += 1;
    }break;
  case 24: /* only RGB/8 */
    {
      long int nx, ny;
      pngparts_png_adam7_reverse_xy(idat->level, &nx, &ny, idat->x, idat->y);
      if (nx < p->header.width) {
        unsigned int red, green, blue;
        unsigned int /*EXTRA*/ alpha;
        (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
        pngparts_pngwrite_put8clamp16(idat->nextbuf+ 8, red);
        pngparts_pngwrite_put8clamp16(idat->nextbuf+ 9, green);
        pngparts_pngwrite_put8clamp16(idat->nextbuf+10, blue);
      }
      idat->x += 1;
    }break;
  case 32: /* either LA/16 or RGBA/8 */
    {
      long int nx, ny;
      pngparts_png_adam7_reverse_xy(idat->level, &nx, &ny, idat->x, idat->y);
      if (nx < p->header.width) {
        switch (color_type) {
        case 4: /* LA/16 */
          {
            unsigned int red;
            unsigned int /*EXTRA*/ green;
            unsigned int /*EXTRA*/ blue;
            unsigned int alpha;
            (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
            pngparts_pngwrite_put16clamp16(idat->nextbuf+ 8, red);
            pngparts_pngwrite_put16clamp16(idat->nextbuf+10, alpha);
          }break;
        case 6: /* RGBA/8 */
          {
            unsigned int red, green, blue, alpha;
            (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
            pngparts_pngwrite_put8clamp16(idat->nextbuf+ 8, red);
            pngparts_pngwrite_put8clamp16(idat->nextbuf+ 9, green);
            pngparts_pngwrite_put8clamp16(idat->nextbuf+10, blue);
            pngparts_pngwrite_put8clamp16(idat->nextbuf+11, alpha);
          }break;
        }
      }
      idat->x += 1;
    }break;
  case 48: /* only RGB/16 */
    {
      long int nx, ny;
      pngparts_png_adam7_reverse_xy(idat->level, &nx, &ny, idat->x, idat->y);
      if (nx < p->header.width) {
        unsigned int red, green, blue;
        unsigned int /*EXTRA*/ alpha;
        (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
        pngparts_pngwrite_put16clamp16(idat->nextbuf+ 8, red);
        pngparts_pngwrite_put16clamp16(idat->nextbuf+10, green);
        pngparts_pngwrite_put16clamp16(idat->nextbuf+12, blue);
      }
      idat->x += 1;
    }break;
  case 64: /* only RGBA/16 */
    {
      long int nx, ny;
      pngparts_png_adam7_reverse_xy(idat->level, &nx, &ny, idat->x, idat->y);
      if (nx < p->header.width) {
        unsigned int red, green, blue, alpha;
        (*img.get_cb)(img.cb_data, nx, ny, &red, &green, &blue, &alpha);
        pngparts_pngwrite_put16clamp16(idat->nextbuf+ 8, red);
        pngparts_pngwrite_put16clamp16(idat->nextbuf+10, green);
        pngparts_pngwrite_put16clamp16(idat->nextbuf+12, blue);
        pngparts_pngwrite_put16clamp16(idat->nextbuf+14, alpha);
      }
      idat->x += 1;
    }break;
  default:
    break;
  }
  return;
}

void pngparts_pngwrite_idat_shift(struct pngparts_pngwrite_idat* d, int shift) {
  d->next_left += shift;
  memmove(d->nextbuf, d->nextbuf + shift, 16 - shift);
  return;
}

void pngparts_pngwrite_idat_add(struct pngparts_pngwrite_idat* d, int shift) {
  if (d->inpos >= shift) {
    memcpy(d->inbuf + d->inpos - shift, d->nextbuf + 8 - shift, shift);
  }
  d->inpos += shift;
  return;
}


int pngparts_pngwrite_generate_chunk
  (struct pngparts_png* p, struct pngparts_pngwrite_idat* idat)
{
  int total_result = PNGPARTS_API_OK;
  int loop_trap = idat->outsize*2;
  int input_pending = !(*idat->z.input_done_cb)(idat->z.cb_data);
  /* prepare the output buffer */{
    (*idat->z.set_output_cb)(idat->z.cb_data, idat->outbuf, idat->outsize);
    idat->outlen = 0;
    idat->outpos = 0;
  }
  /* send data for compression */
  while  ((idat->filter_mode < 5 || input_pending)
    &&    idat->outlen < idat->outsize
    &&    loop_trap > 0)
  {
    loop_trap -= 1;
    if (input_pending){
      int const churn_mode = ((idat->filter_mode == 5)
        ? PNGPARTS_API_Z_FINISH : PNGPARTS_API_Z_NORMAL);
      /* part of a scan line awaits compression */
      int const churn_result =
        (*idat->z.churn_cb)(idat->z.cb_data, churn_mode);
      if (churn_result < PNGPARTS_API_OK){
        total_result = churn_result;
        break;
      }
      idat->outlen = (*idat->z.output_left_cb)(idat->z.cb_data);
    } else switch (idat->filter_mode){
    case -1: /* no filter yet */
      {
        if (idat->y >= idat->line_height){
          if (idat->level > 0){
            int line_result = PNGPARTS_API_OVERFLOW;
            while (idat->level < 7){
              idat->level += 1;
              line_result = pngparts_pngwrite_start_line(p, idat);
              if (line_result == PNGPARTS_API_OK)
                break;
            }
            if (line_result == PNGPARTS_API_OVERFLOW){
              idat->filter_mode = 5;/*done */
              break;
            } else /* reset the cursor */{
              idat->y = 0;
              idat->x = 0;
            }
          } else /* regular single-pass image */{
            idat->filter_mode = 5;/*done */
            break;
          }
        }
        idat->filter_byte = 0;/* choose identity filter for now */
        (*idat->z.set_input_cb)(idat->z.cb_data, &idat->filter_byte, 1);
        idat->filter_mode = idat->filter_byte;
      }break;
    case 0: /* identity filter */
      {
        int const shift_size = (idat->pixel_size+7)/8;
        idat->inpos = 0;
        if (shift_size <= 0){
          /* fast quit by */loop_trap = 0;
        } else for (idat->x = 0; idat->x < idat->line_width; ){
          int const x_pre = idat->x;
          /* generate data */{
            pngparts_pngwrite_idat_fetch(p, idat);
          }
          if (idat->x <= x_pre){
            /*ensure termination by setting */idat->x = x_pre+1;
          }
          /* apply the filter */{
            /* do nothing for identity */;
          }
          /* add and shift */{
            pngparts_pngwrite_idat_add(idat, shift_size);
            pngparts_pngwrite_idat_shift(idat, shift_size);
          }
        }
        /* add the last pixel for the scan line */
        pngparts_pngwrite_idat_add(idat, shift_size);
        /* post to compressor */
        (*idat->z.set_input_cb)(idat->z.cb_data, idat->inbuf, idat->insize);
        /* prepare for the next line */{
          idat->y += 1;
          idat->filter_mode = -1;
        }
      }break;
    }
    input_pending = !(*idat->z.input_done_cb)(idat->z.cb_data);
  }
  /* stream finish */if (total_result == PNGPARTS_API_OK
  &&  idat->filter_mode == 5
  &&  (!input_pending)
  &&  (idat->outlen < idat->outsize))
  {
    int const churn_mode = PNGPARTS_API_Z_FINISH;
    (*idat->z.churn_cb)(idat->z.cb_data, churn_mode);
    idat->outlen = (*idat->z.output_left_cb)(idat->z.cb_data);
    if (idat->outlen == 0){
      /* stream is really done */
      idat->filter_mode = 6;
    }
  }
  idat->outpos = 0;
  return total_result;
}

int pngparts_pngwrite_idat_msg
  (struct pngparts_png* p, void* cb_data, struct pngparts_png_message* msg)
{
  int result;
  struct pngparts_pngwrite_idat *idat =
    (struct pngparts_pngwrite_idat *)cb_data;
  switch (msg->type) {
  case PNGPARTS_PNG_M_READY:
    {
      if (idat->filter_mode == 6)
        result = PNGPARTS_API_DONE;
      else {
        result = PNGPARTS_API_OK;
      }
    }break;
  case PNGPARTS_PNG_M_START:
    {
      if (idat->level == -1) {
        idat->outlen = 0;
        /* get level */
        if (p->header.interlace == 1) {
          idat->level = 1;
          result = PNGPARTS_API_OK;
        } else if (p->header.interlace == 0) {
          idat->level = 0;
          result = PNGPARTS_API_OK;
        } else {
          result = PNGPARTS_API_UNSUPPORTED;
          break;
        }
        /* compute the sample size in bytes */ {
          switch (p->header.color_type) {
          case 0: /* gray */
          case 3: /* palette */
            idat->pixel_size = p->header.bit_depth;
            break;
          case 4: /* gray alpha */
            idat->pixel_size = p->header.bit_depth * 2;
            break;
          case 2: /* red green blue*/
            idat->pixel_size = p->header.bit_depth * 3;
            break;
          case 6: /* red green blue alpha*/
            idat->pixel_size = p->header.bit_depth * 4;
            break;
          }
        }
        /* prepare the line */{
          int const line_out = pngparts_pngwrite_start_line(p, idat);
          if (line_out == PNGPARTS_API_OVERFLOW) {
            /* give up */
            idat->filter_mode = 5;
            break;
          } else if (line_out == PNGPARTS_API_MEMORY){
            result = PNGPARTS_API_MEMORY;
            break;
          }
        }
        /* connect the z compressor */{
          (*idat->z.set_output_cb)
            (idat->z.cb_data, idat->outbuf, idat->outsize);
        }
        idat->filter_mode = -1;
        idat->next_left = 8;
        idat->x = 0;
        idat->y = 0;
        /* generate a chunk */{
          int const chunk_out = pngparts_pngwrite_generate_chunk(p, idat);
          if (chunk_out >= PNGPARTS_API_OK){
            pngparts_png_set_chunk_size(p, idat->outlen);
          }
          result = chunk_out;
        }
      } else {
        /* generate a chunk */{
          int const chunk_out = pngparts_pngwrite_generate_chunk(p, idat);
          if (chunk_out >= PNGPARTS_API_OK){
            pngparts_png_set_chunk_size(p, idat->outlen);
          }
          result = chunk_out;
        }
      }
    }break;
  case PNGPARTS_PNG_M_PUT:
    {
      if (idat->outpos < idat->outlen){
        msg->byte = idat->outbuf[idat->outpos];
        idat->outpos += 1;
        result = PNGPARTS_API_OK;
      } else {
        result = PNGPARTS_API_BAD_STATE;
      }
    }break;
  case PNGPARTS_PNG_M_FINISH:
    {
      if (idat->outpos != idat->outlen){
        result = PNGPARTS_API_BAD_STATE;
      } else {
        result = PNGPARTS_API_OK;
      }
    }break;
  case PNGPARTS_PNG_M_ALL_DONE:
    {
      if (idat->filter_mode == 6
      &&  idat->outpos == idat->outlen)
      {
        result = PNGPARTS_API_OK;
      } else {
        result = PNGPARTS_API_BAD_STATE;
      }
    }break;
  case PNGPARTS_PNG_M_DESTROY:
    {
      free(idat->outbuf);
      free(idat->inbuf);
      free(idat);
      result = PNGPARTS_API_OK;
    }break;
  default:
    result = PNGPARTS_API_BAD_STATE;
    break;
  }
  return result;
}

int pngparts_pngwrite_assign_idat_api
  ( struct pngparts_png_chunk_cb* cb, struct pngparts_api_z const* z,
    int chunk_size)
{
  unsigned char static const name[4] = { 0x49,0x44,0x41,0x54 };
  struct pngparts_pngwrite_idat* ptr;
  unsigned char* outptr;
  unsigned long int used_chunk_size;
  if (chunk_size < 0)
    return PNGPARTS_API_BAD_PARAM;
  else if (chunk_size == 0)
    used_chunk_size = (PNGPARTS_PNGWRITE_CHUNK_SIZE);
  else
    used_chunk_size = chunk_size;
  /* sanitize the result chunk length */
  if (used_chunk_size > 0x3fFFffFF
  ||  used_chunk_size > (INT_MAX/2))
  {
    /* too big! give up. */
    return PNGPARTS_API_CHUNK_TOO_LONG;
  }
  ptr = (struct pngparts_pngwrite_idat*)malloc
    (sizeof(struct pngparts_pngwrite_idat));
  outptr = (unsigned char*)malloc(sizeof(unsigned char)*used_chunk_size);
  if (ptr == NULL || outptr == NULL) {
    free(ptr);
    free(outptr);
    return PNGPARTS_API_MEMORY;
  }
  /* fill in the structure */{
    memcpy(cb->name, name, 4 * sizeof(unsigned char));
    memcpy(&ptr->z, z, sizeof(*z));
    ptr->level = -1;
    ptr->inbuf = NULL;
    ptr->insize = 0;
    ptr->inpos = 0;
    ptr->outbuf = outptr;
    ptr->outsize = (int)used_chunk_size;
    ptr->outpos = 0;
    ptr->outlen = 0;
    ptr->filter_mode = -1;
    cb->cb_data = ptr;
    cb->message_cb = pngparts_pngwrite_idat_msg;
    return PNGPARTS_API_OK;
  }
}
/*END   IDAT*/
