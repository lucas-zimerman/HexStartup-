#pragma once
#include "Image.h"

Image *Image_Create_IndexedHexenGraphic(Image *image, Pixel *palette);
void Save_HexenPlanarLump(char *PATH, Image *HexenLump);
void Save_HexenBitmapLump(char *PATH, Image *HexenLump);