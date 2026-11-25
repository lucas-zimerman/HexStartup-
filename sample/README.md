# Sample images

Ready-made inputs for trying HexStartup, in both supported formats:

| File                          | Size    | Lump          |
|-------------------------------|---------|---------------|
| `startup.png` / `startup.ppm` | 640x480 | `STARTUP.DAT` |
| `notch.png` / `notch.ppm`     | 16x23   | `NOTCH.DAT`   |

The startup image uses exactly 16 colors, and the notch only uses colors from that palette (HexStartup indexes every
lump with the startup palette). All palette values are multiples of 4, so nothing is lost in the 6-bit VGA palette.

```sh
HexStartup sample/startup.png sample/notch.png
HexStartup sample/startup.ppm sample/notch.ppm
```

`tests/test_conversion.cpp` (run with `ctest`) converts both sets, decodes the resulting `STARTUP.DAT` and
`NOTCH.DAT` and checks every pixel against these images, so regenerated images must keep those rules.

To regenerate them (needs Pillow): `python3 sample/generate.py`.
