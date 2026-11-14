# Butano RPG Framework (GBA)

[Butano](https://github.com/GValiente/butano) で作る、クラシックなドラクエ風ターンベースRPGの基礎フレームワークです。
完成品ではなく「ここから町・ダンジョン・敵・魔法を足していける土台」を目指しています。

## デモでできること

- タイトル画面（New Game / セーブがあれば Continue）
- 村を歩く（十字キー、タイル単位の移動、壁・NPCとの衝突判定、カメラ追従）
- NPC・看板と会話（A）、宿屋で全回復（10G）
- 南の門から外のフィールドへ（マップ間ワープ）
- フィールドでランダムエンカウント → ターンベース戦闘（たたかう / 技 / どうぐ / にげる）
- 素早さベースの行動順、攻撃・技（MP消費）・アイテム、経験値とレベルアップ（最大Lv100）、レベルによる技習得
- 敵のランク（通常 / ボス / ラスボス）に応じた複数回行動
- フィールドメニュー（START）：ステータス / 道具 / 技（回復系のみ） / セーブ（SRAM）

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
│   ├── rpg_spells.h            技データ（物理技・魔法共通。コード上の名前は Spell）
│   ├── rpg_items.h             アイテムデータ
│   ├── rpg_classes.h           キャラデータ（初期値・成長率・習得技）、経験値カーブ
│   ├── rpg_enemies.h           敵データ、敵ランクと行動回数、出現グループ
│   ├── rpg_maps.h              タイル属性、マップ、イベント
│   ├── rpg_party.h             パーティの実行時状態（HP/MP/Lv/所持品/ゴールド）
│   ├── rpg_game_state.h        シーン間で共有する状態 + セーブ/ロード
│   ├── rpg_ui.h                ウィンドウ / メニュー / メッセージボックス
│   ├── rpg_field_scene.h       マップ探索シーン
│   ├── rpg_field_menu.h        フィールドメニュー
│   ├── rpg_battle_scene.h      戦闘シーン
│   └── common_variable_8x16_sprite_font.h   Butano 付属フォント定義（zlib License）
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
- 表示の重なり順: マップ背景(3) < プレイヤー・NPC・敵(2) < ウィンドウ(0) < 文字。

### GBA のメモリ制約への配慮
- ヒープ（`new`）は使いません。可変長データは `bn::vector<T, N>` / `bn::string<N>` の固定容量です。
- 背景セルの大きなバッファ（マップ 64x64 セル = 8KB、UI 32x32 セル = 2KB）は `BN_DATA_EWRAM_BSS` で EWRAM に置き、32KB しかない IWRAM（スタック）を圧迫しません。
- **文字はスプライトです。** GBA のスプライトは最大128個（1スプライトに4〜6文字）なので、同時に大量の文字を出す画面を作るときは注意してください。
- ウィンドウ同士は重ねない前提です（片方を消すともう片方に穴が開きます）。サブメニューは隣に開くか、親を閉じてから開きます。

## 拡張のしかた

| やりたいこと | 編集する場所 |
| --- | --- |
| 技を追加 | `SpellId`（rpg_spells.h）に追加 → `rpg_spells.cpp` に1行 → `rpg_classes.cpp` の習得表に `{ レベル, 技 }` を追加 |
| 技の名前を変更 | `rpg_spells.cpp` の表の文字列を書き換えるだけ（他のコードは名前に依存しません） |
| アイテムを追加 | `ItemId` に追加 → `rpg_items.cpp` に1行 |
| 敵を追加 | 32x32 の BMP+JSON を `graphics/` に置く → `EnemyId` に追加 → `rpg_enemies.cpp` に1行 → マップの `EncounterGroup` に入れる |
| 職業・初期パーティを変更 | `rpg_classes.cpp` / `Party::init_new_game()`（rpg_party.cpp） |
| 町・ダンジョンを追加 | `MapId` に追加 → `rpg_maps.cpp` に ASCII レイアウト・イベント・`maps[]` の項目を書く → 既存マップに `warp` イベントを置いてつなぐ |
| タイルの種類を追加 | `TileId` と `rpg_maps.cpp` の `tiles[]`（記号・通行可否・エンカウント倍率）、タイル画像を同じ順番で追加 |
| イベントの種類を追加（店・宝箱など） | `EventType` に追加 → `FieldScene::_interact()` / `_on_step()` に処理を書く |
| 敵の AI | `BattleScene::_enemy_turn()` |
| 敵の行動回数の確率 | `action_chances[]`（rpg_enemies.cpp）。敵ごとのランクは `EnemyData::rank` |
| ダメージ計算式 | `roll_physical_damage()` / `roll_effect()`（rpg_party.cpp） |
| 経験値カーブ | `exp_for_level()` / `enemy_exp_for_level()`（rpg_classes.h） |
| セーブ内容を変更 | `SaveData`（rpg_game_state.cpp）。変更したら `save_version` を上げる |

マップは最大 32x32 タイル（512x512 px）です。それより大きいマップが必要になったら、Butano の big map 対応や
マップのストリーミングに `FieldScene` を置き換えてください。

### 技（レベルで覚える）
パーティは Hero（主人公）/ Sion（シオン）/ Mirea（ミレア）/ Nox（ノクス）。戦闘の「Skill」コマンドで MP を消費して使います。
表示名は ASCII の仮名です（同梱フォントが ASCII のみのため）。

| キャラ | 習得Lv: 技（MP）― 効果 |
| --- | --- |
| Hero | 5: Charge/ためる (2) 次の武器攻撃が2倍 ・ 10: Sweep/なぎ払い (4) 全体80% ・ 15: Mineuchi/みねうち (5) 120%+50%で気絶 ・ 20: Full Slash/全力斬り (8) 200% ・ 30: Ultima Slash/究極斬り (16) 350% |
| Sion | 6: Pierce/つきさす (3) 130% ・ 12: Double Thrust/連続突き (6) 80%×2 ・ 18: Whirl Spear/旋風槍 (9) 全体110% ・ 28: Infinite Spear/無限槍 (18) ランダム85%×4 |
| Mirea | 4: Heal/ヒール (3) 30〜40 ・ 9: Behoimi/ベホイミ (6) 80〜100 ・ 14: Behoma/ベホマ (10) 全快 ・ 19: Zaoral/ザオラル (12) 50%で蘇生(HP半分) ・ 27: Full Restore/完全回復 (30) 全員全快 |
| Nox | 5: Mera/メラ (2) 10〜16 ・ 10: Merami/メラミ (6) 55〜70 ・ 15: Merazoma/メラゾーマ (12) 120〜150 ・ 30: Doomfire/終焉の炎 (28) 全体220〜270 |

- 「%」は通常攻撃ダメージに対する倍率（`EffectType::physical`）。魔法は固定ダメージ（`EffectType::damage`）。
- 気絶した敵は次の自分のターンを失います。ボス・ラスボスには効きません。
- 1キャラが覚えられる技は `max_known_spells`（rpg_party.h、現在8）までです。増やすときはこの値も上げてください。

### 経験値バランス（最大 Lv100・本編クリア Lv60 前後）
必要経験値は 3 次式（`exp_for_level()`）で、序盤は数戦で上がり、後半ほど 1 レベルが重くなります。

| Lv | 2 | 5 | 10 | 20 | 30 | 40 | 50 | 60 | 100 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 累計EXP | 12 | 312 | 3,132 | 28,272 | 99,412 | 240,552 | 475,692 | 828,832 | 3,901,392 |

敵の経験値は手書きせず、**`enemy_exp_for_level(その敵が出るエリアの想定Lv, 倍率%)`** で決めます（rpg_enemies.cpp）。
想定Lvどおりの敵 2〜3 体と戦うと「2 + Lv×0.3 戦」で 1 レベル上がる計算です。

| 想定Lv | 1 | 5 | 10 | 20 | 30 | 40 | 50 | 60 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 敵1体のEXP | 2 | 34 | 102 | 264 | 436 | 613 | 791 | 971 |
| 1レベルに必要な戦闘数 | 2 | 4 | 5 | 8 | 11 | 14 | 17 | 20 |

Lv1 → Lv60 は合計およそ 670 戦。ボス（倍率 1000% が目安）のボーナスを含めると、
**最終エリアを想定Lv55〜60で作れば、普通に進めてクリア時 Lv55〜65** になります。Lv61〜100 はやり込み用です。

### 敵の行動回数（標準難易度）
`EnemyData::rank` で決まり、確率は `action_chances[]`（rpg_enemies.cpp）で調整できます。

| ランク | 1回 | 2回 | 3回 |
| --- | --- | --- | --- |
| `normal` 通常の敵 | 100% | – | – |
| `boss` 強敵 | 70% | 30% | – |
| `last_boss` ラスボス | 60% | 30% | 10% |

複数回行動で理不尽にならないよう、「呪文は1ターンに1回まで」「連続攻撃では、他に生存者がいれば直前と同じ相手を狙わない」
という制限を入れています。サンプルとして Goblin King（boss、Lv12想定）と Dark Lord（last_boss、Lv60想定）を
敵テーブルに入れてありますが、まだどの出現グループにも入っていません。

### アセットを差し替える
`graphics/` の BMP を同じサイズ・16色インデックスカラー（色0が透過）で上書きして再ビルドするだけです。

| ファイル | 内容 |
| --- | --- |
| `field_tiles.bmp` / `field_palette.bmp` | マップタイル。幅16pxで、16x16タイルを `TileId` の順に縦に並べる |
| `ui_tiles.bmp` / `ui_palette.bmp` | ウィンドウ枠（8x8 × 10枚の横並び） |
| `player.bmp` | 16x16 × 8コマ（下・上・左・右 × 歩行2コマ） |
| `npc.bmp` | 16x16 × N コマ。`MapEvent::npc_graphics` がコマ番号 |
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

蘇生アイテム（蘇生は宿屋と Zaoral のみ）、ボス戦イベント（ボスのデータはあるが配置手段が未実装）、装備、店、状態異常、防御コマンド、宝箱やフラグ管理、建物の内部マップ、歩くNPC、BGM/SE。

## ライセンス表記

`graphics/common_variable_8x16_font.*` と `include/common_variable_8x16_sprite_font.h` は Butano（zlib License,
Copyright (c) 2020-2026 Gustavo Valiente）の `common` フォルダからのコピーです。
