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
	img->data = (Pixel*)malloc(img->x * img->y * sizeof(Pixel));

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

void PPM_GetPixel(PPMImage *img, int Offset,Pixel *p) {
	p->blue = img->data[Offset].blue;
	p->red = img->data[Offset].red;
	p->green = img->data[Offset].green;
}