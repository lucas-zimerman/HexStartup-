#include <stdio.h>
#include <stdlib.h>
#include "PPM.h"
#include "Hexen.h"
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <vector>

extern char str_buffer[200];


Hexen_Startup_Lump *GetPPM_IndexedHexenStartupImage(PPMImage *image, PPMPixel *palette) {
	Hexen_Startup_Lump *Hex = (Hexen_Startup_Lump*)malloc(sizeof(Hexen_Startup_Lump));
	Hex->Ray_size = image->x * image->y;
	Hex->Raw = (unsigned char*)malloc(Hex->Ray_size * sizeof(unsigned char));
	int i = 0, j = 0, k = 0;
	//save the color palette struct
	Hex->Palette_size = 16;
	Hex->Palette = (PPMPixel*)malloc(Hex->Palette_size * sizeof(PPMPixel));
	for (i = 0; i < Hex->Palette_size; i++) {
		Hex->Palette[i].red = palette[i].red;
		Hex->Palette[i].green = palette[i].green;
		Hex->Palette[i].blue = palette[i].blue;
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
			*pln1 = ((read[0] & 0x01) << 7 | (read[1] & 0x01) << 6 | (read[2] & 0x01) << 5 | (read[3] & 0x01) << 4
				| (read[4] & 0x01) << 3 | (read[5] & 0x01) << 2 | (read[6] & 0x01) << 1 | (read[7] & 0x01));
			*pln2 = ((read[0] & 0x02) << 6 | (read[1] & 0x02) << 5 | (read[2] & 0x02) << 4 | (read[3] & 0x02) << 3
				| (read[4] & 0x02) << 2 | (read[5] & 0x02) << 1 | (read[6] & 0x02) | (read[7] & 0x02) >> 1);
			*pln3 = ((read[0] & 0x04) << 5 | (read[1] & 0x04) << 4 | (read[2] & 0x04) << 3 | (read[3] & 0x04) << 2
				| (read[4] & 0x04) << 1 | (read[5] & 0x04) | (read[6] & 0x04) >> 1 | (read[7] & 0x04) >> 2);
			*pln4 = ((read[0] & 0x08) << 4 | (read[1] & 0x08) << 3 | (read[2] & 0x08) << 2 | (read[3] & 0x08) << 1
				| (read[4] & 0x08) | (read[5] & 0x08) >> 1 | (read[6] & 0x08) >> 2 | (read[7] & 0x08) >> 3);
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
