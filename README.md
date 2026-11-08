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
| `player` | Object | Player position, orientation, and active scene. |
| `inventory` | Object | Active hotbar tool selection and container slot contents. |
| `crop_field` | Array | Full state of farm plots, soil hydration, and planted crops. |

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
