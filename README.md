# xrick-libretro

A libretro core for **xrick**, BigOrno's open source reimplementation of Core
Design's *Rick Dangerous*.

Upstream engine: <http://www.bigorno.net/xrick/>

## Game data

The core needs the original game data, as a zip, in the frontend's system
directory:

    system/xrick/data.zip

Without it `retro_load_game()` fails and the core reports the failure to the
frontend rather than loading an empty session. The core takes no content file,
so it can be started with "Start Core" / no game.

## Building

The core builds from the standard libretro makefile with no external
dependencies. zlib is vendored in `deps/libz`; nothing links against SDL.

    make -f Makefile.libretro platform=unix

Other targets include `windows_msvc2010_x64`, `vita`, `switch`, `libnx`,
`wiiu`, `wii`, `ngc`, `ctr`, `psl1ght`, `emscripten`, `gcw0`, `miyoo` and
`tvos-arm64`. See `Makefile.libretro` for the full list.

The default build is warning free.

## Controls

| RetroPad | Action |
| --- | --- |
| D-Pad Up | Jump / climb |
| D-Pad Down | Crawl |
| D-Pad Left / Right | Move |
| A | Jump |
| B | Fire gun |
| X | Jab stick (hold with Left/Right) |
| Y | Set dynamite |
| Start | Pause |

## Core options

| Option | Default | Notes |
| --- | --- | --- |
| Crop Borders | enabled | Removes the 32 pixels of empty padding either side of the game screen. Cheat indicators are only drawn when cropping is disabled. |
| Cheat: Trainer Mode | disabled | Always 6 bullets, 6 dynamite, 6 lives. |
| Cheat: Invulnerability Mode | disabled | Nothing kills Rick. May break game progression. |
| Cheat: Expose Mode | disabled | Highlights all entities on the map. |

## Frontend features

- **Savestates**, and therefore rewind, run-ahead and netplay. The state
  covers the full session: game and entity state, map state and mutated map
  marks, the screen sequencers, the frame buffer and palette, and the input
  edge-detection latches. Multi-byte fields are little endian and the palette
  is stored unpacked, so a state written by an RGB565 build loads into an
  XRGB8888 one.
- **Reset**, which returns the core to the state a fresh load produces without
  re-reading the data files.
- **Save RAM**, holding the hall of fame. 128 bytes: eight 16-byte entries,
  each a little endian `uint32` score, ten name bytes and two zeroed padding
  bytes. On a little endian host this is byte for byte the layout older
  builds wrote, so existing save files carry over.

## Video and audio

320x200 at 25fps. RGB565 by default; XRGB8888 when `FRONTEND_SUPPORTS_RGB565`
is not defined. Audio is 22050 Hz stereo, submitted as one batch of 882
samples per frame — the rate divides exactly into the frame rate, so there is
no drift or remainder to carry.

## Licence

See `README`, which carries BigOrno's original statement. The engine code is
released in the spirit of the GNU GPL; the graphics, maps and sounds belong to
the authors of the original *Rick Dangerous*, which remains a trademark of its
owner(s).
