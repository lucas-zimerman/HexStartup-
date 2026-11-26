// Converts the images in sample/ the same way `HexStartup startup.<ext> notch.<ext>` does, then decodes the
// STARTUP.DAT and NOTCH.DAT it wrote and checks every pixel against the source image.
//
// The reference pixels come from the .ppm files, read by the small parser in lump_check.h rather than by HexStartup's
// own loader, so a loader bug cannot hide itself. The PNG and PPM inputs hold the same pixels and must produce
// byte-identical lumps.
//
// usage: test_conversion <sample dir> <output dir>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "Image.h"
#include "Pixel.h"
#include "lump_check.h"
#include "mt2d_stubs.h"

// Runs the conversion steps of the command line mode in Source.cpp. Returns false if anything failed.
static bool Convert(const std::string &startup_in, const std::string &notch_in, const std::string &startup_out,
                    const std::string &notch_out) {
	Stub_ClearMessages();
	std::string s_in = startup_in, n_in = notch_in, s_out = startup_out, n_out = notch_out;

	Image *startup = Image_Load(&s_in[0]);
	Image *notch = Image_Load(&n_in[0]);
	CHECK(startup && startup->Loaded, "could not load %s", startup_in.c_str());
	CHECK(notch && notch->Loaded, "could not load %s", notch_in.c_str());
	if (!startup || !notch) return false;
	CHECK(startup->Width == STARTUP_W && startup->Height == STARTUP_H, "%s is %ux%u", startup_in.c_str(),
	      startup->Width, startup->Height);
	CHECK(notch->Width == NOTCH_W && notch->Height == NOTCH_H, "%s is %ux%u", notch_in.c_str(), notch->Width,
	      notch->Height);

	Pixel *palette = Image_GetPalette(startup, 16);
	CHECK(palette, "no palette from %s", startup_in.c_str());
	if (!palette) return false;

	Image *startup_lump = Image_ImageConvert(startup, palette, TYPE_Planar);
	Image *notch_lump = Image_ImageConvert(notch, palette, TYPE_Planar);
	CHECK(startup_lump && startup_lump->Type == TYPE_Planar, "startup was not converted to a planar lump");
	CHECK(notch_lump && notch_lump->Type == TYPE_4BitBitmap, "notch was not converted to a 4 bit bitmap lump");
	if (!startup_lump || !notch_lump) return false;

	CHECK(Image_Save(startup_lump, &s_out[0]), "could not save %s", startup_out.c_str());
	CHECK(Image_Save(notch_lump, &n_out[0]), "could not save %s", notch_out.c_str());

	for (const std::string &m : Stub_Messages()) {
		CHECK(false, "conversion showed a message box: %s", m.c_str());
	}
	return Stub_Messages().empty();
}

// PNG_read_file must fail cleanly (NULL, one message naming the file) on files it cannot read. Issue #9.
static void CheckBadPng(const std::string &path, const char *what) {
	Stub_ClearMessages();
	std::string p = path;
	Image *img = Image_Load(&p[0]);
	CHECK(img == nullptr, "%s: loading %s did not fail", what, path.c_str());
	CHECK(Stub_Messages().size() == 1, "%s: %zu messages shown, expected 1", what, Stub_Messages().size());
	if (!Stub_Messages().empty()) {
		CHECK(Stub_Messages()[0].find(path) != std::string::npos, "%s: message \"%s\" does not name the file", what,
		      Stub_Messages()[0].c_str());
	}
}

static void WriteFile(const std::string &path, const std::vector<unsigned char> &data) {
	FILE *f = std::fopen(path.c_str(), "wb");
	if (!f) return;
	if (!data.empty()) std::fwrite(&data[0], 1, data.size(), f);
	std::fclose(f);
}

static void CheckBadPngs(const std::string &in, const std::string &out) {
	std::printf("-- unreadable png files\n");
	const std::string missing = out + "missing.png";
	std::remove(missing.c_str());
	CheckBadPng(missing, "missing file");

	const std::string not_png = out + "not_png.png";
	WriteFile(not_png, ReadFile(in + "notch.ppm"));
	CheckBadPng(not_png, "not a png");

	std::vector<unsigned char> truncated = ReadFile(in + "startup.png");
	CHECK(truncated.size() > 100, "cannot read %sstartup.png", in.c_str());
	truncated.resize(truncated.size() / 2);
	const std::string truncated_png = out + "truncated.png";
	WriteFile(truncated_png, truncated);
	CheckBadPng(truncated_png, "truncated png");
}

// Writes a 640x480 PPM made of vertical bands of `colors`, with black at pixel `black_at` (-1 for no black).
static std::string WritePaletteTestPPM(const std::string &path, int colors, int black_at) {
	std::vector<unsigned char> data;
	char header[32];
	int n = std::snprintf(header, sizeof(header), "P6\n%d %d\n255\n", STARTUP_W, STARTUP_H);
	data.insert(data.end(), header, header + n);
	for (int i = 0; i < STARTUP_W * STARTUP_H; i++) {
		int c = (i % STARTUP_W) * colors / STARTUP_W;
		bool black = i == black_at;
		data.push_back(black ? 0 : (unsigned char)(4 * (c + 1)));
		data.push_back(black ? 0 : (unsigned char)(8 * c));
		data.push_back(black ? 0 : 12);
	}
	WriteFile(path, data);
	return path;
}

// Image_GetPalette must give black a palette slot like any other color, so 16 colors plus black is rejected
// wherever the black pixel is. Issue #12.
static void CheckPaletteBlack(const std::string &out) {
	std::printf("-- black in the palette\n");
	const int last = STARTUP_W * STARTUP_H - 1;
	struct Case {
		const char *name;
		int colors, black_at;
		bool valid;
	} cases[] = {
	    {"17colors_black_first.ppm", 16, 0, false},
	    {"17colors_black_last.ppm", 16, last, false},
	    {"16colors_black_first.ppm", 15, 0, true},
	    {"16colors_black_last.ppm", 15, last, true},
	};
	for (const Case &c : cases) {
		std::string path = WritePaletteTestPPM(out + c.name, c.colors, c.black_at);
		Stub_ClearMessages();
		Image *img = Image_Load(&path[0]);
		CHECK(img && img->Loaded, "could not load %s", path.c_str());
		if (!img) continue;
		Pixel *palette = Image_GetPalette(img, 16);
		if (!c.valid) {
			CHECK(palette == nullptr, "%s: more than 16 colors was not rejected", c.name);
			CHECK(Stub_Messages().size() == 1, "%s: %zu messages shown, expected 1", c.name, Stub_Messages().size());
			continue;
		}
		CHECK(palette, "%s: no palette", c.name);
		CHECK(Stub_Messages().empty(), "%s: %zu messages shown, expected none", c.name, Stub_Messages().size());
		if (!palette) continue;
		int black_slot = c.black_at == 0 ? 0 : 15; // slots are given in the order colors first appear
		CHECK(palette[black_slot].red == 0 && palette[black_slot].green == 0 && palette[black_slot].blue == 0,
		      "%s: slot %d is not black", c.name, black_slot);
		for (int j = 0; j < 16; j++) {
			if (j == black_slot) continue;
			CHECK(palette[j].red || palette[j].green || palette[j].blue, "%s: black also in slot %d", c.name, j);
		}
	}
}

// readPPM parses the P6 header byte by byte through MT2D_FILE (issue #17). The notch pixels are written after
// different headers; the valid ones must load the same pixels, the broken ones must fail with one message.
static void CheckPpmHeaders(const std::string &out, const RefImage &notch_ref) {
	std::printf("-- ppm headers\n");
	std::vector<unsigned char> pixels;
	for (const Rgb &c : notch_ref.px) {
		pixels.push_back(c.r);
		pixels.push_back(c.g);
		pixels.push_back(c.b);
	}
	// The first pixel byte is 0xFF, the value MT2D_FILE_READ_BYTE returns at EOF.
	std::vector<unsigned char> white_first = pixels;
	white_first[0] = 0xFF;

	struct Case {
		const char *name, *header;
		const std::vector<unsigned char> *data;
		bool valid;
	};
	const Case cases[] = {
	    {"plain", "P6\n16 23\n255\n", &pixels, true},
	    {"one line", "P6 16 23 255 ", &pixels, true},
	    {"comments", "P6\n# made by a test\n16 # width\n# height next\n23\n255\n", &pixels, true},
	    {"tabs and crlf", "P6\r\n16\t23\r\n255\n", &pixels, true},
	    {"0xff first byte", "P6\n16 23\n255\n", &white_first, true},
	    {"P3", "P3\n16 23\n255\n", &pixels, false},
	    {"no size", "P6\n# only a comment\n", &pixels, false},
	    {"bad width", "P6\nx 23\n255\n", &pixels, false},
	    {"4 bit", "P6\n16 23\n15\n", &pixels, false},
	    {"truncated", "P6\n16 23\n255\n", nullptr, false},
	};
	for (const Case &c : cases) {
		std::vector<unsigned char> file(c.header, c.header + std::string(c.header).size());
		if (c.data) {
			file.insert(file.end(), c.data->begin(), c.data->end());
		} else {
			file.insert(file.end(), pixels.begin(), pixels.begin() + pixels.size() / 2);
		}
		std::string path = out + "header.ppm";
		WriteFile(path, file);

		Stub_ClearMessages();
		Image *img = Image_Load(&path[0]);
		if (!c.valid) {
			CHECK(img == nullptr, "%s: loading did not fail", c.name);
			CHECK(Stub_Messages().size() == 1, "%s: %zu messages shown, expected 1", c.name, Stub_Messages().size());
			continue;
		}
		CHECK(img && img->Width == NOTCH_W && img->Height == NOTCH_H, "%s: did not load as %dx%d", c.name, NOTCH_W,
		      NOTCH_H);
		CHECK(Stub_Messages().empty(), "%s: a message box was shown", c.name);
		if (!img || img->Width != NOTCH_W || img->Height != NOTCH_H) continue;
		int bad = 0;
		for (int i = 0; i < NOTCH_W * NOTCH_H; ++i) {
			Pixel px;
			Image_GetPixel(img, i, &px);
			const unsigned char *e = &(*c.data)[i * 3];
			if (px.red != e[0] || px.green != e[1] || px.blue != e[2]) ++bad;
		}
		CHECK(bad == 0, "%s: %d pixels differ", c.name, bad);
	}
}

int main(int argc, char **argv) {
	if (argc != 3) {
		std::fprintf(stderr, "usage: %s <sample dir> <output dir>\n", argv[0]);
		return 2;
	}
	const std::string in = std::string(argv[1]) + "/";
	const std::string out = std::string(argv[2]) + "/";

	RefImage startup_ref, notch_ref;
	CHECK(ReadReferencePPM(in + "startup.ppm", startup_ref), "cannot read %sstartup.ppm", in.c_str());
	CHECK(ReadReferencePPM(in + "notch.ppm", notch_ref), "cannot read %snotch.ppm", in.c_str());
	if (g_failures) return 1;

	for (const char *ext : {"ppm", "png"}) {
		std::printf("-- %s\n", ext);
		const std::string startup_dat = out + "STARTUP_" + ext + ".DAT";
		const std::string notch_dat = out + "NOTCH_" + ext + ".DAT";
		std::remove(startup_dat.c_str());
		std::remove(notch_dat.c_str());
		if (!Convert(in + "startup." + ext, in + "notch." + ext, startup_dat, notch_dat)) continue;
		std::vector<Rgb> palette = CheckStartup(startup_dat, startup_ref);
		CheckNotch(notch_dat, notch_ref, palette);
	}

	CheckBadPngs(in, out);
	CheckPaletteBlack(out);
	CheckPpmHeaders(out, notch_ref);

	std::printf("-- png and ppm produce the same lumps\n");
	CHECK(ReadFile(out + "STARTUP_png.DAT") == ReadFile(out + "STARTUP_ppm.DAT"), "STARTUP.DAT differs");
	CHECK(ReadFile(out + "NOTCH_png.DAT") == ReadFile(out + "NOTCH_ppm.DAT"), "NOTCH.DAT differs");

	if (g_failures) {
		std::printf("%d check(s) failed\n", g_failures);
		return 1;
	}
	std::printf("all checks passed\n");
	return 0;
}
