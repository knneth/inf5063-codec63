#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "io.h"

// XXX: Should be moved to a struct with FILE*

/* Read planar YUV frames with 4:2:0 chroma sub-sampling */
yuv_t* read_yuv(FILE *file, struct c63_common *cm)
{
  size_t len = 0;
  yuv_t *image = malloc(sizeof(*image));

  /* Read Y. The size of Y is the same as the size of the image. The indices                                   
     represents the color component (0 is Y, 1 is U, and 2 is V) */
  image->Y = calloc(1, cm->padw[Y_COMPONENT]*cm->padh[Y_COMPONENT]);
  len += fread(image->Y, 1, cm->width*cm->height, file);

  /* Read U. Given 4:2:0 chroma sub-sampling, the size is 1/4 of Y                                             
     because (height/2)*(width/2) = (height*width)/4. */
  image->U = calloc(1, cm->padw[U_COMPONENT]*cm->padh[U_COMPONENT]);
  len += fread(image->U, 1, (cm->width*cm->height)/4, file);

  /* Read V. Given 4:2:0 chroma sub-sampling, the size is 1/4 of Y. */
  image->V = calloc(1, cm->padw[V_COMPONENT]*cm->padh[V_COMPONENT]);
  len += fread(image->V, 1, (cm->width*cm->height)/4, file);

  if (ferror(file))
  {
    perror("ferror");
    exit(EXIT_FAILURE);
  }

  if (feof(file))
  {
    free(image->Y);
    free(image->U);
    free(image->V);
    free(image);

    return NULL;
  }
  else if (len != cm->width*cm->height*1.5)
  {
    fprintf(stderr, "Reached end of file, but incorrect bytes read.\n");
    fprintf(stderr, "Wrong input? (height: %d width: %d)\n",
	    cm->height, cm->width);

    free(image->Y);
    free(image->U);
    free(image->V);
    free(image);

    return NULL;
  }

  return image;
}

void put_byte(FILE *fp, int byte)
{
  int status = fputc(byte, fp);

  if (status == EOF)
  {
    fprintf(stderr, "Error writing byte\n");
    exit(EXIT_FAILURE);
  }
}

void put_bytes(FILE *fp, const void* data, unsigned int len)
{
  size_t n = fwrite(data, 1, (size_t) len, fp);

  if(n != (size_t) len)
  {
    fprintf(stderr, "Error writing bytes\n");
    exit(EXIT_FAILURE);
  }
}

uint8_t get_byte(FILE *fp)
{
  int status = fgetc(fp);

  if (status == EOF)
  {
    fprintf(stderr, "End of file.\n");
    exit(EXIT_FAILURE);
  }

  return (uint8_t) status;
}

int read_bytes(FILE *fp, void *data, unsigned int sz)
{
  size_t status = fread(data, 1, (size_t) sz, fp);

  if ((int) status == EOF)
  {
    fprintf(stderr, "End of file.\n");
    exit(EXIT_FAILURE);
  }
  else if (status != (size_t) sz)
  {
    fprintf(stderr, "Error reading bytes\n");
    exit(EXIT_FAILURE);
  }

  return (int) status;
}

/**
 * Adds a bit to the bitBuffer. A call to bit_flush() is needed
 * in order to write any remainding bits in the buffer before
 * writing using another function.
 */
void put_bits(struct entropy_ctx *c, uint16_t bits, uint8_t n)
{
  assert(n <= 24  && "Error writing bit");

  if(n == 0) { return; }

  c->bit_buffer <<= n;
  c->bit_buffer |= bits & ((1 << n) - 1);
  c->bit_buffer_width += n;

  while(c->bit_buffer_width >= 8)
  {
    uint8_t b = (uint8_t)(c->bit_buffer >> (c->bit_buffer_width - 8));

    put_byte(c->fp, b);

    if(b == 0xff) { put_byte(c->fp, 0); }

    c->bit_buffer_width -= 8;
  }
}

uint16_t get_bits(struct entropy_ctx *c, uint8_t n)
{
  uint16_t ret = 0;

  while(c->bit_buffer_width < n)
  {
    uint8_t b = get_byte(c->fp);
    if (b == 0xff) { get_byte(c->fp); } /* Discard stuffed byte */

    c->bit_buffer <<= 8;
    c->bit_buffer |= b;
    c->bit_buffer_width += 8;
  }

  ret = c->bit_buffer >> (c->bit_buffer_width - n);
  c->bit_buffer_width -= n;

  /* Clear grabbed bits */
  c->bit_buffer &= (1 << c->bit_buffer_width) - 1;

  return ret;
}

/**
 * Flushes the bitBuffer by writing zeroes to fill a full byte
 */
void flush_bits(struct entropy_ctx *c)
{
  if(c->bit_buffer > 0)
  {
    uint8_t b = c->bit_buffer << (8 - c->bit_buffer_width);
    put_byte(c->fp, b);

    if(b == 0xff) { put_byte(c->fp, 0); }
  }

  c->bit_buffer = 0;
  c->bit_buffer_width = 0;
}
