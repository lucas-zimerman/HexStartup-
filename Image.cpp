#include <stdlib.h>
#include <string.h>
#include "Image.h"
#include "_png.h"
#include "PPM.h"
#include "Hexen.h"
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MT2D.h>

extern char str_buffer[200];

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
	png_info *png;

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
			break;
		case TYPE_PNG:
			Img = PNG_read_file(PATH);
			Img->Loaded = true;
			Img->Type = Type;
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
void Image_GetPixel(Image *img, unsigned int X, unsigned int Y, Pixel *p) {
	switch (img->Type) {
	case TYPE_PPM:
		Image_GetPixel(img, X + Y * img->Width,p);
		break;
	case TYPE_PNG:
		PNG_Get_Pixel(img, X, Y,p);
	}
}

void Image_GetPixel(Image *img,int offset,Pixel *p) {
	int X, Y;
	switch (img->Type) {
	case TYPE_PPM:
		PPM_GetPixel((PPMImage*)img->ImagePointer, offset, p);
		break;
	case TYPE_PNG:
		Y = offset / img->Width;
		X = offset - Y * img->Width;
		Image_GetPixel(img, X, Y,p);
		break;
	default:

		Y = offset / img->Width;
		X = offset - Y * img->Width;
		break;
	}

}


Pixel *Image_GetPalette(Image *image, int paletteLength) {
	int i = 0, j = 0;
	int ImgLength = image->Width * image->Height;
	Pixel *palette = (Pixel*)malloc(paletteLength * sizeof(Pixel));//hard coded
	Pixel *tmp = (Pixel*)malloc(sizeof(Pixel));;
	bool *palette_started = (bool*)malloc(paletteLength * sizeof(bool));
	for (int i = 0; i < paletteLength; i++) {
		palette[i].blue = 0;
		palette[i].green = 0;
		palette[i].red = 0;
		palette_started[i] = false;
	}
	int disp_pos = 8;
	insert_string_on_display("INDEX  R   ,G    ,B", 7, 10, DISPLAY_WINDOW1);
	for (i = 0; i < ImgLength; i++) {
		for (j = 0; j < paletteLength; j++) {
			Image_GetPixel(image, i,tmp);
			if (palette[j].red == tmp->red && palette[j].green == tmp->green && palette[j].blue == tmp->blue) {
				//we found the same color on index, jump.
				break;
			}
			if (palette_started[j] == false) {
				//color not found in the index, jump.
				palette[j].red = tmp->red;
				palette[j].green = tmp->green;
				palette[j].blue = tmp->blue;
				palette_started[j] = true;
				/*MT2D STUFF*/
				sprintf(str_buffer, "%2d    %3d  ,%3d  ,%3d", j, palette[j].red, palette[j].green, palette[j].blue);
				insert_string_on_display(str_buffer, disp_pos, 10, DISPLAY_WINDOW1);
				disp_pos++;
				MT2D_Draw_Window(DISPLAY_WINDOW1);
				/*=========*/
				break;
			}
		}
		if (j == 16) {
			sprintf(str_buffer, "ERROR:this image has more than 16 colors, you can fix that with the  gimp software... More info here:https://docs.gimp.org/en/gimp-image-convert-indexed.html");
			MT2D_MessageBox(str_buffer);
			free(tmp);
			return 0;
		}
	}
	free(tmp);
	return palette;
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