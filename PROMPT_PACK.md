# Sunny Hollow Farm: Prompt Pack cho Prototype

Tài liệu gồm 3 phần: Tiếng Việt, English, 日本語. Nội dung hướng dẫn được dịch sang từng ngôn ngữ. Các prompt gửi cho AI (Antigravity và gpt-image-2) giữ bằng tiếng Anh vì chúng hoạt động ổn định hơn.

---

# PHẦN 1: TIẾNG VIỆT

## 1. Tổng quan game

- **Tên tạm:** Sunny Hollow Farm
- **Thể loại:** Nông trại pixel 2D (trồng trọt, nấu ăn, buôn bán)
- **Nền tảng:** PC, viết bằng C++17 với SDL2
- **Mục tiêu prototype:** Chạy được vòng lặp: trồng → thu hoạch → nấu → bán → mua đồ
- **Phong cách:** Pixel art tươi sáng, màu ấm, kiểu Stardew Valley
- **Kích thước tile/sprite:** 32x32

## 2. Setup môi trường

**Công cụ cần cài:**
- Trình biên dịch C++17: GCC/MinGW (Windows), Clang (macOS) hoặc GCC (Linux)
- CMake 3.16+
- SDL2, SDL2_image, SDL2_ttf (hoặc dùng vcpkg / brew / apt)
- Antigravity (dùng để AI viết và sửa code)
- gpt-image-2 (tạo asset)
- Công cụ chỉnh ảnh: Aseprite, Piskel hoặc GIMP (để resize và dọn nền)

**Cấu trúc thư mục đề xuất:**

```
SunnyHollowFarm/
├── CMakeLists.txt
├── assets/
│   ├── sprites/        (player, npc, tiles, crops, items, tools, ui, buildings)
│   ├── fonts/
│   └── data/           (recipes.json hoặc recipes.txt, prices.txt)
└── src/
    ├── core/           (Game, Time, Input, Renderer, AssetManager)
    ├── world/          (Map, Tile, TileType)
    ├── entities/       (Player, NPC, Crop)
    ├── systems/        (FarmSystem, CookingSystem, ShopSystem, TimeSystem)
    ├── ui/             (InventoryUI, HUD, ShopUI)
    └── main.cpp
```

## 3. Prompt cho Antigravity

Copy đoạn sau vào Antigravity. Nên làm theo từng milestone, không yêu cầu làm hết một lần.

```
You are building a 2D pixel farming game in C++17 using SDL2 (plus SDL2_image and SDL2_ttf) with CMake.
Game name: Sunny Hollow Farm.

Architecture: split code into modules under src/: core, world, entities, systems, ui.
Use a fixed timestep game loop. Keep code readable for a beginner C++ developer:
prefer simple classes, avoid heavy templates, comment non-obvious parts.

Tile size: 32x32 pixels. Map: a grid of about 20x15 tiles.
Assets are loaded from assets/sprites/ and assets/fonts/. Until real art is available,
draw colored placeholder rectangles with the same size so the game stays playable.

Milestones (build and test each before moving on):
1. Window, game loop, input, tile map rendering, player movement in 4 directions with a simple walk animation.
2. Farming: hoe to till soil, plant tomato seeds, crop has 4 stages (seed, sprout, growing, harvest)
   that advance by time, watering can raises growth, harvest adds tomato to inventory.
3. Inventory grid and HUD: money counter, energy bar. Actions cost energy. Plant, water, and harvest cost energy.
4. Cooking: a stove/kitchen interaction takes ingredients from inventory and produces
   a Salad or Bread from recipes defined in assets/data/recipes.txt.
5. Shops: a seller NPC sells seeds, a buyer NPC buys crops and dishes at set prices.
   Show a simple shop UI with buy/sell buttons.
6. Day cycle: each day crops grow, energy restores at night, save state is optional.

For each milestone: list the files you create or change, explain briefly what changed,
and tell me exactly how to build and run the game.
```

## 4. Style guide cho gpt-image-2

Dán đoạn này vào **đầu mọi prompt tạo asset**:

```
Style: cozy bright pixel art, inspired by Stardew Valley. Pixel grid 32x32 per tile.
Top-down 3/4 view. Warm earthy palette: grass greens, soil browns, sunny yellow,
orange accents, tomato red highlights, cream and milk-brown for UI.
Clean 1px dark brown outline, flat soft shading with 2-3 tones per color, no anti-aliasing,
no gradients, no blur. Transparent background. No text, no watermark, no border frame.
Single subject centered, consistent lighting from the top-left.
```

**Mẹo quan trọng:** gpt-image-2 không đảm bảo đúng từng pixel. Hãy tạo ảnh ở kích thước lớn hơn (ví dụ 512x512), sau đó dùng Aseprite, Piskel hoặc công cụ resize với nearest-neighbor để về 32x32. Giữ bảng màu cố định bằng cách chọn lại màu về palette sau khi resize.

## 5. Danh sách asset (25 asset)

| # | Nhóm | Tên file | Mô tả ngắn |
|---|------|----------|------------|
| 1 | Nhân vật | player_sheet.png | Sprite sheet 4 hướng x 3 frame đi bộ (12 frame 32x32) |
| 2 | NPC | npc_seller.png | Người bán hạt giống và nguyên liệu, mặc tạp dề |
| 3 | NPC | npc_buyer.png | Người mua nông sản, đội mũ rơm |
| 4 | Tile | tiles_ground.png | Sheet gồm: đất khô, đất đã cày, cỏ, nước |
| 5 | Tile | tiles_fence.png | Hàng rào ngang và dọc |
| 6 | Cây | tomato_stage1.png | Hạt giống trong đất |
| 7 | Cây | tomato_stage2.png | Mầm nhỏ |
| 8 | Cây | tomato_stage3.png | Cây đang lớn |
| 9 | Cây | tomato_stage4.png | Cây có quả chín đỏ, sẵn sàng thu hoạch |
| 10 | Nông sản | item_tomato.png | Cà chua đỏ |
| 11 | Nông sản | item_carrot.png | Cà rốt cam |
| 12 | Nông sản | item_wheat.png | Bó lúa mì vàng |
| 13 | Món ăn | item_salad.png | Đĩa salad |
| 14 | Món ăn | item_bread.png | Ổ bánh mì |
| 15 | Công cụ | tool_hoe.png | Cuốc |
| 16 | Công cụ | tool_watering_can.png | Bình tưới nước |
| 17 | Công cụ | item_seed_packet.png | Gói hạt giống cà chua |
| 18 | UI | ui_slot.png | Ô túi đồ 32x32 khung kem |
| 19 | UI | ui_coin.png | Đồng xu vàng |
| 20 | UI | ui_energy_bar.png | Khung và phần tô thanh năng lượng |
| 21 | Công trình | building_farmhouse.png | Nhà chính, nhiều hơn 1 tile |
| 22 | Công trình | building_shop_stall.png | Quầy bán hàng có mái bạt |
| 23 | Bếp | building_stove.png | Bếp nấu với nồi |
| 24 | Đất | tile_tilled_watered.png | Đất đã tưới, tối màu hơn |
| 25 | Hiệu ứng | fx_sparkle.png | Ánh sáng nhỏ khi thu hoạch |

**Ví dụ prompt cho asset số 10 (ghép với style guide ở trên):**

```
[paste style guide]
Subject: a single ripe red tomato with a small green leaf on top, centered, item icon.
```

**Ví dụ prompt cho sprite sheet nhân vật (asset số 1):**

```
[paste style guide]
Subject: a cheerful farmer with a straw hat and blue overalls, sprite sheet with 12 frames
arranged in 4 rows (down, left, right, up) and 3 columns (walk cycle), each frame 32x32,
frames evenly spaced on a transparent background.
```

## 6. Quy trình làm việc đề xuất

1. Làm milestone 1 trong Antigravity với placeholder, chạy được nhân vật đi lại.
2. Tạo asset nhóm player và tile trước, thay placeholder và kiểm tra lại.
3. Làm farming (milestone 2) với asset cây trồng.
4. Làm inventory, cooking và shop theo từng bước.
5. Mỗi asset mới, lưu vào đúng thư mục assets/sprites/ và đặt tên đúng như bảng trên.
6. Nếu asset lệch style, tạo lại với cùng style guide, không sửa prompt gốc.

---

# PART 2: ENGLISH

## 1. Game overview

- **Working title:** Sunny Hollow Farm
- **Genre:** 2D pixel farming (farming, cooking, trading)
- **Platform:** PC, written in C++17 with SDL2
- **Prototype goal:** A working loop: plant → harvest → cook → sell → buy
- **Art style:** Bright pixel art with warm colors, Stardew Valley inspired
- **Tile/sprite size:** 32x32

## 2. Environment setup

**Tools to install:**
- C++17 compiler: GCC/MinGW (Windows), Clang (macOS), or GCC (Linux)
- CMake 3.16+
- SDL2, SDL2_image, SDL2_ttf (via vcpkg, brew, or apt)
- Antigravity (for AI-written and AI-edited code)
- gpt-image-2 (for assets)
- Image tools: Aseprite, Piskel, or GIMP (for resizing and cleaning backgrounds)

**Suggested folder structure:**

```
SunnyHollowFarm/
├── CMakeLists.txt
├── assets/
│   ├── sprites/        (player, npc, tiles, crops, items, tools, ui, buildings)
│   ├── fonts/
│   └── data/           (recipes.txt, prices.txt)
└── src/
    ├── core/           (Game, Time, Input, Renderer, AssetManager)
    ├── world/          (Map, Tile, TileType)
    ├── entities/       (Player, NPC, Crop)
    ├── systems/        (FarmSystem, CookingSystem, ShopSystem, TimeSystem)
    ├── ui/             (InventoryUI, HUD, ShopUI)
    └── main.cpp
```

## 3. Prompt for Antigravity

Copy the block below into Antigravity. Work through it one milestone at a time, not all at once.

```
You are building a 2D pixel farming game in C++17 using SDL2 (plus SDL2_image and SDL2_ttf) with CMake.
Game name: Sunny Hollow Farm.

Architecture: split code into modules under src/: core, world, entities, systems, ui.
Use a fixed timestep game loop. Keep code readable for a beginner C++ developer:
prefer simple classes, avoid heavy templates, comment non-obvious parts.

Tile size: 32x32 pixels. Map: a grid of about 20x15 tiles.
Assets are loaded from assets/sprites/ and assets/fonts/. Until real art is available,
draw colored placeholder rectangles with the same size so the game stays playable.

Milestones (build and test each before moving on):
1. Window, game loop, input, tile map rendering, player movement in 4 directions with a simple walk animation.
2. Farming: hoe to till soil, plant tomato seeds, crop has 4 stages (seed, sprout, growing, harvest)
   that advance by time, watering can raises growth, harvest adds tomato to inventory.
3. Inventory grid and HUD: money counter, energy bar. Actions cost energy. Plant, water, and harvest cost energy.
4. Cooking: a stove/kitchen interaction takes ingredients from inventory and produces
   a Salad or Bread from recipes defined in assets/data/recipes.txt.
5. Shops: a seller NPC sells seeds, a buyer NPC buys crops and dishes at set prices.
   Show a simple shop UI with buy/sell buttons.
6. Day cycle: each day crops grow, energy restores at night, save state is optional.

For each milestone: list the files you create or change, explain briefly what changed,
and tell me exactly how to build and run the game.
```

## 4. Style guide for gpt-image-2

Paste this at the **start of every asset prompt**:

```
Style: cozy bright pixel art, inspired by Stardew Valley. Pixel grid 32x32 per tile.
Top-down 3/4 view. Warm earthy palette: grass greens, soil browns, sunny yellow,
orange accents, tomato red highlights, cream and milk-brown for UI.
Clean 1px dark brown outline, flat soft shading with 2-3 tones per color, no anti-aliasing,
no gradients, no blur. Transparent background. No text, no watermark, no border frame.
Single subject centered, consistent lighting from the top-left.
```

**Tip:** gpt-image-2 does not guarantee exact pixels. Generate at a larger size (e.g. 512x512), then resize to 32x32 with nearest-neighbor in Aseprite, Piskel, or another tool. After resizing, snap colors back to the palette.

## 5. Asset list (25 assets)

| # | Group | File name | Short description |
|---|-------|-----------|-------------------|
| 1 | Character | player_sheet.png | 4 directions x 3 walk frames (12 frames 32x32) |
| 2 | NPC | npc_seller.png | Seed and supply seller, wearing an apron |
| 3 | NPC | npc_buyer.png | Crop buyer, wearing a straw hat |
| 4 | Tile | tiles_ground.png | Sheet: dry soil, tilled soil, grass, water |
| 5 | Tile | tiles_fence.png | Horizontal and vertical fence |
| 6 | Crop | tomato_stage1.png | Seed in soil |
| 7 | Crop | tomato_stage2.png | Small sprout |
| 8 | Crop | tomato_stage3.png | Growing plant |
| 9 | Crop | tomato_stage4.png | Plant with ripe red tomatoes, ready to harvest |
| 10 | Produce | item_tomato.png | Red tomato |
| 11 | Produce | item_carrot.png | Orange carrot |
| 12 | Produce | item_wheat.png | Yellow wheat bundle |
| 13 | Dish | item_salad.png | Plate of salad |
| 14 | Dish | item_bread.png | Loaf of bread |
| 15 | Tool | tool_hoe.png | Hoe |
| 16 | Tool | tool_watering_can.png | Watering can |
| 17 | Item | item_seed_packet.png | Tomato seed packet |
| 18 | UI | ui_slot.png | Inventory slot, cream frame, 32x32 |
| 19 | UI | ui_coin.png | Gold coin |
| 20 | UI | ui_energy_bar.png | Energy bar frame and fill |
| 21 | Building | building_farmhouse.png | Main house, larger than 1 tile |
| 22 | Building | building_shop_stall.png | Market stall with a striped awning |
| 23 | Building | building_stove.png | Cooking stove with a pot |
| 24 | Tile | tile_tilled_watered.png | Watered soil, darker tone |
| 25 | Effect | fx_sparkle.png | Small sparkle when harvesting |

**Example prompt for asset 10 (combine with the style guide above):**

```
[paste style guide]
Subject: a single ripe red tomato with a small green leaf on top, centered, item icon.
```

**Example prompt for the character sheet (asset 1):**

```
[paste style guide]
Subject: a cheerful farmer with a straw hat and blue overalls, sprite sheet with 12 frames
arranged in 4 rows (down, left, right, up) and 3 columns (walk cycle), each frame 32x32,
frames evenly spaced on a transparent background.
```

## 6. Suggested workflow

1. Build milestone 1 in Antigravity with placeholders and get the character moving.
2. Create the player and tile assets first, swap in the placeholders, and test.
3. Build farming (milestone 2) with the crop assets.
4. Add inventory, cooking, and shops step by step.
5. Save each new asset in the correct folder under assets/sprites/ using the names above.
6. If an asset drifts in style, regenerate it with the same style guide. Do not change the original guide.

---

# PART 3: 日本語

## 1. ゲーム概要

- **仮タイトル:** Sunny Hollow Farm
- **ジャンル:** 2Dピクセルファーミング（畑仕事・料理・売買）
- **プラットフォーム:** PC、C++17 と SDL2 で開発
- **プロトタイプの目標:** 「植える → 収穫する → 料理する → 売る → 買う」のループを動かす
- **アートスタイル:** 明るく暖色系のピクセルアート（Stardew Valley風）
- **タイル/スプライトサイズ:** 32x32

## 2. 開発環境のセットアップ

**インストールするツール:**
- C++17 コンパイラ: GCC/MinGW（Windows）、Clang（macOS）、GCC（Linux）
- CMake 3.16 以上
- SDL2、SDL2_image、SDL2_ttf（vcpkg、brew、apt のいずれかで導入）
- Antigravity（AIによるコード作成・修正用）
- gpt-image-2（アセット作成用）
- 画像ツール: Aseprite、Piskel、GIMP（リサイズ・背景処理用）

**推奨フォルダ構成:**

```
SunnyHollowFarm/
├── CMakeLists.txt
├── assets/
│   ├── sprites/        (player, npc, tiles, crops, items, tools, ui, buildings)
│   ├── fonts/
│   └── data/           (recipes.txt, prices.txt)
└── src/
    ├── core/           (Game, Time, Input, Renderer, AssetManager)
    ├── world/          (Map, Tile, TileType)
    ├── entities/       (Player, NPC, Crop)
    ├── systems/        (FarmSystem, CookingSystem, ShopSystem, TimeSystem)
    ├── ui/             (InventoryUI, HUD, ShopUI)
    └── main.cpp
```

## 3. Antigravity 用プロンプト

下のブロックを Antigravity にコピーしてください。一度に全部ではなく、マイルストーンごとに進めるのがおすすめです。

```
You are building a 2D pixel farming game in C++17 using SDL2 (plus SDL2_image and SDL2_ttf) with CMake.
Game name: Sunny Hollow Farm.

Architecture: split code into modules under src/: core, world, entities, systems, ui.
Use a fixed timestep game loop. Keep code readable for a beginner C++ developer:
prefer simple classes, avoid heavy templates, comment non-obvious parts.

Tile size: 32x32 pixels. Map: a grid of about 20x15 tiles.
Assets are loaded from assets/sprites/ and assets/fonts/. Until real art is available,
draw colored placeholder rectangles with the same size so the game stays playable.

Milestones (build and test each before moving on):
1. Window, game loop, input, tile map rendering, player movement in 4 directions with a simple walk animation.
2. Farming: hoe to till soil, plant tomato seeds, crop has 4 stages (seed, sprout, growing, harvest)
   that advance by time, watering can raises growth, harvest adds tomato to inventory.
3. Inventory grid and HUD: money counter, energy bar. Actions cost energy. Plant, water, and harvest cost energy.
4. Cooking: a stove/kitchen interaction takes ingredients from inventory and produces
   a Salad or Bread from recipes defined in assets/data/recipes.txt.
5. Shops: a seller NPC sells seeds, a buyer NPC buys crops and dishes at set prices.
   Show a simple shop UI with buy/sell buttons.
6. Day cycle: each day crops grow, energy restores at night, save state is optional.

For each milestone: list the files you create or change, explain briefly what changed,
and tell me exactly how to build and run the game.
```

## 4. gpt-image-2 用スタイルガイド

**すべてのアセット生成プロンプトの先頭に**このテキストを貼り付けてください:

```
Style: cozy bright pixel art, inspired by Stardew Valley. Pixel grid 32x32 per tile.
Top-down 3/4 view. Warm earthy palette: grass greens, soil browns, sunny yellow,
orange accents, tomato red highlights, cream and milk-brown for UI.
Clean 1px dark brown outline, flat soft shading with 2-3 tones per color, no anti-aliasing,
no gradients, no blur. Transparent background. No text, no watermark, no border frame.
Single subject centered, consistent lighting from the top-left.
```

**ヒント:** gpt-image-2 はピクセル単位の正確さを保証しません。512x512 など大きめのサイズで生成し、Aseprite、Piskel などで最近傍補間（nearest-neighbor）を使って 32x32 に縮小してください。縮小後はパレットの色に合わせて補正すると統一感が出ます。

## 5. アセット一覧（25点）

| # | グループ | ファイル名 | 簡単な説明 |
|---|---------|-----------|-----------|
| 1 | キャラクター | player_sheet.png | 4方向 x 歩行3フレーム（32x32 の12フレーム） |
| 2 | NPC | npc_seller.png | 種と資材の売り手、エプロン姿 |
| 3 | NPC | npc_buyer.png | 作物の買い手、麦わら帽子 |
| 4 | タイル | tiles_ground.png | 乾いた土、耕した土、草、水のシート |
| 5 | タイル | tiles_fence.png | 横と縦の柵 |
| 6 | 作物 | tomato_stage1.png | 土の中の種 |
| 7 | 作物 | tomato_stage2.png | 小さな芽 |
| 8 | 作物 | tomato_stage3.png | 成長中の植物 |
| 9 | 作物 | tomato_stage4.png | 赤く熟したトマトの実る株、収穫可能 |
| 10 | 農作物 | item_tomato.png | 赤いトマト |
| 11 | 農作物 | item_carrot.png | オレンジ色のニンジン |
| 12 | 農作物 | item_wheat.png | 黄色い小麦の束 |
| 13 | 料理 | item_salad.png | サラダの皿 |
| 14 | 料理 | item_bread.png | パンの塊 |
| 15 | 道具 | tool_hoe.png | クワ |
| 16 | 道具 | tool_watering_can.png | じょうろ |
| 17 | アイテム | item_seed_packet.png | トマトの種の袋 |
| 18 | UI | ui_slot.png | インベントリのスロット、クリーム色の枠、32x32 |
| 19 | UI | ui_coin.png | 金貨 |
| 20 | UI | ui_energy_bar.png | エネルギーバーの枠と中身 |
| 21 | 建物 | building_farmhouse.png | 母屋、1タイルより大きい |
| 22 | 建物 | building_shop_stall.png | ストライプの日よけ付きの露店 |
| 23 | 建物 | building_stove.png | 鍋付きの調理コンロ |
| 24 | タイル | tile_tilled_watered.png | 水をまいた土、暗めの色調 |
| 25 | エフェクト | fx_sparkle.png | 収穫時の小さなきらめき |

**アセット10のプロンプト例（上のスタイルガイドと組み合わせる）:**

```
[paste style guide]
Subject: a single ripe red tomato with a small green leaf on top, centered, item icon.
```

**キャラクターのシートのプロンプト例（アセット1）:**

```
[paste style guide]
Subject: a cheerful farmer with a straw hat and blue overalls, sprite sheet with 12 frames
arranged in 4 rows (down, left, right, up) and 3 columns (walk cycle), each frame 32x32,
frames evenly spaced on a transparent background.
```

## 6. 推奨ワークフロー

1. Antigravity でマイルストーン1を仮の四角い絵で実装し、キャラクターが動くところまで確認する。
2. プレイヤーとタイルのアセットを先に作り、仮の絵と差し替えてテストする。
3. 作物のアセットを使ってファーミング（マイルストーン2）を実装する。
4. インベントリ、料理、ショップを一つずつ追加する。
5. 新しいアセットは上の名前で assets/sprites/ の正しいフォルダに保存する。
6. スタイルがずれたアセットは、元のスタイルガイドを変えずに同じガイドで作り直す。
