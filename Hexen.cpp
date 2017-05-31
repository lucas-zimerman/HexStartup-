#include <stdio.h>
#include <stdlib.h>
#include "Image.h"
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <vector>

extern char str_buffer[200];

ImageFormat Hexen_GetImageType(unsigned int Width, unsigned int Height) {
	ImageFormat _return = TYPE_UNSUPPORTED;
	if(Width == 640 && Height == 480){
		_return = TYPE_Planar;
	}
	else if (Width == 4 && Height == 16) {
		_return = TYPE_4BitBitmap;
	}
	else if (Width == 16 && Height == 23) {
		_return = TYPE_4BitBitmap;
	}
	return _return;
}

/*DataArray load 8 bytes from the given index
**/
unsigned char Get_PlanarBitsFilter(unsigned char *DataArray, unsigned char Mask) {
	unsigned char Output = 0;
	int Moffset = Mask;
	int i = 1,j=0;
	while (Moffset != 1 /*0xb00000001*/) { //lets get how many steps the offset should be
		Moffset = Moffset >> 1;
		i++;
	}
	Moffset = i;
	for (j = 8,i=0; j > 0; j--, i++) {
		if (j - Moffset >= 0) {
			Output = Output | (DataArray[i] & Mask) << (j - Moffset);
		}
		else {
			Output = Output | (DataArray[i] & Mask) >> abs(j - Moffset); 
		}
		// we cant just let << -1 because for the internal code the number after >> is unsigned, so << -1 is  << 12u8348932u4 and not >> 1.
	}
	/* how the data came
		DataArray = {12345678,12345678,12345678,12345678,12345678,12345678,12345678,12345678,...};
	   what the output should look like
	    Output = 11111111; Output = 22222222; Output = 33333333; ... Output = 88888888;
	*/
	return Output;
}

Image *Image_Create_IndexedHexenGraphic(Image *image, Pixel *palette) {
	int Raw_size = image->Width * image->Height;
	unsigned char *Raw = (unsigned char*)malloc(Raw_size * sizeof(unsigned char));
	int i = 0, j = 0, k = 0;
	Pixel *tmp;
	//save the indexed color in a new struct
	j = 0;
	for (i = 0; i < Raw_size; i++) {
		for (k = 0; k < 16; k++) {
			tmp = Image_GetPixel(image, i);
			if (palette[k].red == tmp->red && palette[k].green == tmp->green && palette[k].blue == tmp->blue) {
				Raw[j] = k;
				j++;
				break;
			}
		}

	}
	Image *img = Image_CreateBlank();
	img->Height = image->Height;
	img->Width = image->Width;
	img->ImagePointer = Raw;
	img->Loaded = true;
	img->Palette = palette;
	img->Type = Hexen_GetImageType(img->Width,img->Height);
	return img;
}

void Save_HexenPlanarLump(char *PATH, Image *HexenLump) {
	FILE *f = fopen(PATH, "wb");
	if (!f) {
		sprintf(str_buffer, "When trying to create the file %s:Error: %d (%s)", PATH, errno, strerror(errno));
		MT2D_MessageBox(str_buffer);
		return;
	}
	int x0, y0, PlanarSize;
	bool NoPalette = false;

	if (HexenLump->Width == 640 && HexenLump->Height == 480) {
		//check if the file is a startup lump
		x0 = 640;
		y0 = 480;
		PlanarSize = (x0*y0) / 2;
	}
	/*there's no other format size, for now...*/
	if (!NoPalette) {
		// Create planar palette
		uint8_t* mycolors = new uint8_t[3];
		for (size_t i = 0; i < 16; ++i)
		{
			mycolors[0] = HexenLump->Palette[i].red >> 2;
			mycolors[1] = HexenLump->Palette[i].green >> 2;
			mycolors[2] = HexenLump->Palette[i].blue >> 2;
			fputc(mycolors[0], f);
			fputc(mycolors[1], f);
			fputc(mycolors[2], f);
		}
		delete[] mycolors;
	}

	// Create bitplanes
	uint8_t* planes = new uint8_t[PlanarSize];

	uint8_t* pln1, *pln2, *pln3, *pln4, *read;
	size_t plane_size = PlanarSize / 4;

	read = (uint8_t*)HexenLump->ImagePointer;
	pln1 = planes;				// 80: 10000000	08: 00001000
	pln2 = pln1 + plane_size;	// 40: 01000000 04: 00000100
	pln3 = pln2 + plane_size;	// 20: 00100000 02: 00000010
	pln4 = pln3 + plane_size;	// 10: 00010000 01: 00000001

	for (int y = y0; y > 0; --y)
	{
		for (int x = x0; x > 0; x -= 8)
		{
			*pln1 = Get_PlanarBitsFilter(&read[0], 1 << 0/*0xb00000001*/); //pln1 receives the 8 1st bits from the first 8 arrays
			*pln2 = Get_PlanarBitsFilter(&read[0], 1 << 1/*0xb00000010*/); //pln2 receives the 8 2nd bits from the first 8 arrays
			*pln3 = Get_PlanarBitsFilter(&read[0], 1 << 2/*0xb00000100*/); //pln3 receives the 8 3rd bits from the first 8 arrays
			*pln4 = Get_PlanarBitsFilter(&read[0], 1 << 3/*0xb00001000*/); //pln4 receives the 8 4rt bits from the first 8 arrays
			read += 8;
			pln1 += 1;
			pln2 += 1;
			pln3 += 1;
			pln4 += 1;
		}
	}

	// Write image and cleanup
	for (int i = 0; i < PlanarSize; i++) {
		fputc(planes[i], f);
	}
	delete[] planes;
	fclose(f);
}

void Save_HexenBitmapLump(char *PATH, Image *HexenLump) {
	FILE *f = fopen(PATH, "wb");
	int x0, y0, BitmapSize;
	unsigned char *Raw = (unsigned char*)HexenLump->ImagePointer;

	if (HexenLump->Width == 4 && HexenLump->Height == 16) {
		//check if the file is a NETNOTCH lump
		x0 = 4;
		y0 = 16;
		BitmapSize = (x0*y0) / 2;
	}
	else/* if (HexenLump->Ray_size == 16 * 23) */ {
		//check if the file is a notch
		x0 = 16;
		y0 = 23;
		BitmapSize = (x0*y0) / 2;
	}

	// Create 4-bit bitmap
	uint8_t* temp = new uint8_t[BitmapSize];

	for (int i = 0; i < HexenLump->Width * HexenLump->Height; i += 2)
	{
		// AAAABBBB = 0000AAAA << 4 |  0000BBBB
		temp[i / 2] = Raw[i] << 4 | Raw[i + 1];
	}

	// Write image and cleanup
	for (int i = 0; i < BitmapSize; i++) {
		fputc(temp[i], f);
	}
	delete[] temp;
	fclose(f);
}
