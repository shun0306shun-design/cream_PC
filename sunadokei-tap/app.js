/* 砂時計タップ — 本体（砂のシミュレーション・抽選・保存・画面） */
(() => {
  'use strict';

  /* ===================== 定数 ===================== */
  const GW = 101, BULB = 85, NECK = 7, GH = BULB * 2 + NECK; // 砂のマス目：横101×縦177
  const CX = 50, CXF = CX + 0.5;                             // 中心の列
  const NECK_TOP = BULB, BOT_TOP = BULB + NECK;              // くびれ・下の球のはじまる行
  const NECK_HALF = 1.9, MAX_HALF = 44, SHOULDER = 54, RC = 16;
  const FILL_U = 60;                 // 上の砂の高さ（くびれからの行数）
  const PADX = 4, PADY = 7;          // 木枠ぶんの余白（style.css の数値と対応）
  const GRAINS_PER_TAP = 12;         // 1タップで落ちる粒の数
  const P_GOLD = 1 / 260;            // 金の砂（1タップあたり）
  const P_RELIC = 1 / 110;           // 遺物（1タップあたり）
  const GOLD_GRAINS = 60;
  const HOLD_DELAY = 320, HOLD_INTERVAL = 165; // 長押しで流れつづける
  const STEP_HZ = 120;
  const STORE_KEY = 'sunadokei-tap:v1';
  const N = GW * (GH + 1);
  const rnd = Math.random;
  const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);

  function hash01(a, b) {
    let h = (Math.imul(a, 374761393) + Math.imul(b, 668265263)) | 0;
    h = Math.imul(h ^ (h >>> 13), 1274126177);
    return ((h ^ (h >>> 16)) >>> 0) / 4294967296;
  }

  /* ===================== ガラスの形 ===================== */
  // u = くびれからの距離。球の半分の幅を返す
  function halfW(u) {
    if (u <= 0) return NECK_HALF;
    const t = Math.min(1, u / SHOULDER);
    const s = 0.55 * Math.sin(t * Math.PI / 2) + 0.45 * (1 - Math.cos(t * Math.PI)) / 2;
    let w = NECK_HALF + (MAX_HALF - NECK_HALF) * s;
    const ub = BULB - RC;
    if (u > ub) {
      const d = Math.min(RC, u - ub);
      w = Math.min(w, MAX_HALF - RC + Math.sqrt(RC * RC - d * d));
    }
    return w;
  }
  function rowHalf(y) {
    const yc = y + 0.5;
    if (yc < BULB) return halfW(BULB - yc);
    if (yc < BOT_TOP) return NECK_HALF;
    return halfW(yc - BOT_TOP);
  }

  const m = new Uint8Array(N);            // 砂が動ける場所（くびれ＋下の球）
  const rowMin = new Int16Array(GH), rowMax = new Int16Array(GH);
  const Y_SURF = Math.ceil(BULB - 0.5 - FILL_U);
  const topList = [];
  for (let y = 0; y < GH; y++) {
    const lim = rowHalf(y) - 0.45;
    let a = GW, b = -1;
    for (let x = 0; x < GW; x++) {
      if (Math.abs(x - CX) > lim) continue;
      if (x < a) a = x;
      if (x > b) b = x;
      if (y >= NECK_TOP) m[y * GW + x] = 1;
      else if (y >= Y_SURF) {
        // 上の砂は「まんなかの表面から」順に減る（すり鉢状にへこむ）
        topList.push({ i: y * GW + x, d: (y - Y_SURF) + 0.55 * Math.abs(x - CX) + hash01(x, y) * 1.6 });
      }
    }
    rowMin[y] = a; rowMax[y] = b;
  }
  topList.sort((p, q) => p.d - q.d);
  const G = topList.length;               // 砂時計1本ぶんの粒の数
  const topIdx = Int32Array.from(topList, p => p.i);

  /* ===================== 色 ===================== */
  const WHITE = [255, 255, 255], BLACK = [0, 0, 0];
  const hexRgb = h => { const n = parseInt(h.slice(1), 16); return [n >> 16, (n >> 8) & 255, n & 255]; };
  const mix = (a, b, t) => [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t];
  const pack = c => (0xff000000 | (c[2] << 16) | (c[1] << 8) | (c[0] & 255)) >>> 0;
  function hsl(h, s, l) {
    h = (((h % 360) + 360) % 360) / 60;
    const c = (1 - Math.abs(2 * l - 1)) * s, x = c * (1 - Math.abs(h % 2 - 1)), k = l - c / 2;
    const p = h < 1 ? [c, x, 0] : h < 2 ? [x, c, 0] : h < 3 ? [0, c, x] : h < 4 ? [0, x, c] : h < 5 ? [x, 0, c] : [c, 0, x];
    return [(p[0] + k) * 255, (p[1] + k) * 255, (p[2] + k) * 255];
  }
  const SPARK = pack([255, 255, 255]);

  const sandIndex = {};
  SANDS.forEach((s, i) => { sandIndex[s.id] = i; });
  // 砂1色につき4段階の濃淡。マスの値 = (色番号+1)*4 + 濃淡
  const sandShades = SANDS.map(s => {
    if (s.fx === 'rainbow') return [0, 90, 180, 270].map(h => hsl(h + 330, 0.8, 0.68));
    const c = hexRgb(s.hex);
    return [mix(c, BLACK, 0.17), mix(c, BLACK, 0.07), mix(c, WHITE, 0.05), mix(c, WHITE, 0.17)];
  });
  const SAND_MAX = SANDS.length * 4 + 3;
  const GOLD_CI = sandIndex.kin, GOLD_V = (GOLD_CI + 1) * 4;
  const RAINBOW_V = (sandIndex.niji + 1) * 4, GLOW_CI = sandIndex.yakou, GLOW_V = (GLOW_CI + 1) * 4;

  // 遺物のドット絵 → マス値（256以上）に変換
  const relicColors = [];
  const RDEF = RELICS.map(r => {
    const w = r.px[0].length, h = r.px.length, cells = [], bot = new Array(w).fill(-1), local = {};
    r.px.forEach((row, dy) => {
      for (let dx = 0; dx < w; dx++) {
        const ch = row[dx];
        if (ch === '.') continue;
        if (!(ch in local)) { local[ch] = 256 + relicColors.length; relicColors.push(hexRgb(r.pal[ch])); }
        cells.push({ dx, dy, v: local[ch] });
        bot[dx] = dy;
      }
    });
    return { w, h, cells, bot };
  });

  const lut = new Uint32Array(256 + relicColors.length); // マス値 → 画面の色
  const spk = new Uint8Array(lut.length);                // きらめく色
  SANDS.forEach((s, ci) => {
    for (let k = 0; k < 4; k++) {
      lut[(ci + 1) * 4 + k] = pack(sandShades[ci][k]);
      if (s.fx === 'sparkle') spk[(ci + 1) * 4 + k] = 1;
    }
  });
  relicColors.forEach((c, k) => { lut[256 + k] = pack(c); });

  const sandByTier = [[], [], [], []], relicByTier = [[], [], [], []];
  SANDS.forEach((s, i) => { if (!s.special) sandByTier[s.r].push(i); });
  RELICS.forEach((r, i) => { relicByTier[r.r].push(i); });
  function pickTier(w) {
    let r = Math.random() * w.reduce((a, b) => a + b, 0);
    for (let i = 0; i < w.length; i++) { r -= w[i]; if (r < 0) return i; }
    return 0;
  }
  function rollSand(prev) {
    for (let n = 0; n < 8; n++) {
      const pool = sandByTier[pickTier(SAND_WEIGHT)];
      const ci = pool[(Math.random() * pool.length) | 0];
      if (ci !== prev) return ci;
    }
    return sandByTier[0][0] === prev ? sandByTier[0][1] : sandByTier[0][0];
  }

  /* ===================== 図鑑用の絵 ===================== */
  function makeImg(w, h, fn) {
    const c = document.createElement('canvas');
    c.width = w; c.height = h;
    const x = c.getContext('2d'), id = x.createImageData(w, h);
    fn(new Uint32Array(id.data.buffer));
    x.putImageData(id, 0, 0);
    return c.toDataURL();
  }
  const sandImg = SANDS.map((s, ci) => makeImg(12, 12, u => {
    for (let i = 0; i < 144; i++) {
      u[i] = pack(sandShades[ci][(hash01(i, ci * 7 + 3) * 4) | 0]);
      if (s.fx === 'sparkle' && hash01(i, ci + 91) < 0.07) u[i] = SPARK;
    }
  }));
  const SIL = pack([62, 58, 96]);
  const relicPic = sil => RDEF.map(R => makeImg(11, 11, u => {
    const ox = (11 - R.w) >> 1, oy = (11 - R.h) >> 1;
    for (const c of R.cells) u[(oy + c.dy) * 11 + ox + c.dx] = sil ? SIL : lut[c.v];
  }));
  const relicImg = relicPic(false), relicSil = relicPic(true);

  const el = (tag, cls, text) => {
    const e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text != null) e.textContent = text;
    return e;
  };
  function sandIcon(ci) {
    const e = el('i', 'sw');
    e.style.backgroundImage = `url(${sandImg[ci]})`;
    return e;
  }
  function relicIcon(ri, locked) {
    const e = el('img', 'px');
    e.src = locked ? relicSil[ri] : relicImg[ri];
    e.alt = '';
    e.draggable = false;
    return e;
  }

  /* ===================== 保存データ ===================== */
  const fresh = () => ({
    v: 1, taps: 0, grains: 0, flips: 0, gold: 0, finds: 0,
    sinceGold: 0, sinceRelic: 0, since: Date.now(),
    sands: {}, relics: {}, opt: { sound: true, vib: true }, cur: null,
  });
  function load() {
    const d = fresh();
    try {
      const o = JSON.parse(localStorage.getItem(STORE_KEY));
      if (o && o.v === 1) {
        for (const k of ['taps', 'grains', 'flips', 'gold', 'finds', 'sinceGold', 'sinceRelic', 'since']) {
          if (Number.isFinite(o[k]) && o[k] >= 0) d[k] = o[k];
        }
        const copy = (src, dst, list) => {
          if (!src || typeof src !== 'object') return;
          for (const it of list) {
            const r = src[it.id];
            if (Array.isArray(r) && r[0] > 0) dst[it.id] = [r[0] | 0, Number(r[1]) || Date.now()];
          }
        };
        copy(o.sands, d.sands, SANDS);
        copy(o.relics, d.relics, RELICS);
        if (o.opt) { d.opt.sound = o.opt.sound !== false; d.opt.vib = o.opt.vib !== false; }
        d.cur = o.cur || null;
      }
    } catch (e) { /* 壊れたデータは無視して最初から */ }
    return d;
  }
  let S = load();
  let dirty = false;

  /* ===================== 砂時計の状態 ===================== */
  const g = new Uint16Array(N);   // 0:空 / 4〜:砂 / 256〜:遺物（動かない）
  const mv = new Uint8Array(N);   // この一歩で動いた粒
  const topVal = new Uint8Array(G);
  let layers = [], layerEnd = [], curLayer = -1, emitted = 0, seed = 1;
  let pending = 0, goldLeft = 0, quiet = 0, stall = 0, stepNo = 0;
  let sprites = [];               // 落下中の遺物
  let phase = 'play';             // 'play' | 'flip'
  let lean = 0, tapBias = 0, playTime = 0;
  const swayPhase = Math.random() * 6.28;

  function rollLayers(first) {
    const n = 5 + ((Math.random() * 3) | 0);
    const w = [];
    let sum = 0;
    for (let i = 0; i < n; i++) { const v = 0.6 + Math.random(); w.push(v); sum += v; }
    const out = [];
    let used = 0, prev = -1;
    for (let i = 0; i < n; i++) {
      const size = i === n - 1 ? G - used : Math.round(G * w[i] / sum);
      used += size;
      const ci = first && i === 0 ? sandIndex.shiro : rollSand(prev);
      prev = ci;
      out.push([ci, size]);
    }
    return out;
  }
  function buildTop() {
    layerEnd = [];
    let k = 0;
    for (const [ci, size] of layers) {
      const base = (ci + 1) * 4;
      for (const e = k + size; k < e; k++) topVal[k] = base + ((hash01(k, seed) * 4) | 0);
      layerEnd.push(k);
    }
  }
  function newHourglass() {
    seed = (Math.random() * 2147483647) | 0;
    layers = rollLayers(S.taps === 0 && S.flips === 0);
    buildTop();
    emitted = 0; curLayer = -1; pending = 0; quiet = 0; stall = 0;
    sprites = [];
    g.fill(0);
  }

  const GRID_OFF = NECK_TOP * GW * 2, GRID_LEN = (GH - NECK_TOP) * GW * 2;
  function encGrid() {
    const u8 = new Uint8Array(g.buffer, GRID_OFF, GRID_LEN);
    let s = '';
    for (let i = 0; i < u8.length; i += 8192) s += String.fromCharCode.apply(null, u8.subarray(i, i + 8192));
    return btoa(s);
  }
  function decGrid(str) {
    let bin;
    try { bin = atob(str); } catch (e) { return false; }
    if (bin.length !== GRID_LEN) return false;
    const u8 = new Uint8Array(g.buffer, GRID_OFF, GRID_LEN);
    for (let i = 0; i < GRID_LEN; i++) u8[i] = bin.charCodeAt(i);
    for (let i = 0; i < N; i++) {
      const v = g[i];
      if (v && (!m[i] || v < 4 || (v > SAND_MAX && v < 256) || v >= lut.length)) g[i] = 0;
    }
    return true;
  }
  function restoreCur(c) {
    if (!c || !Array.isArray(c.layers) || typeof c.grid !== 'string') return false;
    const ls = [];
    let sum = 0;
    for (const L of c.layers) {
      if (!Array.isArray(L)) return false;
      const ci = sandIndex[L[0]], n = L[1] | 0;
      if (ci === undefined || n <= 0) return false;
      ls.push([ci, n]);
      sum += n;
    }
    if (sum !== G) return false;
    g.fill(0);
    if (!decGrid(c.grid)) { g.fill(0); return false; }
    layers = ls;
    seed = c.seed | 0;
    emitted = clamp(c.emitted | 0, 0, G);
    curLayer = Number.isInteger(c.li) ? clamp(c.li, -1, ls.length - 1) : -1;
    buildTop();
    return true;
  }
  function save() {
    dirty = false;
    S.cur = {
      seed, emitted, li: curLayer,
      layers: layers.map(([ci, n]) => [SANDS[ci].id, n]),
      grid: encGrid(),
    };
    try { localStorage.setItem(STORE_KEY, JSON.stringify(S)); } catch (e) { /* 保存できない環境でも遊べる */ }
  }

  /* ===================== 砂の動き ===================== */
  // 上の砂を1粒とる（層が変われば図鑑に記録）
  function nextGrain() {
    const k = emitted;
    let li = curLayer < 0 ? 0 : curLayer;
    while (k >= layerEnd[li]) li++;
    if (li !== curLayer) { curLayer = li; onLayer(li); }
    emitted++; pending--; S.grains++;
    dirty = true;
    let v = topVal[k];
    if (goldLeft > 0) { goldLeft--; v = GOLD_V + (v & 3); }
    return v;
  }
  function emitOne() {
    if (emitted >= G) { pending = 0; return; }
    const base = NECK_TOP * GW + CX, o = (rnd() * 3) | 0;
    for (let k = 0; k < 3; k++) {
      const i = base + ((o + k) % 3) - 1;
      if (g[i] === 0) { g[i] = nextGrain(); stall = 0; return; }
    }
    // くびれが詰まったまま動かないときの保険（通常は起きない）
    if (++stall > 360) { nextGrain(); stall = 300; }
  }

  function step() {
    stepNo++;
    mv.fill(0);
    let moved = 0;
    const al = Math.abs(lean), ld = lean < 0 ? -1 : 1;
    const ltr = (stepNo & 1) === 0;
    for (let y = GH - 2; y >= NECK_TOP; y--) {
      const a = rowMin[y], b = rowMax[y], base = y * GW;
      // 流れのゆらぎ：この行で斜めに落ちるかどうか（粒がそろって傾くので、流れが一本の筋になる）
      const yy = y - BOT_TOP;
      const slant = yy >= 0 && (((yy + 1) * al) | 0) !== ((yy * al) | 0);
      for (let k = a; k <= b; k++) {
        const i = base + (ltr ? k : a + b - k), v = g[i];
        if (v === 0 || v > 255) continue;
        const d = i + GW;
        let t = -1;
        if (m[d] && !g[d]) {
          t = d;
          if (slant || rnd() < 0.02) {
            const t2 = d + (slant ? ld : rnd() < 0.5 ? -1 : 1);
            if (m[t2] && !g[t2]) t = t2;
          }
        } else {
          const s = rnd() < 0.5 ? -1 : 1;
          if (m[d + s] && !g[d + s] && !g[i + s]) t = d + s;
          else if (m[d - s] && !g[d - s] && !g[i - s]) t = d - s;
          else if (v & 2) {
            // 丸い粒はもう一つ先まで転がる（山がなだらかになる）
            const s2 = s * 2;
            if (m[d + s2] && !g[d + s2] && !g[i + s] && !g[i + s2]) t = d + s2;
            else if (m[d - s2] && !g[d - s2] && !g[i - s] && !g[i - s2]) t = d - s2;
          }
        }
        if (t >= 0) { g[t] = v; g[i] = 0; mv[t] = 1; moved++; }
      }
    }
    return moved;
  }

  /* 遺物：落ちて、砂山に着いたら埋まっていく */
  function fits(R, x, y) {
    for (const c of R.cells) {
      const i = (y + c.dy) * GW + x + c.dx;
      if (!m[i] || g[i] > 255) return false;
    }
    return true;
  }
  function supported(sp) {
    const R = sp.def;
    let cols = 0, sup = 0;
    for (let c = 0; c < R.w; c++) {
      const b = R.bot[c];
      if (b < 0) continue;
      cols++;
      const i = (sp.y + b + 1) * GW + sp.x + c;
      if (!m[i] || g[i] > 255) return true;   // ガラス・ほかの遺物
      if (g[i] && !mv[i]) sup++;              // 止まっている砂
    }
    return sup * 2 >= cols;
  }
  function stamp(sp) {
    for (const c of sp.def.cells) {
      const i = (sp.y + c.dy) * GW + sp.x + c.dx;
      if (!m[i]) continue;
      const old = g[i];
      g[i] = c.v;
      if (old > 0 && old < 256) {
        // 重なった砂は上へ逃がす
        for (let j = i - GW; j >= NECK_TOP * GW && m[j]; j -= GW) {
          if (g[j] === 0) { g[j] = old; break; }
        }
      }
    }
    quiet = 0;
  }
  function stepSprites() {
    for (let n = sprites.length - 1; n >= 0; n--) {
      const sp = sprites[n];
      sp.fx += lean;
      const nx = Math.round(sp.fx);
      if (nx !== sp.x) {
        if (fits(sp.def, nx, sp.y)) sp.x = nx;
        else sp.fx = sp.x;
      }
      if (supported(sp) || !fits(sp.def, sp.x, sp.y + 1)) { stamp(sp); sprites.splice(n, 1); }
      else sp.y++;
    }
  }
  function spawnRelic(ri) {
    const R = RDEF[ri], x = CX - (R.w >> 1);
    for (let y = BOT_TOP + 1; y < GH - R.h; y++) {
      if (fits(R, x, y)) { sprites.push({ def: R, x, y, fx: x }); quiet = 0; return; }
    }
  }

  function tick() {
    playTime += 1 / STEP_HZ;
    lean = clamp(
      0.2 * Math.sin(playTime * 0.21 + swayPhase) + 0.09 * Math.sin(playTime * 0.57 + 1.7) + tapBias * 0.14,
      -0.36, 0.36);
    if (pending > 0) emitOne();
    const moved = step();
    if (sprites.length) stepSprites();
    if (moved === 0 && pending === 0) quiet++; else quiet = 0;
  }

  /* ===================== 画面 ===================== */
  const $ = id => document.getElementById(id);
  const app = $('app'), play = $('play'), hud = $('hud'), stage = $('stage'), glass = $('glass'), glow = $('glow');
  const cv = $('sand'), svg = $('glassSvg'), toasts = $('toasts'), hint = $('hint');
  const tapEl = $('tapCount'), glassNo = $('glassNo'), sandName = $('sandName'), sandChip = $('sandChip');
  const btnBook = $('btnBook'), bookCount = $('bookCount'), btnSound = $('btnSound');
  const sheet = $('sheet'), sheetBody = $('sheetBody'), sheetTitle = $('sheetTitle'), sheetCount = $('sheetCount');
  const detail = $('detail'), tabs = [...document.querySelectorAll('.sheet-foot .tab')];
  const fmt = n => Math.floor(n).toLocaleString('ja-JP');
  const fmtDate = ts => { const d = new Date(ts); return `${d.getFullYear()}/${d.getMonth() + 1}/${d.getDate()}`; };

  cv.width = GW; cv.height = GH;
  const ctx = cv.getContext('2d');
  const img = ctx.createImageData(GW, GH);
  const px = new Uint32Array(img.data.buffer);

  function buildGlass() {
    const L = [], R = [];
    for (let y = 0; y <= GH + 1e-6; y += 0.5) {
      const hw = y <= BULB ? halfW(BULB - y) : y < BOT_TOP ? NECK_HALF : halfW(y - BOT_TOP);
      L.push([CXF - hw, y]);
      R.push([CXF + hw, y]);
    }
    const line = pts => 'M' + pts.map(p => p[0].toFixed(2) + ' ' + p[1].toFixed(2)).join('L');
    const d = line(L.concat(R.reverse())) + 'Z';
    // つや：180°回しても同じ見た目になるよう、点対称に2本
    const sh = [];
    for (let u = 30; u <= 64; u += 2) sh.push([CXF - halfW(u) + 4.4, BULB - u]);
    const sh2 = sh.map(p => [GW - p[0], GH - p[1]]);
    svg.setAttribute('viewBox', `${-PADX} ${-PADY} ${GW + PADX * 2} ${GH + PADY * 2}`);
    svg.innerHTML =
      '<defs><linearGradient id="wood" x1="0" y1="0" x2="0" y2="1">' +
      '<stop offset="0" stop-color="#6a4426"/><stop offset=".45" stop-color="#b17d4f"/><stop offset="1" stop-color="#6a4426"/>' +
      '</linearGradient></defs>' +
      `<rect x="0.6" y="-1.5" width="2" height="${GH + 3}" fill="#8a5c36"/>` +
      `<rect x="${GW - 2.6}" y="-1.5" width="2" height="${GH + 3}" fill="#8a5c36"/>` +
      '<rect x="-1.5" y="-6.6" width="104" height="5.8" rx="2" fill="url(#wood)"/>' +
      `<rect x="-1.5" y="${GH + 0.8}" width="104" height="5.8" rx="2" fill="url(#wood)"/>` +
      `<path d="${d}" fill="rgba(170,200,255,.07)" stroke="rgba(208,226,255,.62)" stroke-width="1.3" stroke-linejoin="round"/>` +
      `<path d="${line(sh)}" fill="none" stroke="rgba(255,255,255,.2)" stroke-width="1.5" stroke-linecap="round"/>` +
      `<path d="${line(sh2)}" fill="none" stroke="rgba(255,255,255,.2)" stroke-width="1.5" stroke-linecap="round"/>`;
  }

  let playRect = { left: 0, top: 0, width: 360, height: 600 };
  function layout() {
    playRect = play.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    let s = Math.min((playRect.width - 16) / (GW + PADX * 2), (playRect.height - hud.offsetHeight - 10) / (GH + PADY * 2));
    s = clamp(Math.floor(s * dpr) / dpr, 1, 5);
    stage.style.setProperty('--s', s + 'px');
  }

  const glowBase = sandShades[GLOW_CI];
  let lastRender = -1e9, needRender = true;
  function render(t) {
    lastRender = t;
    // 動く色：虹の砂・夜光の砂
    const h0 = t * 0.03;
    const p = 0.08 + 0.24 * (0.5 + 0.5 * Math.sin(t * 0.0022));
    for (let s = 0; s < 4; s++) {
      lut[RAINBOW_V + s] = pack(hsl(h0 + s * 90, 0.8, 0.68));
      lut[GLOW_V + s] = pack(mix(glowBase[s], WHITE, p));
    }
    const tw = Math.imul((t / 170) | 0, 0x85ebca6b);
    px.fill(0);
    for (let k = emitted; k < G; k++) {
      const i = topIdx[k], v = topVal[k];
      px[i] = spk[v] && ((Math.imul(i, 0x9e3779b1) + tw) >>> 26) === 0 ? SPARK : lut[v];
    }
    for (let y = NECK_TOP; y < GH; y++) {
      const base = y * GW;
      for (let i = base + rowMin[y], e = base + rowMax[y]; i <= e; i++) {
        const v = g[i];
        if (v) px[i] = spk[v] && ((Math.imul(i, 0x9e3779b1) + tw) >>> 26) === 0 ? SPARK : lut[v];
      }
    }
    for (const sp of sprites) {
      for (const c of sp.def.cells) px[(sp.y + c.dy) * GW + sp.x + c.dx] = lut[c.v];
    }
    ctx.putImageData(img, 0, 0);
  }

  function updateHud() {
    const li = curLayer < 0 ? 0 : curLayer, ci = layers[li][0];
    sandName.textContent = SANDS[ci].name;
    sandChip.style.backgroundImage = `url(${sandImg[ci]})`;
    tapEl.textContent = fmt(S.taps);
    glassNo.textContent = fmt(S.flips + 1);
    const n = Object.keys(S.sands).length + Object.keys(S.relics).length;
    bookCount.textContent = `${n} / ${SANDS.length + RELICS.length}`;
    btnSound.classList.toggle('off', !S.opt.sound);
    hint.classList.toggle('off', S.taps >= 4);
  }

  const NEWB = '<span class="new-b">NEW</span>';
  function toast(icon, html, cls) {
    const e = el('div', 'toast' + (cls ? ' ' + cls : ''));
    if (icon) e.appendChild(icon);
    const tx = el('div', 'tx');
    tx.innerHTML = html; // 表示するのはこのファイルと data.js の固定文言だけ
    e.appendChild(tx);
    toasts.appendChild(e);
    while (toasts.children.length > 3) toasts.firstChild.remove();
    setTimeout(() => e.classList.add('out'), 2400);
    setTimeout(() => e.remove(), 2800);
  }
  function buzz(pattern) {
    if (S.opt.vib && navigator.vibrate) { try { navigator.vibrate(pattern); } catch (e) { /* 非対応 */ } }
  }

  const ripples = [];
  for (let i = 0; i < 6; i++) { const r = el('i', 'ripple'); play.appendChild(r); ripples.push(r); }
  let ripN = 0;
  function ripple(x, y) {
    const r = ripples[ripN++ % ripples.length];
    r.style.left = x + 'px';
    r.style.top = y + 'px';
    if (r.animate) {
      r.animate([{ opacity: 0.7, transform: 'scale(.25)' }, { opacity: 0, transform: 'scale(1)' }],
        { duration: 420, easing: 'ease-out' });
    }
  }

  /* ===================== 音（その場で合成。音声ファイルなし） ===================== */
  let ac = null, master = null, noiseBuf = null;
  function audioWake() {
    if (!S.opt.sound) return;
    if (!ac) {
      const AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return;
      try { ac = new AC(); } catch (e) { return; }
      master = ac.createGain();
      master.gain.value = 0.5;
      master.connect(ac.destination);
      noiseBuf = ac.createBuffer(1, ac.sampleRate, ac.sampleRate);
      const d = noiseBuf.getChannelData(0);
      for (let i = 0; i < d.length; i++) d[i] = rnd() * 2 - 1;
    }
    if (ac.state !== 'running') { const pr = ac.resume(); if (pr && pr.catch) pr.catch(() => {}); }
  }
  const audible = () => ac && S.opt.sound && ac.state === 'running';
  function noise(dur, freq, q, vol, type) {
    if (!audible()) return null;
    const t = ac.currentTime, src = ac.createBufferSource(), f = ac.createBiquadFilter(), a = ac.createGain();
    src.buffer = noiseBuf; src.loop = true;
    f.type = type || 'bandpass'; f.frequency.value = freq; f.Q.value = q;
    a.gain.setValueAtTime(0.0001, t);
    a.gain.exponentialRampToValueAtTime(vol, t + 0.012);
    a.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    src.connect(f); f.connect(a); a.connect(master);
    src.start(t, rnd() * 0.8);
    src.stop(t + dur + 0.03);
    return f;
  }
  function tone(freq, delay, dur, vol, type) {
    if (!audible()) return;
    const t = ac.currentTime + delay, o = ac.createOscillator(), a = ac.createGain();
    o.type = type || 'sine'; o.frequency.value = freq;
    a.gain.setValueAtTime(0.0001, t);
    a.gain.exponentialRampToValueAtTime(vol, t + 0.012);
    a.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(a); a.connect(master);
    o.start(t); o.stop(t + dur + 0.03);
  }
  const sfx = {
    tap() { noise(0.15 + rnd() * 0.06, 3400 + rnd() * 2600, 0.9, 0.1); },
    layer() { tone(660, 0, 0.5, 0.07); tone(880, 0.1, 0.6, 0.07); },
    gold() { [1319, 1568, 2093, 2637].forEach((f, i) => tone(f, i * 0.07, 0.7, 0.07, 'triangle')); },
    relic(r) {
      tone(196, 0, 0.18, 0.16, 'triangle');
      tone(1175, 0.03, 0.45, 0.06);
      if (r >= 2) { tone(1568, 0.14, 0.6, 0.06); tone(2093, 0.26, 0.8, 0.06); }
    },
    flip() {
      const f = noise(0.9, 400, 0.7, 0.13, 'lowpass');
      if (f) f.frequency.exponentialRampToValueAtTime(3200, ac.currentTime + 0.8);
    },
  };

  /* ===================== できごと ===================== */
  function addRec(map, id) {
    const r = map[id];
    if (r) { r[0]++; return false; }
    map[id] = [1, Date.now()];
    return true;
  }
  function onLayer(li) {
    const ci = layers[li][0], s = SANDS[ci];
    const isNew = addRec(S.sands, s.id);
    updateHud();
    if (isNew) {
      toast(sandIcon(ci), `${NEWB}${s.name}<small style="color:${RARITY[s.r].color}">あたらしい砂の色 ・ ${RARITY[s.r].label}</small>`, 'new');
      sfx.layer();
      buzz(20);
    }
  }
  function dropGold() {
    S.sinceGold = 0;
    S.gold++;
    goldLeft += GOLD_GRAINS;
    const isNew = addRec(S.sands, 'kin');
    toast(sandIcon(GOLD_CI), `${isNew ? NEWB : ''}金の砂が降ってきた<small>${fmt(S.gold)}回目</small>`, 'new');
    glow.classList.remove('gold');
    void glow.offsetWidth;
    glow.classList.add('gold');
    sfx.gold();
    buzz([20, 40, 20]);
    updateHud();
  }
  function dropRelic() {
    S.sinceRelic = 0;
    S.finds++;
    const pool = relicByTier[pickTier(RELIC_WEIGHT)];
    const ri = pool[(Math.random() * pool.length) | 0], it = RELICS[ri];
    const isNew = addRec(S.relics, it.id);
    spawnRelic(ri);
    toast(relicIcon(ri),
      `${isNew ? NEWB : ''}${it.name}<small style="color:${RARITY[it.r].color}">${RARITY[it.r].label}${isNew ? '' : ' ・ ×' + S.relics[it.id][0]}</small>`,
      isNew ? 'new' : '');
    sfx.relic(it.r);
    buzz(isNew ? [30, 50, 30] : 15);
    updateHud();
  }

  function tap(x, y) {
    if (phase !== 'play') return;
    const left = G - emitted - pending;
    if (left <= 0) return;
    pending += Math.min(GRAINS_PER_TAP, left);
    quiet = 0;
    S.taps++; S.sinceGold++; S.sinceRelic++;
    dirty = true;
    // タップした側へ、砂の流れが少しだけ寄る
    const bias = clamp((x - (playRect.left + playRect.width / 2)) / (playRect.width * 0.42), -1, 1);
    tapBias += (bias - tapBias) * 0.1;
    // 低確率のできごと（長く出ないときは必ず出る）
    if (Math.random() < P_GOLD || S.sinceGold >= 900 || (S.gold === 0 && S.taps >= 120)) dropGold();
    if (Math.random() < P_RELIC || S.sinceRelic >= 420 || (S.finds === 0 && S.taps >= 18)) dropRelic();
    tapEl.textContent = fmt(S.taps);
    if (S.taps === 4) hint.classList.add('off');
    ripple(x - playRect.left, y - playRect.top);
    sfx.tap();
  }

  function startFlip() {
    phase = 'flip';
    sfx.flip();
    glass.classList.add('flip');
    setTimeout(() => { cv.style.opacity = '0'; }, 820);
    setTimeout(() => {
      S.flips++;
      newHourglass();
      glass.classList.add('snap');
      glass.classList.remove('flip');
      void glass.offsetWidth;
      glass.classList.remove('snap');
      render(performance.now());
      cv.style.opacity = '1';
      phase = 'play';
      updateHud();
      toast(null, `ひっくり返した ・ ${fmt(S.flips + 1)}本目`, 'mini');
      save();
    }, 1020);
  }

  /* ===================== メインループ ===================== */
  let lastT = 0, acc = 0;
  function frame(t) {
    requestAnimationFrame(frame);
    const dt = Math.min(0.1, (t - lastT) / 1000);
    lastT = t;
    if (phase === 'play') {
      if (pending > 0 || quiet < 40 || sprites.length) {
        acc += dt * STEP_HZ;
        let n = acc | 0;
        acc -= n;
        if (n > 8) n = 8;
        while (n-- > 0) tick();
        needRender = true;
      } else {
        acc = 0;
        if (emitted >= G) startFlip();   // 砂が落ちきったら自動でひっくり返す
      }
    }
    if (needRender || t - lastRender > 170) { render(t); needRender = false; }
  }

  /* ===================== 入力 ===================== */
  const ptrs = new Map();
  let lastPtr = [0, 0], holdT = 0, holdI = 0;
  function stopHold() { clearTimeout(holdT); clearInterval(holdI); holdT = holdI = 0; }
  function startHold() {
    stopHold();
    holdT = setTimeout(() => {
      holdI = setInterval(() => tap(lastPtr[0], lastPtr[1]), HOLD_INTERVAL);
    }, HOLD_DELAY);
  }
  play.addEventListener('pointerdown', e => {
    if (e.pointerType === 'mouse' && e.button !== 0) return;
    e.preventDefault();
    audioWake();
    try { play.setPointerCapture(e.pointerId); } catch (err) { /* 古い端末 */ }
    lastPtr = [e.clientX, e.clientY];
    ptrs.set(e.pointerId, 1);
    tap(e.clientX, e.clientY);
    startHold();
  });
  play.addEventListener('pointermove', e => { if (ptrs.has(e.pointerId)) lastPtr = [e.clientX, e.clientY]; });
  const ptrEnd = e => { ptrs.delete(e.pointerId); if (!ptrs.size) stopHold(); };
  play.addEventListener('pointerup', ptrEnd);
  play.addEventListener('pointercancel', ptrEnd);
  play.addEventListener('lostpointercapture', ptrEnd);
  play.addEventListener('contextmenu', e => e.preventDefault());
  document.addEventListener('gesturestart', e => e.preventDefault());
  document.addEventListener('pointerup', audioWake);   // iOS は指を離した瞬間に音が有効になる
  document.addEventListener('touchend', audioWake);
  const allStop = () => { ptrs.clear(); stopHold(); };
  window.addEventListener('blur', allStop);

  /* ===================== 図鑑・きろく ===================== */
  let tab = 'sands';
  function setSound(on) {
    S.opt.sound = on;
    if (on) audioWake();
    updateHud();
    save();
  }
  function renderStats() {
    const dl = el('dl', 'stats');
    const row = (k, v) => { const d = el('div'); d.appendChild(el('dt', '', k)); d.appendChild(el('dd', '', v)); dl.appendChild(d); };
    row('タップ', fmt(S.taps) + ' 回');
    row('落とした砂', fmt(S.grains) + ' つぶ');
    row('ひっくり返した砂時計', fmt(S.flips) + ' 本');
    row('金の砂', fmt(S.gold) + ' 回');
    row('見つけた遺物', fmt(S.finds) + ' こ');
    row('砂の色', `${Object.keys(S.sands).length} / ${SANDS.length}`);
    row('遺物の種類', `${Object.keys(S.relics).length} / ${RELICS.length}`);
    row('はじめた日', fmtDate(S.since));
    sheetBody.appendChild(dl);

    const opts = el('div', 'opts');
    const toggle = (label, on, fn) => {
      const b = el('button', on ? '' : 'off');
      b.type = 'button';
      b.appendChild(document.createTextNode(label));
      b.appendChild(el('b', '', on ? 'ON' : 'OFF'));
      b.addEventListener('click', () => { fn(!on); renderSheet(); });
      opts.appendChild(b);
    };
    toggle('音', S.opt.sound, setSound);
    if (navigator.vibrate) toggle('振動', S.opt.vib, v => { S.opt.vib = v; save(); });
    sheetBody.appendChild(opts);

    const wipe = el('button', 'wipe', '記録をすべて消して最初から');
    wipe.type = 'button';
    let armed = 0;
    wipe.addEventListener('click', () => {
      if (!armed) {
        armed = setTimeout(() => { armed = 0; wipe.classList.remove('arm'); wipe.textContent = '記録をすべて消して最初から'; }, 4000);
        wipe.classList.add('arm');
        wipe.textContent = 'もう一度押すと、本当に消えます';
        return;
      }
      clearTimeout(armed);
      wiped = true;
      try { localStorage.removeItem(STORE_KEY); } catch (e) { /* 無視 */ }
      location.reload();
    });
    sheetBody.appendChild(wipe);
    sheetBody.appendChild(el('p', 'note', 'データはこの端末のブラウザの中だけに保存されます。課金・広告・アカウント登録・外部への送信はありません。'));
  }
  function showDetail(i) {
    detail.textContent = '';
    if (i == null) { detail.appendChild(el('span', 'empty', 'マスをタップすると、くわしく見られます')); return; }
    const isSand = tab === 'sands', it = (isSand ? SANDS : RELICS)[i], rec = (isSand ? S.sands : S.relics)[it.id];
    let icon;
    if (isSand) { icon = rec ? sandIcon(i) : el('i', 'sw'); if (!rec) icon.style.background = '#17162c'; }
    else icon = relicIcon(i, !rec);
    detail.appendChild(icon);
    const box = el('div');
    const head = el('div');
    head.appendChild(el('b', '', rec ? it.name : '？？？'));
    const em = el('em', '', RARITY[it.r].label);
    em.style.color = RARITY[it.r].color;
    head.appendChild(em);
    box.appendChild(head);
    box.appendChild(el('p', '', rec ? it.desc : 'まだ見つかっていません。'));
    if (rec) {
      const unit = !isSand ? 'こ 見つけた' : it.special ? '回 降ってきた' : '回 流れた';
      box.appendChild(el('small', '', `${fmt(rec[0])}${unit} ・ はじめて ${fmtDate(rec[1])}`));
    }
    detail.appendChild(box);
  }
  function renderSheet() {
    tabs.forEach(b => b.classList.toggle('on', b.dataset.tab === tab));
    sheetBody.textContent = '';
    sheetBody.scrollTop = 0;
    sheetCount.className = '';
    if (tab === 'stats') {
      sheetTitle.textContent = 'きろく';
      sheetCount.textContent = '';
      detail.classList.add('hide');
      renderStats();
      return;
    }
    detail.classList.remove('hide');
    const isSand = tab === 'sands', list = isSand ? SANDS : RELICS, got = isSand ? S.sands : S.relics;
    const grid = el('div', 'grid');
    let n = 0;
    list.forEach((it, i) => {
      const rec = got[it.id];
      if (rec) n++;
      const card = el('button', 'card' + (rec ? '' : ' lock'));
      card.type = 'button';
      card.dataset.i = i;
      const dot = el('i', 'dot');
      dot.style.background = RARITY[it.r].color;
      card.appendChild(dot);
      if (isSand) card.appendChild(rec ? sandIcon(i) : el('i', 'sw', '?'));
      else card.appendChild(relicIcon(i, !rec));
      card.appendChild(el('span', 'nm', rec ? it.name : '？？？'));
      if (rec) card.appendChild(el('span', 'ct', '×' + fmt(rec[0])));
      grid.appendChild(card);
    });
    sheetBody.appendChild(grid);
    sheetTitle.textContent = isSand ? '砂の色' : '遺物';
    sheetCount.textContent = (n === list.length ? 'コンプリート！ ' : '') + `${n} / ${list.length}`;
    sheetCount.classList.toggle('done', n === list.length);
    showDetail(null);
  }
  sheetBody.addEventListener('click', e => {
    const card = e.target.closest('.card');
    if (!card) return;
    sheetBody.querySelectorAll('.card.sel').forEach(c => c.classList.remove('sel'));
    card.classList.add('sel');
    showDetail(+card.dataset.i);
  });
  function openSheet() {
    allStop();
    renderSheet();
    app.classList.add('sheet-open');
    sheet.setAttribute('aria-hidden', 'false');
  }
  function closeSheet() {
    app.classList.remove('sheet-open');
    sheet.setAttribute('aria-hidden', 'true');
  }
  btnBook.addEventListener('click', openSheet);
  $('btnClose').addEventListener('click', closeSheet);
  $('backdrop').addEventListener('click', closeSheet);
  tabs.forEach(b => b.addEventListener('click', () => { tab = b.dataset.tab; renderSheet(); }));
  btnSound.addEventListener('click', () => setSound(!S.opt.sound));

  /* ===================== 起動 ===================== */
  let wiped = false;
  if (!restoreCur(S.cur)) newHourglass();
  buildGlass();
  layout();
  updateHud();
  window.addEventListener('resize', layout);
  window.addEventListener('orientationchange', layout);
  if (window.ResizeObserver) new ResizeObserver(layout).observe(play);
  requestAnimationFrame(frame);

  const autosave = () => { if (!wiped && phase === 'play') save(); };
  setInterval(() => { if (dirty) autosave(); }, 5000);
  document.addEventListener('visibilitychange', () => { if (document.hidden) { allStop(); autosave(); } });
  window.addEventListener('pagehide', autosave);

  // オフラインでも開けるように（http/https で配信したときだけ）
  if ('serviceWorker' in navigator && /^https?:$/.test(location.protocol)) {
    navigator.serviceWorker.register('sw.js').catch(() => { /* 使えなくても遊べる */ });
  }
})();
