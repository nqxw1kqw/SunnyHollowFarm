# Sunny Hollow Farm

2D pixel farming game prototype (C++17 + SDL2). Build spec: see PROMPT_PACK.md.

## Save / Load System

### Overview
- **Auto-Save:** The game automatically saves whenever the player sleeps (at either the outdoor farmhouse rest area or the cozy indoor bed) to `savegame.json` in the current working directory.
- **Auto-Load:** On launch, if `savegame.json` is present in the working directory, the game automatically restores the saved session; otherwise it starts a standard new game.
- **In-Game Reset (F9):** Pressing **F9** at any time deletes `savegame.json` and resets the world to a fresh Day 1 state, announced by an on-screen banner (`RESET TO FRESH DAY 1`).

### Save File Format (`savegame.json`)

The save file uses standard JSON formatted with the following top-level keys:

| Key | Type | Description |
|---|---|---|
| `version` | Integer | Format schema version (e.g. `1`). |
| `day` | Integer | Current in-game day counter (starts at 1). |
| `wallet` | Integer | Farmer's currency balance in Gold (G). |
| `energy` | Integer | Farmer's current stamina/energy (0 to 100). |
| `muted` | Boolean | Audio mute state (`true` if muted, `false` otherwise). |
| `weather` | String | Current daily weather (`"Sunny"` or `"Rainy"`). |
| `player` | Object | Player position, orientation, and active scene. |
| `inventory` | Object | Active hotbar tool selection and container slot contents. |
| `crop_field` | Array | Full state of farm plots, soil hydration, and planted crops. |
| `chicken_count` | Integer | Total number of chickens owned on the farm (0 to 6). |
| `ground_eggs` | Array | Coordinates `[{"x": int, "y": int}, ...]` of uncollected eggs lying in the chicken pen. |

## Eating Food for Energy

Prepared dishes can be consumed to restore stamina:
- **Consuming Dishes (Key F):** With **Salad**, **Bread**, or **Omelette** selected in the hotbar or highlighted in the Inventory modal, press **F** to eat the dish.
- **Energy Restored:**
  - **Salad:** +25 Energy (displays banner `Ate Salad: +25 Energy`)
  - **Bread:** +35 Energy (displays banner `Ate Bread: +35 Energy`)
  - **Omelette:** +40 Energy (displays banner `Ate Omelette: +40 Energy`)
- **Stamina Clamping & Refusal:** Energy is clamped to maximum (100). If the farmer's energy is already full (100/100), consumption is refused with banner `ENERGY ALREADY FULL!`, and the item is retained in inventory.

## Weather System

### Overview
- **Deterministic Day Roll:** Each new day rolls weather deterministically from the day number using a seeded formula (reproducible in automated tests). Sunny most days, Rainy about 1 day in 3 (~31% occurrence). Specifically, Days 1-5 roll Sunny, Day 6 rolls Rainy, Day 7 rolls Sunny, Day 8 rolls Rainy.
- **Rainy Days & Automatic Crop Watering:** On a Rainy day:
  - An animated rain particle overlay falls across outdoor scenes.
  - All outdoor planted crops automatically count as watered that day: soil displays as `WateredSoil` and the 2.5x growth boost applies continuously without manual watering.
  - Farmhouse interiors and indoor objects remain completely unaffected.
- **HUD Weather Indicator:** A dedicated weather indicator badge is positioned directly next to the Day Counter display at the top-center:
  - **Sunny Day:** Displays a golden sun badge with cardinal rays and "SUN" text label.
  - **Rainy Day:** Displays a cool blue-gray rain cloud glyph with falling raindrops and "RAIN" text label.
- **Save / Load Persistence:** Daily weather condition is stored under the top-level `"weather"` key in `savegame.json` and restored on loading.

## Audio System (SDL2_mixer)

### Overview
- **Background Music:** Looping pastoral chiptune background music track (`assets/audio/music.wav`).
- **Sound Effects (SFX):** Audio feedback for farming actions (till, water, plant, harvest), cooking (success jingle, failure buzz), merchant trading (buy chime, sell register), resting (sleep chime), door transitions (door creak), and UI interactions (open modal, confirm/navigate).
- **Mute Toggle (Key M):** Pressing **M** toggles audio mute, displaying on-screen HUD feedback (`AUDIO MUTED` / `AUDIO UNMUTED`). The mute setting is persisted across sessions in `savegame.json`.
- **Headless / Robust Runtime:** Audio is fully optional at runtime. If no audio hardware or driver is present (e.g. running under `SDL_AUDIODRIVER=dummy`), the engine runs completely silently without errors or crashes.
- **Audio Assets & CC0 Licensing:** All sound effects and music are 100% public domain (CC0 1.0 Universal) generated procedurally via `tools/generate_audio.py`. Complete attribution and technical details are recorded in `assets/audio/CREDITS.md`.

### Packaging & Runtime Dependencies
For standalone packaging or distribution, ensure the following runtime libraries are bundled or installed:
- **Linux:** `libSDL2_mixer-2.0.so.0` (provided by Debian/Ubuntu `libsdl2-mixer-2.0-0`)
- **Windows:** `SDL2_mixer.dll`

#### `player` Object Keys
- `x` (Float): Farmer X coordinate in world pixels.
- `y` (Float): Farmer Y coordinate in world pixels.
- `facing` (String): Facing direction (`"Down"`, `"Left"`, `"Right"`, or `"Up"`).
- `location` (String): Current active location (`"Farm"` or `"FarmhouseInterior"`).

#### `inventory` Object Keys
- `activeTool` (Integer): Selected tool index (0: Hoe, 1: Watering Can, 2: Tomato Seeds, 3: Wheat Seeds, 4: Carrot Seeds, 5: Tomato, 6: Salad, 7: Bread).
- `slots` (Array of Objects): Each entry represents an inventory slot:
  - `type` (String): Item identifier name (e.g. `"TomatoSeeds"`, `"Wheat"`, `"Bread"`, `"Empty"`).
  - `count` (Integer): Number of items stored in this slot.

#### `crop_field` Plot Object Keys
- `x` (Integer): Plot tile grid X coordinate (0 to 19).
- `y` (Integer): Plot tile grid Y coordinate (0 to 14).
- `soil_state` (String): Ground state (`"Dirt"`, `"TilledSoil"`, or `"WateredSoil"`).
- `watered` (Boolean): `true` if plot is currently watered; `false` otherwise.
- `crop_type` (String): Planted crop type (`"Tomato"`, `"Wheat"`, `"Carrot"`, or `"None"`).
- `growth_stage` (Integer): Growth stage enum (1: Seed, 2: Sprout, 3: Growing, 4: Harvest, 0: None).
- `stage_timer` (Float): Accumulated stage growth progress timer in seconds.

## Pond Fishing System

### Overview & Controls
- **Starting Fishing (Key E):** Face any natural pond water tile (bottom-right pond area, coordinates x=16..18, y=11..13) from the bank and press **E** (or Space/Action).
  - Casting line costs **3 Energy** and plays a splash sound effect (`SFX::Water`).
  - An animated fishing float/bobber appears gently bobbing on the targeted water tile.
- **Bite Window & Catching:**
  - After a randomized wait of **2 to 5 seconds** (seeded RNG; deterministic under test injection), a bite occurs.
  - A bite window opens for approximately **800 ms**: an animated sparkle (`fx_sparkle`) and an exclamation prompt (`!`) appear directly on the water tile.
  - Press **E** inside the bite window to reel in **1x Fish** (announced by HUD banner `CAUGHT 1 FISH!`).
- **Scare & Escape Rules:**
  - **Early Press:** Pressing **E** too early before the bite window scares the fish away; fishing cancels with banner `TOO EARLY! FISH SCARED AWAY!` and no energy refund.
  - **Timeout Escape:** Letting the ~800 ms bite window expire without pressing **E** allows the fish to escape (`THE FISH ESCAPED!`).
  - **Inventory Capacity:** If inventory has no free space, the fish cannot be stored and escapes with banner `INVENTORY FULL! FISH ESCAPED!`.
  - **Movement & Modals:** Moving with directional keys (WASD/arrows) or opening any modal (Inventory, Cooking, Shop) immediately cancels the active fishing session.
- **State Machine Integration:** Fishing is only allowed outdoors on the farm; it cannot be triggered indoors, while any modal is open, or during screen transitions.

### Prices & Economy (`assets/data/prices.txt`)
- **Fish:** Not sold by the Seller NPC. Silas the Buyer NPC purchases fresh Fish for **30G**.
- **Grilled Fish:** Silas the Buyer NPC purchases Grilled Fish for **55G**.
- **Chicken:** The Seller NPC sells Chickens for **100G**. Silas the Buyer NPC does not buy chickens.
- **Egg:** Not sold by the Seller NPC. Silas the Buyer NPC purchases Eggs for **15G**.
- **Omelette:** Silas the Buyer NPC purchases Omelettes for **50G**.

### Cooking Recipes (`assets/data/recipes.txt`)
- **Grilled Fish:** 1x Fish (cooked at either the outdoor farmhouse patio stove or the indoor kitchen stove).
- **Omelette:** 2x Egg (cooked at either stove; restores +40 Energy when eaten).

## Chicken Livestock & Egg Production

### Pen Location & Architecture
- **Location:** Built in the open southwest grass pasture of the farm:
  - **Outer Fence Bounds:** Tile $X \in [1, 6]$, Tile $Y \in [10, 14]$.
  - **Interior Pen Area:** 4 tiles wide by 3 tiles tall ($X \in [2, 5]$, $Y \in [11, 13]$), giving 12 interior pasture tiles.
  - **Gate Gap:** 1-tile entrance opening at tile $(3, 10)$, allowing the player to freely walk in and out.
  - **Collision Safety:** Does not overlap the Farmhouse ($X \in [1, 3], Y \in [1, 2]$), Patio Stove ($(4, 2)$), Dirt Paths ($Y=3$ and $X=4, Y \in [4, 6]$), Crop Field ($X \in [5, 12], Y \in [4, 9]$), Pond ($X \in [16, 18], Y \in [11, 13]$), or Market Stalls ($X \in [13, 18], Y=1$).

### Purchasing Chickens
- **Seller NPC:** The Seller NPC offers **Chicken** for **100G**.
- **Immediate Pen Spawning:** Purchasing does not occupy an inventory bag slot. Instead, it increments the farm's chicken count and instantly spawns a chicken within the pen pasture.
- **Pen Capacity Cap:** The pen supports a maximum of **6 chickens**. Attempting to purchase beyond this cap is refused with a feedback banner (`Chicken pen is already at full capacity (6/6)!`) and no gold is deducted.
- **No Selling:** Chickens cannot be sold back to merchants.

### Chicken Behavior & Animation
- **Autonomous AI:** Chickens roam independently within the pen using a random walk state machine:
  - **Wandering:** Chickens pick random target positions and walk slowly towards them using 2-frame walking animations (`chicken_walk_0`, `chicken_walk_1`).
  - **Pausing & Pecking:** Upon stopping, chickens pause, occasionally pecking at the grass (`chicken_peck`) or standing idle (`chicken_idle`).
  - **Fence & Gate Containment:** Chicken movement is strictly clamped to the pen interior boundary ($X \in [64, 160], Y \in [352, 416]$ pixels). They are physically contained by the fences and never exit through the gate opening at $Y=10$.
  - **Art Fallback:** If sprite PNG assets (`chicken_idle.png`, `chicken_peck.png`, `chicken_walk_0.png`, `chicken_walk_1.png`) are not yet present, the engine renders animated pixel-art fallback sprites (white body, yellow beak, red comb, and animated stepping feet).

### Overnight Egg Laying & Egg Collection
- **Overnight Laying:** Each morning at sunrise, every chicken on the farm has a seeded **~60% chance** to lay an egg on its current pen tile.
- **Ground Cap:** Ground eggs are capped at **$2 \times \text{chicken count}$** eggs (up to 12 eggs for a full 6-chicken pen) to prevent overcrowding.
- **Egg Collection (Key E):**
  - The player walks into the pen, faces or stands on any egg tile, and presses **E** (Interact/Action).
  - Collecting an egg respects inventory bag capacity. If the inventory is full, collection is refused with banner `INVENTORY FULL!` and the egg stays on the ground.
  - On successful pickup, **1x Egg** is added to inventory, the egg sprite disappears from the pen ground, `SFX::Harvest` plays, and HUD displays `COLLECTED 1 EGG!`.

### Cooking & Economy
- **Egg:** Fresh produce item. Silas the Buyer NPC purchases Eggs for **15G**.
- **Omelette:** Cooked at either the indoor kitchen stove or outdoor patio stove using **2x Egg**. Silas the Buyer NPC buys Omelettes for **50G**. When eaten (**Key F**), an Omelette restores **+40 Energy**.

### Save / Load Persistence
- The farm's chicken count is serialized as `chicken_count` in `savegame.json`.
- Ground egg positions are serialized under `ground_eggs` as an array of `{"x": int, "y": int}` coordinates in `savegame.json`.
- Starting a new game starts with 0 chickens and 0 ground eggs. Loading restores both chickens and ground eggs exactly.

