# HexStartup-
A tool for making with custom 16 colors palette the startup,notch and netnotch lumps.
MORE INFO HERE: https://forum.zdoom.org/viewtopic.php?f=44&t=56556

## Building

HexStartup uses the [MT2D](https://github.com/lucas-zimerman/MT2D) engine (4.0.0), included as a git submodule in
`external/MT2D`, and libpng.

```sh
git clone --recursive https://github.com/lucas-zimerman/HexStartup-.git
# or, in an existing clone:
git submodule update --init

cmake -S . -B build
cmake --build build
```

Dependencies:

- **Linux / macOS**: libpng and ncursesw (e.g. `sudo apt install libpng-dev libncurses-dev`).
- **Windows (MSVC)**: libpng is taken from the system/vcpkg when found, otherwise the prebuilt libs in `Libs/` are used.
- **SDL backend** (optional, a window instead of the terminal): `cmake -S . -B build -DMT2D_SDL=ON`, needs SDL2 and SDL2_image.

## Usage

```sh
HexStartup                              # interactive: asks for the image file
HexStartup startup.png notch.png netnotch.ppm
```

Images can be `.png` or `.ppm` (P6), with at most 16 colors. The size picks the lump:
640x480 -> `STARTUP.DAT`, 16x23 -> `NOTCH.DAT`, 4x16 -> `NETNOTCH.DAT`. When several files are passed, the palette of
the 640x480 image is used for all of them.

## Updating MT2D

```sh
git -C external/MT2D fetch --tags
git -C external/MT2D checkout <tag>
git add external/MT2D
```
