#pragma once
/*
PPMImage *readPPM(const char *filename);
Pixel *Get_Palette(PPMImage *image, int paletteLength);

*/
#include <png.h>

struct PNG_Info {
	png_structp png;
	png_infop info;
	png_bytep *row_pointers;
};

Image *PNG_read_file(char *filename);
void PNG_Get_Pixel(Image *img, int X, int Y, Pixel *P);