#include <stdio.h>
#include <stdlib.h>

#include <MT2D/MT2D.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MT2D_Keyboard.h>
#include <MT2D/MT2D_System_Calls.h>
#include <MT2D/InputBox/MT2D_InputBox_String.h>
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <MT2D/MessageBox/MT2D_MessageBox_With_Result.h>
#include "PPM.h"
#include "Hexen.h"

char str_buffer[200];





void screen_reset() {
	MT2D_Clear_Main_Window();
	insert_string_on_display("+++++++++++++++", 20, 60, DISPLAY_WINDOW1);
	insert_string_on_display("+  powered by +", 21, 60, DISPLAY_WINDOW1);
	insert_string_on_display("+ MT2D ENGINE +", 22, 60, DISPLAY_WINDOW1);
	insert_string_on_display("+++++++++++++++", 23, 60, DISPLAY_WINDOW1);
	insert_string_on_display("HexStartup++ v1.0, created by IBM5155", 24, 40, DISPLAY_WINDOW1);
	MT2D_Draw_Window(DISPLAY_WINDOW1);
}


Hexen_Startup_Lump *HexenLumps = 0;
PPMImage *Images = 0;
int       ImagesCnt =0;
PPMPixel *palette = 0;


void SaveHexenLump(int Index) {
	if (HexenLumps[Index].Ray_size == 640 * 480) {
		Save_HexenPlanarLump("STARTUP.DAT", &HexenLumps[Index]);
	}
	else if (HexenLumps[Index].Ray_size == 16 * 23) {
		Save_HexenBitmapLump("NOTCH.DAT", &HexenLumps[Index]);
	}
	else if (HexenLumps[Index].Ray_size == 4 * 16) {
		Save_HexenBitmapLump("NETNOTCH.DAT", &HexenLumps[Index]);
	}
}


void GenerateHexenStartup(char * PATH, int left) {
	int palette_Length = 0;
	int HexenIMG_Size = 0;

	for (int i = 0; i < 4; i++) {
		insert_string_on_display("                                              ", i, 0, DISPLAY_WINDOW1);
	}

	insert_string_on_display("PART 1 - LOADING IMAGE:", 0, 0, DISPLAY_WINDOW1);
	MT2D_Draw_Window(DISPLAY_WINDOW1);
	PPMImage *image = readPPM(PATH);
	if (image) {
		insert_string_on_display(" OK    ", 0, 25, DISPLAY_WINDOW1);
	}
	else {
		insert_string_on_display(" FAILED", 0, 25, DISPLAY_WINDOW1);
		return;
	}
	insert_string_on_display("PART 2 - CREATING PALETTE:", 1, 0, DISPLAY_WINDOW1);
	MT2D_Draw_Window(DISPLAY_WINDOW1);
	if (image->x == 640 && image->y == 480) {
		palette = Get_Palette(image, 16);
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
	Hexen_Startup_Lump *IndexedImage = GetPPM_IndexedHexenStartupImage(image, palette);
	if (IndexedImage) {
		insert_string_on_display(" OK     ", 2, 32, DISPLAY_WINDOW1);
	}
	else {
		insert_string_on_display(" FAILED", 2, 32, DISPLAY_WINDOW1);
		return;
	}


	if (IndexedImage->Ray_size == 640 * 480) {
		insert_string_on_display("PART 4 - GENERATING PLANAR STARTUP:          ", 3, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		Save_HexenPlanarLump("STARTUP.DAT", IndexedImage);
	}
	else if (IndexedImage->Ray_size == 16 * 23) {
		insert_string_on_display("PART 4 - GENERATING BITMAP NOTCH:          ", 3, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		Save_HexenBitmapLump("NOTCH.DAT", IndexedImage);
	}
	else if (IndexedImage->Ray_size == 4 * 16) {
		insert_string_on_display("PART 4 - GENERATING BITMAP NOTCH:          ", 3, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		Save_HexenBitmapLump("NETNOTCH.DAT", IndexedImage);
	}
	else {
		insert_string_on_display("PART 4 - UNKNOWN TYPE OF LUMP TO BE GENERATED", 3, 0, DISPLAY_WINDOW1);
		sprintf(str_buffer, "ERROR: file %s has a non standart size (%dx%d)", PATH, image->x, image->y);
		MT2D_MessageBox(str_buffer);
		return;
	}
	insert_string_on_display(" OK       ", 3, 35, DISPLAY_WINDOW1);
	sprintf(str_buffer, "File '%s'  was created in the image folder.                     Press any key to %s", PATH, (left == 0 ? "exit." : "continue."));
	MT2D_MessageBox(str_buffer);
}
/*
void menu() {
	draw
}
*/
void main(int argc, char *argv[]) {


/*  MT2D STUFF*/
	MT2D_Init();
	screen_reset();
	/*  ==========*/
	if (argc == 1) {
		char *PATH = 0;
		FILE *test;
		do {
			//PART 1: Get a valid file + PATH in case the file is in a folder other than the one where this software is
			do {
				PATH = MT2D_InputBox_String("type the name of the STARTUP.ppm image (and the file address if the image is in   another folder) - example 'test.ppm' ,' 'c:/test/img.ppm'");
				test = fopen(PATH, "r");
				if (!test) {
					if (MT2D_MessageBox_With_Result("FILE NOT FOUND!", "Do you want to retry or exit?", "try again", "Exit") == 2) {
						exit(1);
					}
					else {
						if (PATH) {
							free(PATH);
						}
						PATH = 0;
					}
				}
				else {
					fclose(test);
				}
			} while (!PATH);
			// END PART 1
			GenerateHexenStartup(PATH, 0);
		}while(MT2D_MessageBox_With_Result("MessageBox", "Do you want to load another file?", "Yes", "No") == 1);

	}
	else {
		//lets find the main palette, apply to all the others images and also save as planar or 4bit bitmap
		int i = 1;
		int PaletteLength = 0;
		ImagesCnt = argc;
		sprintf(str_buffer, "PART 1 - LOADING %d IMAGE%s:", argc-1,(argc==2? "" : "S"));
		insert_string_on_display(str_buffer, 0, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		Images = (PPMImage*)malloc(ImagesCnt * sizeof(PPMImage));
		for (i = 1; i < argc; i++) {
			Images[i - 1] = *readPPM(argv[i]);
		}
		insert_string_on_display(" OK", 0, 28, DISPLAY_WINDOW1);
		//if we reach this area, we have an array of valid images.
		insert_string_on_display("PART 2 - CREATING PALETTE:", 1, 0, DISPLAY_WINDOW1);
		for (i = 0; i < argc; i++) {
			if (Images[i].x == 640 && Images[i].y == 480) {
				palette = Get_Palette(&Images[i], 16);
				break;
			}
		}
		if (!palette) {
			insert_string_on_display(" FAILED", 1, 27, DISPLAY_WINDOW1);
			sprintf(str_buffer, "WARNING!: no STARTUP image found, you need this file in order to generate the palette for NOTCH and NETNOTCH. The files are going to be saved but with all palette set to zero.");
			MT2D_MessageBox(str_buffer);
		}
		else {
			insert_string_on_display(" OK", 1, 27, DISPLAY_WINDOW1);
		}
		//if we reach this area, we have a valid palette
		insert_string_on_display("PART 3 - CREATING INDEXED IMAGE:", 2, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		HexenLumps = (Hexen_Startup_Lump*)malloc(ImagesCnt * sizeof(Hexen_Startup_Lump));
		//apply the palette to all those images
		for (i = 0; i < argc -1; i++) {
			HexenLumps[i] = *GetPPM_IndexedHexenStartupImage(&Images[i], palette);
		}
		insert_string_on_display(" OK", 2, 32, DISPLAY_WINDOW1);
		//if we reach this area, we have a valid palette
		//now we just need to save all the input images in all their respective formats
		insert_string_on_display("PART 4 - GENERATING HEXEN LUMPS:", 3, 0, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		for (i = 0; i < argc -1; i++) {
			SaveHexenLump(i);
		}
		insert_string_on_display(" OK", 3, 35, DISPLAY_WINDOW1);
		MT2D_Draw_Window(DISPLAY_WINDOW1);
		//if we reach this area, we saved all the image files.
		MT2D_MessageBox("All the input files were successfully converted and were saved in the folder where the images were.");
	}
}