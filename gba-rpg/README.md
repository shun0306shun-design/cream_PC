# Butano RPG Framework (GBA)

[Butano](https://github.com/GValiente/butano) で作る、クラシックなドラクエ風ターンベースRPGの基礎フレームワークです。
完成品ではなく「ここから町・ダンジョン・敵・魔法を足していける土台」を目指しています。

## デモでできること

- タイトル画面（New Game / セーブがあれば Continue）
- 村を歩く（十字キー、タイル単位の移動、壁・NPCとの衝突判定、カメラ追従）
- NPC・看板と会話（A）、宿屋で全回復（10G）、宝箱（一度開けたら空。開封状態はセーブされる）、道具屋（購入・売却）、回復の泉（生存者のHP/MPを全回復。戦闘不能は蘇生できない）、最初のダンジョン「洞窟」（フィールド南東の岩山の門から入る。3フロア、階段で移動、宝箱3つ、フロアごとに強くなる敵、スケルトンは3Fのみ）、歩き回るNPC（村人と長老。ランダムに歩き、話しかけも可能）、装備（STARTメニュー > Equip。武器・防具で攻撃/防御などが変わり、戦闘にも反映。職業ごとに装備できる物が違う）
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
│   ├── rpg_events.h            マップイベントの処理（会話・宿屋・宝箱・泉、共通の ask_yes_no）
│   ├── rpg_equip_menu.h        装備メニュー（キャラ → 部位 → 装備品の選択）
│   ├── rpg_shop.h              道具屋（購入・売却メニュー、売値の割合）
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
- データ表（敵・アイテムなど）は行の順番が enum の並びと一致している必要があります。新しい行は enum の最後に足した位置と同じ場所（表の最後）に書いてください。
- 装備品は `ItemData` の一種で、キャラごとの装備枠は `PartyMember::weapon/armor`。`PartyMember::stats()` が「職業+レベル+装備補正」を返し、戦闘も各メニューもこれを使います。
- 歩くNPCの実行時の状態（`Walker`、最大4体）は `FieldScene` が固定容量で持ちます。プレイヤーと同じく「移動先を先に予約」し、1歩の間は元のマスも塞ぐので重なりません。AIは待ち時間が尽きたときだけ働くので、毎フレームの負荷はほぼカウントダウンだけです。
- 「通れるか」の判定は `MapData::walkable()` の1か所だけ（移動・ミニマップ・セーブ読み込み時の位置チェックで共通）。
- セーブデータの大きさは固定です（所持品は `max_items` = 32種分の枠、フラグは64個）。アイテムやフラグを足しても形式は変わりません。形式を変えるとビルド時の `static_assert`（`sizeof(SaveData)`）が止まるので、`save_version` を上げて数値を更新してください。
- ロード時、HP/MPが現在の最大値を超えていたら（装備の補正を調整した後など）セーブを捨てずに最大値へ丸めます。
- ロード時、保存位置がもう通れない場所なら（マップを書き換えた後など）開始位置に戻します。
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
| 装備品を追加 | `ItemId` に追加 → `rpg_items.cpp` に1行（`kind` = weapon/armor、`classes` = 装備できる職業、`bonus` = 能力の加算値）。店の `stock`・宝箱の `item` にそのまま使える。装備は「アイテムの一種」なので所持数・売買・セーブの仕組みを共有する |
| ダンジョンの階・町を追加 | `MapId` に追加 → `rpg_maps.cpp` に ASCII レイアウト・イベント・出現テーブルを書く。階段は `stairs_up`/`stairs_down` タイル（`u`/`d`）+ `warp` イベント。ワープ元・先が通れないタイルだとビルドエラーになる（`valid_events()`） |
| NPCを歩かせる | その `MapEvent`（`npc_graphics` ありの会話・宿屋など。宝箱は不可）に `.movement = NpcMovement::wander, .wander_radius = 3` を足す。1マップに最大4体（`max_map_walkers`）。位置はセーブされず、マップを読み込むと元の場所に戻る（プレイヤーがその場所にいれば近くの空きマスに出る） |
| 道具屋を置く | マップの `events` に `EventType::shop`（`npc_graphics = 5`、`pages` = あいさつ、`stock` = 売る `ItemId` の配列）を書く |
| 回復の泉を置く | マップに `TileId::spring` のタイル（記号 `F`）を置き、同じ座標に `EventType::spring`（`pages` = 説明文、`price` = 1回の料金、0 なら無料）のイベントを書く。蘇生は宿屋だけ、という差をつけてある |
| 回復アイテムの効果・価格を調整 | `rpg_items.cpp` の表（`power` + `variance`、`price`）。魔法も同じ `Effect` 形式なので `rpg_spells.cpp` で同様に調整できる |
| 価格を変更 | 購入価格は `rpg_items.cpp` の `price`。売値は購入価格の何%かを `sell_percent`（rpg_shop.h）で一括変更 |
| 宝箱を置く | `FlagId`（rpg_flags.h）に名前を追加 → マップの `events` に `EventType::chest`（`npc_graphics = 3`、`flag`、`item` + `item_amount` / `gold`）を書く |
| フラグを追加（ボス撃破・イベント済みなど） | `FlagId` に追加し、`state.flags.get/set()` で読み書きする。セーブに自動で含まれる（最大64個、`max_flags`） |
| イベントの種類を追加（店など） | `EventType` に追加 → 処理を `rpg_events.cpp` に書き（はい/いいえは `ask_yes_no()`）、`FieldScene::_interact()` から呼ぶ。踏んで発動するものは `_on_step()` |
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
| `field_tiles.bmp` / `field_palette.bmp` | マップタイル（14番目が泉）。幅16pxで、16x16タイルを `TileId` の順に縦に並べる |
| `ui_tiles.bmp` / `ui_palette.bmp` | ウィンドウ枠（8x8 × 10枚の横並び） |
| `player.bmp` | 16x16 × 8コマ（下・上・左・右 × 歩行2コマ） |
| `npc.bmp` | 16x16 × N コマ。`MapEvent::npc_graphics` がコマ番号（3 = 閉じた宝箱、4 = 開いた宝箱、5 = 道具屋） |
| `enemy_*.bmp` | 32x32 の敵グラフィック |
| `cursor.bmp` | 8x8 のカーソル |

`python3 tools/gen_placeholder_assets.py` を実行するとプレースホルダーを再生成（上書き）します。

### フォントを差し替える（日本語化）
`Ui` のコンストラクタ（rpg_ui.cpp）で渡している `common::variable_8x16_sprite_font` を自作の `bn::sprite_font` に変えます。
かな・漢字は UTF-8 文字として `bn::utf8_characters_map` に登録します（Butano の `examples/text` が参考になります）。
16x16 の全角フォントは1文字1スプライトになりやすいので、上記のスプライト数制限に特に注意してください。

### BGM・効果音を入れる
`Makefile` の `AUDIOBACKEND` を `maxmod` に、`AUDIO := audio` にして、`audio/` に .mod/.xm/.it/.wav を置きます。

## ストーリー進行（第1章〜第2章）

メインストーリーは `GameState::story_progress`（u8、セーブ対象）1つで管理します。章ごとに10刻み（第1章 0–9 / 第2章 10–19 / … / クリア = 100）。
ビットフラグ（`FlagId`）は宝箱などの任意イベント専用です。

| 値 | 定数（`rpg_story.h` の `progress::`） | 状態 |
| --- | --- | --- |
| 0 | `ch1_start` | ニューゲーム直後（オープニング未表示） |
| 1 | `ch1_morning` | 平和なハルト村 |
| 2 | `ch1_errand` | 父に水くみを頼まれた → 泉を調べると襲撃イベント |
| 3 | `ch1_ashes` | 村焼失・両親の死。村マップが「焼け跡」状態に切り替わる |
| 4 | `ch1_departed` | 長老から父の剣を受け取り旅立ち。村の門が開く |
| 10 | `ch2_start` | 見張りの洞窟3Fで教団の斥候を撃破（紋章を発見）。ベルクへの道が開く |
| 11 | `ch2_rivals` | ベルクでシオンと対立。東の街道（廃砦）が開く |
| 12 | `ch2_allied` | 廃砦でシオンが共闘加入（ドレイク戦に負けても加入状態は維持） |
| 20 | `ch3_start` | 灰将ドレイク撃退。シオン正式加入 |

仕組み:

- **イベントの出現条件**: `MapEvent::min_progress` / `max_progress`。範囲外のイベントは存在しません（スプライト・当たり判定・会話すべて）。同じタイルに複数置いた場合は先に書いたものが優先。
- **章ごとの台詞**: `MapEvent::chapter_pages` に章ごとのページ集合を並べると、`chapter_index(story_progress)`（= `story_progress / 10`、0 が第1章）で切り替わります。
- **スクリプトイベント**: `EventType::story`（Aボタン）/ `EventType::trigger`（踏む）+ `StoryId`。台本は `src/rpg_story.cpp` に集約。ボス戦は `StoryResult::battle` で開始（逃走不可）、勝利後の処理は `run_story_victory()`。
- **マップの状態差分**: `rpg_maps.cpp` の `map_states[]`。ハルト村は平和（基本）／焼け跡（3〜99）。クリア後（100）は基本レイアウトに戻るので、これが「再建」状態の仮置きです。
- **パーティ**: `Party::size` 人だけが在籍（開始時は主人公1人）。`Party::join()` で加入。
- **固有名**: `include/rpg_names.h` のマクロ1か所（アレン / シオン / ハルト / ベルク / ドレイク…）。差し替えはここだけ。
- **全滅時の復帰地点**: `respawn_points[]`（第1章 = ハルト村、第2章以降 = ベルクの宿前）。

## 未実装（次に足すと良いもの）

蘇生手段（現状は宿屋のみ。蘇生アイテム・魔法は未実装）、装備の比較表示（装備前の能力変化プレビュー）、装備の特殊効果、武具屋、まとめ買い（数量指定）、状態異常、防御コマンド、建物の内部マップ、NPCの向きアニメーション（今は1コマ）、決まった道順を歩くNPC、BGM/SE。

## ライセンス表記

`graphics/common_*_8x16_font.*` と `include/common_*_8x16_sprite_font.h` は Butano（zlib License,
Copyright (c) 2020-2026 Gustavo Valiente）の `common` フォルダからのコピーです。
