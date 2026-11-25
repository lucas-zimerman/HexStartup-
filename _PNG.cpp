#include <stdio.h>
#include <stdlib.h>
#include "Image.h"
#include <png.h>
#include "_PNG.h"
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MT2D.h>
#include <MT2D/File/MT2D_File.h>

/*
* A simple libpng example program
* http://zarb.org/~gc/html/libpng.html
*
* Modified by Yoshimasa Niwa to make it much simpler
* and support all defined color_type.
*
* To build, use the next instruction on OS X.
* $ brew install libpng
* $ clang -lz -lpng15 libpng_test.c
*
* Copyright 2002-2010 Guillaume Cottenceau.
*
* This software may be freely redistributed under the terms
* of the X11 license.
*
*/

extern char str_buffer[200];

/* libpng reads through MT2D_FILE instead of a FILE*, so this also works where MT2D_FILE is not a FILE (Android). */
static void PNG_Read_Data(png_structp png, png_bytep data, png_size_t length) {
	MT2D_FILE *file = (MT2D_FILE*)png_get_io_ptr(png);
	if (MT2D_FILE_READ(file, data, 1, length) != length) {
		png_error(png, "unexpected end of file");
	}
}

/* Frees everything PNG_read_file allocated. Every pointer may be NULL. */
static void PNG_Free(Image *img, PNG_Info *img_struct, MT2D_FILE *fp) {
	if (img_struct) {
		if (img_struct->row_pointers) {
			for (unsigned int y = 0; y < img_struct->rows; y++) {
				free(img_struct->row_pointers[y]);
			}
			free(img_struct->row_pointers);
		}
		if (img_struct->png) {
			png_destroy_read_struct(&img_struct->png, img_struct->info ? &img_struct->info : NULL, NULL);
		}
		free(img_struct);
	}
	free(img);
	if (fp) {
		MT2D_FILE_CLOSE(fp);
	}
}

static Image *PNG_Fail(const char *message, char *filename, Image *img, PNG_Info *img_struct, MT2D_FILE *fp) {
	snprintf(str_buffer, sizeof(str_buffer), "PNG ERROR: %s: %s", message, filename);
	MT2D_MessageBox(str_buffer);
	PNG_Free(img, img_struct, fp);
	return 0;
}

Image *PNG_read_file(char *filename) {
	/* Only heap memory is changed after setjmp, so these locals keep their values when libpng longjmps back. */
	MT2D_FILE *fp = MT2D_FILE_OPEN(filename, "rb");
	if (!fp) {
		return PNG_Fail("unable to open the file", filename, 0, 0, 0);
	}
	Image *img = Image_CreateBlank();
	PNG_Info *img_struct = (PNG_Info*)calloc(1, sizeof(PNG_Info));
	if (!img || !img_struct) {
		return PNG_Fail("out of memory", filename, img, img_struct, fp);
	}

	png_byte header[8];
	if (MT2D_FILE_READ(fp, header, 1, sizeof(header)) != sizeof(header) || png_sig_cmp(header, 0, sizeof(header))) {
		return PNG_Fail("not a PNG file", filename, img, img_struct, fp);
	}

	img_struct->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!img_struct->png) {
		return PNG_Fail("unable to create the read struct", filename, img, img_struct, fp);
	}

	img_struct->info = png_create_info_struct(img_struct->png);
	if (!img_struct->info) {
		return PNG_Fail("unable to create the info struct", filename, img, img_struct, fp);
	}

	if (setjmp(png_jmpbuf(img_struct->png))) {
		return PNG_Fail("unable to decode the image", filename, img, img_struct, fp);
	}

	png_set_read_fn(img_struct->png, fp, PNG_Read_Data);
	png_set_sig_bytes(img_struct->png, sizeof(header));

	png_read_info(img_struct->png, img_struct->info);

	img->Width = png_get_image_width(img_struct->png, img_struct->info);
	img->Height = png_get_image_height(img_struct->png, img_struct->info);
	png_byte color_type = png_get_color_type(img_struct->png, img_struct->info);
	png_byte bit_depth = png_get_bit_depth(img_struct->png, img_struct->info);

	// Read any color_type into 8bit depth, RGBA format.
	// See http://www.libpng.org/pub/png/libpng-manual.txt

	if (bit_depth == 16)
		png_set_strip_16(img_struct->png);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(img_struct->png);

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(img_struct->png);

	if (png_get_valid(img_struct->png, img_struct->info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(img_struct->png);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if (color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(img_struct->png, 0xFF, PNG_FILLER_AFTER);

	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(img_struct->png);

	png_read_update_info(img_struct->png, img_struct->info);

	/* Kept in img_struct (not in locals) so the error path can free them. */
	img_struct->row_pointers = (png_bytep*)calloc(img->Height, sizeof(png_bytep));
	if (!img_struct->row_pointers) {
		png_error(img_struct->png, "out of memory");
	}
	img_struct->rows = img->Height;
	png_size_t row_bytes = png_get_rowbytes(img_struct->png, img_struct->info);
	for (unsigned int y = 0; y < img->Height; y++) {
		img_struct->row_pointers[y] = (png_byte*)malloc(row_bytes);
		if (!img_struct->row_pointers[y]) {
			png_error(img_struct->png, "out of memory");
		}
	}

	png_read_image(img_struct->png, img_struct->row_pointers);
	img->ImagePointer = img_struct;

	MT2D_FILE_CLOSE(fp);
	return img;
}

void PNG_Get_Pixel(Image *img, int X, int Y, Pixel *P) {
	PNG_Info *Pi = (PNG_Info*)img->ImagePointer;
	if (png_get_color_type(Pi->png, Pi->info) == PNG_COLOR_TYPE_RGB);
//		abort_("[process_file] input file is PNG_COLOR_TYPE_RGB but must be PNG_COLOR_TYPE_RGBA "
//			"(lacks the alpha channel)");

	if (png_get_color_type(Pi->png, Pi->info) != PNG_COLOR_TYPE_RGBA);
//		abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)",
//			PNG_COLOR_TYPE_RGBA, png_get_color_type(img->png, img->info));

		png_byte* row = Pi->row_pointers[Y];
		png_byte* ptr = &(row[X * 4]);
//			printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
//				x, y, ptr[0], ptr[1], ptr[2], ptr[3]);
		P->red = ptr[0];
		P->green = ptr[1];
		P->blue = ptr[2];
}