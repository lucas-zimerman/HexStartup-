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
