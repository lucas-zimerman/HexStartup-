// Converts the images in sample/ the same way `HexStartup startup.<ext> notch.<ext>` does, then decodes the
// STARTUP.DAT and NOTCH.DAT it wrote and checks every pixel against the source image.
//
// The reference pixels come from the .ppm files, read by the small parser below rather than by HexStartup's own
// loader, so a loader bug cannot hide itself. The PNG and PPM inputs hold the same pixels and must produce
// byte-identical lumps.
//
// usage: test_conversion <sample dir> <output dir>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "Image.h"
#include "Pixel.h"
#include "mt2d_stubs.h"

static int g_failures = 0;

#define CHECK(cond, ...)                                                    \
	do {                                                                    \
		if (!(cond)) {                                                      \
			std::fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);       \
			std::fprintf(stderr, __VA_ARGS__);                              \
			std::fprintf(stderr, "\n");                                     \
			++g_failures;                                                   \
		}                                                                   \
	} while (0)

static const int STARTUP_W = 640, STARTUP_H = 480;
static const int NOTCH_W = 16, NOTCH_H = 23;
static const size_t PALETTE_BYTES = 16 * 3;
static const size_t STARTUP_BYTES = PALETTE_BYTES + STARTUP_W * STARTUP_H / 2; // palette + 4 bit planes
static const size_t NOTCH_BYTES = NOTCH_W * NOTCH_H / 2;                        // 4 bits per pixel, no palette

struct Rgb {
	unsigned char r, g, b;
};

struct RefImage {
	int w = 0, h = 0;
	std::vector<Rgb> px;
	const Rgb &at(int x, int y) const { return px[y * w + x]; }
};

static std::vector<unsigned char> ReadFile(const std::string &path) {
	std::vector<unsigned char> data;
	FILE *f = std::fopen(path.c_str(), "rb");
	if (!f) return data;
	int c;
	while ((c = std::fgetc(f)) != EOF) data.push_back((unsigned char)c);
	std::fclose(f);
	return data;
}

// Minimal binary PPM (P6, maxval 255) reader, independent from PPM.cpp.
static bool ReadReferencePPM(const std::string &path, RefImage &out) {
	std::vector<unsigned char> d = ReadFile(path);
	size_t pos = 0;
	auto skip_space = [&]() {
		while (pos < d.size()) {
			if (d[pos] == '#') {
				while (pos < d.size() && d[pos] != '\n') ++pos;
			} else if (d[pos] == ' ' || d[pos] == '\t' || d[pos] == '\n' || d[pos] == '\r') {
				++pos;
			} else {
				break;
			}
		}
	};
	auto read_int = [&]() {
		skip_space();
		int v = 0;
		while (pos < d.size() && d[pos] >= '0' && d[pos] <= '9') v = v * 10 + (d[pos++] - '0');
		return v;
	};
	if (d.size() < 2 || d[0] != 'P' || d[1] != '6') return false;
	pos = 2;
	out.w = read_int();
	out.h = read_int();
	int maxval = read_int();
	++pos; // single whitespace before the pixel data
	if (maxval != 255 || d.size() - pos < (size_t)out.w * out.h * 3) return false;
	out.px.resize((size_t)out.w * out.h);
	for (size_t i = 0; i < out.px.size(); ++i) {
		out.px[i] = Rgb{d[pos + i * 3], d[pos + i * 3 + 1], d[pos + i * 3 + 2]};
	}
	return true;
}

// The lump palette is 6-bit VGA, so compare against the source color shifted the same way.
static bool SameColor(const Rgb &palette6, const Rgb &source) {
	return palette6.r == (source.r >> 2) && palette6.g == (source.g >> 2) && palette6.b == (source.b >> 2);
}

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

// Decodes STARTUP.DAT and compares it to the reference image. Returns the decoded palette.
static std::vector<Rgb> CheckStartup(const std::string &lump_path, const RefImage &ref) {
	std::vector<Rgb> palette;
	std::vector<unsigned char> d = ReadFile(lump_path);
	CHECK(d.size() == STARTUP_BYTES, "%s has %zu bytes, expected %zu", lump_path.c_str(), d.size(), STARTUP_BYTES);
	if (d.size() != STARTUP_BYTES) return palette;

	for (int i = 0; i < 16; ++i) {
		palette.push_back(Rgb{d[i * 3], d[i * 3 + 1], d[i * 3 + 2]});
		CHECK(palette[i].r < 64 && palette[i].g < 64 && palette[i].b < 64, "palette entry %d is not 6-bit", i);
	}
	for (int i = 0; i < 16; ++i) {
		for (int j = i + 1; j < 16; ++j) {
			const Rgb &a = palette[i], &b = palette[j];
			CHECK(a.r != b.r || a.g != b.g || a.b != b.b, "palette entries %d and %d are the same color", i, j);
		}
	}

	// Four bit planes, plane N holds bit N of every pixel's index, 8 pixels per byte with the leftmost in the MSB.
	const unsigned char *planes = &d[PALETTE_BYTES];
	const size_t plane_size = STARTUP_W * STARTUP_H / 8;
	int bad = 0;
	for (int y = 0; y < STARTUP_H; ++y) {
		for (int x = 0; x < STARTUP_W; ++x) {
			size_t offset = (size_t)y * (STARTUP_W / 8) + x / 8;
			int bit = 7 - (x % 8);
			int index = 0;
			for (int p = 0; p < 4; ++p) index |= ((planes[p * plane_size + offset] >> bit) & 1) << p;
			if (!SameColor(palette[index], ref.at(x, y)) && bad++ < 5) {
				const Rgb &s = ref.at(x, y);
				CHECK(false, "startup pixel (%d,%d) decodes to index %d, source color is (%d,%d,%d)", x, y, index,
				      s.r, s.g, s.b);
			}
		}
	}
	CHECK(bad == 0, "%d startup pixels differ from the source", bad);
	return palette;
}

// Decodes NOTCH.DAT with the startup palette and compares it to the reference image.
static void CheckNotch(const std::string &lump_path, const RefImage &ref, const std::vector<Rgb> &palette) {
	std::vector<unsigned char> d = ReadFile(lump_path);
	CHECK(d.size() == NOTCH_BYTES, "%s has %zu bytes, expected %zu", lump_path.c_str(), d.size(), NOTCH_BYTES);
	if (d.size() != NOTCH_BYTES || palette.size() != 16) return;

	// Two pixels per byte, the left one in the high nibble.
	int bad = 0;
	for (int y = 0; y < NOTCH_H; ++y) {
		for (int x = 0; x < NOTCH_W; ++x) {
			int i = y * NOTCH_W + x;
			int index = (i % 2 == 0) ? (d[i / 2] >> 4) : (d[i / 2] & 0x0F);
			if (!SameColor(palette[index], ref.at(x, y)) && bad++ < 5) {
				const Rgb &s = ref.at(x, y);
				CHECK(false, "notch pixel (%d,%d) decodes to index %d, source color is (%d,%d,%d)", x, y, index, s.r,
				      s.g, s.b);
			}
		}
	}
	CHECK(bad == 0, "%d notch pixels differ from the source", bad);
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
