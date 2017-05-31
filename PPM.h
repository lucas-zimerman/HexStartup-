#pragma once
#include "Pixel.h"

typedef struct {
	int x, y;
	Pixel *data;
} PPMImage;

PPMImage *readPPM(const char *filename);
void PPM_GetPixel(PPMImage *img, int Offset, Pixel *p);