#include <stdio.h>

#include <MT2D/MT2D.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include "Image.h"
#include "Interactive.h"

extern char str_buffer[200];

// Shared with the command line mode in Source.cpp. Interactive mode keeps it between files, so a NOTCH or NETNOTCH
// converted after a STARTUP image in the same session uses the STARTUP palette.
Pixel *palette = 0;

void GenerateHexenStartup(char * PATH, int left) {
	int palette_Length = 0;
	int HexenIMG_Size = 0;

	for (int i = 0; i < 4; i++) {
		insert_string_on_display("                                              ", i, 0, DISPLAY_WINDOW1);
	}

	insert_string_on_display("PART 1 - LOADING IMAGE:", 0, 0, DISPLAY_WINDOW1);
	MT2D_Draw_Window(DISPLAY_WINDOW1);
	Image *image = Image_Load(PATH);
	if (image) {
		insert_string_on_display(" OK    ", 0, 25, DISPLAY_WINDOW1);
	}
	else {
		insert_string_on_display(" FAILED", 0, 25, DISPLAY_WINDOW1);
		return;
	}
	insert_string_on_display("PART 2 - CREATING PALETTE:", 1, 0, DISPLAY_WINDOW1);
	MT2D_Draw_Window(DISPLAY_WINDOW1);
	if (image->Width == 640 && image->Height == 480) {
		palette = Image_GetPalette(image, 16);
	}
	if (!palette) {
		insert_string_on_display(" FAILED", 1, 27, DISPLAY_WINDOW1);
		sprintf(str_buffer, "WARNING!: no STARTUP image found, you need this file in order to generate the palette for NOTCH and NETNOTCH. The files are going to be saved but with all palette set to zero.");
		MT2D_MessageBox(str_buffer);
	}
	else {
		insert_string_on_display(" OK      ", 1, 27, DISPLAY_WINDOW1);
	}

	insert_string_on_display("PART 3 - CREATING INDEXED IMAGE:", 2, 0, DISPLAY_WINDOW1);
	MT2D_Draw_Window(DISPLAY_WINDOW1);
	Image *IndexedImage = Image_ImageConvert(image, palette,TYPE_Planar /*or 4BitBMP*/);
	if (IndexedImage) {
		insert_string_on_display(" OK     ", 2, 32, DISPLAY_WINDOW1);
	}
	else {
		insert_string_on_display(" FAILED", 2, 32, DISPLAY_WINDOW1);
		return;
	}


	if (IndexedImage->Width * IndexedImage->Height == 640 * 480) {
		insert_string_on_display("PART 4 - GENERATING PLANAR STARTUP:          ", 3, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		Image_Save(IndexedImage, "STARTUP.DAT");
	}
	else if (IndexedImage->Width * IndexedImage->Height == 16 * 23) {
		insert_string_on_display("PART 4 - GENERATING BITMAP NOTCH:          ", 3, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		Image_Save(IndexedImage, "NOTCH.DAT");
	}
	else if (IndexedImage->Width * IndexedImage->Height == 4 * 16) {
		insert_string_on_display("PART 4 - GENERATING BITMAP NOTCH:          ", 3, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		Image_Save(IndexedImage, "NETNOTCH.DAT");
	}
	else {
		insert_string_on_display("PART 4 - UNKNOWN TYPE OF LUMP TO BE GENERATED", 3, 0, DISPLAY_WINDOW1);
		sprintf(str_buffer, "ERROR: file %s has a non standart size (%dx%d)", PATH, image->Width, image->Height);
		MT2D_MessageBox(str_buffer);
		return;
	}
	insert_string_on_display(" OK       ", 3, 35, DISPLAY_WINDOW1);
	sprintf(str_buffer, "File '%s'  was created in the image folder.                     Press any key to %s", PATH, (left == 0 ? "exit." : "continue."));
	MT2D_MessageBox(str_buffer);
}
