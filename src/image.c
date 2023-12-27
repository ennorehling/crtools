/*
 *  crimage - a tool for creating png maps.
 *  Copyright (C) 2006 Enno Rehling
 *
 * This file is part of crtools.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdlib.h>
#include <assert.h>
#include <png.h>
#include "config.h"
#include "image.h"

void
image_bitblt(image* dest, image * src, int xof, int yof)
{
  int x, y;
  int xmin = max(0, -xof);
  int ymin = max(0, -yof);
  int width = min(dest->width-xof, min(src->width, src->width+xof));
  int height = min(min(src->height, dest->height-yof), min(src->height, src->height+yof));
  for (y=ymin;y<ymin+height;++y) {
    for (x=xmin;x<xmin+width;++x) {
      if (src->data[y][x].alpha!=0) {
        dest->data[y+yof][x+xof] = src->data[y][x];
      }
    }
  }
}


#if 1
static void
image_postprocess(image * src)
{
  int x, y;
  pixel p = src->data[0][0];
  for (y=0;y<src->height;++y) {
    for (x=0;x<src->width;++x) {
      if (!memcmp(&src->data[y][x], &p, sizeof(pixel))) {
        memset(&src->data[y][x], 0, sizeof(pixel));
      }
    }
  }
}

int
image_read(image * pic, FILE * fp)  /* We need to open the file */
{
   png_structp png_ptr;
   png_infop info_ptr;
   unsigned int sig_read = 0;
   png_uint_32 width, height;
   int bit_depth, color_type, interlace_type;
   png_color_16 my_background, *image_background;
   int intent, number_passes;
  unsigned row;
  char * gamma_str;
  double screen_gamma;

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED
    */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);

   if (png_ptr == NULL)
   {
      fclose(fp);
      return -1;
   }

   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      return -1;
   }

   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.
    */
   if (setjmp(png_ptr->jmpbuf))
   {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      fclose(fp);
      /* If we get here, we had a problem reading the file */
      return -1;
   }

   /* Set up the input control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* If we have already read some of the signature */
   png_set_sig_bytes(png_ptr, sig_read);

   /* The call to png_read_info() gives us all of the information from the
    * PNG file before the first IDAT (image data chunk).  REQUIRED
    */
   png_read_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       &interlace_type, NULL, NULL);

/**** Set up the data transformations you want.  Note that these are all
 **** optional.  Only call them if you want/need them.  Many of the
 **** transformations only work on specific types of images, and many
 **** are mutually exclusive.
 ****/

#if 0
   /* tell libpng to strip 16 bit/color files down to 8 bits/color */
   png_set_strip_16(png_ptr);
#endif
#if 0
   /* Strip alpha bytes from the input data without combining with the
    * background (not recommended).
    */
   png_set_strip_alpha(png_ptr);
#endif
   /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
   png_set_packing(png_ptr);
#if 0
   /* Change the order of packed pixels to least significant bit first
    * (not useful if you are using png_set_packing). */
   png_set_packswap(png_ptr);
#endif
   /* Expand paletted colors into true RGB triplets */
   if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_expand(png_ptr);

   /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
   if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand(png_ptr);

   /* Expand paletted or RGB images with transparency to full alpha channels
    * so the data will be available as RGBA quartets.
    */
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
      png_set_expand(png_ptr);

   /* Set the background color to draw transparent and alpha images over.
    * It is possible to set the red, green, and blue components directly
    * for paletted images instead of supplying a palette index.  Note that
    * even if the PNG file supplies a background, you are not required to
    * use it - you should use the (solid) application background if it has one.
    */

   if (png_get_bKGD(png_ptr, info_ptr, &image_background))
      png_set_background(png_ptr, image_background,
                         PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
   else
      png_set_background(png_ptr, &my_background,
                         PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);

   /* Some suggestions as to how to get a screen gamma value */

   /* Note that screen gamma is the display_exponent, which includes
    * the CRT_exponent and any correction for viewing conditions */
   if ((gamma_str = getenv("SCREEN_GAMMA")) != NULL)
   {
      screen_gamma = atof(gamma_str);
   }
   /* If we don't have another value */
   else
   {
      screen_gamma = 2.2;  /* A good guess for a PC monitors in a dimly
                              lit room */
   }

   /* Tell libpng to handle the gamma conversion for you.  The second call
    * is a good guess for PC generated images, but it should be configurable
    * by the user at run time by the user.  It is strongly suggested that
    * your application support gamma correction.
    */

   if (png_get_sRGB(png_ptr, info_ptr, &intent))
      png_set_sRGB(png_ptr, info_ptr, intent);
   else
   {
      double image_gamma;
      if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
         png_set_gamma(png_ptr, screen_gamma, image_gamma);
      else
         png_set_gamma(png_ptr, screen_gamma, 0.45455);
   }
#if 0
   /* Dither RGB files down to 8 bit palette or reduce palettes
    * to the number of colors available on your screen.
    */
   if (color_type & PNG_COLOR_MASK_COLOR)
   {
      int num_palette;
      png_colorp palette;

      /* This reduces the image to the application supplied palette */
      if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette))
      {
         png_uint_16p histogram;

         png_get_hIST(png_ptr, info_ptr, &histogram);

         png_set_dither(png_ptr, palette, num_palette,
                        256, histogram, 0);
      }
   }
#endif
#if 0
   /* invert monocrome files to have 0 as white and 1 as black */
   png_set_invert_mono(png_ptr);
#endif
#if 0
   /* If you want to shift the pixel values from the range [0,255] or
    * [0,65535] to the original [0,7] or [0,31], or whatever range the
    * colors were originally in:
    */
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT))
   {
      png_color_8p sig_bit;

      png_get_sBIT(png_ptr, info_ptr, &sig_bit);
      png_set_shift(png_ptr, sig_bit);
   }
#endif
#if 0
   /* flip the RGB pixels to BGR (or RGBA to BGRA) */
   png_set_bgr(png_ptr);
#endif
#if 0
   /* swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
   png_set_swap_alpha(png_ptr);
#endif
#if 0
   /* swap bytes of 16 bit files to least significant byte first */
   png_set_swap(png_ptr);
#endif
   /* Add filler (or alpha) byte (before/after each RGB triplet) */
   png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

   /* Turn on interlace handling.  REQUIRED if you are not using
    * png_read_image().  To see how to handle interlacing passes,
    * see the png_read_row() method below:
    */
   number_passes = png_set_interlace_handling(png_ptr);

   /* Optional call to gamma correct and add the background to the palette
    * and update info structure.  REQUIRED if you are expecting libpng to
    * update the palette for you (ie you selected such a transform above).
    */
   png_read_update_info(png_ptr, info_ptr);

   /* Allocate the memory to hold the image using the fields of info_ptr. */

   /* The easiest way to read the image: */
   pic->data = malloc(sizeof(png_bytep) * height);
  pic->height = height;
  pic->width = width;

   for (row = 0; row < height; row++)
   {
    png_bytep * row_pointers = (png_bytep *)pic->data;
      row_pointers[row] = malloc(png_get_rowbytes(png_ptr, info_ptr));
   }

   /* Now it's time to read the image.  One of these methods is REQUIRED */
   png_read_image(png_ptr, (png_bytepp)pic->data);

   /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
   png_read_end(png_ptr, info_ptr);

   /* clean up after the read, and free any memory allocated - REQUIRED */
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

   /* close the file */
   fclose(fp);

  image_postprocess(pic);
   /* that's it */
   return 0;
}

#else

int
image_read(image * pic, FILE * f)
{
  png_structp  png_ptr = NULL;
  png_infop info_ptr = NULL, end_info = NULL;
  png_uint_32  width, height;
  int     bit_depth, color_type, interlace_type;
  FILE     *fp = f;
  int     y;
  double     gamma;

  pic->data = NULL;

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL)
  goto error_exit;
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL)
  goto error_exit;
  end_info = png_create_info_struct(png_ptr);
  if (end_info == NULL)
    goto error_exit;
#ifdef USE_FAR_KEYWORD
   if (setjmp(jmpbuf))
#else
   if (setjmp(png_ptr->jmpbuf))
#endif
  /*
   * Set error handling if you are using the setjmp/longjmp method (this is
   * the normal method of doing things with libpng).  REQUIRED unless you
   * set up your own error handlers in the png_create_read_struct() earlier.
   */
  goto error_exit;
  png_init_io(png_ptr, fp);
  /*
   * The call to png_read_info() gives us all of the information from the
   * PNG file before the first IDAT (image data chunk).  REQUIRED
   */
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
     &interlace_type, NULL, NULL);

  png_set_strip_16(png_ptr);
  png_set_packing(png_ptr);
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_expand(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY) {
    fprintf(stderr, "grayscale nicht unterstützt\n");
    exit(1);
  }
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
  png_set_expand(png_ptr);

  if (png_get_gAMA(png_ptr, info_ptr, &gamma))
  png_set_gamma(png_ptr, 2.0, gamma);
  else
  png_set_gamma(png_ptr, 2.0, 0.45455);

  png_read_update_info(png_ptr, info_ptr);

  pic->width = width;
  pic->height = height;
  pic->data = malloc(pic->height * sizeof(pixel*));
  for (y = 0; y < pic->height; y++)
    pic->data[y] = malloc(pic->width * sizeof(pixel));
  png_read_image(png_ptr, (png_bytep*)pic->data);
  /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
  png_read_end(png_ptr, end_info);
  /* clean up after the read, and free any memory allocated - REQUIRED */
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

  return 0;
 error_exit:
  fprintf(stderr, "error exit\n");
  if (pic) {
    if (pic->data) {
      for (y = 0; y < pic->height; y++)
        free(pic->data[y]);
      free(pic->data);
    }
  }
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  return -1;
}
#endif

int
image_write(image * pic, FILE * f)
{
  png_structp   png_ptr;
  png_infop  info_ptr;
  png_color_8   sig_bit;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
            NULL, NULL, NULL);
  if (png_ptr == NULL) {
    return -1;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct(&png_ptr, NULL);
    return -1;
  }
  /*
   * Set error handling.  REQUIRED if you aren't supplying your own
   * error hadnling functions in the png_create_write_struct() call.
   */
  if (setjmp(png_ptr->jmpbuf)) {
    /* If we get here, we had a problem writing the file */
    png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
    return -1;
  }
  png_init_io(png_ptr, f);
  png_set_IHDR(png_ptr, info_ptr, pic->width, pic->height, 8,
     PNG_COLOR_TYPE_RGB,
     PNG_INTERLACE_NONE,
     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  sig_bit.red   = 8;
  sig_bit.green = 8;
  sig_bit.blue  = 8;
  png_set_sBIT(png_ptr, info_ptr, &sig_bit);
  png_set_gAMA(png_ptr, info_ptr, 0.6);
  /*
   * Hier könnte man Text-Chunks schreiben.
   * Bild-Info in Datei schreiben.
   */
  png_write_info(png_ptr, info_ptr);
  /*
   * Jetzt wird das Bild geschrieben:
   */
  png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
  png_write_image(png_ptr, (png_bytep*)pic->data);

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

  return 0;
}

int
image_create(image * img, int width, int height)
{
  int y;
  assert(img);
  img->width = width;
  img->height = height;
  img->data = malloc(height * sizeof(pixel*));
  if (!img->data) return -1;
  for (y = 0; y < height; y++) {
    img->data[y] = calloc(width, sizeof(pixel));
    if (!img->data[y]) return -1;
  }
  return 0;
}
