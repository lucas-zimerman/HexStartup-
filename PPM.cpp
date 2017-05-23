#include <stdio.h>
#include <stdlib.h>
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MT2D.h>
#include "PPM.h"


extern char str_buffer[200];


#define RGB_COMPONENT_COLOR 255

PPMImage *readPPM(const char *filename)
{//Code Created by: RPFELGUEIRAS
	char buff[16];
	PPMImage *img;
	FILE *fp;
	int c, rgb_comp_color;
	//open PPM file for reading
	fp = fopen(filename, "rb");
	if (!fp) {
		//		fprintf(stderr, "Unable to open file '%s'\n", filename);
		sprintf(str_buffer, "ERROR:Unable to open file '%s'", filename);
		MT2D_MessageBox(str_buffer);
		return 0;
	}

	//read image format
	if (!fgets(buff, sizeof(buff), fp)) {
		perror(filename);
		sprintf(str_buffer, "ERROR:Unable to read image format", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	//check the image format
	if (buff[0] != 'P' || buff[1] != '6') {
		//		fprintf(stderr, "Invalid image format (must be 'P6')\n");
		sprintf(str_buffer, "ERROR:Invalid image format (must be 'P6')                            Are you loading a .ppm image? %s   ", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	//alloc memory form image
	img = (PPMImage *)malloc(sizeof(PPMImage));
	if (!img) {
		//		fprintf(stderr, "Unable to allocate memory\n");
		sprintf(str_buffer, "ERROR:Unable to allocate memory");
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	//check for comments
	c = getc(fp);
	while (c == '#') {
		while (getc(fp) != '\n');
		c = getc(fp);
	}

	ungetc(c, fp);
	//read image size information
	if (fscanf(fp, "%d %d", &img->x, &img->y) != 2) {
		//		fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
		sprintf(str_buffer, "ERROR:Invalid image size (error loading '%s')", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	//read rgb component
	if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
		//		fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
		sprintf(str_buffer, "ERROR:Invalid rgb component (error loading '%s')", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	//check rgb component depth
	if (rgb_comp_color != RGB_COMPONENT_COLOR) {
		//		fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
		sprintf(str_buffer, "ERROR:'%s' does not have 8-bits components\n", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	while (fgetc(fp) != '\n');
	//memory allocation for pixel data
	img->data = (PPMPixel*)malloc(img->x * img->y * sizeof(PPMPixel));

	if (!img) {
		//		fprintf(stderr, "Unable to allocate memory\n");
		sprintf(str_buffer, "ERROR:Unable to allocate memory");
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	//read pixel data from file
	if (fread(img->data, 3 * img->x, img->y, fp) != img->y) {
		//		fprintf(stderr, "Error loading image '%s'\n", filename);
		sprintf(str_buffer, "Error loading image '%s'\n", filename);
		MT2D_MessageBox(str_buffer);
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return img;
}

PPMPixel *Get_Palette(PPMImage *image, int paletteLength) {
	int i = 0, j = 0;
	int ImgLength = image->x * image->y;
	PPMPixel *palette = (PPMPixel*)malloc(paletteLength * sizeof(PPMPixel));//hard coded
	bool *palette_started = (bool*)malloc(paletteLength* sizeof(bool));
	for (int i = 0; i < paletteLength; i++) {
		palette[i].blue = 0;
		palette[i].green = 0;
		palette[i].red = 0;
		palette_started[i] = false;
	}
	int disp_pos = 8;
	insert_string_on_display("INDEX  R   ,G    ,B", 7, 10, DISPLAY_WINDOW1);
	for (i = 0; i < ImgLength; i++) {
		for (j = 0; j < paletteLength; j++) {
			if (palette[j].red == image->data[i].red && palette[j].green == image->data[i].green && palette[j].blue == image->data[i].blue) {
				//we found the same color on index, jump.
				break;
			}
			if (palette_started[j] == false) {
				//color not found in the index, jump.
				palette[j].red = image->data[i].red;
				palette[j].green = image->data[i].green;
				palette[j].blue = image->data[i].blue;
				palette_started[j] = true;
				/*MT2D STUFF*/
				sprintf(str_buffer, "%2d    %3d  ,%3d  ,%3d", j, palette[j].red, palette[j].green, palette[j].blue);
				insert_string_on_display(str_buffer, disp_pos, 10, DISPLAY_WINDOW1);
				disp_pos++;
				MT2D_Draw_Window(DISPLAY_WINDOW1);
				/*=========*/
				break;
			}
		}
		if (j == 16) {
			sprintf(str_buffer, "ERROR:this image has more than 16 colors, you can fix that with the  gimp software... More info here:https://docs.gimp.org/en/gimp-image-convert-indexed.html");
			MT2D_MessageBox(str_buffer);
			return 0;
		}
	}
	return palette;
}
