#pragma once
#include "Pixel.h"

typedef struct {
	int x, y;
	Pixel *data;
} PPMImage;

PPMImage *readPPM(const char *filename);
Pixel *Get_Palette(PPMImage *image, int paletteLength);
Pixel *PPM_GetPixel(PPMImage *img, int Offset);