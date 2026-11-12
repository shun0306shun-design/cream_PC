/* 石ころころ — 本体（外部依存なし・通信なし・保存は localStorage のみ） */
(function () {
  'use strict';

  const S = window.Stones;
  const $ = (id) => document.getElementById(id);
  const TAU = Math.PI * 2;
  const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);
  const lerp = (a, b, t) => a + (b - a) * t;
  const smooth = (t) => { t = clamp(t, 0, 1); return t * t * (3 - 2 * t); };
  const mix = (a, b, t) => [lerp(a[0], b[0], t), lerp(a[1], b[1], t), lerp(a[2], b[2], t)];
  const css = (c, a) => 'rgba(' + (c[0] | 0) + ',' + (c[1] | 0) + ',' + (c[2] | 0) + ',' + (a == null ? 1 : +a.toFixed(3)) + ')';
  const rand = (a, b) => a + Math.random() * (b - a);
  const hex = (h) => { const n = parseInt(h.slice(1), 16); return [n >> 16, (n >> 8) & 255, n & 255]; };

  /* ================= 設定値 ================= */
  // めずらしい石の出る確率（石ひとつごと）。課金・ガチャ要素はありません。
  const RATE = { gem: 0.04, fossil: 0.03, meteorDay: 0.006, meteorNight: 0.028, gold: 0.004, pity: 30 };
  const NEED_MIN = 5000, NEED_VAR = 3000; // 石ひとつを仕上げるのに必要な、ころがし距離（1px = 1mm あつかい）
  const SCENE_LEN = 90, SCENE_HOLD = 40;  // 景色ひとつ 90 秒（40 秒とどまり、50 秒かけて次へ）
  const DAY_LEN = 540;                    // 朝→昼→夕→夜のひとめぐり（秒）
  const GRAV = 1500, SUB = 1 / 120, VMAX = 2400, GSTEP = 4;

  /* ================= 保存 ================= */
  const KEY = 'ishikorokoro.v1';
  function fresh() {
    return { v: 1, intro: false, sound: true, vib: true, hand: 'r', col: {}, total: 0, dist: 0, dry: 0, clock: 0, cur: null };
  }
  function loadSave() {
    try {
      const d = JSON.parse(localStorage.getItem(KEY) || 'null');
      if (!d || typeof d !== 'object') return fresh();
      const s = Object.assign(fresh(), d);
      if (!s.col || typeof s.col !== 'object') s.col = {};
      s.total = +s.total || 0; s.dist = +s.dist || 0; s.clock = +s.clock || 0; s.dry = +s.dry || 0;
      if (s.cur && (!S.get(s.cur.k) || !(s.cur.n > 0))) s.cur = null;
      return s;
    } catch (e) { return fresh(); }
  }
  let save = loadSave();
  let dirty = false, saveT = 0;
  function persist() {
    dirty = false; saveT = 0;
    if (stone && stone.state === 'play' && save.cur) save.cur.p = Math.round(stone.prog);
    try { localStorage.setItem(KEY, JSON.stringify(save)); } catch (e) { /* 保存できない環境でも遊べる */ }
  }

  /* ================= 音（Web Audio で合成。音声ファイルなし） ================= */
  const Snd = (function () {
    let ac = null, out = null, nbuf = null, rg = null, rf = null, lastTick = 0, lastThud = 0;
    function init() {
      if (ac || !save.sound) return;
      const AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return;
      try { ac = new AC(); } catch (e) { ac = null; return; }
      out = ac.createGain(); out.gain.value = 0.6;
      const comp = ac.createDynamicsCompressor();
      out.connect(comp); comp.connect(ac.destination);
      nbuf = ac.createBuffer(1, (ac.sampleRate * 2) | 0, ac.sampleRate);
      const d = nbuf.getChannelData(0);
      for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
      const src = ac.createBufferSource(); src.buffer = nbuf; src.loop = true;
      rf = ac.createBiquadFilter(); rf.type = 'lowpass'; rf.frequency.value = 160;
      rg = ac.createGain(); rg.gain.value = 0;
      src.connect(rf); rf.connect(rg); rg.connect(out); src.start();
    }
    function wake() { init(); if (ac && ac.state === 'suspended' && save.sound && !document.hidden) ac.resume(); }
    function ok() { return ac && save.sound && ac.state === 'running'; }
    function burst(dur, type, f0, f1, q, vol, delay) {
      if (!ok()) return;
      const t = ac.currentTime + (delay || 0);
      const s = ac.createBufferSource(); s.buffer = nbuf;
      const f = ac.createBiquadFilter(); f.type = type; f.Q.value = q;
      f.frequency.setValueAtTime(f0, t);
      if (f1) f.frequency.exponentialRampToValueAtTime(f1, t + dur);
      const g = ac.createGain();
      g.gain.setValueAtTime(Math.max(0.0002, vol), t);
      g.gain.exponentialRampToValueAtTime(0.0002, t + dur);
      s.connect(f); f.connect(g); g.connect(out);
      s.start(t, Math.random() * 1.4, dur + 0.05);
    }
    function tone(f0, f1, dur, vol, delay, attack) {
      if (!ok()) return;
      const t = ac.currentTime + (delay || 0);
      const o = ac.createOscillator(); o.type = 'sine';
      o.frequency.setValueAtTime(f0, t);
      if (f1) o.frequency.exponentialRampToValueAtTime(f1, t + dur);
      const g = ac.createGain();
      g.gain.setValueAtTime(0.0002, t);
      g.gain.exponentialRampToValueAtTime(Math.max(0.0003, vol), t + (attack || 0.004));
      g.gain.exponentialRampToValueAtTime(0.0002, t + dur);
      o.connect(g); g.connect(out);
      o.start(t); o.stop(t + dur + 0.05);
    }
    function bell(f, vol, delay, dur) { tone(f, 0, dur, vol, delay, 0.008); tone(f * 2.76, 0, dur * 0.4, vol * 0.22, delay, 0.008); }
    return {
      wake: wake,
      apply() { if (out) out.gain.value = save.sound ? 0.6 : 0; if (save.sound) wake(); },
      sleep() { if (ac && ac.state === 'running') ac.suspend(); },
      tick(v, r) {
        if (!ok() || ac.currentTime - lastTick < 0.038) return;
        lastTick = ac.currentTime;
        const k = 36 / r;
        burst(0.045, 'bandpass', (1300 + Math.random() * 900) * k, 0, 2.5, 0.42 * v);
        tone((170 + Math.random() * 40) * k, 110 * k, 0.05, 0.3 * v);
      },
      thud(v) {
        if (!ok() || ac.currentTime - lastThud < 0.05) return;
        lastThud = ac.currentTime;
        burst(0.09, 'lowpass', 900, 200, 0.7, 0.55 * v); tone(150, 60, 0.12, 0.6 * v);
      },
      splash(v) { burst(0.34, 'bandpass', 1800, 500, 0.8, 0.45 * v); burst(0.5, 'highpass', 3200, 0, 0.5, 0.07 * v, 0.04); },
      chime() {
        const notes = [523.25, 587.33, 659.25, 783.99, 880];
        const f = notes[(Math.random() * notes.length) | 0];
        bell(f, 0.22, 0, 1.1); bell(f * 1.5, 0.16, 0.1, 1.3);
      },
      rare() {
        [659.25, 783.99, 987.77, 1318.5, 1567.98, 1975.5].forEach((f, i) => bell(f, 0.2, i * 0.085, 1.6));
        burst(1.2, 'highpass', 6000, 0, 0.5, 0.05, 0.1);
      },
      whoosh() { burst(1.1, 'bandpass', 2600, 300, 1.2, 0.3); },
      boom() { tone(95, 34, 0.55, 0.9); burst(0.45, 'lowpass', 520, 80, 0.7, 0.8); },
      rumble(level, speed) {
        if (!rg) return;
        const t = ac.currentTime;
        rg.gain.setTargetAtTime(save.sound ? level : 0, t, 0.06);
        rf.frequency.setTargetAtTime(110 + speed * 0.3, t, 0.08);
      }
    };
  })();
  function buzz(p) { if (save.vib && navigator.vibrate) { try { navigator.vibrate(p); } catch (e) { /* 非対応 */ } } }

  /* ================= 景色 ================= */
  // 高さは画面の高さに対する割合（下ほど大きい）。water は水面の高さ（0.84 は「水なし」）。
  const SCENES = [
    { name: '野原', tag: 'grass', base: 0.668, tilt: 0, valley: 0.008, w1: 0.011, w2: 0.005, w3: 0.002, lipL: 0.012, lipR: 0.012, water: 0.84, tide: 0, grass: 1, flower: 1, reed: 0, pebble: 0.15, sea: 0, edge: [168, 212, 122], a: [127, 184, 95], b: [72, 128, 74] },
    { name: '坂', tag: 'slope', base: 0.655, tilt: 0.13, valley: 0, w1: 0.004, w2: 0.003, w3: 0.0015, lipL: 0, lipR: 0.06, water: 0.84, tide: 0, grass: 1, flower: 0.35, reed: 0, pebble: 0.3, sea: 0, edge: [182, 214, 124], a: [136, 184, 92], b: [84, 130, 70] },
    { name: '水辺', tag: 'water', base: 0.65, tilt: 0.16, valley: 0, w1: 0.004, w2: 0.004, w3: 0.002, lipL: 0, lipR: 0.03, water: 0.67, tide: 0.0012, grass: 0.7, flower: 0.1, reed: 1, pebble: 0.6, sea: 0, edge: [176, 206, 128], a: [132, 174, 100], b: [84, 124, 78] },
    { name: '谷', tag: 'valley', base: 0.645, tilt: 0, valley: 0.04, w1: 0.003, w2: 0.004, w3: 0.002, lipL: 0, lipR: 0, water: 0.84, tide: 0, grass: 0.35, flower: 0, reed: 0, pebble: 1, sea: 0, edge: [204, 186, 156], a: [170, 150, 122], b: [112, 96, 78] },
    { name: '小川', tag: 'water', base: 0.648, tilt: 0, valley: 0.046, w1: 0.003, w2: 0.003, w3: 0.002, lipL: 0, lipR: 0, water: 0.664, tide: 0.001, grass: 0.8, flower: 0.3, reed: 0.8, pebble: 0.9, sea: 0, edge: [184, 210, 134], a: [140, 178, 104], b: [90, 126, 80] },
    { name: '坂', tag: 'slope', base: 0.655, tilt: -0.13, valley: 0, w1: 0.004, w2: 0.003, w3: 0.0015, lipL: 0.06, lipR: 0, water: 0.84, tide: 0, grass: 1, flower: 0.5, reed: 0, pebble: 0.3, sea: 0, edge: [182, 214, 124], a: [136, 184, 92], b: [84, 130, 70] },
    { name: '浜辺', tag: 'beach', base: 0.655, tilt: -0.13, valley: 0, w1: 0.003, w2: 0.002, w3: 0.001, lipL: 0.04, lipR: 0, water: 0.675, tide: 0.009, grass: 0, flower: 0, reed: 0, pebble: 0.7, sea: 1, edge: [246, 232, 196], a: [232, 210, 164], b: [196, 166, 118] }
  ];
  const NUM = ['base', 'tilt', 'valley', 'w1', 'w2', 'w3', 'lipL', 'lipR', 'water', 'tide', 'grass', 'flower', 'reed', 'pebble', 'sea'];
  const COL = ['edge', 'a', 'b'];
  const SKY = [
    { p: 0.00, top: [112, 134, 190], bot: [250, 206, 172], night: 0.18, warm: 0.55 },
    { p: 0.09, top: [104, 174, 230], bot: [222, 240, 252], night: 0, warm: 0 },
    { p: 0.47, top: [100, 166, 226], bot: [228, 238, 244], night: 0, warm: 0.06 },
    { p: 0.60, top: [86, 90, 152], bot: [248, 172, 118], night: 0.28, warm: 0.9 },
    { p: 0.70, top: [20, 26, 62], bot: [48, 62, 112], night: 1, warm: 0 },
    { p: 0.92, top: [20, 26, 62], bot: [52, 66, 116], night: 1, warm: 0 },
    { p: 1.00, top: [112, 134, 190], bot: [250, 206, 172], night: 0.18, warm: 0.55 }
  ];

  const app = $('app'), cv = $('scene'), mainCtx = cv.getContext('2d');
  let ctx = mainCtx;
  const cmp = document.createElement('canvas'), cctx = cmp.getContext('2d');
  // 空・山・地面はほとんど動かないので、別のキャンバスにためておいて使い回す（電池の節約）
  const bg = document.createElement('canvas'), bgCtx = bg.getContext('2d');
  let bgAt = -1;
  let W = 0, H = 0, DPR = 1;
  const world = {
    p: {}, g: new Float32Array(4), n: 0, gmin: 0, gmax: 0, waterY: 9999, scene: SCENES[0], tod: '昼',
    sky: { ph: 0, top: [0, 0, 0], bot: [0, 0, 0], night: 0, warm: 0 }
  };

  // かざり（草・花・あし・小石・雲・星）の配置は固定の乱数で決める
  const deco = (function () {
    const r = S.rng(20261112), d = { tufts: [], flowers: [], reeds: [], pebbles: [], soil: [], clouds: [], stars: [] };
    for (let i = 0; i < 56; i++) d.tufts.push({ u: r(), h: 7 + r() * 10, ph: r() * TAU, front: r() < 0.3, lean: (r() - 0.5) * 0.6 });
    const fc = [[255, 248, 232], [255, 211, 94], [255, 179, 199], [255, 255, 255]];
    for (let i = 0; i < 16; i++) d.flowers.push({ u: 0.03 + r() * 0.94, h: 9 + r() * 9, c: fc[(r() * 4) | 0], ph: r() * TAU });
    for (let i = 0; i < 34; i++) d.reeds.push({ u: r(), h: 24 + r() * 30, ph: r() * TAU, head: r() < 0.6 });
    for (let i = 0; i < 26; i++) { const v = 120 + r() * 80; d.pebbles.push({ u: r(), rx: 2.5 + r() * 5, ry: 1.8 + r() * 2.4, c: [v, v - 4, v - 12] }); }
    for (let i = 0; i < 48; i++) d.soil.push({ u: r(), d: 16 + r() * r() * 260, rx: 2 + r() * 7, ry: 1.5 + r() * 3 });
    for (let i = 0; i < 4; i++) d.clouds.push({ u: r(), v: 0.07 + r() * 0.24, s: 0.7 + r() * 0.7, sp: 2 + r() * 3.5 });
    for (let i = 0; i < 52; i++) d.stars.push({ u: r(), v: r() * 0.44, s: 1 + r() * 1.4, ph: r() * TAU, a: 0.5 + r() * 0.5 });
    return d;
  })();

  const lip = (u) => { const x = clamp(u / 0.2, 0, 1); return 1 - x * x * (3 - 2 * x); };

  function computeWorld() {
    const c = save.clock;
    const si = Math.floor(c / SCENE_LEN) % SCENES.length;
    const k = smooth(((c % SCENE_LEN) - SCENE_HOLD) / (SCENE_LEN - SCENE_HOLD));
    const A = SCENES[si], B = SCENES[(si + 1) % SCENES.length], p = world.p;
    for (let i = 0; i < NUM.length; i++) p[NUM[i]] = lerp(A[NUM[i]], B[NUM[i]], k);
    for (let i = 0; i < COL.length; i++) p[COL[i]] = mix(A[COL[i]], B[COL[i]], k);
    world.scene = k < 0.5 ? A : B;

    const ph = ((c + DAY_LEN * 0.12) / DAY_LEN) % 1;
    let j = 0;
    while (j < SKY.length - 2 && ph >= SKY[j + 1].p) j++;
    const K0 = SKY[j], K1 = SKY[j + 1], t = smooth((ph - K0.p) / (K1.p - K0.p)), sk = world.sky;
    sk.ph = ph; sk.top = mix(K0.top, K1.top, t); sk.bot = mix(K0.bot, K1.bot, t);
    sk.night = lerp(K0.night, K1.night, t); sk.warm = lerp(K0.warm, K1.warm, t);
    world.tod = ph < 0.07 || ph >= 0.95 ? '朝' : ph < 0.52 ? '昼' : ph < 0.67 ? '夕方' : '夜';

    const g = world.g, n = world.n;
    let gmin = 1e9, gmax = -1e9;
    for (let i = 0; i < n; i++) {
      const u = clamp(i * GSTEP / W, 0, 1);
      let y = p.base + p.tilt * (u - 0.5) + p.valley * Math.cos(TAU * (u - 0.5)) +
        p.w1 * Math.sin(TAU * 1.5 * u + 0.7) + p.w2 * Math.sin(TAU * 2.6 * u + 2.1) + p.w3 * Math.sin(TAU * 4.3 * u + 0.3);
      y -= p.lipL * lip(u) + p.lipR * lip(1 - u);
      y *= H;
      g[i] = y;
      if (y < gmin) gmin = y;
      if (y > gmax) gmax = y;
    }
    world.gmin = gmin; world.gmax = gmax;
    world.waterY = (p.water + p.tide * Math.sin(c * TAU / 7.5)) * H;
  }
  function groundY(x) {
    const f = clamp(x / GSTEP, 0, world.n - 1.001), i = f | 0, t = f - i;
    return world.g[i] * (1 - t) + world.g[i + 1] * t;
  }
  function groundSlope(x) {
    const g = world.g, f = clamp(x / GSTEP, 1, world.n - 2.001), i = f | 0, t = f - i;
    const s0 = (g[i + 1] - g[i - 1]) / (2 * GSTEP), s1 = (g[i + 2] - g[i]) / (2 * GSTEP);
    return s0 * (1 - t) + s1 * t;
  }
  function amb(c) {
    const k = world.sky;
    let r = c;
    if (k.warm > 0.01) r = mix(r, [255, 168, 112], k.warm * 0.2);
    if (k.night > 0.01) r = mix(r, [16, 24, 62], k.night * 0.58);
    return r;
  }

  function resize() {
    const r = app.getBoundingClientRect();
    const nw = Math.max(240, Math.round(r.width)), nh = Math.max(320, Math.round(r.height));
    const ndpr = Math.min(window.devicePixelRatio || 1, 2);
    if (nw === W && nh === H && ndpr === DPR) return;
    if (stone && W) { stone.x *= nw / W; stone.y *= nh / H; stone.step = -1; }
    W = nw; H = nh; DPR = ndpr;
    cv.width = Math.round(W * DPR); cv.height = Math.round(H * DPR);
    bg.width = cv.width; bg.height = cv.height; bgAt = -1;
    world.n = Math.ceil(W / GSTEP) + 3;
    world.g = new Float32Array(world.n);
    computeWorld();
  }

  /* ================= 石 ================= */
  let stone = null, spawnWait = 0;
  const ptr = { down: false, id: -1, x: 0, y: 0, held: false };

  function pickKind() {
    const night = world.sky.night > 0.6;
    const p = { gold: RATE.gold, meteor: night ? RATE.meteorNight : RATE.meteorDay, fossil: RATE.fossil, gem: RATE.gem };
    const tot = p.gold + p.meteor + p.fossil + p.gem;
    let x = Math.random(), cat = 'common';
    if (save.dry >= RATE.pity) x *= tot; // しばらく出ていなければ、次はかならずレア
    if (x < p.gold) cat = 'gold';
    else if (x < p.gold + p.meteor) cat = 'meteor';
    else if (x < p.gold + p.meteor + p.fossil) cat = 'fossil';
    else if (x < tot) cat = 'gem';
    save.dry = cat === 'common' ? save.dry + 1 : 0;
    const list = S.KINDS.filter((k) => k.cat === cat), tag = world.scene.tag;
    const ws = list.map((k) => (k.w || 1) * (k.likes && k.likes.indexOf(tag) >= 0 ? 3 : 1));
    let y = Math.random() * ws.reduce((a, b) => a + b, 0);
    for (let i = 0; i < list.length; i++) { y -= ws[i]; if (y <= 0) return list[i].id; }
    return list[list.length - 1].id;
  }

  function spawn(cur, force) {
    const kind = cur ? cur.k : (force || pickKind()), def = S.get(kind);
    const seed = cur ? cur.s >>> 0 : (Math.random() * 4294967296) >>> 0;
    const need = cur ? cur.n : Math.round(NEED_MIN + Math.random() * NEED_VAR);
    const r = clamp(W * 0.094, 30, 46) * S.sizeOf(kind, seed);
    const s = stone = {
      kind: kind, def: def, seed: seed, need: need, prog: cur ? clamp(+cur.p || 0, 0, need - 1) : 0, r: r,
      x: W * rand(0.3, 0.7), y: 0, vx: rand(-40, 40), vy: 120, ang: rand(0, TAU), om: rand(-3, 3),
      state: 'play', t: 0, grounded: false, sub: 0, vt: 0, hd: 0, hot: 0, falling: false, spk: 0,
      step: -1, sprite: document.createElement('canvas'), size: 0, shape: null, gloss: 0.2, reveal: 0
    };
    s.y = groundY(s.x) - H * 0.36;
    if (def.cat === 'meteor' && !cur) {
      s.falling = true; s.x = W * rand(0.72, 0.95); s.y = -r * 2; s.vx = -rand(160, 300); s.vy = 760;
      Snd.whoosh();
    }
    save.cur = { k: kind, s: seed, n: need, p: s.prog };
    refreshSprite(s);
    persist();
  }

  // ころがした分だけ角が取れ、つやが出る。レアは後半から少しずつ正体をあらわす。
  function refreshSprite(s) {
    const p = clamp(s.prog / s.need, 0, 1);
    const step = s.state === 'play' ? Math.floor(p * 24) : 25;
    if (step === s.step) return;
    s.step = step;
    const polish = s.state === 'play' ? step / 24 : 1;
    const reveal = s.def.covers ? (s.state === 'play' ? smooth((polish - 0.5) / 0.45) * 0.92 : 1) : 1;
    const rp = s.r * DPR, size = Math.ceil(rp * 3) + 4;
    s.sprite.width = s.sprite.height = size;
    const c = s.sprite.getContext('2d');
    c.translate(size / 2, size / 2);
    s.shape = S.draw(c, s.kind, s.seed, { r: rp, polish: polish, reveal: reveal });
    s.gloss = S.gloss(s.kind, s.seed, polish, reveal);
    s.reveal = s.def.covers ? reveal : 0;
    s.size = size;
  }

  function land(s, imp, cx, cy) {
    const v = clamp((imp - 70) / 800, 0, 1);
    if (v > 0.03) Snd.thud(v);
    if (imp > 170 && s.sub < 0.3) {
      const n = 2 + ((v * 8) | 0), col = amb(mix(world.p.edge, [255, 250, 235], 0.45));
      for (let i = 0; i < n; i++) emit({ t: 'dust', x: cx + rand(-8, 8), y: cy, vx: rand(-90, 90) * (0.4 + v), vy: -rand(10, 70), g: 60, life: rand(0.35, 0.8), size: rand(3, 6), grow: 14, c: col });
    }
  }
  function splash(x, y, sp) {
    const v = clamp(sp / 1400, 0.1, 1);
    Snd.splash(v);
    const n = 6 + ((v * 16) | 0);
    for (let i = 0; i < n; i++) emit({ t: 'drop', x: x + rand(-12, 12), y: y, vx: rand(-200, 200) * v, vy: -rand(120, 120 + 440 * v), g: 1300, life: rand(0.5, 0.9), size: rand(1.5, 3.6) });
    ripple(x, 1); ripple(x + rand(-10, 10), 0.6);
  }
  function ripple(x, k) { emit({ t: 'ripple', x: x, y: 0, vx: 0, vy: 0, life: 1.1 * k + 0.3, size: 6, grow: 46 * k + 12 }); }

  function stepStone(dt) {
    const s = stone;
    if (!s || s.state === 'fly') return;
    let ax = 0, ay = GRAV;

    // 水：しずむ石はゆっくり、かるい石は浮く
    let sub = 0;
    if (world.waterY < groundY(s.x) + 2) sub = clamp((s.y + s.r - world.waterY) / (2 * s.r), 0, 1);
    if (sub > 0) {
      ay -= GRAV * (s.def.float ? 1.3 : 0.45) * sub;
      const dr = Math.exp(-2.4 * sub * dt);
      s.vx *= dr; s.vy *= dr;
      if (s.sub === 0) { const sp = Math.hypot(s.vx, s.vy); if (sp > 140) splash(s.x, world.waterY, sp); }
    }
    s.sub = sub;

    // 指：近ければつかむ。遠ければ、石が寄ってくる
    if (ptr.down && s.state === 'play' && !s.falling) {
      const dx = ptr.x - s.x, dy = ptr.y - s.y, grab = s.r + 46;
      if (!ptr.held && dx * dx + dy * dy < grab * grab) ptr.held = true;
      if (ptr.held) { ax += 600 * dx - 42 * s.vx; ay += 600 * dy - 42 * s.vy; }
      else ax += clamp(dx * 12, -1900, 1900) - 2.5 * s.vx;
    }

    s.vx += ax * dt; s.vy += ay * dt;
    const sp = Math.hypot(s.vx, s.vy);
    if (sp > VMAX) { s.vx *= VMAX / sp; s.vy *= VMAX / sp; }
    s.x += s.vx * dt; s.y += s.vy * dt;

    // 画面のはし
    const m = s.r;
    if (s.x < m) { s.x = m; if (s.vx < 0) { if (s.vx < -240) Snd.thud(clamp(-s.vx / 1400, 0, 0.7)); s.vx *= -0.45; } }
    else if (s.x > W - m) { s.x = W - m; if (s.vx > 0) { if (s.vx > 240) Snd.thud(clamp(s.vx / 1400, 0, 0.7)); s.vx *= -0.45; } }
    if (s.y < -H) { s.y = -H; if (s.vy < 0) s.vy = 0; }

    // 地面：石の形（支持関数）で接地するので、いびつな石ほど「ことこと」転がる
    const gy = groundY(s.x), sl = groundSlope(s.x), inv = 1 / Math.hypot(sl, 1);
    const nx = sl * inv, ny = -inv;
    const phi = Math.atan2(-ny, -nx) - s.ang;
    const h = s.r * S.sample(s.shape.H, phi);
    const d = (gy - s.y) * inv;
    if (d < h + 0.6) {
      if (d < h) { const pen = h - d; s.x += nx * pen; s.y += ny * pen; }
      let vn = s.vx * nx + s.vy * ny;
      const tx = -ny, ty = nx;
      let vt = s.vx * tx + s.vy * ty;
      if (vn < 0) {
        const imp = -vn;
        if (s.falling) meteorHit(s, imp);
        if (imp > 70) { vn = imp * 0.26; land(s, imp, s.x - nx * h, s.y - ny * h); } else vn = 0;
      }
      const dh = S.sample(s.shape.dH, phi);
      vt += 0.7 * GRAV * dh * dt;
      vt *= Math.exp(-(0.42 + sub * 1.2) * dt);
      const dec = 55 * dt;
      vt = Math.abs(vt) <= dec ? 0 : vt - Math.sign(vt) * dec;
      const hd = -dh * s.om;
      if (s.hd < 0 && hd >= 0) { // 平らな面がぺたんと着く：かどが鋭いほど勢いが逃げ、ことんと鳴る（速いときは跳ねて逃げにくい）
        const kk = S.sample(s.shape.K, phi);
        vt *= 1 - clamp(0.3 * kk, 0.12, 0.25) * (1 - 0.7 * smooth((Math.abs(vt) - 120) / 500));
        if (Math.abs(s.om) > 1.2) Snd.tick(clamp(Math.abs(s.om) / 16, 0.08, 1) * clamp(kk * 2.2, 0.2, 1), s.r);
      }
      s.hd = hd;
      s.vx = tx * vt + nx * vn; s.vy = ty * vt + ny * vn;
      s.om += (vt / s.r - s.om) * Math.min(1, 40 * dt);
      s.grounded = true; s.vt = vt;
      if (s.state === 'play') { const dd = Math.abs(vt) * dt; s.prog += dd; save.dist += dd; if (dd > 0) dirty = true; }
    } else {
      s.grounded = false; s.vt = 0; s.hd = 0;
      // はねて飛んでいるあいだも、進んだ分は「ころがした距離」に数える（つかんで運ぶ分は数えない）
      if (s.state === 'play' && !ptr.held) { const dd = Math.abs(s.vx) * dt; s.prog += dd; save.dist += dd; if (dd > 0) dirty = true; }
    }
    s.ang += s.om * dt;

    if (!isFinite(s.x + s.y + s.vx + s.vy + s.ang)) { s.x = W / 2; s.y = groundY(W / 2) - s.r * 2; s.vx = s.vy = s.om = 0; s.ang = 0; }
  }

  function meteorHit(s, imp) {
    s.falling = false; s.hot = 1;
    fx.shake = 1; fx.flash = 0.5;
    Snd.boom(); buzz([30, 40, 70]);
    const col = amb(mix(world.p.a, [255, 240, 210], 0.4));
    for (let i = 0; i < 26; i++) emit({ t: 'dust', x: s.x + rand(-20, 20), y: s.y + s.r, vx: rand(-260, 260), vy: -rand(40, 320), g: 420, life: rand(0.5, 1.2), size: rand(4, 9), grow: 16, c: col });
    for (let i = 0; i < 12; i++) emit({ t: 'spark', x: s.x, y: s.y, vx: rand(-320, 320), vy: -rand(80, 420), g: 700, life: rand(0.4, 0.9), size: rand(4, 9), c: [255, 190, 110] });
    if (imp > 900) s.vy = Math.min(s.vy, 0);
  }

  // 仕上がり：棚に記録して、石は棚ボタンへ飛んでいく
  function complete() {
    const s = stone, rare = s.def.cat !== 'common', isNew = !save.col[s.kind];
    s.state = 'done'; s.t = 0; ptr.held = false;
    const e = save.col[s.kind] || (save.col[s.kind] = { n: 0, first: Date.now() });
    e.n++; save.total++; save.cur = null;
    persist();
    refreshSprite(s);
    if (s.grounded) s.vy = -300;
    const col = rare ? hex(s.def.glow) : [255, 255, 255];
    emit({ t: 'ring', x: s.x, y: s.y, vx: 0, vy: 0, life: rare ? 0.75 : 0.55, size: s.r, grow: rare ? 260 : 130, c: col });
    const n = rare ? 32 : 8;
    for (let i = 0; i < n; i++) {
      const a = rand(0, TAU), v = rand(60, rare ? 440 : 190);
      emit({ t: 'spark', x: s.x + Math.cos(a) * s.r * 0.6, y: s.y + Math.sin(a) * s.r * 0.6, vx: Math.cos(a) * v, vy: Math.sin(a) * v - 70, g: 220, life: rand(0.5, rare ? 1.4 : 0.8), size: rand(5, rare ? 14 : 8), c: col });
    }
    if (s.def.covers) {
      for (let i = 0; i < 16; i++) {
        const a = rand(0, TAU), v = rand(120, 380);
        emit({ t: 'shard', x: s.x + Math.cos(a) * s.r * 0.7, y: s.y + Math.sin(a) * s.r * 0.7, vx: Math.cos(a) * v, vy: Math.sin(a) * v - 120, g: 1300, life: rand(0.5, 1), size: rand(3, 7), rot: rand(0, TAU), spin: rand(-12, 12), c: [150, 144, 134] });
      }
    }
    if (rare) { fx.flash = 0.42; Snd.rare(); buzz([20, 50, 20, 50, 70]); } else { Snd.chime(); buzz(12); }
    setTimeout(() => showToast(s.kind, isNew), rare ? 320 : 120);
  }

  function updateFlow(dt) {
    const s = stone;
    if (!s) { spawnWait -= dt; if (spawnWait <= 0) spawn(null); return; }
    if (s.state === 'play') { refreshSprite(s); if (s.prog >= s.need) complete(); }
    else if (s.state === 'done') {
      s.t += dt;
      if (s.t > 0.9) {
        const a = app.getBoundingClientRect(), b = $('btn-shelf').getBoundingClientRect();
        s.state = 'fly'; s.t = 0; s.fx = s.x; s.fy = s.y;
        s.tx = b.left - a.left + b.width / 2; s.ty = b.top - a.top + b.height / 2;
      }
    } else if (s.state === 'fly') {
      s.t += dt; s.ang += 9 * dt;
      if (s.t >= 0.62) {
        stone = null; spawnWait = 0.45;
        const btn = $('btn-shelf');
        btn.classList.remove('bump'); void btn.offsetWidth; btn.classList.add('bump');
      }
    }
  }

  /* ================= 小さな演出 ================= */
  const parts = [], fx = { shake: 0, flash: 0 };
  function emit(o) { if (parts.length > 240) parts.shift(); o.max = o.life; parts.push(o); }

  function updateFx(dt) {
    for (let i = parts.length - 1; i >= 0; i--) {
      const p = parts[i];
      p.life -= dt;
      if (p.life <= 0) { parts.splice(i, 1); continue; }
      if (p.g) p.vy += p.g * dt;
      p.x += p.vx * dt; p.y += p.vy * dt;
      if (p.spin) p.rot += p.spin * dt;
      if (p.grow) p.size += p.grow * dt;
      if (p.t === 'drop' && p.vy > 0 && p.y > world.waterY && groundY(p.x) > world.waterY) parts.splice(i, 1);
    }
    fx.shake = Math.max(0, fx.shake - dt * 2.6);
    fx.flash = Math.max(0, fx.flash - dt * 2.2);

    const s = stone;
    if (!s || s.state === 'fly') { Snd.rumble(0, 0); return; }
    Snd.rumble(s.grounded ? clamp(Math.abs(s.vt) / 900, 0, 1) * 0.22 : 0, Math.abs(s.vt));
    if (s.hot > 0) s.hot = Math.max(0, s.hot - dt / 9);
    if (s.falling) {
      for (let i = 0; i < 3; i++) emit({ t: 'fire', x: s.x + rand(-9, 9), y: s.y + rand(-9, 9), vx: -s.vx * 0.12 + rand(-30, 30), vy: -s.vy * 0.12 + rand(-30, 30), life: rand(0.25, 0.6), size: rand(9, 19) });
    }
    if (s.reveal > 0.04 && s.state === 'play') {
      s.spk += dt * s.reveal * (s.def.cat === 'fossil' ? 3 : 9);
      while (s.spk > 1) {
        s.spk -= 1;
        const a = rand(0, TAU), d = s.r * rand(0.3, 1.25);
        emit({ t: 'spark', x: s.x + Math.cos(a) * d, y: s.y + Math.sin(a) * d, vx: rand(-14, 14), vy: -rand(6, 30), life: rand(0.45, 0.95), size: rand(4, 9), c: hex(s.def.glow) });
      }
    }
    if (s.grounded && s.sub < 0.1 && Math.abs(s.vt) > 520 && Math.random() < dt * 14) {
      emit({ t: 'dust', x: s.x, y: s.y + s.r * 0.9, vx: -s.vt * 0.08 + rand(-20, 20), vy: -rand(10, 50), g: 40, life: rand(0.3, 0.6), size: rand(2.5, 5), grow: 12, c: amb(mix(world.p.edge, [255, 250, 235], 0.45)) });
    }
    if (s.grounded && s.sub > 0.12 && Math.abs(s.vt) > 70 && Math.random() < dt * 5) ripple(s.x + rand(-s.r, s.r), 0.5);
  }

  /* ================= 描画 ================= */
  function ell(x, y, rx, ry) { ctx.moveTo(x + rx, y); ctx.ellipse(x, y, rx, ry, 0, 0, TAU); }

  function drawSky(t) {
    const k = world.sky;
    let g = ctx.createLinearGradient(0, 0, 0, H * 0.62);
    g.addColorStop(0, css(k.top)); g.addColorStop(1, css(k.bot));
    ctx.fillStyle = g; ctx.fillRect(0, 0, W, H);

    if (k.night > 0.25) {
      const a = (k.night - 0.25) / 0.75;
      ctx.fillStyle = '#fff';
      for (const s of deco.stars) {
        const tw = Math.sin(t * 1.4 + s.ph);
        ctx.globalAlpha = a * s.a * (0.45 + 0.55 * tw * tw);
        ctx.fillRect(s.u * W, s.v * H, s.s, s.s);
      }
      ctx.globalAlpha = 1;
    }
    const sunT = k.ph / 0.66;
    if (sunT <= 1) {
      const x = W * (0.06 + 0.88 * sunT), y = H * (0.5 - 0.36 * Math.sin(Math.PI * sunT));
      const col = mix([255, 248, 220], [255, 186, 116], smooth(Math.abs(sunT - 0.5) * 2.6 - 0.5));
      g = ctx.createRadialGradient(x, y, 0, x, y, 78);
      g.addColorStop(0, css(col, 0.9)); g.addColorStop(0.22, css(col, 0.42)); g.addColorStop(1, css(col, 0));
      ctx.fillStyle = g; ctx.beginPath(); ctx.arc(x, y, 78, 0, TAU); ctx.fill();
      ctx.fillStyle = css(mix(col, [255, 255, 255], 0.5)); ctx.beginPath(); ctx.arc(x, y, 16, 0, TAU); ctx.fill();
    }
    const moonT = (k.ph - 0.66) / 0.32;
    if (moonT >= 0 && moonT <= 1) {
      const x = W * (0.08 + 0.84 * moonT), y = H * (0.47 - 0.31 * Math.sin(Math.PI * moonT));
      g = ctx.createRadialGradient(x, y, 0, x, y, 56);
      g.addColorStop(0, 'rgba(230,236,255,0.5)'); g.addColorStop(0.3, 'rgba(230,236,255,0.16)'); g.addColorStop(1, 'rgba(230,236,255,0)');
      ctx.fillStyle = g; ctx.beginPath(); ctx.arc(x, y, 56, 0, TAU); ctx.fill();
      ctx.fillStyle = '#f4f0dc'; ctx.beginPath(); ctx.arc(x, y, 13, 0, TAU); ctx.fill();
      ctx.fillStyle = 'rgba(190,186,170,0.5)'; ctx.beginPath(); ell(x - 4, y - 3, 3.2, 2.8); ell(x + 4, y + 4, 2.2, 2); ell(x + 5, y - 5, 1.5, 1.4); ctx.fill();
    }

    ctx.fillStyle = css(amb([255, 255, 255]), lerp(0.8, 0.2, k.night));
    for (const c of deco.clouds) {
      const span = W + 260, x = ((c.u * span + t * c.sp) % span) - 130, y = c.v * H, s = c.s;
      ctx.beginPath();
      ell(x, y, 34 * s, 13 * s); ell(x - 22 * s, y + 5 * s, 22 * s, 10 * s); ell(x + 24 * s, y + 4 * s, 26 * s, 11 * s); ell(x + 4 * s, y - 8 * s, 20 * s, 12 * s);
      ctx.fill();
    }
  }

  function drawFar(t) {
    const k = world.sky, p = world.p;
    ctx.fillStyle = css(amb(mix([138, 168, 200], k.bot, 0.38)));
    ctx.beginPath(); ctx.moveTo(-10, H);
    for (let x = -10; x <= W + 16; x += 8) {
      const u = x / W;
      ctx.lineTo(x, H * (0.455 - 0.075 * (0.5 + 0.5 * Math.sin(u * 5.1 + 1.3)) - 0.03 * Math.sin(u * 11.7 + 0.4) - 0.01 * Math.sin(u * 23 + 2)));
    }
    ctx.lineTo(W + 16, H); ctx.fill();
    const na = 1 - p.sea * 0.85;
    if (na > 0.02) {
      ctx.fillStyle = css(amb(mix([110, 156, 126], k.bot, 0.2)), na);
      ctx.beginPath(); ctx.moveTo(-10, H);
      for (let x = -10; x <= W + 16; x += 8) {
        const u = x / W;
        ctx.lineTo(x, H * (0.545 - 0.04 * (0.5 + 0.5 * Math.sin(u * 3.7 + 4.2)) - 0.014 * Math.sin(u * 8.9 + 1)));
      }
      ctx.lineTo(W + 16, H); ctx.fill();
    }
    if (p.sea > 0.02) {
      const hy = H * 0.548, g = ctx.createLinearGradient(0, hy, 0, H * 0.7);
      g.addColorStop(0, css(amb([92, 156, 198]), p.sea)); g.addColorStop(1, css(amb([140, 204, 214]), p.sea));
      ctx.fillStyle = g; ctx.fillRect(-10, hy, W + 20, H - hy);
      ctx.strokeStyle = css([255, 255, 255], 0.38 * p.sea * (1 - k.night * 0.6)); ctx.lineWidth = 1.2; ctx.lineCap = 'round';
      ctx.beginPath();
      for (let i = 0; i < 10; i++) {
        const y = hy + 5 + i * 5 + i * i * 0.55, x = ((i * 137.5 + t * (3 + (i % 3))) % (W + 60)) - 30, w = 10 + (i % 4) * 7;
        ctx.moveTo(x, y); ctx.lineTo(x + w, y);
      }
      ctx.stroke();
    }
  }

  function drawGround() {
    const p = world.p, g = world.g, n = world.n, xe = (n - 1) * GSTEP + 24;
    const gr = ctx.createLinearGradient(0, world.gmin, 0, H);
    gr.addColorStop(0, css(amb(p.a))); gr.addColorStop(1, css(amb(p.b)));
    ctx.fillStyle = gr;
    ctx.beginPath(); ctx.moveTo(-24, H + 24); ctx.lineTo(-24, g[0]);
    for (let i = 0; i < n; i++) ctx.lineTo(i * GSTEP, g[i]);
    ctx.lineTo(xe, g[n - 1]); ctx.lineTo(xe, H + 24); ctx.closePath(); ctx.fill();

    ctx.save(); ctx.translate(0, 3.5);
    ctx.lineJoin = 'round'; ctx.lineCap = 'round'; ctx.lineWidth = 7; ctx.strokeStyle = css(amb(p.edge));
    ctx.beginPath(); ctx.moveTo(-24, g[0]);
    for (let i = 0; i < n; i++) ctx.lineTo(i * GSTEP, g[i]);
    ctx.lineTo(xe, g[n - 1]); ctx.stroke();
    ctx.restore();

    ctx.fillStyle = css(amb(mix(p.b, [30, 24, 14], 0.55)), 0.13);
    ctx.beginPath();
    for (const s of deco.soil) { const x = s.u * W, y = groundY(x) + s.d; if (y < H + 8) ell(x, y, s.rx, s.ry); }
    ctx.fill();

    if (p.pebble > 0.03) {
      for (const b of deco.pebbles) {
        const x = b.u * W, y = groundY(x);
        ctx.fillStyle = css(amb(b.c), p.pebble);
        ctx.beginPath(); ell(x, y + b.ry * 0.25, b.rx, b.ry); ctx.fill();
      }
    }
  }

  function drawGrass(t, front) {
    const a = world.p.grass;
    if (a < 0.03) return;
    ctx.lineCap = 'round'; ctx.lineWidth = 1.7;
    ctx.strokeStyle = css(amb(mix(world.p.edge, [52, 110, 52], front ? 0.5 : 0.32)), a);
    ctx.beginPath();
    for (const g of deco.tufts) {
      if (g.front !== front) continue;
      const x = g.u * W, y = groundY(x);
      if (y > world.waterY + 2) continue;
      const sway = Math.sin(t * 1.1 + g.ph) * 2.2, h = g.h * (0.5 + 0.5 * a), y0 = y + (front ? 6 : 2);
      for (let k = -1; k <= 1; k++) {
        const bx = x + k * 2.6;
        ctx.moveTo(bx, y0);
        ctx.quadraticCurveTo(bx + k * 1.2, y0 - h * 0.6, bx + k * 3.5 + sway + g.lean * 4, y0 - h * (k ? 0.75 : 1));
      }
    }
    ctx.stroke();
  }
  function drawFlowers(t) {
    const a = world.p.flower;
    if (a < 0.03) return;
    ctx.lineWidth = 1.2; ctx.strokeStyle = css(amb([96, 150, 80]), a);
    ctx.beginPath();
    for (const f of deco.flowers) {
      const x = f.u * W, y = groundY(x);
      if (y > world.waterY + 2) continue;
      f.x = x + Math.sin(t * 1.3 + f.ph) * 1.8; f.y = y - f.h;
      ctx.moveTo(x, y + 2); ctx.quadraticCurveTo(x, y - f.h * 0.5, f.x, f.y);
    }
    ctx.stroke();
    for (const f of deco.flowers) {
      if (groundY(f.u * W) > world.waterY + 2) continue;
      ctx.fillStyle = css(amb(f.c), a);
      ctx.beginPath(); ctx.arc(f.x, f.y, 2.6, 0, TAU); ctx.fill();
      ctx.fillStyle = css(amb([240, 170, 60]), a);
      ctx.beginPath(); ctx.arc(f.x, f.y, 1, 0, TAU); ctx.fill();
    }
  }
  function drawReeds(t) {
    const a = world.p.reed;
    if (a < 0.03 || world.waterY > world.gmax + 6) return;
    const wy = world.waterY;
    ctx.lineCap = 'round'; ctx.lineWidth = 2; ctx.strokeStyle = css(amb([104, 138, 78]), a);
    const heads = [];
    ctx.beginPath();
    for (const r of deco.reeds) {
      const x = r.u * W, y = groundY(x), d = y - wy;
      if (d < -14 || d > 13) continue;
      const sway = Math.sin(t * 0.9 + r.ph) * 3.5, h = r.h * (0.6 + 0.4 * a);
      ctx.moveTo(x, y + 2); ctx.quadraticCurveTo(x + sway * 0.3, y - h * 0.5, x + sway, y - h);
      if (r.head) heads.push(x + sway, y - h);
    }
    ctx.stroke();
    ctx.fillStyle = css(amb([122, 84, 52]), a);
    ctx.beginPath();
    for (let i = 0; i < heads.length; i += 2) ell(heads[i], heads[i + 1] + 3, 2.3, 6.5);
    ctx.fill();
  }

  function drawWater(t) {
    const wy = world.waterY;
    if (wy > world.gmax + 2) return;
    const g = world.g, n = world.n, k = world.sky;
    const top = (i) => wy + Math.sin(i * 0.35 + t * 1.6) * 1.3 + Math.sin(i * 0.13 - t * 0.9) * 0.9;
    const gr = ctx.createLinearGradient(0, wy, 0, wy + 44);
    gr.addColorStop(0, css(amb([126, 200, 228]), 0.56)); gr.addColorStop(1, css(amb([48, 120, 176]), 0.74));
    const line = css(amb([255, 255, 255]), 0.6), foam = css(amb([255, 255, 255]), 0.55);
    let a = -1;
    for (let i = 0; i <= n; i++) {
      const under = i < n && g[i] > wy + 0.5;
      if (under && a < 0) a = i;
      else if (!under && a >= 0) {
        const b = i - 1;
        ctx.fillStyle = gr; ctx.beginPath();
        ctx.moveTo((a - 1) * GSTEP, top(a - 1));
        for (let j = a; j <= b; j++) ctx.lineTo(j * GSTEP, top(j));
        ctx.lineTo((b + 1) * GSTEP, top(b + 1));
        ctx.lineTo((b + 1) * GSTEP, g[Math.min(b + 1, n - 1)] + 1);
        for (let j = b; j >= a; j--) ctx.lineTo(j * GSTEP, g[j] + 1);
        ctx.lineTo((a - 1) * GSTEP, g[Math.max(a - 1, 0)] + 1);
        ctx.closePath(); ctx.fill();

        ctx.strokeStyle = line; ctx.lineWidth = 1.5; ctx.beginPath();
        ctx.moveTo((a - 1) * GSTEP, top(a - 1));
        for (let j = a; j <= b + 1; j++) ctx.lineTo(j * GSTEP, top(j));
        ctx.stroke();

        ctx.fillStyle = foam; ctx.beginPath();
        if (a > 1) ell((a - 1) * GSTEP, wy + 1, 5, 2);
        if (b < n - 2) ell((b + 1) * GSTEP, wy + 1, 5, 2);
        ctx.fill();

        const len = (b - a + 1) * GSTEP;
        if (len > 50) {
          ctx.strokeStyle = css([255, 255, 255], 0.3 * (1 - k.night * 0.5)); ctx.lineWidth = 1.2; ctx.lineCap = 'round'; ctx.beginPath();
          for (let q = 0; q < 5; q++) {
            const x = a * GSTEP + ((q * 83 + t * (5 + q)) % (len - 20)) + 4, y = wy + 5 + (q % 3) * 4;
            if (groundY(x) > y + 3) { ctx.moveTo(x, y); ctx.lineTo(x + 7 + q * 2, y); }
          }
          ctx.stroke();
        }
        a = -1;
      }
    }
  }

  function drawStone(t) {
    const s = stone;
    if (!s) return;
    let x = s.x, y = s.y, scale = 1;
    if (s.state === 'fly') {
      const k = smooth(s.t / 0.62);
      x = lerp(s.fx, s.tx, k); y = lerp(s.fy, s.ty, k) - Math.sin(Math.PI * k) * 70; scale = lerp(1, 0.26, k);
    } else {
      const gy = groundY(x), hgt = clamp((gy - y - s.r) / 240, 0, 1), sw = s.r * (1.05 - 0.5 * hgt);
      ctx.fillStyle = 'rgba(20,30,24,' + (0.27 * (1 - hgt)).toFixed(3) + ')';
      ctx.save(); ctx.translate(x, gy + 1.5); ctx.rotate(Math.atan(groundSlope(x)));
      ctx.beginPath(); ctx.ellipse(0, 0, sw, sw * 0.22, 0, 0, TAU); ctx.fill(); ctx.restore();
    }
    const ga = s.def.covers ? s.reveal * (0.55 + 0.12 * Math.sin(t * 3.1)) : s.hot * 0.75;
    if (s.def.glow && ga > 0.02) {
      const col = hex(s.def.glow), R = s.r * 2.4 * scale, g = ctx.createRadialGradient(x, y, s.r * 0.5 * scale, x, y, R);
      g.addColorStop(0, css(col, ga)); g.addColorStop(1, css(col, 0));
      ctx.fillStyle = g; ctx.beginPath(); ctx.arc(x, y, R, 0, TAU); ctx.fill();
    }
    const size = s.size, k = world.sky;
    if (cmp.width !== size) cmp.width = cmp.height = size;
    cctx.setTransform(1, 0, 0, 1, 0, 0);
    cctx.globalCompositeOperation = 'source-over';
    cctx.clearRect(0, 0, size, size);
    cctx.translate(size / 2, size / 2); cctx.rotate(s.ang);
    cctx.drawImage(s.sprite, -size / 2, -size / 2);
    cctx.setTransform(1, 0, 0, 1, 0, 0);
    const tint = k.night > 0.02 ? [14, 22, 60, k.night * 0.36] : (k.warm > 0.02 ? [255, 150, 80, k.warm * 0.12] : null);
    S.light(cctx, size, s.r * DPR, s.gloss, tint, s.hot);
    const d = size / DPR * scale;
    ctx.drawImage(cmp, x - d / 2, y - d / 2, d, d);
  }

  function drawParts() {
    for (const p of parts) {
      const k = p.life / p.max;
      if (p.t === 'dust') { ctx.fillStyle = css(p.c, 0.42 * k); ctx.beginPath(); ctx.arc(p.x, p.y, p.size, 0, TAU); ctx.fill(); }
      else if (p.t === 'drop') { ctx.fillStyle = css([228, 243, 255], 0.9 * Math.min(1, k * 2)); ctx.beginPath(); ctx.arc(p.x, p.y, p.size, 0, TAU); ctx.fill(); }
      else if (p.t === 'spark') { const e = Math.sin(k * Math.PI); S.star(ctx, p.x, p.y, p.size * (0.5 + 0.5 * e), e, css(mix(p.c, [255, 255, 255], 0.5))); }
      else if (p.t === 'shard') {
        ctx.save(); ctx.translate(p.x, p.y); ctx.rotate(p.rot); ctx.fillStyle = css(p.c, Math.min(1, k * 2));
        ctx.beginPath(); ctx.moveTo(-p.size, p.size * 0.6); ctx.lineTo(p.size * 0.9, p.size * 0.4); ctx.lineTo(0, -p.size * 0.8); ctx.closePath(); ctx.fill(); ctx.restore();
      } else if (p.t === 'fire') {
        ctx.globalCompositeOperation = 'lighter';
        ctx.fillStyle = css([255, 130 + 90 * k, 50], 0.5 * k); ctx.beginPath(); ctx.arc(p.x, p.y, p.size * (0.4 + 0.6 * k), 0, TAU); ctx.fill();
        ctx.globalCompositeOperation = 'source-over';
      } else if (p.t === 'ring') { ctx.strokeStyle = css(p.c, 0.75 * k); ctx.lineWidth = 3.5 * k + 0.5; ctx.beginPath(); ctx.arc(p.x, p.y, p.size, 0, TAU); ctx.stroke(); }
      else if (p.t === 'ripple') { ctx.strokeStyle = css([255, 255, 255], 0.55 * k); ctx.lineWidth = 1.4; ctx.beginPath(); ctx.ellipse(p.x, world.waterY + 1, p.size, p.size * 0.2, 0, 0, TAU); ctx.stroke(); }
    }
  }

  function draw() {
    const t = save.clock, fly = stone && stone.state === 'fly';
    const shaking = fx.shake > 0;
    if (!shaking && (bgAt < 0 || t < bgAt || t - bgAt > 0.2)) {
      bgAt = t; ctx = bgCtx;
      ctx.setTransform(DPR, 0, 0, DPR, 0, 0);
      drawSky(t); drawFar(t); drawGround();
      ctx = mainCtx;
    }
    if (shaking) {
      ctx.setTransform(DPR, 0, 0, DPR, 0, 0);
      drawSky(t); drawFar(t);
      ctx.save();
      ctx.translate(rand(-1, 1) * fx.shake * 7, rand(-1, 1) * fx.shake * 7);
      drawGround();
    } else {
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      ctx.drawImage(bg, 0, 0);
      ctx.setTransform(DPR, 0, 0, DPR, 0, 0);
      ctx.save();
    }
    drawGrass(t, false); drawFlowers(t);
    if (!fly) drawStone(t);
    drawWater(t); drawReeds(t); drawGrass(t, true);
    drawParts();
    if (fly) drawStone(t);
    ctx.restore();
    if (fx.flash > 0) { ctx.fillStyle = css([255, 250, 235], fx.flash); ctx.fillRect(0, 0, W, H); }
  }

  /* ================= 画面の部品 ================= */
  const hud = { p: -1, d: -1, l: '', t: -1 };
  const RING = 2 * Math.PI * 32;
  function refreshHud() {
    const s = stone, p = !s ? 0 : s.state === 'play' ? clamp(s.prog / s.need, 0, 1) : 1;
    const pi = Math.round(p * 200);
    if (pi !== hud.p) { hud.p = pi; const fg = $('ring-fg'); fg.style.strokeDashoffset = (RING * (1 - p)).toFixed(1); fg.style.opacity = pi ? 1 : 0; }
    const d = Math.floor(save.dist / 100);
    if (d !== hud.d) { hud.d = d; $('hud-dist').textContent = 'のべ ' + (d / 10).toFixed(1) + ' m'; }
    const l = world.scene.name + '・' + world.tod;
    if (l !== hud.l) { hud.l = l; $('hud-scene').textContent = l; }
    if (save.total !== hud.t) { hud.t = save.total; $('shelf-count').textContent = save.total; }
  }

  let toastTimer = 0;
  function showToast(kind, isNew) {
    const def = S.get(kind), cat = S.cat(def.cat), rare = def.cat !== 'common';
    const box = $('toast');
    box.textContent = '';
    const card = document.createElement('div'), tx = document.createElement('div'), sm = document.createElement('small'), b = document.createElement('b');
    card.className = 'toast-card' + (rare ? ' rare cat-' + def.cat : '');
    card.appendChild(S.icon(kind, 168));
    sm.textContent = rare ? '✦ ' + cat.mark + (isNew ? '・はじめて！' : '') + ' ✦' : (isNew ? 'はじめての石！' : '棚にならべました');
    b.textContent = def.name;
    tx.appendChild(sm); tx.appendChild(b); card.appendChild(tx); box.appendChild(card);
    requestAnimationFrame(() => requestAnimationFrame(() => card.classList.add('show')));
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => card.classList.remove('show'), rare ? 4200 : 2600);
  }

  /* ---- 棚（図鑑） ---- */
  const iconCache = {};
  function iconURL(kind, known) {
    const key = kind + (known ? '' : '?');
    if (!iconCache[key]) iconCache[key] = S.icon(kind, 204, { silhouette: !known }).toDataURL();
    return iconCache[key];
  }
  function el(tag, cls, text) { const e = document.createElement(tag); if (cls) e.className = cls; if (text != null) e.textContent = text; return e; }
  function fmtDate(ms) { const d = new Date(ms); return d.getFullYear() + '/' + (d.getMonth() + 1) + '/' + d.getDate(); }

  function buildShelf() {
    const box = $('shelf-list');
    box.textContent = '';
    let known = 0;
    for (const cat of S.CATS) {
      const kinds = S.KINDS.filter((k) => k.cat === cat.id);
      const have = kinds.filter((k) => save.col[k.id]).length;
      known += have;
      const sec = el('section', 'shelf-sec'), h = el('h3', '', cat.label);
      h.appendChild(el('small', '', have + ' / ' + kinds.length));
      sec.appendChild(h);
      for (let i = 0; i < kinds.length; i += 4) {
        const row = el('div', 'shelf-row');
        kinds.slice(i, i + 4).forEach((k) => {
          const e = save.col[k.id];
          const cell = el('button', 'cell ' + (e ? 'known' : 'unknown') + (cat.id !== 'common' ? ' rare' : ''));
          cell.type = 'button';
          cell.setAttribute('aria-label', e ? k.name : 'まだ見つけていない石');
          const img = el('img'); img.alt = ''; img.src = iconURL(k.id, !!e); img.draggable = false;
          cell.appendChild(img);
          cell.appendChild(el('span', 'nm', e ? k.name : '？？？'));
          if (e) cell.appendChild(el('span', 'ct', '×' + e.n));
          cell.addEventListener('click', () => showDetail(k.id));
          row.appendChild(cell);
        });
        sec.appendChild(row);
      }
      box.appendChild(sec);
    }
    $('sheet-stats').textContent = '図鑑 ' + known + ' / ' + S.KINDS.length + ' ・ あつめた石 ' + save.total + ' こ ・ のべ ' + (Math.floor(save.dist / 100) / 10).toFixed(1) + ' m';
  }
  function showDetail(kind) {
    const def = S.get(kind), cat = S.cat(def.cat), e = save.col[kind];
    $('d-img').src = iconURL(kind, !!e);
    const chip = $('d-cat'); chip.textContent = cat.label; chip.className = 'chip cat-' + def.cat;
    $('d-name').textContent = e ? def.name : '？？？';
    $('d-desc').textContent = e ? def.desc : cat.hint;
    $('d-meta').textContent = e ? 'あつめた数 ' + e.n + ' こ ・ はじめて ' + fmtDate(e.first) : '';
    $('detail').hidden = false;
  }

  let paused = false, last = 0;
  const sheet = $('sheet');
  function setTab(set) {
    $('pane-shelf').hidden = set; $('pane-set').hidden = !set; $('detail').hidden = true;
    $('tab-shelf').classList.toggle('on', !set); $('tab-set').classList.toggle('on', set);
    $('sheet-title').textContent = set ? '設定' : '石の棚';
  }
  function openShelf() {
    persist(); buildShelf(); setTab(false);
    sheet.hidden = false; paused = true; ptr.down = false; ptr.held = false;
    Snd.rumble(0, 0);
    requestAnimationFrame(() => requestAnimationFrame(() => sheet.classList.add('open')));
  }
  function closeShelf() {
    sheet.classList.remove('open');
    paused = false; last = performance.now();
    setTimeout(() => { if (!sheet.classList.contains('open')) sheet.hidden = true; }, 300);
  }
  function syncSettings() {
    const sw = (id, on) => { const e = $(id); e.classList.toggle('on', on); e.setAttribute('aria-checked', on ? 'true' : 'false'); };
    sw('sw-sound', save.sound); sw('sw-vib', save.vib);
    $('hand-l').classList.toggle('on', save.hand === 'l'); $('hand-r').classList.toggle('on', save.hand !== 'l');
    app.classList.toggle('hand-l', save.hand === 'l');
    $('btn-sound').classList.toggle('off', !save.sound);
    $('intro-on').classList.toggle('on', save.sound); $('intro-off').classList.toggle('on', !save.sound);
  }
  function setSound(on) { save.sound = on; Snd.apply(); syncSettings(); persist(); }

  function bindUi() {
    const pct = (v) => (Math.round(v * 1000) / 10) + '%';
    $('rate-text').textContent = '石ひとつごとに、宝石 ' + pct(RATE.gem) + '・化石 ' + pct(RATE.fossil) + '・隕石 ' + pct(RATE.meteorDay) +
      '（夜は ' + pct(RATE.meteorNight) + '）・黄金 ' + pct(RATE.gold) + ' の確率で出会えます。ふつうの石が ' + RATE.pity +
      ' こ続いたら、次はかならず めずらしい石です。景色によって、出やすい石も変わります。';
    if (!navigator.vibrate) $('row-vib').hidden = true;

    $('btn-shelf').addEventListener('click', () => { Snd.wake(); openShelf(); });
    $('btn-close').addEventListener('click', closeShelf);
    $('sheet-bg').addEventListener('click', closeShelf);
    $('tab-shelf').addEventListener('click', () => setTab(false));
    $('tab-set').addEventListener('click', () => setTab(true));
    $('detail').addEventListener('click', () => { $('detail').hidden = true; });
    $('btn-sound').addEventListener('click', () => { setSound(!save.sound); if (save.sound) Snd.chime(); });
    $('sw-sound').addEventListener('click', () => { setSound(!save.sound); if (save.sound) Snd.chime(); });
    $('sw-vib').addEventListener('click', () => { save.vib = !save.vib; syncSettings(); persist(); buzz(15); });
    $('hand-l').addEventListener('click', () => { save.hand = 'l'; syncSettings(); persist(); });
    $('hand-r').addEventListener('click', () => { save.hand = 'r'; syncSettings(); persist(); });

    $('btn-reset').addEventListener('click', () => { $('confirm').hidden = false; });
    $('cf-no').addEventListener('click', () => { $('confirm').hidden = true; });
    $('cf-yes').addEventListener('click', () => {
      const keep = { intro: true, sound: save.sound, vib: save.vib, hand: save.hand };
      save = Object.assign(fresh(), keep);
      stone = null; parts.length = 0; spawnWait = 0.3;
      computeWorld(); persist();
      $('confirm').hidden = true; closeShelf();
    });

    $('intro-on').addEventListener('click', () => { save.sound = true; syncSettings(); });
    $('intro-off').addEventListener('click', () => { save.sound = false; syncSettings(); });
    $('intro-go').addEventListener('click', () => {
      save.intro = true; persist(); Snd.apply();
      if (save.sound) Snd.chime();
      const it = $('intro'); it.classList.add('bye');
      setTimeout(() => { it.hidden = true; }, 450);
    });

    // 指（マウスでも動く）
    const pos = (e) => { const r = cv.getBoundingClientRect(); ptr.x = e.clientX - r.left; ptr.y = e.clientY - r.top; };
    cv.addEventListener('pointerdown', (e) => {
      if (ptr.down) return;
      ptr.down = true; ptr.id = e.pointerId; ptr.held = false; pos(e);
      try { cv.setPointerCapture(e.pointerId); } catch (err) { /* 古い端末 */ }
      Snd.wake(); e.preventDefault();
    });
    cv.addEventListener('pointermove', (e) => { if (ptr.down && e.pointerId === ptr.id) pos(e); });
    const up = (e) => { if (e.pointerId === ptr.id) { ptr.down = false; ptr.held = false; Snd.wake(); } };
    cv.addEventListener('pointerup', up);
    cv.addEventListener('pointercancel', up);
    cv.addEventListener('contextmenu', (e) => e.preventDefault());
    document.addEventListener('gesturestart', (e) => e.preventDefault());
    document.addEventListener('dblclick', (e) => e.preventDefault());

    window.addEventListener('resize', resize);
    window.addEventListener('orientationchange', () => setTimeout(resize, 200));
    document.addEventListener('visibilitychange', () => {
      if (document.hidden) { persist(); Snd.sleep(); } else { last = performance.now(); if (!paused) Snd.wake(); }
    });
    window.addEventListener('pagehide', persist);
  }

  /* ================= まわす ================= */
  let acc = 0, frame = 0, drawn = 0;
  function busy() {
    if (ptr.down || parts.length || fx.shake > 0 || fx.flash > 0) return true;
    const s = stone;
    if (!s || s.state !== 'play' || !s.grounded) return true;
    return Math.abs(s.vx) + Math.abs(s.vy) > 4 || Math.abs(s.om) > 0.05;
  }
  function tick(now) {
    requestAnimationFrame(tick);
    if (paused || document.hidden) { last = now; return; }
    frame++;
    if (!busy() && frame % 3) return; // 石が止まっているあいだは、ゆっくり描いて電池を節約
    const dt = clamp((now - last) / 1000, 0, 0.1);
    last = now;
    save.clock += dt;
    computeWorld();
    acc += dt;
    let n = 0;
    while (acc >= SUB && n < 14) { stepStone(SUB); acc -= SUB; n++; }
    if (n === 14) acc = 0;
    updateFx(dt);
    updateFlow(dt);
    draw(); drawn++;
    refreshHud();
    saveT += dt;
    if (saveT > 5) { if (dirty) persist(); else saveT = 0; }
  }

  function start() {
    resize();
    bindUi();
    syncSettings();
    if (!save.intro) $('intro').hidden = false;
    spawn(save.cur);
    refreshHud();
    last = performance.now();
    requestAnimationFrame(tick);
    if ('serviceWorker' in navigator && /^https?:$/.test(location.protocol)) {
      navigator.serviceWorker.register('sw.js').catch(() => { /* オフライン対応は任意 */ });
    }
    if (/[?&]debug\b/.test(location.search)) {
      window.__ishi = {
        get save() { return save; }, get stone() { return stone; }, get frames() { return drawn; }, world: world,
        give(kind, prog) { spawn({ k: kind, s: (Math.random() * 4294967296) >>> 0, n: 9000, p: (prog || 0) * 9000 }); },
        fall(kind) { spawn(null, kind); },
        clock(c) { save.clock = c; computeWorld(); },
        fill(n) { S.KINDS.slice(0, n).forEach((k, i) => { save.col[k.id] = { n: 1 + (i % 5), first: Date.now() }; }); save.total = n * 2; }
      };
    }
  }
  start();
})();
