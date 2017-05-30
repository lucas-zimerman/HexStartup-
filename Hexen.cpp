#include <stdio.h>
#include <stdlib.h>
#include "PPM.h"
#include "Hexen.h"
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <vector>

extern char str_buffer[200];
/*DataArray loads 8 bytes from the given index
**/
unsigned char Get_PlanarBitsFilter(unsigned char *DataArray, unsigned char Mask) {
	unsigned char Output = 0;
	int Moffset = Mask;
	int i = 1,j=0;
	while (Moffset != 1 /*0xb00000001*/) { //lets get how much we hould move
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
	}
	return Output;
}

Hexen_Startup_Lump *GetPPM_IndexedHexenStartupImage(PPMImage *image, PPMPixel *palette) {
	Hexen_Startup_Lump *Hex = (Hexen_Startup_Lump*)malloc(sizeof(Hexen_Startup_Lump));
	Hex->Ray_size = image->x * image->y;
	Hex->Raw = (unsigned char*)malloc(Hex->Ray_size * sizeof(unsigned char));
	int i = 0, j = 0, k = 0;
	//save the color palette struct
	Hex->Palette_size = 16;
	Hex->Palette = (PPMPixel*)malloc(Hex->Palette_size * sizeof(PPMPixel));
	for (i = 0; i < Hex->Palette_size; i++) {
		if (palette) {
			Hex->Palette[i].red = palette[i].red;
			Hex->Palette[i].green = palette[i].green;
			Hex->Palette[i].blue = palette[i].blue;
		}
		else {
			//happens when the user load notch or netnotch but not startup palette
			Hex->Palette[i].red = 0;
			Hex->Palette[i].green = 0;
			Hex->Palette[i].blue = 0;
		}
	}
	//save the indexed color
	j = 0;
	for (i = 0; i < image->x * image->y; i++) {
		for (k = 0; k < Hex->Palette_size; k++) {
			if (palette[k].red == image->data[i].red && palette[k].green == image->data[i].green && palette[k].blue == image->data[i].blue) {
				Hex->Raw[j] = k;
				j++;
				break;
			}
		}

	}
	return Hex;
}

void Save_HexenPlanarLump(char *PATH, Hexen_Startup_Lump *HexenLump) {
	FILE *f = fopen(PATH, "wb");
	if (!f) {
		sprintf(str_buffer, "When trying to create the file %s:Error: %d (%s)", PATH, errno, strerror(errno));
		MT2D_MessageBox(str_buffer);
		return;
	}
	int x0, y0, PlanarSize;
	bool NoPalette = false;

	if (HexenLump->Ray_size == 640 * 480) {
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

	read = HexenLump->Raw;
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

void Save_HexenBitmapLump(char *PATH, Hexen_Startup_Lump *HexenLump) {
	FILE *f = fopen(PATH, "wb");
	int x0, y0, BitmapSize;

	if (HexenLump->Ray_size == 4 * 16) {
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

	for (int i = 0; i < HexenLump->Ray_size; i += 2)
	{
		// AAAABBBB = 0000AAAA << 4 |  0000BBBB
		temp[i / 2] = HexenLump->Raw[i] << 4 | HexenLump->Raw[i + 1];
	}

	// Write image and cleanup
	for (int i = 0; i < BitmapSize; i++) {
		fputc(temp[i], f);
	}
	delete[] temp;
	fclose(f);
}
