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
