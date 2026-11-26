#include <stdio.h>
#include <stdlib.h>

#include <MT2D/MT2D.h>
#include <MT2D/MT2D_Terminal_Define.h>
#include <MT2D/ObjectCore/Sprites.h>
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include "SpriteImage.h"

extern char str_buffer[200];

static Image *Sprite_Fail(const char *message, char *filename) {
	snprintf(str_buffer, sizeof(str_buffer), "ERROR:%s (%s)", message, filename);
	MT2D_MessageBox(str_buffer);
	return 0;
}

Image *Sprite_read_file(char *filename) {
#if defined(MSDOS_USE_GRAPHICS) || defined(MT2D_LINUX_SIXEL)
	// Image sprites only keep palette indexes here, and loading one also replaces the screen palette with the palette of
	// the file (MT2D issue #73), so the file is not even opened.
	return Sprite_Fail("unsupported image format, use .png or .ppm", filename);
#else
	// Size 0 keeps the size of the file. A missing file gives NULL, a file the backend can not read gives the
	// "<!>" ASCII placeholder (type 0): both are failures here.
	Sprite *sprite = Load_Sprite_Image(filename, 0, 0);
	if (!sprite) {
		return Sprite_Fail("could not open the image", filename);
	}
	if (sprite->type != 1) {
		Sprite_Delete(sprite);
		return Sprite_Fail("unsupported image format, use .png or .ppm", filename);
	}
	MT2D_SpriteRaw *raw = MT2D_Sprite_To_SpriteRaw(sprite);
	Sprite_Delete(sprite);
	if (!raw) {
		return Sprite_Fail("could not read the pixels of the image", filename);
	}
	if (!raw->IsRgba) {
		MT2D_SpriteRaw_Free(raw);
		return Sprite_Fail("unsupported image format, use .png or .ppm", filename);
	}

	Image *img = Image_CreateBlank();
	img->Width = raw->Width;
	img->Height = raw->Height;
	img->ImagePointer = raw;
	return img;
#endif
}

void Sprite_Get_Pixel(Image *img, int X, int Y, Pixel *P) {
	MT2D_SpriteRaw *raw = (MT2D_SpriteRaw*)img->ImagePointer;
	const MT2D_RGBA *c = &raw->Data.Rgba[Y * raw->Width + X];
	P->red = c->R;
	P->green = c->G;
	P->blue = c->B;
}
