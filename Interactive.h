#pragma once
#include "Pixel.h"

// The STARTUP palette, set by the first 640x480 image converted. 0 until then.
extern Pixel *palette;

// Converts the image at PATH (interactive mode) and writes STARTUP.DAT, NOTCH.DAT or NETNOTCH.DAT, picked by the
// image size, into the current directory. left == 0 makes the final message say "exit" instead of "continue".
void GenerateHexenStartup(char *PATH, int left);
