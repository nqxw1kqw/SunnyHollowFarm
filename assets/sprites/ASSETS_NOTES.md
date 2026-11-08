# Sunny Hollow Farm art assets

All 25 raw PNGs were generated individually with the built-in image generator. Every
prompt begins verbatim with PROMPT_PACK.md Section 4. The exact prompts are in
[`../sprites_raw/PROMPTS.json`](../sprites_raw/PROMPTS.json). Raw files are untouched
copies of the generator outputs, with the same filenames as the runtime PNGs.

## Final dimensions

| Filename | Width × height (pixels) |
|---|---|
| `player_sheet.png` | 96×128 |
| `npc_seller.png` | 32×32 |
| `npc_buyer.png` | 32×32 |
| `tiles_ground.png` | 64×64 |
| `tiles_fence.png` | 64×32 |
| `tomato_stage1.png` | 32×32 |
| `tomato_stage2.png` | 32×32 |
| `tomato_stage3.png` | 32×32 |
| `tomato_stage4.png` | 32×32 |
| `item_tomato.png` | 32×32 |
| `item_carrot.png` | 32×32 |
| `item_wheat.png` | 32×32 |
| `item_salad.png` | 32×32 |
| `item_bread.png` | 32×32 |
| `tool_hoe.png` | 32×32 |
| `tool_watering_can.png` | 32×32 |
| `item_seed_packet.png` | 32×32 |
| `ui_slot.png` | 32×32 |
| `ui_coin.png` | 32×32 |
| `ui_energy_bar.png` | 32×32 |
| `building_farmhouse.png` | 96×64 |
| `building_shop_stall.png` | 96×64 |
| `building_stove.png` | 32×64 |
| `tile_tilled_watered.png` | 32×32 |
| `fx_sparkle.png` | 32×32 |

## Sheet coordinates

Coordinates below use a top-left origin; rectangles are (x, y, width, height).

- **Player:** 3 columns × 4 rows, each 32×32. Rows 0–3: down, left, right, up.
  Columns 0–2: neutral, left step, right step. Runtime walk cycle: 0,1,0,2.
  Source rectangle: (column×32, row×32, 32, 32).
  **Dimension correction:** the request's “128×96” conflicts with its 3-column,
  4-row layout. That layout requires **96×128**, which is used here.
- **Ground:** 2×2. Dry soil (0,0,32,32); tilled soil (32,0,32,32);
  grass (0,32,32,32); water (32,32,32,32).
- **Fence:** horizontal (0,0,32,32), vertical (32,0,32,32).
  The current map uses horizontal fences along its top edge.
- **Energy:** one 32×32 PNG, containing frame (0,0,32,16) and separate fill
  (2,20,28,8). Other pixels are transparent. Draw the frame, then crop the fill
  width by the energy fraction and draw it into the frame interior. HUD displays
  the frame at 70×16 and fill inside (x+5,y+4,60,8), reducing fill width with energy.
  The empty interior remains dark; leaves and fruit decorate the generated frame.

## Processing

Run `python3 assets/sprites_raw/process_assets.py` with Pillow installed.
All resizing uses `Image.Resampling.NEAREST`, with no dithering. RGB colors are
snapped to the explicit shared 38-color warm palette in the script; generated
alpha is thresholded at 128 to retain crisp transparent edges without halos.
No subject art is drawn or synthesized in the processing script.
All runtime files are RGBA PNGs. Ground tiles fill their cells and can be fully
opaque; sprites and UI retain transparent surrounding space.

The ground generation has a centered 2×2 sheet on a wider transparent canvas:
the script crops its observed bounds before scaling. Buildings and watered soil
are cropped to visible bounds. The energy frame and fill are cropped separately
from their raw generation and repacked into the rectangles above. Other sheets
retain their canvas/cell arrangement; small crop stages retain their bottom
alignment and space above.

## Runtime integration

The existing AssetManager could load PNGs but had no rendering callers.
A small `Renderer::drawSprite` helper now uses that manager with nearest-neighbor
SDL scaling. Player, NPCs, tomato stages, ground, fences, buildings, supported
item icons, tools, slots, coin and energy bar use the PNGs, with existing
placeholders retained for unavailable art.

Buildings preserve the existing map collision and interaction positions:
farmhouse 96×64 covers tiles (1..3,1..2); each 96×64 stall is anchored above its
existing three-tile row (13..15 or 16..18, y=1); the 32×64 stove extends upward
from interaction tile (4,2). Buildings render at native pixel density.
The sparkle is displayed beside harvest-ready tomatoes. Wheat crop stages and
wheat/carrot seed packets retain their existing placeholders because separate
art for those subjects is outside the 25-asset pack.

Smoke mode explicitly loads **all 25** PNGs through AssetManager, queries the SDL
texture dimensions, prints each success, and fails initialization on a missing
file or wrong size. This also covers assets not visible in the initial scene.

## Verification

- Debug CMake configure and build succeeded.
- `SDL_VIDEODRIVER=dummy ./build/sunny_hollow_farm --smoke-test` exited 0,
  logged all 25 PNGs loaded with the expected dimensions, and completed all
  150 gameplay test ticks successfully.
- Additional asset validation passed: 25 exact filenames in each directory,
  exact style-guide prefix in every saved prompt, correct RGBA PNG dimensions,
  binary alpha, shared palette, 12 nonempty player frames, separated energy pieces.
