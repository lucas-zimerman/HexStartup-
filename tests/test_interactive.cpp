// Runs GenerateHexenStartup (interactive mode, Interactive.cpp) on the images in sample/ and checks the lumps it
// writes, the same way test_conversion does for the command line mode.
//
// GenerateHexenStartup writes into the current directory, so ctest runs this from the output directory.
//
// usage: test_interactive <sample dir>
#include <cstdio>
#include <string>
#include <vector>

#include "Interactive.h"
#include "lump_check.h"
#include "mt2d_stubs.h"

static const char *NO_STARTUP_WARNING = "no STARTUP image found";
static const char *CREATED_MESSAGE = "was created in the image folder";

static int CountMessages(const char *text) {
	int n = 0;
	for (const std::string &m : Stub_Messages()) {
		if (m.find(text) != std::string::npos) ++n;
	}
	return n;
}

// Runs one GenerateHexenStartup call, as the interactive loop in Source.cpp does for every file the user types.
static void Generate(const std::string &path) {
	Stub_ClearMessages();
	std::string p = path;
	GenerateHexenStartup(&p[0], 0);
}

static void ExpectOnlyCreatedMessage(const std::string &path) {
	CHECK(CountMessages(NO_STARTUP_WARNING) == 0, "%s showed the no STARTUP image warning", path.c_str());
	CHECK(CountMessages(CREATED_MESSAGE) == 1, "%s did not show the file created message", path.c_str());
	for (const std::string &m : Stub_Messages()) {
		CHECK(m.find(CREATED_MESSAGE) != std::string::npos, "%s showed a message box: %s", path.c_str(), m.c_str());
	}
}

// A fresh session, as if the program was just started.
static void NewSession() {
	palette = 0;
	std::remove("STARTUP.DAT");
	std::remove("NOTCH.DAT");
	std::remove("NETNOTCH.DAT");
}

int main(int argc, char **argv) {
	if (argc != 2) {
		std::fprintf(stderr, "usage: %s <sample dir>\n", argv[0]);
		return 2;
	}
	const std::string in = std::string(argv[1]) + "/";

	RefImage startup_ref, notch_ref;
	CHECK(ReadReferencePPM(in + "startup.ppm", startup_ref), "cannot read %sstartup.ppm", in.c_str());
	CHECK(ReadReferencePPM(in + "notch.ppm", notch_ref), "cannot read %snotch.ppm", in.c_str());
	if (g_failures) return 1;

	for (const char *ext : {"ppm", "png"}) {
		std::printf("-- startup.%s, then notch.%s in the same session\n", ext, ext);
		NewSession();
		const std::string startup_in = in + "startup." + ext;
		const std::string notch_in = in + "notch." + ext;

		Generate(startup_in);
		ExpectOnlyCreatedMessage(startup_in);
		CHECK(palette, "no palette after converting %s", startup_in.c_str());
		std::vector<Rgb> lump_palette = CheckStartup("STARTUP.DAT", startup_ref);

		// The NOTCH lump has no palette of its own: it is indexed with the STARTUP palette kept from the first call.
		Generate(notch_in);
		ExpectOnlyCreatedMessage(notch_in);
		CheckNotch("NOTCH.DAT", notch_ref, lump_palette);
	}

	std::printf("-- notch.ppm without a STARTUP image first\n");
	{
		NewSession();
		const std::string notch_in = in + "notch.ppm";
		Generate(notch_in);
		CHECK(CountMessages(NO_STARTUP_WARNING) == 1, "%s did not show the no STARTUP image warning",
		      notch_in.c_str());
		CHECK(CountMessages(CREATED_MESSAGE) == 1, "%s did not show the file created message", notch_in.c_str());
		std::vector<unsigned char> d = ReadFile("NOTCH.DAT");
		CHECK(d.size() == NOTCH_BYTES, "NOTCH.DAT has %zu bytes, expected %zu", d.size(), NOTCH_BYTES);
	}

	if (g_failures) {
		std::printf("%d check(s) failed\n", g_failures);
		return 1;
	}
	std::printf("all checks passed\n");
	return 0;
}
