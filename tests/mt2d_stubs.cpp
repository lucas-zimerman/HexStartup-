// Headless replacements for the few MT2D calls the conversion code makes, so the tests run without a terminal or a
// window. Defining them here means the linker never pulls the real ones out of the static mt2d library.
//
// The conversion code only shows a message box when something goes wrong, so every message is recorded and the
// tests fail if any was shown.
#include <string>
#include <vector>

#include <MT2D/MT2D.h>
#include <MT2D/MT2D_Display.h>
#include <MT2D/MessageBox/MT2D_MessageBox.h>

#include "mt2d_stubs.h"

char str_buffer[200]; // defined by Source.cpp in the real program

static std::vector<std::string> g_messages;

const std::vector<std::string> &Stub_Messages() { return g_messages; }
void Stub_ClearMessages() { g_messages.clear(); }

void MT2D_MessageBox(char *mensage) { g_messages.push_back(mensage ? mensage : ""); }
void insert_string_on_display(char *, int, int, unsigned char (*)[MAX_HOR]) {}
void MT2D_Draw_Window(unsigned char (*)[MAX_HOR]) {}

#include "Preview.h"
bool Preview_Available() { return false; }
void Preview_Show(Image *, Image *, Image *) {}

#include <MT2D/MessageBox/MT2D_MessageBox_With_Result.h>
int MT2D_MessageBox_With_Result(char *, char *, char *, char *) { return 2; }

// Image sprites: the real Load_Sprite_Image depends on the backend (and pulls in the whole engine). Here a path given
// to Stub_SetSpriteImage loads as an image sprite with that content, any other file that exists loads as the "<!>"
// ASCII placeholder (what backends without a reader for it return), and a missing file gives NULL.
#include <cstdio>
#include <cstdlib>
#include <map>
#include <MT2D/ObjectCore/Sprites.h>

static std::map<std::string, MT2D_SpriteRaw*> g_sprite_files;
static std::map<const Sprite*, const MT2D_SpriteRaw*> g_sprite_content;

void Stub_SetSpriteImage(const std::string &path, const MT2D_SpriteRaw *raw) {
	MT2D_SpriteRaw *&slot = g_sprite_files[path];
	MT2D_SpriteRaw_Free(slot);
	slot = MT2D_SpriteRaw_Clip(raw, 0, 0, raw->Width, raw->Height);
}

Sprite *Load_Sprite_Image(char *file, int ScaleX, int ScaleY) {
	Sprite *s = (Sprite*)calloc(1, sizeof(Sprite));
	s->ScaleX = ScaleX;
	s->ScaleY = ScaleY;
	auto it = g_sprite_files.find(file);
	if (it != g_sprite_files.end()) {
		s->type = 1;
		s->SizeX = it->second->Width;
		s->SizeY = it->second->Height;
		g_sprite_content[s] = it->second;
		return s;
	}
	FILE *f = fopen(file, "rb");
	if (!f) {
		free(s);
		return 0;
	}
	fclose(f);
	s->type = 0;
	s->SizeX = 3;
	s->SizeY = 1;
	return s;
}

MT2D_SpriteRaw *MT2D_Sprite_To_SpriteRaw(const Sprite *Me) {
	auto it = g_sprite_content.find(Me);
	if (it == g_sprite_content.end()) return 0;
	return MT2D_SpriteRaw_Clip(it->second, 0, 0, it->second->Width, it->second->Height);
}

void Sprite_Delete(Sprite *Me) {
	g_sprite_content.erase(Me);
	free(Me);
}
