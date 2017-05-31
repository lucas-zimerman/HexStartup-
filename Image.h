#pragma once
#include "Pixel.h"

enum ImageFormat {
	TYPE_UNSUPPORTED = -1,
	TYPE_PPM,
	TYPE_PNG,
	TYPE_Planar,
	TYPE_4BitBitmap
};

struct Image {
	void *ImagePointer;
	unsigned int Width;
	unsigned int Height;
	ImageFormat Type;
	Pixel *Palette;
	bool Loaded;
	bool Saved;

};

Image *Image_CreateBlank();
Image *Image_Load(char*PATH);
bool Image_Save(Image *img,char *PATH);
Pixel *Image_GetPixel(Image *img, unsigned int X, unsigned int Y);
Pixel *Image_GetPixel(Image *img,int offset);
Pixel *Image_GetPalette(Image *image, int paletteLength);
Image *Image_ImageConvert(Image *image,Pixel *Palette, ImageFormat ConverTo);