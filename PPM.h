#pragma once
typedef struct {
	unsigned char red, green, blue;
} PPMPixel;

typedef struct {
	int x, y;
	PPMPixel *data;
} PPMImage;

PPMImage *readPPM(const char *filename);
PPMPixel *Get_Palette(PPMImage *image, int *paletteLength);