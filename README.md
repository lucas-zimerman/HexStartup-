# HexStartup-
A tool for making with custom 16 colors palette the startup,notch and netnotch lumps.
MORE INFO HERE: https://forum.zdoom.org/viewtopic.php?f=44&t=56556

## Building

HexStartup uses the [MT2D](https://github.com/lucas-zimerman/MT2D) engine (4.0.0), included as a git submodule in
`external/MT2D`, and libpng. libpng 1.6.59 and zlib 1.4 are also included as submodules in `external/libpng` and
`external/zlib`; they are only built when no system libpng is found.

```sh
git clone --recursive https://github.com/lucas-zimerman/HexStartup-.git
# or, in an existing clone:
git submodule update --init

cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure   # converts sample/ and checks the generated lumps
```

Dependencies:

- **Linux / macOS**: ncursesw, and libpng if available (e.g. `sudo apt install libpng-dev libncurses-dev`).
- **Windows (MSVC)**: libpng is taken from the system/vcpkg when found, otherwise libpng and zlib are built from the
  submodules.
- `-DHEXSTARTUP_BUNDLED_PNG=ON` always builds libpng and zlib from the submodules, even when a system libpng exists.
- **SDL backend** (optional, a window instead of the terminal): `cmake -S . -B build -DMT2D_SDL=ON`, needs SDL2 and SDL2_image.

## Usage

```sh
HexStartup                              # interactive: asks for the image file
HexStartup startup.png notch.png netnotch.ppm
```

Images can be `.png` or `.ppm` (P6), with at most 16 colors. The size picks the lump:
640x480 -> `STARTUP.DAT`, 16x23 -> `NOTCH.DAT`, 4x16 -> `NETNOTCH.DAT`. When several files are passed, the palette of
the 640x480 image is used for all of them.

Ready-made images to try it with are in [`sample/`](sample/).

## Updating MT2D

```sh
git -C external/MT2D fetch --tags
git -C external/MT2D checkout <tag>
git add external/MT2D
```
