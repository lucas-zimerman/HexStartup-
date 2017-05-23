#pragma once
#include "PPM.h"

typedef struct {
	unsigned char *Raw;
	int Ray_size;
	PPMPixel *Palette;
	int Palette_size;
} Hexen_Startup_Lump;

Hexen_Startup_Lump *GetPPM_IndexedHexenStartupImage(PPMImage *image, PPMPixel *palette);
void Save_HexenPlanarLump(char *PATH, Hexen_Startup_Lump *HexenLump);
void Save_HexenBitmapLump(char *PATH, Hexen_Startup_Lump *HexenLump);