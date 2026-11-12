/* 砂時計タップ — データ定義（砂の色・遺物） */
'use strict';

/* レア度 0:ふつう 1:めずらしい 2:レア 3:幻 4:とくべつ(金の砂) */
const RARITY = [
  { label: 'ふつう', color: '#b9b4cc' },
  { label: 'めずらしい', color: '#7fd6a4' },
  { label: 'レア', color: '#b79bff' },
  { label: '幻', color: '#ffd666' },
  { label: 'とくべつ', color: '#ffd666' },
];

/* 出現の重み（レア度ごと）。合計に対する割合で抽選 */
const SAND_WEIGHT = [62, 27, 9, 2];
const RELIC_WEIGHT = [60, 28, 10, 2];

/* 砂の色。id は保存データのキーなので変更しないこと */
const SANDS = [
  { id: 'shiro', name: '白砂', r: 0, hex: '#e9dcc0', desc: 'どこにでもある、いちばん落ち着く砂。' },
  { id: 'ki', name: '黄砂', r: 0, hex: '#d9b166', desc: '遠い砂漠から風に乗ってきた砂。' },
  { id: 'aka', name: '赤土の砂', r: 0, hex: '#b9633c', desc: '夕焼けの丘でとれる、あたたかい色。' },
  { id: 'hai', name: '灰の砂', r: 0, hex: '#9c9aa0', desc: '火山のふもとの、さらさらした灰色。' },
  { id: 'kuro', name: '黒砂', r: 0, hex: '#4a4650', desc: '砂鉄まじり。磁石を近づけたくなる。' },
  { id: 'cha', name: '茶の砂', r: 0, hex: '#8c6238', desc: '川底でみがかれた、土の色の砂。' },

  { id: 'sakura', name: '桜色の砂', r: 1, hex: '#f2a9bb', desc: '貝がらが砕けてできた、うすい桃色。' },
  { id: 'wakakusa', name: '若草の砂', r: 1, hex: '#9cc56a', desc: 'かんらん石のつぶ。春の野原の色。' },
  { id: 'sora', name: '空色の砂', r: 1, hex: '#7cc3ea', desc: '晴れた日の浅瀬を閉じこめた色。' },
  { id: 'sango', name: '珊瑚の砂', r: 1, hex: '#f08868', desc: '南の島の浜辺でとれる、朱色の砂。' },
  { id: 'fuji', name: '藤色の砂', r: 1, hex: '#a98fdc', desc: '夕暮れどきにだけ見つかるという砂。' },
  { id: 'yuki', name: '雪の砂', r: 1, hex: '#eef4ff', desc: '白い砂丘から。ひんやりして見える。' },

  { id: 'hisui', name: '翡翠の砂', r: 2, hex: '#2fae8a', desc: '翡翠の原石が長い時間をかけて砕けた砂。' },
  { id: 'ruri', name: '瑠璃の砂', r: 2, hex: '#2f55c8', desc: '深い青。絵の具の材料にもなった石の粉。' },
  { id: 'kougyoku', name: '紅玉の砂', r: 2, hex: '#d42a4c', desc: 'ルビーのかけら。光にかざすと赤く燃える。' },
  { id: 'murasaki', name: '紫水晶の砂', r: 2, hex: '#8b42cc', desc: 'アメジストの細かな結晶。' },

  { id: 'hoshi', name: '星の砂', r: 3, hex: '#fff3b4', fx: 'sparkle', desc: 'ひとつぶひとつぶが星の形。ときどき瞬く。' },
  { id: 'yakou', name: '夜光の砂', r: 3, hex: '#55efc4', fx: 'glow', desc: '暗がりでぼんやり光る、ふしぎな砂。' },
  { id: 'niji', name: '虹の砂', r: 3, hex: '#ff9ad0', fx: 'rainbow', desc: '七色がまざって流れる、まぼろしの砂。' },

  { id: 'kin', name: '金の砂', r: 4, hex: '#f6c544', fx: 'sparkle', special: true, desc: 'ごくまれに砂にまじって降ってくる砂金。' },
];

/* 遺物。px はドット絵（'.' は透明）、pal は文字→色 */
const RELICS = [
  /* ---- ふつう ---- */
  { id: 'kosen', name: '古銭', r: 0, desc: 'まんなかに穴のあいた、むかしのお金。',
    pal: { o: '#6e3b1c', A: '#c27a44', h: '#eeb588', d: '#96552c' },
    px: ['..ooo..', '.oAAAo.', 'oAhAAAo', 'oAAoAAo', 'oAAAdAo', '.oAAAo.', '..ooo..'] },
  { id: 'sakuragai', name: '桜貝', r: 0, desc: 'うすくて割れやすい、ピンクの貝がら。',
    pal: { o: '#c47f97', P: '#f7b9cb', h: '#ffe3ec' },
    px: ['.ooooo.', 'oPhPhPo', 'oPhPhPo', 'oPPhPPo', '.oPPPo.', '..ooo..'] },
  { id: 'makigai', name: '巻貝', r: 0, desc: '耳にあてると、波の音がする気がする。',
    pal: { o: '#8a6a48', A: '#f2e2c0', B: '#c8966a' },
    px: ['....o..', '...oAo.', '..oAAo.', '..oBBAo', '.oAAAAo', '.oBBBAo', 'oAAAAo.', '.oooo..'] },
  { id: 'seaglass', name: 'シーグラス', r: 0, desc: '波にもまれて角のとれたガラス片。',
    pal: { o: '#3f9a86', G: '#7fd6c2', h: '#d2f7ee' },
    px: ['.oooo.', 'oGGhGo', 'oGhGGo', 'oGGGo.', '.ooo..'] },
  { id: 'doki', name: '土器のかけら', r: 0, desc: '縄の模様がついた、古い器の一部。',
    pal: { o: '#6a3820', T: '#c47a4a', d: '#8a4a28' },
    px: ['oooooo.', 'oTTTTTo', 'oTdTdTo', '.oTTTTo', '.oTdTo.', '..ooo..'] },
  { id: 'button', name: '古いボタン', r: 0, desc: 'だれかの上着から落ちた、骨のボタン。',
    pal: { o: '#8f7f66', B: '#ece1cd', d: '#6f604e' },
    px: ['..ooo..', '.oBBBo.', 'oBdBdBo', 'oBBBBBo', 'oBdBdBo', '.oBBBo.', '..ooo..'] },
  { id: 'hotate', name: 'ホタテ貝', r: 0, desc: '扇のかたちの、りっぱな貝がら。',
    pal: { o: '#b3735a', W: '#fdebd8', r: '#eaa57c' },
    px: ['..ooooo..', '.oWrWrWo.', 'oWrWrWrWo', 'oWrWrWrWo', 'oWrWrWrWo', '.oWrWrWo.', '..oWWWo..', '..ooooo..'] },
  { id: 'same', name: 'サメの歯', r: 0, desc: '何百万年も前のサメが落とした歯。',
    pal: { o: '#5f5a52', R: '#8d8576', W: '#ece7db', h: '#ffffff' },
    px: ['ooooooo', 'oRRRRRo', '.oWhWo.', '.oWhWo.', '..oWo..', '..oWo..', '...o...'] },

  /* ---- めずらしい ---- */
  { id: 'ginka', name: '銀貨', r: 1, desc: '黒ずんでいるが、みがけば光りそう。',
    pal: { o: '#6b7280', S: '#c9d1dc', h: '#ffffff', d: '#98a2b0' },
    px: ['..ooo..', '.oShSo.', 'oShSSSo', 'ohSdSSo', 'oSSSSdo', '.oSSdo.', '..ooo..'] },
  { id: 'ammonite', name: 'アンモナイト', r: 1, desc: 'ぐるぐる巻きの、太古の海のいきもの。',
    pal: { o: '#6e5a3e', A: '#cdb68c' },
    px: ['..ooooo..', '.oAAAAAo.', 'oAAoooAAo', 'oAoAAAoAo', 'oAoAoAoAo', 'oAoAooAAo', 'oAAAAAAo.', '.oooooo..'] },
  { id: 'takaragai', name: 'タカラガイ', r: 1, desc: 'つやつやの貝。むかしはお金がわりだった。',
    pal: { o: '#a07850', W: '#f8ead2', s: '#c98f58', d: '#5a3c22' },
    px: ['..ooo..', '.oWsWo.', 'oWWdWWo', 'oWsdWso', 'oWWdWWo', '.oWsWo.', '..ooo..'] },
  { id: 'kagi', name: '古い鍵', r: 1, desc: 'どこの扉のものか、もうだれも知らない。',
    pal: { k: '#8a6a2a', K: '#d2aa50' },
    px: ['.kkk.', 'kK.Kk', 'kK.Kk', '.kKk.', '..K..', '..K..', '..KK.', '..K..', '..KK.'] },
  { id: 'yajiri', name: '矢じり', r: 1, desc: '黒曜石を割ってつくった、石の矢の先。',
    pal: { o: '#16181d', B: '#3d4350', h: '#8f9ab0' },
    px: ['..o..', '.oBo.', '.oBo.', 'oBhBo', 'oBBBo', 'oBoBo', '.o.o.'] },
  { id: 'hitode', name: 'ヒトデ', r: 1, desc: 'すっかり乾いた、星形の海のいきもの。',
    pal: { o: '#b85a30', S: '#f28c5c', s: '#ffd0aa' },
    px: ['....o....', '...oSo...', 'oooSsSooo', 'oSSsSsSSo', '.ooSSSoo.', '..oSSSo..', '.oSSoSSo.', '.oo...oo.'] },
  { id: 'sangoeda', name: 'サンゴ', r: 1, desc: '赤い枝のような、海の宝石。',
    pal: { C: '#e5505c', h: '#ff9aa2', c: '#a8303c' },
    px: ['h..h..h', 'C..C.C.', '.C.CC..', '.CCC...', '..CC.h.', '..CCC..', '...C...', '..ccc..'] },
  { id: 'happa', name: '葉の化石', r: 1, desc: '石にのこった、一枚の葉のあと。',
    pal: { o: '#6a6052', S: '#aa9c88', L: '#5c5244' },
    px: ['.oooooo.', 'oSSSSLSo', 'oSSLLLSo', 'oSLLSLSo', 'oSLLLSSo', 'oLSSSSSo', '.oooooo.'] },

  /* ---- レア ---- */
  { id: 'kinka', name: '金貨', r: 2, desc: 'ずしりと重い。海賊の落としものかも。',
    pal: { o: '#a8741a', G: '#f6c945', h: '#fff3b8', d: '#d89c22' },
    px: ['...ooo...', '.ooGGGoo.', '.oGhhGGo.', 'oGhGGGGGo', 'oGhGGdGGo', 'oGGGGdGGo', '.oGGddGo.', '.ooGGGoo.', '...ooo...'] },
  { id: 'sanyochu', name: '三葉虫', r: 2, desc: '5億年前の海の底を歩いていた。',
    pal: { o: '#463a2c', T: '#94836c', d: '#6a5a46', e: '#231a12' },
    px: ['..ooo..', '.oTTTo.', 'oTeTeTo', 'oTTTTTo', 'odTdTdo', 'oTTdTTo', 'odTdTdo', '.oTdTo.', '..ooo..'] },
  { id: 'kohaku', name: '琥珀', r: 2, desc: '樹液のかたまり。中に小さな虫がいる。',
    pal: { o: '#a8621a', A: '#f2a534', h: '#ffe493', i: '#5a3410' },
    px: ['..ooo..', '.oAhAo.', 'oAhAAAo', 'oAAiAAo', '.oAAAo.', '..ooo..'] },
  { id: 'magatama', name: '勾玉', r: 2, desc: '翡翠をみがいた、いにしえのお守り。',
    pal: { o: '#1f6e50', J: '#45b98a', h: '#a6efd0' },
    px: ['..ooo.', '.oJhJo', 'oJJoJo', 'oJhJJo', 'oJJJo.', 'oJJo..', '.oJJo.', '..ooo.'] },
  { id: 'yubiwa', name: '古代の指輪', r: 2, desc: '赤い石のはまった、王族の指輪。',
    pal: { o: '#9a7420', G: '#f2c84a', R: '#e0304a', h: '#ffa3b0' },
    px: ['..oRo..', '.oRhRo.', '..oRo..', '.oGGGo.', 'oGo.oGo', 'oG...Go', 'oGo.oGo', '.oGGGo.'] },

  /* ---- 幻 ---- */
  { id: 'oukan', name: '小さな王冠', r: 3, desc: 'だれの頭にのっていたのだろう。',
    pal: { o: '#9a7420', Y: '#f5c542', r: '#e0304a', b: '#4a8af0' },
    px: ['r...b...r', 'oY.oYo.Yo', 'oYoYYYoYo', 'oYYYYYYYo', 'oYrYbYrYo', 'oYYYYYYYo', 'ooooooooo'] },
  { id: 'tamago', name: '恐竜のたまご', r: 3, desc: 'いまにも動きだしそうな、石のたまご。',
    pal: { o: '#857f66', E: '#ebe3cc', s: '#8fa37e' },
    px: ['..ooo..', '.oEEEo.', '.oEsEo.', 'oEEEEEo', 'oEsEEso', 'oEEEsEo', 'oEEEEEo', '.oEsEo.', '..ooo..'] },
  { id: 'hoshikake', name: '星のかけら', r: 3, desc: '夜空から落ちてきた、光るかけら。',
    pal: { o: '#c8961e', Y: '#ffe66e', h: '#ffffff' },
    px: ['....o....', '...oYo...', '...oYo...', '.ooYhYoo.', 'oYYhhhYYo', '.ooYhYoo.', '...oYo...', '...oYo...', '....o....'] },
];
