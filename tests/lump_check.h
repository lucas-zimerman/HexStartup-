// Checks shared by the tests: a CHECK macro, a reference PPM reader independent from HexStartup's loader, and
// decoders for the STARTUP.DAT and NOTCH.DAT lumps. Each test executable includes this from a single file.
#pragma once
#include <cstdio>
#include <string>
#include <vector>

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
