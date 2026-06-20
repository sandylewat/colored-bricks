# Colored Bricks

A LEGO brick-inspired Pebble watchface for the Pebble Time 2 (emery).
The time is displayed in a brick font, every digit is built from individual brick-style tiles.

## Features

- **Full-screen brick baseplate** — tiled with 8 colours (red,
  blue, yellow, green, orange, cyan, magenta, purple) plus white
- **Two digit layouts** — **3×5** (14 px bricks, bold) or **6×7** (7 px bricks,
  finer 2-brick-stroke detail); both share the same 42 px wide digit footprint
- **Animated minute transitions** — only the digit(s) that actually changed
  rebuild row-by-row (top → bottom, 90 ms per row); unchanged digits stay static
- **Battery-efficient** — redraws on `MINUTE_UNIT` only; the brief build
  animation lasts at most 7 × 90 ms = 630 ms
- **Configurable via the Pebble app** — all visual options are changeable
  without recompiling (see [Settings](#settings))

## Settings

Open the watchface settings from the Pebble app on your phone. Changes are
saved to persistent storage and survive reboots. Tap **Reset to Defaults** to
restore the values listed below, then **Save Settings** to apply.

### Background

| Setting | Options | Default |
|---|---|---|
| **Match digit brick size** | On / Off | **On** |
| **Uniform (no brick pattern)** | On / Off | Off |
| **Brick colour** | Random · Red · Blue · Yellow · Green · Orange · Cyan · Magenta · Purple · **White** | **White** |

- **Match digit brick size** — when on, background bricks resize to match the
  current digit layout (14 px for 3×5, 7 px for 6×7) for a seamless grid.
- **Uniform** — replaces the brick grid with a flat solid fill.
- **Random** picks a colour per brick based on its grid position; changes
  automatically in uniform mode.

### Digit Colour

| Setting | Options | Default |
|---|---|---|
| **Digit outline** | On / Off | **On** |
| **6×7 brick layout** | On / Off | Off |
| **Mode** | Uniform colour · Per-digit colours | **Per-digit colours** |
| **Colour** *(uniform mode only)* | White · Red · Blue · Yellow · Green · Orange · Cyan · Magenta · Purple | White |
| **Digit background** | Transparent · Black · Red · Blue · Yellow · Green · Orange · Cyan · Magenta · Purple | **Transparent** |

- **Per-digit colours** assigns a fixed bricks colour to each position:
  `H₁` → Red · `H₂` → Blue · `M₁` → Yellow · `M₂` → Green.
- **Digit background** controls the colour of OFF (unlit) bricks in the digit
  area; transparent lets the background baseplate show through.

### Time Format

| Setting | Options | Default |
|---|---|---|
| **24-hour format** | On / Off | **Off (12 h)** |

12 h mode suppresses the leading zero (e.g. `9:05`, not `09:05`).

## Building & running

```sh
npm install                           # first time only — installs pebble-clay
pebble build                          # compile → build/bricksface.pbw
pebble install --emulator emery       # test in the emery QEMU emulator
pebble install --cloudpebble          # deploy to a paired real device
```

> **Note:** whenever you add new `messageKeys` to `package.json`, run
> `rm -rf build && pebble build` to force regeneration of `message_keys.json`.

## Target platform

This watchface targets **emery** (Pebble Time 2, 200 × 228 px, 64-colour
rectangular display).

## Project layout

```
src/c/bricksface.c     Watchface source — background, digit renderer, animation,
                        settings (AppMessage + persist)
src/pkjs/index.js      Clay boilerplate + reset-button customFn
src/pkjs/config.json   Clay UI definition (all setting controls)
package.json           Project metadata (UUID, platform, messageKeys, Clay dep)
wscript                Build rules
```

## Customisation

All settings in [Settings](#settings) are configurable at runtime. For deeper
changes, the relevant constants live near the top of `src/c/bricksface.c`.

### Brick sizes

```c
#define CELL      14   // 3×5 digit brick size  — also drives TIME_Y
#define CELL6      7   // 6×7 digit brick size
#define BG_CELL   10   // background brick size (when match-digit is off)
```

The horizontal layout formula keeps digit width at 42 px regardless of layout:

```
3 + D(42) + 3 + D(42) + 3 + C(14) + 3 + D(42) + 3 + D(42) + 3 = 200 px
```

`TIME_Y` / `TIME_Y6` centre their respective digit blocks vertically:
`TIME_Y = (228 − 5×CELL) / 2 = 79`,  `TIME_Y6 = (228 − 7×CELL6) / 2 = 90`.

### Stud sizes

```c
#define DIG_STUD   3   // stud radius on 3×5 digit bricks
#define DIG_STUD6  2   // stud radius on 6×7 digit bricks
#define BG_STUD    2   // stud radius on background bricks (fixed-size mode)
```

Set any of these to `0` for a flat tile look.

### Animation speed

```c
#define ANIM_MS  90   // milliseconds between each row appearing
```

Full animation: `rows × ANIM_MS` (5 rows → 450 ms for 3×5; 7 rows → 630 ms
for 6×7). Set to `0` to skip animation entirely.

### Digit shapes

**3×5 font** — `DIGIT_MAP[11][5]`, 3-bit rows (bit 2 = left, bit 0 = right):

```
0b111  →  ■ ■ ■       0b101  →  ■ · ■
0b001  →  · · ■       0b110  →  ■ ■ ·
```

**6×7 font** — `DIGIT_MAP6[11][7]`, 6-bit rows (bit 5 = left, bit 0 = right),
bold 2-brick-wide strokes:

```
0b110011  →  ■ ■ · · ■ ■
0b111111  →  ■ ■ ■ ■ ■ ■
```

Index `10` in both maps is `DIGIT_BLANK` (all zeros) — used to suppress the
leading zero in 12 h mode.

### Bricks colour palette

Eight colours shared by background, digit ON, and digit background pickers:

```c
// indices 0-7: Red  Blue  Yellow  Green  Orange  Cyan  Magenta  Purple
static const uint8_t R[8] = { 255,   0, 255,   0, 255, 170,   0, 255 };
static const uint8_t G[8] = {   0,  85, 255, 170,  85,   0, 170,   0 };
static const uint8_t B[8] = {   0, 255,   0,   0,   0, 255, 170, 170 };
```

Pebble's display rounds each channel to the nearest of 0, 85, 170, 255.

## Documentation

Full SDK docs, tutorials, and API reference: <https://developer.repebble.com>
