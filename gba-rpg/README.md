# Butano RPG Framework (GBA)

[Butano](https://github.com/GValiente/butano) で作る、クラシックなドラクエ風ターンベースRPGの基礎フレームワークです。
完成品ではなく「ここから町・ダンジョン・敵・魔法を足していける土台」を目指しています。

## デモでできること

- タイトル画面（New Game / セーブがあれば Continue）
- 村を歩く（十字キー、タイル単位の移動、壁・NPCとの衝突判定、カメラ追従）
- NPC・看板と会話（A）、宿屋で全回復（10G）、宝箱（一度開けたら空。開封状態はセーブされる）
- 南の門から外のフィールドへ（マップ間ワープ）
- フィールドでランダムエンカウント → ターンベース戦闘（たたかう / まほう / どうぐ / にげる）
- 素早さベースの行動順、攻撃・回復魔法・攻撃魔法・アイテム、経験値とレベルアップ、呪文習得
- マップ名の一時表示（シーン開始時・ワープ時に画面上部中央へ約1.75秒）と、右上の常時ミニマップ（`@`=自分 / `#`=通れない / `.`=通れる）
- フィールドメニュー（START）：ステータス / 道具 / 魔法 / セーブ（SRAM）

| ボタン | フィールド | メニュー・戦闘 |
| --- | --- | --- |
| 十字キー | 移動 | カーソル |
| A | 話す・調べる | 決定 / メッセージ送り |
| B | – | キャンセル / 前のキャラに戻る |
| START | メニューを開く | – |

> 表示テキストは英語です。同梱フォント（Butano の `common_variable_8x16_font`）が ASCII のみのためです。
> 日本語化は「フォントを差し替える」の項を参照してください。

## セットアップ（devkitPro + Butano）

1. **devkitARM** を入れる: <https://devkitpro.org/wiki/Getting_Started> （`gba-dev` グループ）。
   `DEVKITPRO` / `DEVKITARM` 環境変数が設定されていること。
2. **Python 3** を入れる（Butano のアセット変換で使用）。
3. **Butano** を取得する:
   ```sh
   git clone https://github.com/GValiente/butano.git
   ```
4. ビルドする。`LIBBUTANO` には Butano リポジトリ内の `butano/butano` フォルダを指定します:
   ```sh
   cd gba-rpg
   make -j8 LIBBUTANO=/path/to/butano/butano
   ```
   このリポジトリと `butano` を同じ親フォルダに並べて置いた場合は、既定値（`../../butano/butano`）のまま `make -j8` だけで通ります。
   Windows などで `python3` コマンドが無い場合は `make PYTHON=python` を付けてください。
5. できあがった `butano_rpg.gba` を mGBA などのエミュレータ、または実機で起動します。

Butano 公式の導入ガイド: <https://gvaliente.github.io/butano/getting_started.html>

## ファイル構成

```
gba-rpg/
├── Makefile                    Butano テンプレートベースのビルド設定
├── graphics/                   画像アセット（BMP + JSON）。すべてプレースホルダー
├── tools/
│   └── gen_placeholder_assets.py   プレースホルダー画像の生成スクリプト
├── include/
│   ├── rpg_common.h            定数、Stats、Effect（魔法・道具共通の効果定義）
│   ├── rpg_spells.h            魔法データ
│   ├── rpg_items.h             アイテムデータ
│   ├── rpg_classes.h           職業データ（初期値・成長率・習得呪文）
│   ├── rpg_enemies.h           敵データ、出現グループ
│   ├── rpg_maps.h              タイル属性、マップ、イベント
│   ├── rpg_flags.h             イベントフラグ（FlagId と固定サイズのビット集合）
│   ├── rpg_party.h             パーティの実行時状態（HP/MP/Lv/所持品/ゴールド）
│   ├── rpg_game_state.h        シーン間で共有する状態 + セーブ/ロード
│   ├── rpg_ui.h                ウィンドウ / メニュー / メッセージボックス
│   ├── rpg_field_scene.h       マップ探索シーン
│   ├── rpg_field_menu.h        フィールドメニュー
│   ├── rpg_battle_scene.h      戦闘シーン
│   ├── common_variable_8x16_sprite_font.h   Butano 付属フォント定義（zlib License）
│   └── common_fixed_8x16_sprite_font.h      同・固定幅フォント（ミニマップ用）
└── src/
    ├── main.cpp                エントリポイント、シーン遷移（タイトル → フィールド ⇔ 戦闘）
    ├── rpg_*.cpp               上記ヘッダーの実装
    └── data/                   ★ゲーム内容はここを編集する（すべて constexpr のテーブル）
        ├── rpg_spells.cpp
        ├── rpg_items.cpp
        ├── rpg_classes.cpp
        ├── rpg_enemies.cpp
        └── rpg_maps.cpp
```

## 設計のポイント

### データ駆動
ゲーム内容は `src/data/*.cpp` の `constexpr` 配列だけで定義します。ROM に置かれるので RAM を消費しません。

- **魔法とアイテムは同じ `Effect` 構造体**（種類・対象・威力・ばらつき）を共有し、戦闘でもフィールドでも同じ処理で解決されます。
- **キャラのステータスは保存しません。** 常に「職業 + レベル」から計算するので、バランス調整はテーブルを書き換えるだけで、セーブデータも小さくなります。
- **マップは ASCII アート**で書きます（1文字 = 16x16 タイル）。行の長さ違い・未定義の文字は `static_assert` でビルドエラーになります。

### シーン
`FieldScene` / `BattleScene` はスタック上に1つずつ生成され、抜けると背景・スプライト（VRAM）が解放されます。
シーンをまたいで残す情報はすべて `GameState` に置きます。

### UI
- ウィンドウ枠は動的な背景レイヤー（`bn::regular_bg_map_item`）に描き、文字は `bn::sprite_text_generator` のスプライトです。
- `Window` / `Menu` / `MessageBox` は RAII で、スコープを抜けると枠と文字が消えます。
- `Menu::run()` などはブロッキング（内部で毎フレーム `Ui::update()` を呼ぶ）なので、会話や戦闘の流れを上から下へ素直に書けます。
- 固定幅フォント（`Ui::fixed_text_generator()` / `Window::print_fixed()`）は、桁を揃えたい表示（ミニマップ）用です。
- フィールド右上のマップ名とミニマップは `FieldScene` が `bn::optional<Window>` で持ち、STARTメニューを開く間だけ閉じます（ウィンドウを重ねないため）。
- 表示の重なり順: マップ背景(3) < プレイヤー・NPC・敵(2) < ウィンドウ(0) < 文字。

### GBA のメモリ制約への配慮
- ヒープ（`new`）は使いません。可変長データは `bn::vector<T, N>` / `bn::string<N>` の固定容量です。
- 背景セルの大きなバッファ（マップ 64x64 セル = 8KB、UI 32x32 セル = 2KB）は `BN_DATA_EWRAM_BSS` で EWRAM に置き、32KB しかない IWRAM（スタック）を圧迫しません。
- **文字はスプライトです。** GBA のスプライトは最大128個（1スプライトに4〜6文字）なので、同時に大量の文字を出す画面を作るときは注意してください。
- ウィンドウ同士は重ねない前提です（片方を消すともう片方に穴が開きます）。サブメニューは隣に開くか、親を閉じてから開きます。

## 拡張のしかた

| やりたいこと | 編集する場所 |
| --- | --- |
| 魔法を追加 | `SpellId`（rpg_spells.h）に追加 → `rpg_spells.cpp` に1行 → `rpg_classes.cpp` の習得表に追加 |
| アイテムを追加 | `ItemId` に追加 → `rpg_items.cpp` に1行 |
| 敵を追加 | 32x32 の BMP+JSON を `graphics/` に置く → `EnemyId` に追加 → `rpg_enemies.cpp` に1行 → マップの `EncounterGroup` に入れる |
| 職業・初期パーティを変更 | `rpg_classes.cpp` / `Party::init_new_game()`（rpg_party.cpp） |
| 町・ダンジョンを追加 | `MapId` に追加 → `rpg_maps.cpp` に ASCII レイアウト・イベント・`maps[]` の項目を書く → 既存マップに `warp` イベントを置いてつなぐ |
| タイルの種類を追加 | `TileId` と `rpg_maps.cpp` の `tiles[]`（記号・通行可否・エンカウント倍率）、タイル画像を同じ順番で追加 |
| 宝箱を置く | `FlagId`（rpg_flags.h）に名前を追加 → マップの `events` に `EventType::chest`（`npc_graphics = 3`、`flag`、`item` + `item_amount` / `gold`）を書く |
| フラグを追加（ボス撃破・イベント済みなど） | `FlagId` に追加し、`state.flags.get/set()` で読み書きする。セーブに自動で含まれる（最大64個、`max_flags`） |
| イベントの種類を追加（店など） | `EventType` に追加 → `FieldScene::_interact()` / `_on_step()` に処理を書く |
| 敵の AI | `BattleScene::_enemy_turn()` |
| ダメージ計算式 | `roll_physical_damage()` / `roll_effect()`（rpg_party.cpp） |
| 経験値カーブ | `exp_for_level()`（rpg_classes.cpp） |
| セーブ内容を変更 | `SaveData`（rpg_game_state.cpp）。変更したら `save_version` を上げる（古いセーブは無視される）。フラグの `FlagId` は並べ替え・削除しない |

マップは最大 32x32 タイル（512x512 px）です。それより大きいマップが必要になったら、Butano の big map 対応や
マップのストリーミングに `FieldScene` を置き換えてください。

### アセットを差し替える
`graphics/` の BMP を同じサイズ・16色インデックスカラー（色0が透過）で上書きして再ビルドするだけです。

| ファイル | 内容 |
| --- | --- |
| `field_tiles.bmp` / `field_palette.bmp` | マップタイル。幅16pxで、16x16タイルを `TileId` の順に縦に並べる |
| `ui_tiles.bmp` / `ui_palette.bmp` | ウィンドウ枠（8x8 × 10枚の横並び） |
| `player.bmp` | 16x16 × 8コマ（下・上・左・右 × 歩行2コマ） |
| `npc.bmp` | 16x16 × N コマ。`MapEvent::npc_graphics` がコマ番号（3 = 閉じた宝箱、4 = 開いた宝箱） |
| `enemy_*.bmp` | 32x32 の敵グラフィック |
| `cursor.bmp` | 8x8 のカーソル |

`python3 tools/gen_placeholder_assets.py` を実行するとプレースホルダーを再生成（上書き）します。

### フォントを差し替える（日本語化）
`Ui` のコンストラクタ（rpg_ui.cpp）で渡している `common::variable_8x16_sprite_font` を自作の `bn::sprite_font` に変えます。
かな・漢字は UTF-8 文字として `bn::utf8_characters_map` に登録します（Butano の `examples/text` が参考になります）。
16x16 の全角フォントは1文字1スプライトになりやすいので、上記のスプライト数制限に特に注意してください。

### BGM・効果音を入れる
`Makefile` の `AUDIOBACKEND` を `maxmod` に、`AUDIO := audio` にして、`audio/` に .mod/.xm/.it/.wav を置きます。

## 未実装（次に足すと良いもの）

蘇生手段（現状は宿屋のみ）、装備、店、状態異常、防御コマンド、建物の内部マップ、歩くNPC、BGM/SE。

## ライセンス表記

`graphics/common_*_8x16_font.*` と `include/common_*_8x16_sprite_font.h` は Butano（zlib License,
Copyright (c) 2020-2026 Gustavo Valiente）の `common` フォルダからのコピーです。
