#include <stdio.h>
#include <stdlib.h>
#include <MT2D/MessageBox/MT2D_MessageBox.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MT2D.h>
#include <MT2D/File/MT2D_File.h>
#include "PPM.h"


extern char str_buffer[200];


#define RGB_COMPONENT_COLOR 255

/* Reads one byte. Returns false at the end of the file or on a read error.
   MT2D_FILE_READ is used instead of MT2D_FILE_READ_BYTE because a 0xFF byte cannot be told apart from EOF there. */
static bool PPM_ReadByte(MT2D_FILE *fp, unsigned char *c) {
	return MT2D_FILE_READ(fp, c, 1, 1) == 1;
}

static bool PPM_IsSpace(unsigned char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

/* Skips a '#' comment up to and including the end of the line. */
static bool PPM_SkipComment(MT2D_FILE *fp) {
	unsigned char c;
	do {
		if (!PPM_ReadByte(fp, &c)) {
			return false;
		}
	} while (c != '\n' && c != '\r');
	return true;
}

/* Reads a header number (width, height or maxval), skipping the whitespace and comments before it.
   The byte right after the number is consumed: for maxval it is the single whitespace before the pixel data. */
static bool PPM_ReadHeaderNumber(MT2D_FILE *fp, int *value) {
	unsigned char c;
	do {
		if (!PPM_ReadByte(fp, &c)) {
			return false;
		}
		if (c == '#' && !PPM_SkipComment(fp)) {
			return false;
		}
	} while (c == '#' || PPM_IsSpace(c));

	if (c < '0' || c > '9') {
		return false;
	}
	long number = 0;
	while (c >= '0' && c <= '9') {
		number = number * 10 + (c - '0');
		if (number > 65535) {
			return false;
		}
		if (!PPM_ReadByte(fp, &c)) {
			return false;
		}
	}
	if (c == '#') {
		if (!PPM_SkipComment(fp)) {
			return false;
		}
	}
	else if (!PPM_IsSpace(c)) {
		return false;
	}
	*value = (int)number;
	return true;
}

static PPMImage *PPM_Fail(PPMImage *img, MT2D_FILE *fp) {
	MT2D_MessageBox(str_buffer);
	if (img) {
		free(img->data);
		free(img);
	}
	MT2D_FILE_CLOSE(fp);
	return 0;
}

PPMImage *readPPM(const char *filename)
{//Code Created by: RPFELGUEIRAS
	unsigned char magic[2];
	PPMImage *img = 0;
	MT2D_FILE *fp;
	int rgb_comp_color;
	//open PPM file for reading
	fp = MT2D_FILE_OPEN(filename, "rb");
	if (!fp) {
		snprintf(str_buffer, sizeof(str_buffer), "ERROR:Unable to open file '%s'", filename);
		MT2D_MessageBox(str_buffer);
		return 0;
	}

	//read image format
	if (MT2D_FILE_READ(fp, magic, 1, sizeof(magic)) != sizeof(magic)) {
		snprintf(str_buffer, sizeof(str_buffer), "ERROR:Unable to read image format (%s)", filename);
		return PPM_Fail(img, fp);
	}

	//check the image format
	if (magic[0] != 'P' || magic[1] != '6') {
		snprintf(str_buffer, sizeof(str_buffer), "ERROR:Invalid image format (must be 'P6')                            Are you loading a .ppm image? %s   ", filename);
		return PPM_Fail(img, fp);
	}

	//alloc memory form image
	img = (PPMImage *)calloc(1, sizeof(PPMImage));
	if (!img) {
		snprintf(str_buffer, sizeof(str_buffer), "ERROR:Unable to allocate memory");
		return PPM_Fail(img, fp);
	}

	//read image size information
	if (!PPM_ReadHeaderNumber(fp, &img->x) || !PPM_ReadHeaderNumber(fp, &img->y) || img->x <= 0 || img->y <= 0) {
		snprintf(str_buffer, sizeof(str_buffer), "ERROR:Invalid image size (error loading '%s')", filename);
		return PPM_Fail(img, fp);
	}

	//read rgb component
	if (!PPM_ReadHeaderNumber(fp, &rgb_comp_color)) {
		snprintf(str_buffer, sizeof(str_buffer), "ERROR:Invalid rgb component (error loading '%s')", filename);
		return PPM_Fail(img, fp);
	}

	//check rgb component depth
	if (rgb_comp_color != RGB_COMPONENT_COLOR) {
		snprintf(str_buffer, sizeof(str_buffer), "ERROR:'%s' does not have 8-bits components\n", filename);
		return PPM_Fail(img, fp);
	}

	//memory allocation for pixel data
	img->data = (Pixel*)malloc((size_t)img->x * img->y * sizeof(Pixel));
	if (!img->data) {
		snprintf(str_buffer, sizeof(str_buffer), "ERROR:Unable to allocate memory");
		return PPM_Fail(img, fp);
	}

	//read pixel data from file
	if (MT2D_FILE_READ(fp, img->data, 3 * img->x, img->y) != (size_t)img->y) {
		snprintf(str_buffer, sizeof(str_buffer), "Error loading image '%s'\n", filename);
		return PPM_Fail(img, fp);
	}

	MT2D_FILE_CLOSE(fp);
	return img;
}

void PPM_GetPixel(PPMImage *img, int Offset,Pixel *p) {
	p->blue = img->data[Offset].blue;
	p->red = img->data[Offset].red;
	p->green = img->data[Offset].green;
}