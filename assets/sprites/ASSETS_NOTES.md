# Sunny Hollow Farm art assets

The original 25-asset pack is documented below. The eight additional UI assets
are documented in the **UI art pack** section at the end of this file.

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

## UI art pack

Eight new UI assets were generated individually with the built-in image generator.
`ui_slot.png`, `ui_coin.png`, and `ui_energy_bar.png` were preserved byte-for-byte.
No game code or integration was changed in this run.

| File | Final size | Intended use |
|---|---|---|
| `ui_panel.png` | 96×96 | General inventory, shop, and menu panel; plain opaque cream center |
| `ui_button.png` | 96×32 | Blank wooden/cream action button; add labels in the game |
| `ui_button_hover.png` | 96×32 | Sunny-yellow highlighted version of the same button |
| `ui_dialogue.png` | 96×48 | Dialogue or interaction prompt box with a centered bottom tail |
| `ui_cursor.png` | 32×32 | Cream gloved selection hand pointing right |
| `ui_shop_sign.png` | 64×32 | Hanging wooden shop sign with a coin emblem, no lettering |
| `ui_cooking_icon.png` | 32×32 | Lidded cooking pot for the cooking header |
| `ui_sleep_icon.png` | 32×32 | Crescent moon and two stars for the sleep prompt |

### Borders and stretching

All measurements below refer to final pixels, with a top-left origin. Rectangles
are `(x, y, width, height)`. Keep corners and border thickness at native scale;
stretch the center and the long direction of edge strips using nearest-neighbor.

| Asset | Visible border | Safe 9-slice insets (left, top, right, bottom) | Center rectangle |
|---|---|---|---|
| `ui_panel.png` | 1px dark-brown outer outline + 1px milk-brown rim; 1px transparent margin | (3, 3, 3, 3) | (3, 3, 90, 90) |
| `ui_button.png` | 1px dark outer outline; shaded rim reaches 4px horizontally and 5px vertically at straight edges | (8, 6, 8, 6) | (8, 6, 80, 20) |
| `ui_button_hover.png` | Same geometry and alpha mask as the base button | (8, 6, 8, 6) | (8, 6, 80, 20) |
| `ui_dialogue.png` | 1px dark outline; rim up to 4px; body occupies rows 0–41, tail extends below | (6, 5, 6, 12) | (6, 5, 84, 31) |

The dialogue's bottom 12px strip includes the tail. Ordinary 9-slicing stretches
that tail horizontally. To keep it small, render the rectangular body separately
from the tail: 9-slice source `(0,0,96,42)` with insets `(6,5,6,5)`, then draw
source `(40,37,16,11)` at native size, centered on the bottom edge with 5px overlap.
Use one of these approaches per draw so the tail is not drawn twice. The panel's
90×90 center is a single opaque cream color; button and dialogue centers are blank.

### UI generation and processing

Untouched accepted raw outputs are in `assets/sprites_raw/` under matching names.
`assets/sprites_raw/UI_PROMPTS.json` records every accepted prompt, crop rectangle,
final size, and processing recipe. Every prompt starts verbatim with Section 4's
style guide. The hover was generated by editing the base button as a reference.

The panel transparency generator repeatedly removed its cream interior. Its
accepted raw therefore uses a magenta extraction background: remove pixels with
R>180, B>180, G<100, crop `(30,30,1224,1224)`, downscale to 94×94, and center on
a transparent 96×96 canvas. No panel art was drawn by code. The 1px transparent
margin surrounds its generated 1px outline and 1px rim.

For the other seven assets, crop the raw to the recorded alpha>=128 bounds and
resize directly to its final size. All downscaling uses `Image.Resampling.NEAREST`.
Snap RGB to the existing 38-color palette from `process_assets.py`, without
dithering, and threshold alpha at 128. Copy the base button's final alpha mask
to the hover to align four differing edge pixels. The original processing script
still processes only the original pack; it does not rebuild these UI additions.

Validation passed for all eight: exact RGBA PNG sizes, nonempty subjects, binary
alpha including transparent pixels, shared palette, exact style-guide prompt
prefixes, identical button/hover masks, and a uniform opaque panel center.
Finals were visually inspected. No game build or integration test was needed for
this assets-only change.

## Connected terrain tileset

Generated as one sheet with the built-in image generator. Untouched source:
`../sprites_raw/terrain_tileset.png` (2048×768). Exact prompt:
`../sprites_raw/TERRAIN_PROMPT.txt`, beginning verbatim with Section 4's style guide.
Terrain-specific instructions override transparent surroundings with opaque,
edge-to-edge terrain. No game code or runtime integration changed.

### Layout map

Eight columns × three rows; top-left origin, each raw cell 256×256.
Names below omit the final `tile_` prefix and `.png` suffix.

| Row | Column 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| 1 | grass_0 | grass_1 | grass_2 | grass_3 | dirt_0 | dirt_1 | path_0 | path_1 |
| 2 | water_0 | water_1 | shore_n | shore_s | shore_e | shore_w | spare grass A | spare grass B |
| 3 | shore_ne | shore_nw | shore_se | shore_sw | grass_4 | grass_5 | grass_6 | grass_7 |

The request lists **22 final filenames**, despite specifying 24 files. Row 2
contains only six named cells; its two spare grass cells remain in the raw sheet
and are not exported under invented filenames. All 22 named finals are present.

### Shore naming

Shore tiles are **water with grass banks along the named edges**:
`shore_n` = top, `shore_s` = bottom, `shore_e` = right, `shore_w` = left.
`shore_ne` = top + right, `shore_nw` = top + left,
`shore_se` = bottom + right, `shore_sw` = bottom + left.
Directions describe the bank position, not the direction toward open water.
The two banks in each corner join into an L; the remaining interior is water.

### Processing and validation

Crop each named cell at `(column×256, row×256, 256, 256)` using zero-based
coordinates, then resize directly to 32×32 with Pillow's
`Image.Resampling.NEAREST` and save as RGBA PNG. No procedural terrain drawing,
palette substitution, smoothing, or edge repair was applied. All 22 finals were
checked for 32×32 dimensions and fully opaque alpha. The sheet was visually
inspected for layout and bank orientations. Generated shading and irregular bank
edges remain; exact pixel-for-pixel seamless joins are not guaranteed or verified.

## Fish item icons

| File | Final size | Subject |
|---|---|---|
| `item_fish.png` | 32×32 | Fresh silvery-blue fish, orange fins, side view facing right |
| `item_grilled_fish.png` | 32×32 | Same fish grilled golden-brown, three grill marks, small wooden skewer |

Both were generated with the built-in image generator; the raw fish was used as
the grilled version's visual reference. Untouched 1254×1254 RGBA raw PNGs are in
`../sprites_raw/` under matching filenames. Exact prompts and crop/placement
measurements are recorded in `../sprites_raw/FISH_PROMPTS.json`. Both prompts
begin verbatim with PROMPT_PACK.md Section 4's style guide.

Existing tomato, carrot, wheat, salad, and bread icons were checked for framing:
their visible widths range from 20 to 24 pixels. Each fish is fitted proportionally
to a 24×24 box and centered on a transparent 32×32 canvas. Crop bounds use raw
alpha >=128. All resizing uses `Image.Resampling.NEAREST`; RGB is snapped to the
existing 38-color palette without dithering and alpha is thresholded at 128.
The fish occupies 24×12 pixels; the skewered fish occupies 24×11 pixels.
The original processing script does not rebuild these additions.

Validation: both finals are nonempty 32×32 RGBA PNGs with binary alpha,
transparent margins, and colors from the shared palette. Raw and final images
were visually inspected. Only assets and notes changed; no runtime integration
or code changes were made, and no game build was needed.

## Chicken art set and brighter grilled fish

Seven assets were generated individually with the built-in image generator.
Every exact accepted prompt starts verbatim with Section 4's style guide and is
recorded in `../sprites_raw/CHICKEN_PROMPTS.json`, together with raw dimensions,
crop bounds, resized subject dimensions, and final placement. The idle hen was
the identity reference for all three action poses. Walk 1 uses the accepted
revision with a lifted rear foot to distinguish the opposite step.

| File | Raw size | Final size | Subject |
|---|---|---|---|
| `chicken_idle.png` | 1254×1254 | 32×32 | Plump white hen, red comb, standing facing right |
| `chicken_peck.png` | 1254×1254 | 32×32 | Same hen, head down and beak at ground level |
| `chicken_walk_0.png` | 1254×1254 | 32×32 | Same hen, forward step with separated feet |
| `chicken_walk_1.png` | 1254×1254 | 32×32 | Same hen, opposite step with rear foot lifted |
| `item_egg.png` | 1266×1242 | 32×32 | Single fresh warm cream egg |
| `item_omelette.png` | 1265×1243 | 32×32 | Folded golden omelette on a small cream plate |
| `item_grilled_fish.png` | 1254×1254 | 32×32 | Brighter gold-and-cream grilled fish on a skewer |

Raw PNGs are untouched generator outputs under matching filenames in
`../sprites_raw/`. Both the old grilled-fish raw and final were intentionally
replaced; the older Fish item icons section and FISH_PROMPTS.json describe the
superseded generation for this filename. CHICKEN_PROMPTS.json is authoritative
for its replacement. The fresh fish and tomato icons were preserved.

Processing: crop each raw to its alpha>=128 bounds, fit proportionally into a
24×24 box, resize using `Image.Resampling.NEAREST`, then center on a transparent
32×32 canvas using the recorded integer placement. Snap RGB to the existing
38-color palette from process_assets.py with no dithering and threshold alpha
at 128. No subject art was drawn by code. The original processing script does
not rebuild this addition. Hen standing/walking subjects are 20×24; pecking is
24×22; egg is 20×24; omelette is 24×22; grilled fish is 24×11.

Validation: all seven finals are nonempty RGBA PNGs, exactly 32×32, with binary
alpha, transparent margins, and colors from the shared palette. Finals were
visually inspected together with item_tomato.png at enlarged nearest-neighbor
scale. The replacement fish is visibly brighter and its three grill marks remain
readable. Only assets and notes changed; no game integration or code changes,
and no game build was needed.
