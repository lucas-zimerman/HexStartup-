#pragma once
#include <string>
#include <vector>

// Message boxes the conversion code showed since the last Stub_ClearMessages().
const std::vector<std::string> &Stub_Messages();
void Stub_ClearMessages();

// Makes Load_Sprite_Image(path) return an image sprite holding a copy of raw (see mt2d_stubs.cpp).
#include <MT2D/ObjectCore/SprRaw.h>
void Stub_SetSpriteImage(const std::string &path, const MT2D_SpriteRaw *raw);
