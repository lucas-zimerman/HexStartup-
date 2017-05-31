#include <stdlib.h>
#include <string.h>
#include "Image.h"
#include "png.h"
#include "PPM.h"
#include "Hexen.h"

ImageFormat PATH_GetType(char *PATH) {
	ImageFormat _return= TYPE_UNSUPPORTED;
	int i;
	i = strlen(PATH);
	if (i > 3) {
		if (PATH[i - 3] == 'P' || PATH[i - 3] == 'p') {
			if (PATH[i - 2] == 'P' || PATH[i - 2] == 'p') {
				if (PATH[i - 1] == 'M' || PATH[i - 1] == 'm') {
					_return = TYPE_PPM;
				}
			}
			if (PATH[i - 2] == 'N' || PATH[i - 2] == 'n') {
				if (PATH[i - 1] == 'G' || PATH[i - 1] == 'g') {
					_return = TYPE_PNG;
				}
			}
		}
	}
	return _return;
}

Image *Image_CreateBlank() {
	Image *img = (Image*)malloc(sizeof(Image));
	img->Height = 0;
	img->Width = 0;
	img->ImagePointer = 0;
	img->Loaded = false;
	img->Palette = 0;
	img->Saved = false;
	img->Type = TYPE_UNSUPPORTED;
	return img;
}

Image *Image_Load(char*PATH) {
	Image *Img = 0;
	ImageFormat Type;

	PPMImage *ppmimg;

	if (PATH) {
		Type = PATH_GetType(PATH);
		switch (Type) {
		case TYPE_PPM:
			ppmimg = readPPM(PATH);
			if (ppmimg) {
				Img = Image_CreateBlank();
				Img->Height = ppmimg->y;
				Img->Width = ppmimg->x;
				Img->ImagePointer = ppmimg;
				Img->Loaded = true;
				Img->Type = Type;
			break;
		}
		case TYPE_PNG:
			break;
		}
	}
	return Img;
}
bool Image_Save(Image *img, char *PATH) {
	if (img) {
		if (PATH) {
			if (img->Loaded) {
				switch (img->Type)
				{
				case TYPE_Planar:
					Save_HexenPlanarLump(PATH, img);
					img->Saved = true;
					break;
				case TYPE_4BitBitmap:
					Save_HexenBitmapLump(PATH, img);
					img->Saved = true;
					break;
				default:
					break;
				}
			}
		}
	}
	return img->Saved;
}
Pixel *Image_GetPixel(Image *img, unsigned int X, unsigned int Y) {
	Pixel *p =0;
	switch (img->Type) {
	case TYPE_PPM:
		p = Image_GetPixel(img, X + Y * img->Width);
		break;
	}
	return p;
}

Pixel *Image_GetPixel(Image *img,int offset) {
	int X, Y;
	Pixel *p =0;
	switch (img->Type) {
	case TYPE_PPM:
		p = PPM_GetPixel((PPMImage*)img->ImagePointer, offset);
		break;
	default:

		Y = offset / img->Width;
		X = offset - Y * img->Width;
		break;
	}

	return p;
}


Pixel *Image_GetPalette(Image *image, int paletteLength) {
	Pixel *Palette = 0;
	switch(image->Type){
	case TYPE_PPM:
		Palette = Get_Palette((PPMImage*)image->ImagePointer, paletteLength);
		break;
	case TYPE_PNG:
		break;
	}
	return Palette;
}

Image *Image_ImageConvert(Image *image, Pixel *Palette, ImageFormat ConverTo){
	Image *img =0;
	if (image) {
		if (image->Type != TYPE_UNSUPPORTED) {
			switch (ConverTo) {
			case TYPE_4BitBitmap:
			case TYPE_Planar:
				img = Image_Create_IndexedHexenGraphic(image, Palette);
				break;
			}
		}
	}
	return img;
}