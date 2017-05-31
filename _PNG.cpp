#include <stdlib.h>
#include "Image.h"
#include <png.h>
#include "_PNG.h"
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MT2D.h>

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


Image *PNG_read_file(char *filename) {
	Image *img = Image_CreateBlank();
	PNG_Info *img_struct = (PNG_Info*)malloc(sizeof(PNG_Info));
	FILE *fp = fopen(filename, "rb");

	img_struct->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!img_struct->png) {
		sprintf(str_buffer, "PNG ERROR:Unable to read image:", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	img_struct->info = png_create_info_struct(img_struct->png);
	if (!img_struct->info) {
		sprintf(str_buffer, "PNG ERROR:Unable to create the info of from this image: ", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	if (setjmp(png_jmpbuf(img_struct->png))) {
		sprintf(str_buffer, "PNG ERROR:I really dont know whats this error is but if the original author said it's an error, so is it. : ", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	png_init_io(img_struct->png, fp);

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

	png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * img->Height);
	for (int y = 0; y < img->Height; y++) {
		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(img_struct->png, img_struct->info));
	}

	png_read_image(img_struct->png, row_pointers);
	img_struct->row_pointers = row_pointers;
	img->ImagePointer = img_struct;

	fclose(fp);
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