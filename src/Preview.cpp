#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <MT2D/MT2D.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MT2D_Keyboard.h>
#include <MT2D/MT2D_Settings.h>
#include <MT2D/MT2D_System_Calls.h>
#include <MT2D/ObjectCore/Sprites.h>
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include "Image.h"
#include "Preview.h"

void screen_reset(); // Source.cpp

// Image sprites exist only on some backends (SDL window, Sixel terminal, MS-DOS VGA) and they take different raw formats:
// RGBA cells on SDL, palette indexes on Sixel and VGA. MT2D_Sprite_Create_From_Raw says which one fits by returning NULL,
// so nothing here depends on the platform.
static bool g_rgba = false;

bool Preview_Available() {
	static int available = -1;
	if (available < 0) {
		available = 0;
		for (int rgba = 1; rgba >= 0 && !available; rgba--) {
			MT2D_SpriteRaw *raw = MT2D_SpriteRaw_Create(1, 1, rgba != 0);
			Sprite *s = raw ? MT2D_Sprite_Create_From_Raw(raw, true, 0, 0) : 0;
			MT2D_SpriteRaw_Free(raw);
			if (s) {
				Sprite_Delete(s);
				g_rgba = rgba != 0;
				available = 1;
			}
		}
	}
	return available == 1;
}

// With index cells the screen has one palette. Index 0 is transparent and 16 / 31 are the text colors (black and
// white), so the 16 colors of the image go to these entries. Nothing happens on RGBA backends (MT2D_UNSUPPORTED).
static const int FirstColor = 100;

static void SetScreenPalette(const Pixel *colors) {
	int argv[1 + 256];
	memset(argv, 0, sizeof(argv));
	argv[0] = MT2D_SET_PALETTE;
	argv[1 + 16] = 0x000000;
	argv[1 + 31] = 0xFFFFFF;
	if (colors) {
		for (int i = 0; i < 16; i++) {
			argv[1 + FirstColor + i] = (colors[i].red << 16) | (colors[i].green << 8) | colors[i].blue;
		}
	}
	MT2D_Set_Settings_Argv(1 + 256, argv);
}

// What is shown: the palette indexes of the converted image and the colors they stand for.
struct PreviewData {
	unsigned char *Indexes;
	Pixel Colors[16];
	int Width, Height;
};

// Without a STARTUP image the palette of the lump is all black (the user was warned), which would make a NOTCH
// invisible. In that case the preview takes the colors from the source image instead.
static bool PaletteIsBlack(const Pixel *p) {
	if (!p) return true;
	for (int i = 0; i < 16; i++) {
		if (p[i].red || p[i].green || p[i].blue) return false;
	}
	return true;
}

static bool BuildPreviewData(Image *converted, Image *source, PreviewData *out) {
	int count = converted->Width * converted->Height;
	out->Width = converted->Width;
	out->Height = converted->Height;
	out->Indexes = (unsigned char*)malloc(count);
	if (!out->Indexes) return false;
	memset(out->Colors, 0, sizeof(out->Colors));
	if (!PaletteIsBlack(converted->Palette) || !source) {
		memcpy(out->Indexes, converted->ImagePointer, count);
		if (converted->Palette) memcpy(out->Colors, converted->Palette, sizeof(out->Colors));
		return true;
	}
	int used = 0;
	Pixel p;
	for (int i = 0; i < count; i++) {
		Image_GetPixel(source, i, &p);
		int j;
		for (j = 0; j < used; j++) {
			if (out->Colors[j].red == p.red && out->Colors[j].green == p.green && out->Colors[j].blue == p.blue) break;
		}
		if (j == used && used < 16) out->Colors[used++] = p;
		out->Indexes[i] = (unsigned char)(j < 16 ? j : 0);
	}
	return true;
}

// Raw cells for the indexes of an image, in the format of the backend. indexes == 0 makes a blank (transparent) raw.
static MT2D_SpriteRaw *MakeRaw(const unsigned char *indexes, int w, int h, const Pixel *colors) {
	MT2D_SpriteRaw *raw = MT2D_SpriteRaw_Create(w, h, g_rgba);
	if (!raw || !indexes) return raw;
	for (int i = 0; i < w * h; i++) {
		if (g_rgba) {
			const Pixel *c = &colors[indexes[i] & 15];
			raw->Data.Rgba[i].R = c->red;
			raw->Data.Rgba[i].G = c->green;
			raw->Data.Rgba[i].B = c->blue;
			raw->Data.Rgba[i].A = 255;
		}
		else {
			raw->Data.Cells[i] = (unsigned char)(FirstColor + (indexes[i] & 15));
		}
	}
	return raw;
}

// Where the notches go on the 640x480 startup screen and how many fit, as in the ZDoom start screen
// (startscreen_hexen.cpp): 16 * 32 / 16 = 32 NOTCHes from (64,441), 4 * 8 / 4 = 8 NETNOTCHes from (288,32).
static void NotchLayout(const Image *notch, int *x, int *y, int *count) {
	if (notch->Width == 4) {
		*x = 288;
		*y = 32;
		*count = (4 * 8) / notch->Width;
	}
	else {
		*x = 64;
		*y = 441;
		*count = (16 * 32) / notch->Width;
	}
}

// The MS-DOS VGA screen is 320x200 and draws image sprites at their real size, so a 640x480 image does not fit: there
// the arrow keys move the image. Everywhere else the image is scaled to the screen and any key closes the preview.
#ifdef MSDOS_USE_GRAPHICS
static const bool CanPan = true;
#else
static const bool CanPan = false;
#endif
static const int PanScreenW = 320, PanScreenH = 200, PanStep = 8;

static int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Reads the key that was pressed. Returns false when the preview should close. Arrow keys come as two values, the arrow
// marker and then the arrow (see MT2D_Keyboard.h), and move the offset (ox,oy) of the image when panning is on.
static bool HandleKey(int *ox, int *oy) {
	int key = MT2D_Keyboard_keytouched();
	if (!CanPan || key != arrow_key_pressed) {
		return false;
	}
	switch (MT2D_Keyboard_keytouched()) {
	case arrow_left_pressed:  *ox += PanStep; break;
	case arrow_right_pressed: *ox -= PanStep; break;
	case arrow_up_pressed:    *oy += PanStep; break;
	case arrow_down_pressed:  *oy -= PanStep; break;
	}
	*ox = Clamp(*ox, PanScreenW - 640, 0);
	*oy = Clamp(*oy, PanScreenH - 480, 0);
	return true;
}

// Shows the converted image until a key is pressed. A STARTUP image is shown as it is. A NOTCH / NETNOTCH is drawn
// over the converted STARTUP image (startup, with the palette they share), one more notch at a time, like the loading
// bar of Hexen. Without a STARTUP image yet, the notches are drawn over a black screen with their own colors.
void Preview_Show(Image *converted, Image *source, Image *startup) {
	PreviewData data;
	if (!BuildPreviewData(converted, source, &data)) {
		MT2D_MessageBox("ERROR: could not create the preview image.");
		return;
	}
	bool isStartup = converted->Width == 640 && converted->Height == 480;
	bool animated = !isStartup && startup && startup->Width == 640 && startup->Height == 480;
	bool blankBackground = !isStartup && !animated;
	int notchX = 0, notchY = 0, notchCount = 1;
	if (!isStartup) {
		NotchLayout(converted, &notchX, &notchY, &notchCount);
	}
	const Pixel *colors = data.Colors;
	if (animated && !PaletteIsBlack(startup->Palette)) {
		colors = startup->Palette; // the palette the notches share
	}

	MT2D_SpriteRaw *background = MakeRaw(isStartup ? data.Indexes : (animated ? (const unsigned char*)startup->ImagePointer : 0),
		640, 480, colors);
	MT2D_SpriteRaw *notch = isStartup ? 0 : MakeRaw(data.Indexes, data.Width, data.Height, colors);
	// The virtual screen of the image sprites is 320x240, the startup screen is drawn at half size.
	Sprite *s = background ? MT2D_Sprite_Create_From_Raw(background, true, 320, 240) : 0;
	if (!s || (!isStartup && !notch)) {
		MT2D_MessageBox("ERROR: could not create the preview sprite.");
		MT2D_SpriteRaw_Free(background);
		MT2D_SpriteRaw_Free(notch);
		if (s) Sprite_Delete(s);
		free(data.Indexes);
		return;
	}
	(void)blankBackground;
	SetScreenPalette(colors);
	// drop the key of the message box that was just answered, or the preview would end at once
	while (MT2D_Keyboard_touched()) {
		MT2D_Keyboard_keytouched();
	}
	// Offset of the image on the screen, only used when panning. A notch starts with its bar in view.
	int ox = 0, oy = 0;
	if (CanPan && !isStartup) {
		ox = Clamp(-notchX, PanScreenW - 640, 0);
		oy = Clamp(PanScreenH / 2 - notchY, PanScreenH - 480, 0);
	}
	if (isStartup) {
		do {
			MT2D_Clear_Main_Window();
			Sprite_Render_on_Window(s, DISPLAY_WINDOW1, ox, oy);
			MT2D_Draw_Window(DISPLAY_WINDOW1);
		} while (HandleKey(&ox, &oy));
	}
	else {
		// The notches fill in one by one, stay for a moment and start again, until a key closes it (MT2D has no mouse
		// input).
		int n = 0;
		bool open = true;
		while (open) {
			if (n == 0) {
				MT2D_Sprite_Alter_Block(s, 0, 0, background); // the screen without notches
			}
			else if (n <= notchCount) {
				MT2D_Sprite_Alter_Block(s, notchX + data.Width * (n - 1), notchY, notch);
			}
			MT2D_Clear_Main_Window();
			Sprite_Render_on_Window(s, DISPLAY_WINDOW1, ox, oy);
			MT2D_Draw_Window(DISPLAY_WINDOW1);
			MT2D_System_Delay(n == notchCount ? 1000 : 120);
			if (++n > notchCount) {
				n = 0;
			}
			while (open && MT2D_Keyboard_touched()) {
				open = HandleKey(&ox, &oy);
			}
		}
	}
	Sprite_Delete(s);
	MT2D_SpriteRaw_Free(background);
	MT2D_SpriteRaw_Free(notch);
	free(data.Indexes);
	SetScreenPalette(0);
	screen_reset();
}
