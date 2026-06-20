# Bricks Face

A brick-toys inspired Pebble watchface for the **Pebble Time 2 (emery)**. The entire
screen is tiled with colourful bricks, and the current time is displayed
in a custom **3×5 brick font** — every digit is built from individual
14 px brick-style tiles.

## Features

- **Full-screen brick baseplate** — 10 px brick cells in 8 classic brick colours
  (red, blue, yellow, green, orange, cyan, magenta, purple)
- **3×5 brick digit font** — white bricks for lit pixels, dark-grey for unlit
- **Animated minute transitions** — on each minute tick only the digit(s) that
  actually changed build in row-by-row (top → bottom, 90 ms per row); unchanged
  digits stay fully visible
- **Battery-efficient** — redraws on `MINUTE_UNIT` only; no sub-second timers
  outside of the brief 450 ms build animation

## Building & running

```sh
pebble build                          # compile → build/bricksface.pbw
pebble install --emulator emery       # test in the emery QEMU emulator
pebble install --cloudpebble          # deploy to a paired real device
```

## Target platform

This watchface targets **emery** (Pebble Time 2, 200 × 228 px colour display).

## Project layout

```
src/c/bricksface.c   Watchface source — background, digit renderer, animation
resources/           Images, fonts, and other bundled resources (none currently)
package.json         Project metadata (UUID, platform, watchface flag)
wscript              Build rules
```

## Customisation

All visual and behavioural knobs live near the top of
`src/c/bricksface.c`. After any change run `pebble build` and
`pebble install --emulator emery` to preview.

### Brick sizes

```c
#define CELL     14   // digit brick pixel size  — increase for larger digits
#define BG_CELL  10   // background brick size   — decrease for a denser grid
```

Changing `CELL` also shifts the digit positions. The horizontal layout
formula is documented in the comment above the `DIG*_X` defines; update
those values to keep everything centred:

```
3 + D(CELL×3) + 3 + D(CELL×3) + 3 + C(CELL) + 3 + D(CELL×3) + 3 + D(CELL×3) + 3 = 200
```

`TIME_Y` controls the vertical position of the digit block. The
current value centres it: `(228 - 5×CELL) / 2`.

### Stud sizes

```c
#define DIG_STUD  3   // radius of the raised stud on digit bricks
#define BG_STUD   2   // radius of the raised stud on background bricks
```

Set either to `0` to hide studs entirely for a flat tile look.

### Animation speed

```c
#define ANIM_MS  90   // milliseconds between each row appearing
```

Lower values make the build-in animation faster; raise them to slow it
down. The full animation takes `5 × ANIM_MS` ms. Set to `0` to skip
animation (instant display on every tick — still battery-safe because
the underlying tick is `MINUTE_UNIT`). Only changed digits animate;
unchanged ones remain fully visible throughout.

### Digit shapes (the 3×5 font)

Each digit is a 5-element array of 3-bit row masks. Bit 2 = left
column, bit 1 = middle column, bit 0 = right column. A `1` lights a
white brick; a `0` shows a dark-grey brick.

```c
static const uint8_t DIGIT_MAP[10][5] = {
    { 0b111, 0b101, 0b101, 0b101, 0b111 }, /* 0 */
    ...
};
```

Visualise a row as a 3-cell grid:

```
0b111  →  ■ ■ ■
0b101  →  ■ · ■
0b001  →  · · ■
0b110  →  ■ ■ ·
```

Edit any row of any digit to reshape the font. For example, to give
`7` a mid-bar (like a European 7):

```c
{ 0b111, 0b001, 0b011, 0b001, 0b001 }, /* 7 with mid-bar */
```

### Background colour palette

The eight background colours are plain RGB triples in `bg_brick_color()`:

```c
static const uint8_t R[8] = { 255,   0, 255,   0, 255, 170,   0, 255 };
static const uint8_t G[8] = {   0,  85, 255, 170,  85,   0, 170,   0 };
static const uint8_t B[8] = {   0, 255,   0,   0,   0, 255, 170, 170 };
```

Each index is one colour: `{R[i], G[i], B[i]}`. Change any triple to
swap that colour slot. Pebble's display has 2 bits per channel, so
values are rounded to the nearest of 0, 85, 170, 255.

The colour assigned to a brick is determined by:

```c
int i = ((col * 5) + (row * 7)) % 8;
```

Change the multipliers (`5`, `7`) to alter the pattern — larger primes
produce a more scattered look; equal multipliers produce diagonal
stripes.

### Digit brick colours

In `canvas_update_proc`, the ON/OFF colours are set per brick:

```c
GColor color = on ? GColorWhite : GColorDarkGray;
```

Swap `GColorWhite` for any named Pebble colour (e.g. `GColorYellow`,
`GColorCyan`) to change the lit digit colour, and `GColorDarkGray` for
the unlit cell colour.

## Documentation

Full SDK docs, tutorials, and API reference: <https://developer.repebble.com>
