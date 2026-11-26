#pragma once
#include "Image.h"

// Fallback loader for the formats HexStartup has no reader of its own for (anything that is not .png or .ppm).
// The file is opened with MT2D's Load_Sprite_Image, so what can be read depends on the MT2D backend: every format
// SDL_image knows on SDL builds (BMP, GIF, JPG, TGA, ...). MS-DOS VGA and Linux Sixel are left out for now: their
// sprites only keep palette indexes and loading one replaces the screen palette (MT2D issue #73). There, and on builds
// without image sprites, the load fails with a message.
Image *Sprite_read_file(char *filename);
void Sprite_Get_Pixel(Image *img, int X, int Y, Pixel *P);
