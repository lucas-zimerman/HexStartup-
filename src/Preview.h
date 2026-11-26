#pragma once
#include "Image.h"

// True when the MT2D backend can draw images (SDL window, Sixel terminal or MS-DOS VGA mode).
bool Preview_Available();

// Shows the converted image (indexes + its palette) until a key is pressed. source is the image that was loaded: its
// colors are used when the converted image has no palette (a NOTCH converted before any STARTUP image). NOTCH and
// NETNOTCH are drawn notch by notch over startup (the last converted STARTUP image, or 0), like the Hexen loading bar.
void Preview_Show(Image *converted, Image *source, Image *startup);
