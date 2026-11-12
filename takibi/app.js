/* 焚き火くべ — 片手で薪をくべるだけ。通信・課金・広告なし、記録は localStorage のみ。 */
(() => {
  'use strict';

  // ================= 小道具 =================
  const TAU = Math.PI * 2;
  const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);
  const lerp = (a, b, t) => a + (b - a) * t;
  const rnd = (a, b) => a + Math.random() * (b - a);
  const pick = (arr) => arr[(Math.random() * arr.length) | 0];
  const $ = (id) => document.getElementById(id);
  const rgb = (c) => `rgb(${c[0] | 0},${c[1] | 0},${c[2] | 0})`;
  const rgba = (c, a) => `rgba(${c[0] | 0},${c[1] | 0},${c[2] | 0},${a})`;
  const mix = (a, b, t) => [lerp(a[0], b[0], t), lerp(a[1], b[1], t), lerp(a[2], b[2], t)];
  function seeded(s) {
    return () => {
      s = (s + 0x6d2b79f5) | 0;
      let t = Math.imul(s ^ (s >>> 15), 1 | s);
      t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  // ================= データ定義 =================
  const PAL = {
    fire:   { core: [255, 236, 170], edge: [255, 96, 20],  glow: [255, 120, 30] },
    blue:   { core: [210, 240, 255], edge: [40, 110, 255], glow: [60, 130, 255] },
    green:  { core: [225, 255, 210], edge: [30, 200, 90],  glow: [50, 210, 110] },
    purple: { core: [245, 225, 255], edge: [150, 60, 255], glow: [165, 85, 255] },
    pink:   { core: [255, 235, 242], edge: [255, 90, 150], glow: [255, 115, 170] },
    red:    { core: [255, 225, 210], edge: [255, 45, 45],  glow: [255, 70, 60] },
    yellow: { core: [255, 255, 225], edge: [255, 205, 40], glow: [255, 215, 70] },
  };
  const RAINBOW = ['red', 'fire', 'yellow', 'green', 'blue', 'purple'];
  const FWC = [[255, 214, 130], [255, 96, 84], [120, 255, 160], [120, 180, 255], [206, 146, 255], [255, 150, 205], [255, 255, 236]];
  const GOLD = [255, 200, 110];

  // on:'log' = 薪をくべた瞬間に抽選 / on:'amb' = 火が燃えている間、10秒ごとに抽選
  const RARES = [
    { id: 'blue',    name: '青い炎',     star: 1, glyph: '炎', orb: ['#e2f4ff', '#3d8bff', '#142ac4'], on: 'log', p: 0.005,
      desc: '炎がすうっと青く澄む。', hint: '薪をくべていると、まれに炎の色が変わる。' },
    { id: 'green',   name: '翠の炎',     star: 1, glyph: '炎', orb: ['#e8ffdc', '#2fcf68', '#0a6a34'], on: 'log', p: 0.004,
      desc: '森の色をうつしたような翠（みどり）の炎。', hint: '薪をくべていると、まれに炎の色が変わる。' },
    { id: 'purple',  name: '紫の炎',     star: 2, glyph: '炎', orb: ['#f6e6ff', '#9a4dff', '#4a12a8'], on: 'log', p: 0.002,
      desc: '夜の底のような、深い紫の炎。', hint: '薪をくべていると、まれに炎の色が変わる。' },
    { id: 'pink',    name: '桜色の炎',   star: 2, glyph: '炎', orb: ['#fff0f5', '#ff6aa2', '#c02a6a'], on: 'log', p: 0.0015,
      desc: '季節はずれの花が咲いたような炎。', hint: '薪をくべていると、まれに炎の色が変わる。' },
    { id: 'rainbow', name: '虹色の炎',   star: 3, glyph: '炎', orb: null, on: 'log', p: 0.0003,
      desc: '七つの色がゆらめく、めったに見られない炎。', hint: '薪をくべていると、ごくまれに…。' },
    { id: 'hanabi',  name: '火の粉花火', star: 1, glyph: '花', orb: ['#fff3c8', '#ffb23c', '#a3421a'], on: 'log', p: 0.004,
      desc: '舞い上がった火の粉が、空で小さな花火になる。', hint: '薪をくべたとき、火の粉が高く舞い上がると…。' },
    { id: 'tairin',  name: '大輪の花火', star: 3, glyph: '花', orb: ['#fff6da', '#ff5d7a', '#5a2ad0'], on: 'log', p: 0.0005, minFuel: 65,
      desc: '空いっぱいに咲く大輪と、しだれる金の火。', hint: '大きく育てた火に薪をくべ続けると、ごくまれに…。' },
    { id: 'star',    name: '流れ星',     star: 1, glyph: '星', orb: ['#ffffff', '#8fb4ff', '#16224e'], on: 'amb', p: 0.03, sky: 'night',
      desc: '夜空をすっと横切る、ひとすじの光。', hint: '夜、火を絶やさずにいると…。' },
    { id: 'meteor',  name: '流星群',     star: 3, glyph: '星', orb: ['#ffffff', '#b9a4ff', '#1b1450'], on: 'amb', p: 0.0015, sky: 'night',
      desc: '数えきれない星が降る夜。', hint: '夜、気長に火を守っていると、ごくまれに…。' },
    { id: 'niji',    name: '朝の虹',     star: 2, glyph: '虹', orb: null, on: 'amb', p: 0.01, sky: 'day',
      desc: '朝の空にかかる、大きな虹。', hint: '朝、火を絶やさずにいると…。' },
    { id: 'hinoko',  name: '火の精霊',   star: 2, glyph: '霊', orb: ['#fff7cf', '#ffae3c', '#d2440e'], on: 'amb', p: 0.01, minFuel: 40,
      desc: '炎から飛び出した小さな精霊。ひとしきり跳ねて、火に帰っていく。', hint: 'しっかり育った焚き火のそばに、まれに…。' },
    { id: 'tsuyu',   name: '朝露の精霊', star: 3, glyph: '霊', orb: ['#ffffff', '#a8e0ff', '#3a8ad0'], on: 'amb', p: 0.003, sky: 'day', minFuel: 20,
      desc: '朝露から生まれた精霊が、火にあたりにやって来る。', hint: '朝、火を育てていると、ごくまれに…。' },
  ];
  const RMAP = Object.fromEntries(RARES.map((r) => [r.id, r]));
  const FLAME_IDS = ['blue', 'green', 'purple', 'pink', 'rainbow'];
  const STAGES = [[90, '天を焦がす炎'], [65, '大きな焚き火'], [40, '焚き火'], [20, '小さな火'], [5, '種火'], [0, '熾火']];
  const stageOf = (f) => STAGES.find((s) => f >= s[0])[1];

  // ================= 保存 =================
  const KEY = 'takibi-kube.v1';
  const DEBUG = /[?&]debug\b/.test(location.search);
  const fresh = () => ({
    v: 1, fuel: 0, lastSeen: Date.now(), firstDay: Date.now(), totalLogs: 0, burnSec: 0,
    col: {}, log: [], unseen: false, set: { sound: false, vib: true, sky: 'auto' },
  });
  function loadState() {
    try {
      const s = JSON.parse(localStorage.getItem(KEY) || 'null');
      if (!s || typeof s !== 'object') return fresh();
      const d = fresh();
      return { ...d, ...s, col: s.col || {}, log: Array.isArray(s.log) ? s.log : [], set: { ...d.set, ...(s.set || {}) } };
    } catch (e) { return fresh(); }
  }
  let S = loadState();
  let dirty = false, saveT = 0;
  function save() {
    try { S.lastSeen = Date.now(); localStorage.setItem(KEY, JSON.stringify(S)); dirty = false; } catch (e) { /* 保存不可でも遊べる */ }
  }

  // 燃焼: df/dt = -(A + B f)。離れていた時間ぶんも同じ式で減らす（熾火は消えない）
  const BURN_A = 0.35, BURN_B = 0.01, LOG_FUEL = 9;
  function decayAway(sec) {
    if (S.fuel <= 0 || sec <= 0) return;
    S.fuel = Math.max(0, (S.fuel + BURN_A / BURN_B) * Math.exp(-BURN_B * sec) - BURN_A / BURN_B);
  }
  const awaySec = (Date.now() - S.lastSeen) / 1000;
  decayAway(awaySec);

  // ================= 音（WebAudioで合成。素材ファイルなし） =================
  const Sound = (() => {
    let ctx = null, master, bedG, hissG, white, on = false;
    function build() {
      const AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return false;
      ctx = new AC();
      master = ctx.createGain(); master.gain.value = 0; master.connect(ctx.destination);
      const n = ctx.sampleRate * 3, F = 4096;
      white = ctx.createBuffer(1, n, ctx.sampleRate);
      const brown = ctx.createBuffer(1, n, ctx.sampleRate);
      const w = white.getChannelData(0), b = brown.getChannelData(0), gen = new Float32Array(n + F);
      let last = 0;
      for (let i = 0; i < n + F; i++) {
        const v = Math.random() * 2 - 1;
        if (i < n) w[i] = v;
        last = (last + 0.02 * v) / 1.02; gen[i] = last * 3.5;
      }
      for (let i = 0; i < n; i++) b[i] = i < F ? gen[i] * (i / F) + gen[n + i] * (1 - i / F) : gen[i]; // ループの継ぎ目を消す
      const loop = (buf, type, f, q) => {
        const s = ctx.createBufferSource(); s.buffer = buf; s.loop = true;
        const fl = ctx.createBiquadFilter(); fl.type = type; fl.frequency.value = f; if (q) fl.Q.value = q;
        const gn = ctx.createGain(); gn.gain.value = 0;
        s.connect(fl); fl.connect(gn); gn.connect(master); s.start();
        return gn;
      };
      bedG = loop(brown, 'lowpass', 420);
      hissG = loop(white, 'bandpass', 2600, 0.5);
      return true;
    }
    const quiet = (pr) => { if (pr && pr.catch) pr.catch(() => {}); };
    const live = () => on && ctx && ctx.state === 'running';
    function set(v) {
      on = v;
      if (v) {
        if (!ctx && !build()) return;
        if (ctx.state !== 'running') quiet(ctx.resume());
        master.gain.setTargetAtTime(0.85, ctx.currentTime, 0.15);
      } else if (ctx) {
        master.gain.setTargetAtTime(0, ctx.currentTime, 0.04);
      }
    }
    function unlock() { if (S.set.sound && (!ctx || ctx.state !== 'running')) set(true); }
    function pause(p) { if (!ctx) return; if (p) quiet(ctx.suspend()); else if (on) quiet(ctx.resume()); }
    function level(I, lit) {
      if (!live()) return;
      bedG.gain.setTargetAtTime(0.04 + 0.3 * I * lit, ctx.currentTime, 0.3);
      hissG.gain.setTargetAtTime(0.003 + 0.03 * I * lit, ctx.currentTime, 0.3);
    }
    function noise(dur, type, f, vol, rate = 1, when = 0) {
      const t = ctx.currentTime + when;
      const s = ctx.createBufferSource(); s.buffer = white; s.playbackRate.value = rate;
      const fl = ctx.createBiquadFilter(); fl.type = type; fl.frequency.value = f;
      const gn = ctx.createGain();
      gn.gain.setValueAtTime(0.0001, t); gn.gain.linearRampToValueAtTime(vol, t + 0.003);
      gn.gain.exponentialRampToValueAtTime(0.0001, t + dur);
      s.connect(fl); fl.connect(gn); gn.connect(master);
      s.start(t, rnd(0, 2), dur + 0.05);
    }
    function tone(f0, f1, dur, vol, when = 0, type = 'sine') {
      const t = ctx.currentTime + when;
      const o = ctx.createOscillator(); o.type = type;
      o.frequency.setValueAtTime(f0, t); if (f1 !== f0) o.frequency.exponentialRampToValueAtTime(f1, t + dur);
      const gn = ctx.createGain();
      gn.gain.setValueAtTime(0.0001, t); gn.gain.linearRampToValueAtTime(vol, t + 0.012);
      gn.gain.exponentialRampToValueAtTime(0.0001, t + dur);
      o.connect(gn); gn.connect(master); o.start(t); o.stop(t + dur + 0.05);
    }
    return {
      set, unlock, pause, level,
      crackle(v = 1) { if (live()) noise(rnd(0.012, 0.06), 'highpass', rnd(1200, 5200), rnd(0.12, 0.45) * v, rnd(0.5, 2)); },
      thud() { if (!live()) return; tone(120, 52, 0.14, 0.4); noise(0.09, 'lowpass', 700, 0.3); },
      chime(star) {
        if (!live()) return;
        const notes = [523.25, 659.25, 783.99, 1046.5, 1318.5];
        for (let i = 0; i < 2 + star; i++) { tone(notes[i], notes[i], 1.1, 0.11, i * 0.12); tone(notes[i] * 2, notes[i] * 2, 0.5, 0.025, i * 0.12); }
      },
      boom(big) {
        if (!live()) return;
        noise(big ? 0.9 : 0.5, 'lowpass', big ? 160 : 240, big ? 0.9 : 0.6, 0.6);
        tone(90, 38, 0.35, big ? 0.5 : 0.3);
        for (let i = 0; i < (big ? 16 : 8); i++) noise(rnd(0.01, 0.04), 'highpass', rnd(2500, 6000), rnd(0.05, 0.16), rnd(0.8, 2), rnd(0.15, big ? 1.4 : 0.8));
      },
      swish() { if (live()) tone(2300, 800, 0.7, 0.045); },
      blip() { if (!live()) return; tone(880, 1320, 0.16, 0.08); tone(1320, 1760, 0.16, 0.08, 0.14); tone(1760, 1320, 0.22, 0.07, 0.28); },
    };
  })();

  function haptic(p) { if (S.set.vib && navigator.vibrate) { try { navigator.vibrate(p); } catch (e) { /* 非対応 */ } } }

  // ================= キャンバスと風景 =================
  const app = $('app'), cv = $('scene');
  const G = cv.getContext('2d', { alpha: false });
  let g = G; // いま描いている先（本体 or 低解像度バッファ）
  let W = 0, H = 0, DPR = 1, U = 1, cx = 0, baseY = 0, horizon = 0;
  let fBuf = null, sBuf = null;
  let layerN = null, layerD = null, skyN = null, skyD = null, moonCv = null, moonR = 0;
  let stars = [], clouds = [];

  const spriteCache = new Map();
  function sprite(c) {
    const key = (c[0] | 0) + ',' + (c[1] | 0) + ',' + (c[2] | 0);
    let s = spriteCache.get(key);
    if (!s) {
      s = document.createElement('canvas'); s.width = s.height = 64;
      const x = s.getContext('2d');
      const gr = x.createRadialGradient(32, 32, 0, 32, 32, 32);
      gr.addColorStop(0, rgba(c, 1)); gr.addColorStop(0.35, rgba(c, 0.55)); gr.addColorStop(1, rgba(c, 0));
      x.fillStyle = gr; x.fillRect(0, 0, 64, 64);
      spriteCache.set(key, s);
    }
    return s;
  }
  // ぼんやり光る玉。炎・火の粉・光はすべてこれの重ね合わせ
  function blob(c, x, y, r, a) {
    if (a <= 0.004 || r <= 0) return;
    g.globalAlpha = a > 1 ? 1 : a;
    g.drawImage(sprite(c), x - r, y - r, r * 2, r * 2);
  }
  // 炎と煙は低解像度の別キャンバスに描いてから拡大して重ねる。
  // もともとぼかした玉の集まりなので見た目は変わらず、塗る面積（＝負荷と電池消費）が大きく減る。
  function makeBuf(x0, y0, w, h, scale) {
    const c = document.createElement('canvas');
    c.width = Math.max(2, Math.ceil(w * scale)); c.height = Math.max(2, Math.ceil(h * scale));
    return { c, x: c.getContext('2d'), x0, y0, w, h, scale };
  }
  function beginBuf(b) {
    g = b.x;
    g.setTransform(1, 0, 0, 1, 0, 0); g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1;
    g.clearRect(0, 0, b.c.width, b.c.height);
    g.setTransform(b.scale, 0, 0, b.scale, -b.x0 * b.scale, -b.y0 * b.scale);
  }
  function endBuf(b, op, alpha) {
    g = G;
    if (alpha <= 0.004) return;
    g.globalCompositeOperation = op; g.globalAlpha = alpha > 1 ? 1 : alpha;
    g.drawImage(b.c, b.x0, b.y0, b.w, b.h);
  }
  function rr(x, y, w, h, r) {
    g.beginPath();
    g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r);
    g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath();
  }

  function pine(x, px, base, h, hw) {
    const n = 5;
    x.beginPath(); x.moveTo(px, base - h);
    for (let i = 1; i <= n; i++) { const k = i / n, y = base - h + h * k * 0.9; x.lineTo(px + hw * k, y); x.lineTo(px + hw * k * 0.42, y + h * 0.012); }
    x.lineTo(px + hw * 0.1, base); x.lineTo(px - hw * 0.1, base);
    for (let i = n; i >= 1; i--) { const k = i / n, y = base - h + h * k * 0.9; x.lineTo(px - hw * k * 0.42, y + h * 0.012); x.lineTo(px - hw * k, y); }
    x.closePath(); x.fill();
  }

  // 山・森・地面のシルエット。夜用と朝用を同じ形で2枚作り、クロスフェードする
  function makeLayer(day) {
    const c = document.createElement('canvas');
    c.width = Math.round(W * DPR); c.height = Math.round(H * DPR);
    const x = c.getContext('2d'); x.scale(DPR, DPR);
    const R = seeded(20261112);
    const col = day
      ? { far: '#93abc4', mid: '#4d7268', near: '#1f3d34', g0: '#46503a', g1: '#171d15', tuft: 'rgba(120,150,90,.5)' }
      : { far: '#0f1536', mid: '#090d20', near: '#03040a', g0: '#100d0e', g1: '#020202', tuft: 'rgba(40,44,52,.6)' };
    const p1 = R() * TAU, p2 = R() * TAU;
    x.fillStyle = col.far; x.beginPath(); x.moveTo(0, H);
    for (let px = 0; px <= W + 4; px += 4) {
      const k = px / W;
      x.lineTo(px, horizon - (44 + 28 * Math.sin(k * 5.2 + p1) + 12 * Math.sin(k * 13 + p2)) * U);
    }
    x.lineTo(W, H); x.closePath(); x.fill();
    if (day) { // 朝もや
      const hz = x.createLinearGradient(0, horizon - 110 * U, 0, horizon + 8 * U);
      hz.addColorStop(0, 'rgba(255,232,210,0)'); hz.addColorStop(1, 'rgba(255,226,200,.42)');
      x.fillStyle = hz; x.fillRect(0, horizon - 110 * U, W, 118 * U);
    }
    x.fillStyle = col.mid;
    for (let px = -10 * U; px < W + 20 * U; px += (13 + R() * 11) * U) {
      const k = Math.abs(px / W - 0.5) * 2;
      const h = (62 + 52 * k * k + R() * 44) * U;
      pine(x, px, horizon + 6 * U, h, h * 0.3);
    }
    const gg = x.createLinearGradient(0, horizon, 0, H);
    gg.addColorStop(0, col.g0); gg.addColorStop(1, col.g1);
    x.fillStyle = gg; x.fillRect(0, horizon + 2 * U, W, H - horizon);
    x.fillStyle = col.near;
    for (const side of [-1, 1]) {
      for (let i = 0; i < 3; i++) {
        const off = (i * 34 - 8 + R() * 14) * U;
        const h = (196 - i * 48 + R() * 30) * U;
        pine(x, side < 0 ? off : W - off, horizon + (30 - i * 7) * U, h, h * 0.27);
      }
    }
    x.strokeStyle = col.tuft; x.lineWidth = Math.max(1, 1.2 * U); x.lineCap = 'round';
    for (let i = 0; i < 26; i++) { // 草
      const tx = R() * W, ty = horizon + (14 + R() * R() * 300) * U, th = (5 + R() * 6) * U;
      if (Math.abs(tx - W / 2) < 96 * U && Math.abs(ty - baseY) < 44 * U) continue;
      x.beginPath();
      for (const d of [-0.5, 0, 0.5]) { x.moveTo(tx, ty); x.lineTo(tx + d * th, ty - th * (1 - Math.abs(d) * 0.4)); }
      x.stroke();
    }
    x.fillStyle = 'rgba(0,0,0,.4)'; // 灰床
    x.beginPath(); x.ellipse(W / 2, baseY + 10 * U, 84 * U, 23 * U, 0, 0, TAU); x.fill();
    return c;
  }

  function resize() {
    const r = app.getBoundingClientRect();
    W = Math.max(240, Math.round(r.width)); H = Math.max(320, Math.round(r.height));
    DPR = Math.min(window.devicePixelRatio || 1, 2);
    cv.width = Math.round(W * DPR); cv.height = Math.round(H * DPR);
    U = Math.min(W, H * 0.6) / 360;
    cx = W / 2;
    baseY = Math.round(Math.min(H * 0.7, H - 196));
    horizon = Math.round(baseY - H * 0.1);
    layerN = makeLayer(false); layerD = makeLayer(true);
    fBuf = makeBuf(cx - 180 * U, baseY - 340 * U, 360 * U, 380 * U, 0.5);
    sBuf = makeBuf(cx - 215 * U, baseY - 580 * U, 430 * U, 600 * U, 0.3);
    skyN = g.createLinearGradient(0, 0, 0, horizon);
    skyN.addColorStop(0, '#04050d'); skyN.addColorStop(0.55, '#0a1030'); skyN.addColorStop(1, '#1d1b48');
    skyD = g.createLinearGradient(0, 0, 0, horizon);
    skyD.addColorStop(0, '#5f9bd4'); skyD.addColorStop(0.5, '#a9cfe9'); skyD.addColorStop(0.85, '#ffe2c4'); skyD.addColorStop(1, '#ffc49a');
    const R = seeded(99);
    stars = [];
    const n = Math.round(clamp((W * horizon) / 2400, 40, 160));
    for (let i = 0; i < n; i++) stars.push({ x: R() * W, y: R() * horizon * 0.96, r: 0.7 + R() * 1.3, a: 0.35 + R() * 0.65, sp: 0.6 + R() * 2.4, ph: R() * TAU });
    clouds = [];
    for (let i = 0; i < 4; i++) clouds.push({ x: R() * W, y: horizon * (0.12 + R() * 0.45), w: (90 + R() * 90) * U, h: (16 + R() * 12) * U, sp: (2 + R() * 4) * U });
    moonR = 18 * U;
    moonCv = document.createElement('canvas');
    const ms = Math.ceil(moonR * 2 * DPR) + 4; moonCv.width = moonCv.height = ms;
    const mx = moonCv.getContext('2d');
    mx.fillStyle = '#f6f0d6'; mx.beginPath(); mx.arc(ms / 2, ms / 2, ms / 2 - 2, 0, TAU); mx.fill();
    mx.globalCompositeOperation = 'destination-out';
    mx.beginPath(); mx.arc(ms * 0.3, ms * 0.4, ms * 0.44, 0, TAU); mx.fill();
  }

  // ================= 火と粒子 =================
  let T = 0;                 // 経過秒
  let I = 0, lit = 0, flare = 0, flick = 1, flameH = 0;
  let dayTarget = 0, dayness = 0, skyCheck = 0;
  const flames = [], sparks = [], smokes = [], flying = [], landed = [];
  const rockets = [], fw = [], shoots = [];
  const fx = { flame: null, flameT: 0, fwBusy: 0, meteorT: 0, rain: null, spirit: null, flash: 0, flashX: 0, flashY: 0, flashC: GOLD };
  let flameAcc = 0, sparkAcc = 0, smokeAcc = 0, ambT = 0, lastThrow = 0, lastRoll = 0, crackT = 0;

  const palName = () => {
    if (!fx.flame || fx.flameT <= 0) return 'fire';
    return fx.flame === 'rainbow' ? RAINBOW[Math.floor(T * 2.3) % RAINBOW.length] : fx.flame;
  };

  function spawnFlame() {
    const spread = (10 + 34 * I) * U;
    const off = (Math.random() + Math.random() + Math.random() - 1.5) * spread;
    const center = 1 - Math.min(1, Math.abs(off) / (spread * 1.3));
    flames.push({
      x: cx + off, y: baseY - rnd(0, 8) * U, ox: off * 0.22,
      vx: rnd(-8, 8) * U, vy: -(52 + 150 * I) * U * (0.42 + 0.78 * center) * rnd(0.8, 1.2),
      life: (0.5 + 0.6 * I) * (0.5 + 0.7 * center) * rnd(0.8, 1.25) * (0.5 + 0.5 * lit), age: 0,
      size: (20 + 30 * I) * U * rnd(0.7, 1.2) * (0.45 + 0.55 * lit), seed: rnd(0, TAU), pal: palName(),
    });
  }
  function addSpark(x, y, vx, vy, life, size, c, grav) {
    if (sparks.length < 260) sparks.push({ x, y, vx, vy, life, age: 0, size, c, grav, seed: rnd(0, TAU) });
  }
  function emberBurst(x, y, n, power) {
    const pal = PAL[palName()];
    for (let i = 0; i < n; i++) {
      addSpark(x + rnd(-10, 10) * U, y + rnd(-6, 6) * U, rnd(-70, 70) * U * power, -rnd(60, 220) * U * power,
        rnd(0.7, 1.9), rnd(1, 2.4) * U, mix(pal.core, pal.edge, rnd(0.1, 0.8)), 40);
    }
  }

  function throwLog(px) {
    const now = performance.now();
    if (now - lastThrow < 130 || flying.length >= 10) return;
    lastThrow = now;
    S.totalLogs++; dirty = true;
    flying.push({
      x0: cx + (px - cx) * 0.6, y0: H - 118, x1: cx + rnd(-16, 16) * U, y1: baseY - rnd(0, 14) * U,
      t: 0, dur: rnd(0.5, 0.62), rot: rnd(-0.6, 0.6), spin: (Math.random() < 0.5 ? -1 : 1) * rnd(3, 7), arc: rnd(90, 140) * U, x: 0, y: 0,
    });
    hideHint(); haptic(8);
  }
  function landLog(L) {
    S.fuel = Math.min(100, S.fuel + LOG_FUEL);
    flare = Math.min(0.55, flare + 0.2);
    emberBurst(L.x1, L.y1 - 8 * U, 9 + ((I * 9) | 0), 0.6 + I * 0.5);
    landed.push({ x: L.x1, y: L.y1, rot: L.rot, a: 1 });
    Sound.thud(); Sound.crackle(1.4); Sound.crackle(1);
    const now = performance.now();
    if (now - lastRoll >= 1000) { lastRoll = now; rollRare('log'); } // 連打しても抽選は毎秒1回まで
  }

  // ================= レアな出来事 =================
  function eligible(r) {
    if (S.fuel < (r.minFuel || (r.on === 'log' ? 20 : 5))) return false;
    if (r.sky && r.sky !== (dayTarget ? 'day' : 'night')) return false;
    if (FLAME_IDS.includes(r.id)) return fx.flameT <= 0;
    if (r.id === 'hanabi' || r.id === 'tairin') return fx.fwBusy <= 0;
    if (r.id === 'star' || r.id === 'meteor') return fx.meteorT <= 0;
    if (r.id === 'niji') return !fx.rain;
    return !fx.spirit;
  }
  // 1回の抽選。条件を満たす出来事それぞれが、ちょうど確率 p で選ばれる
  function pickRare(on) {
    let x = Math.random();
    for (const r of RARES) {
      if (r.on !== on || !eligible(r)) continue;
      if (x < r.p) return r.id;
      x -= r.p;
    }
    return null;
  }
  function rollRare(on) { const id = pickRare(on); if (id) trigger(id, true); }
  function trigger(id, record) {
    const r = RMAP[id];
    if (!r) return;
    if (FLAME_IDS.includes(id)) { fx.flame = id; fx.flameT = id === 'rainbow' ? 30 : 22; flare = 0.55; emberBurst(cx, baseY - 20 * U, 16, 1); }
    else if (id === 'hanabi') launchFireworks(4, false);
    else if (id === 'tairin') launchFireworks(10, true);
    else if (id === 'star') { addShoot(0); Sound.swish(); }
    else if (id === 'meteor') {
      fx.meteorT = 16;
      const dir = Math.random() < 0.5 ? -1 : 1, ang = rnd(0.45, 0.6);
      for (let i = 0; i < 40; i++) addShoot(rnd(0, 14), dir, ang + rnd(-0.05, 0.05));
      Sound.swish();
    }
    else if (id === 'niji') fx.rain = { age: 0 };
    else if (id === 'hinoko') { fx.spirit = { kind: 'fire', age: 0, x: cx, y: baseY, s: 0, al: 1 }; Sound.blip(); }
    else if (id === 'tsuyu') { fx.spirit = { kind: 'dew', age: 0, x: W * 0.16, y: H * 0.2, s: 1, al: 0 }; Sound.blip(); }

    let sub = 'プレビュー（記録されません）';
    if (record) {
      const now = Date.now();
      const c = S.col[id] || (S.col[id] = { n: 0, first: now });
      c.n++; c.last = now;
      S.log.unshift({ id, t: now });
      if (S.log.length > 100) S.log.length = 100;
      if (c.n === 1) S.unseen = true;
      sub = c.n === 1 ? 'はじめて出会った！ 図鑑に記録' : `${c.n}回目`;
      save(); syncButtons();
    }
    toast('★'.repeat(r.star), r.name, sub, 5200);
    Sound.chime(r.star);
    haptic(r.star >= 3 ? [40, 60, 40, 60, 90] : [30, 50, 30]);
  }

  function launchFireworks(n, big) {
    let d = 0;
    for (let i = 0; i < n; i++) {
      d += i === 0 ? 0.1 : rnd(0.35, 0.75);
      const isBig = big && (i % 3 === 2 || i === n - 1);
      rockets.push({
        delay: d, age: 0, dur: rnd(0.9, 1.3), sx: cx + rnd(-12, 12) * U, sy: baseY - flameH * 0.5,
        tx: W * (isBig ? rnd(0.32, 0.68) : rnd(0.16, 0.84)), ty: H * (isBig ? rnd(0.14, 0.22) : rnd(0.1, 0.3)),
        big: isBig, c: pick(FWC), x: 0, y: 0,
      });
    }
    fx.fwBusy = d + 5;
    emberBurst(cx, baseY - flameH * 0.4, 20, 1.2);
  }
  function explode(x, y, big, c) {
    const n = big ? 120 : (rnd(46, 64) | 0), sp = (big ? 215 : rnd(110, 150)) * U;
    let c2 = pick(FWC); if (c2 === c) c2 = [255, 255, 236];
    for (let i = 0; i < n && fw.length < 900; i++) {
      const a = (TAU * i) / n + rnd(-0.05, 0.05), inner = big && i % 2 === 1;
      const v = sp * (inner ? 0.55 : 1) * rnd(0.92, 1.05);
      fw.push({ x, y, vx: Math.cos(a) * v, vy: Math.sin(a) * v, age: 0, life: (big ? 2.1 : 1.4) * rnd(0.8, 1.15), c: inner ? c2 : c, drag: big ? 1.6 : 2, grav: 46 * U, tw: false, big });
    }
    if (big) { // しだれ
      for (let i = 0; i < 70 && fw.length < 900; i++) {
        const a = rnd(0, TAU), v = rnd(40, 160) * U;
        fw.push({ x, y, vx: Math.cos(a) * v, vy: Math.sin(a) * v, age: 0, life: rnd(2.2, 3.1), c: GOLD, drag: 2.5, grav: 74 * U, tw: true, big });
      }
    }
    fx.flash = big ? 0.34 : 0.16; fx.flashX = x; fx.flashY = y; fx.flashC = c;
    Sound.boom(big); haptic(big ? 30 : 12);
  }
  function addShoot(delay, dir, ang) {
    dir = dir || (Math.random() < 0.5 ? -1 : 1);
    ang = ang || rnd(0.35, 0.65);
    const sp = rnd(0.85, 1.25) * W;
    shoots.push({
      delay, age: 0, dur: rnd(0.7, 1.05), len: rnd(70, 125) * U,
      x: W * (dir > 0 ? rnd(0.02, 0.55) : rnd(0.45, 0.98)), y: horizon * rnd(0.04, 0.38),
      vx: Math.cos(ang) * sp * dir, vy: Math.sin(ang) * sp,
    });
  }

  // ================= 更新 =================
  function update(dt) {
    // --- 燃焼 ---
    if (S.fuel > 0) S.fuel = Math.max(0, S.fuel - (BURN_A + BURN_B * S.fuel) * dt);
    if (S.fuel >= 5) { S.burnSec += dt; dirty = true; }
    flare *= Math.exp(-2.6 * dt);
    lit = clamp(S.fuel / 6, 0, 1);
    const target = clamp(Math.pow(S.fuel / 100, 0.8) * 0.92 + flare * 0.5, 0, 1.12);
    I += (target - I) * Math.min(1, dt * 4);
    flick = 1 + 0.07 * Math.sin(T * 11.3) + 0.05 * Math.sin(T * 23.7 + 1) + 0.04 * Math.sin(T * 5.1 + 2);
    flameH = (36 + 214 * I) * U * (0.2 + 0.8 * lit);

    // --- 空 ---
    skyCheck -= dt;
    if (skyCheck <= 0) { skyCheck = 1; const t = skyTarget(); if (t !== dayTarget) { dayTarget = t; syncButtons(); } }
    dayness = dayTarget > dayness ? Math.min(dayTarget, dayness + dt * 0.45) : Math.max(dayTarget, dayness - dt * 0.45);
    for (const c of clouds) { c.x += c.sp * dt; if (c.x - c.w > W) c.x = -c.w; }

    // --- 炎 ---
    flameAcc += (lit * (46 + 190 * I) + (1 - lit) * 6) * dt;
    while (flameAcc >= 1) { flameAcc--; if (flames.length < 320) spawnFlame(); }
    const wind = Math.sin(T * 0.7) * 0.6 + Math.sin(T * 1.9 + 1.3) * 0.4;
    for (let i = flames.length - 1; i >= 0; i--) {
      const p = flames[i];
      p.age += dt;
      if (p.age >= p.life) { flames[i] = flames[flames.length - 1]; flames.pop(); continue; }
      const h = (baseY - p.y) / (160 * U);
      p.vx += ((cx + p.ox - p.x) * 3.2 + (wind * 46 + Math.sin(T * 6.3 + p.seed) * 40) * U * h) * dt;
      p.vx *= 1 - Math.min(1, 1.4 * dt);
      p.x += p.vx * dt; p.y += p.vy * dt;
    }

    // --- 火の粉 ---
    sparkAcc += (0.5 + lit * (2 + 11 * I)) * dt;
    while (sparkAcc >= 1) {
      sparkAcc--;
      const pal = PAL[palName()];
      addSpark(cx + rnd(-1, 1) * (12 + 30 * I) * U, baseY - rnd(6, 10 + flameH * 0.55), rnd(-18, 18) * U, -rnd(50, 130) * U * (0.5 + I),
        rnd(0.9, 2.5), rnd(1, 2.2) * U, mix(pal.core, pal.edge, rnd(0.2, 0.8)), -10);
    }
    for (let i = sparks.length - 1; i >= 0; i--) {
      const p = sparks[i];
      p.age += dt;
      if (p.age >= p.life) { sparks[i] = sparks[sparks.length - 1]; sparks.pop(); continue; }
      p.vx += (Math.sin(T * 4 + p.seed) * 46 * U + wind * 14 * U) * dt;
      p.vx *= 1 - Math.min(1, 0.9 * dt);
      p.vy += p.grav * U * dt;
      p.x += p.vx * dt; p.y += p.vy * dt;
    }

    // --- 煙 ---
    smokeAcc += (1.2 + lit * (2 + 5 * I)) * dt;
    while (smokeAcc >= 1) {
      smokeAcc--;
      if (smokes.length < 60) smokes.push({
        x: cx + rnd(-10, 10) * U * (1 + I), y: baseY - (lit > 0.5 ? flameH * rnd(0.5, 0.85) : 10 * U),
        vx: rnd(-6, 6) * U, vy: -rnd(20, 38) * U, life: rnd(3, 5.5), age: 0, size: rnd(12, 20) * U * (0.7 + I * 0.7),
      });
    }
    for (let i = smokes.length - 1; i >= 0; i--) {
      const p = smokes[i];
      p.age += dt;
      if (p.age >= p.life) { smokes[i] = smokes[smokes.length - 1]; smokes.pop(); continue; }
      p.x += (p.vx + wind * 16 * U * (p.age / p.life)) * dt; p.y += p.vy * dt;
    }

    // --- 飛んでいく薪 ---
    for (let i = flying.length - 1; i >= 0; i--) {
      const L = flying[i];
      L.t += dt;
      const k = Math.min(1, L.t / L.dur);
      L.x = lerp(L.x0, L.x1, k); L.y = lerp(L.y0, L.y1, k) - L.arc * 4 * k * (1 - k);
      L.rot += L.spin * dt;
      if (k >= 1) { flying.splice(i, 1); landLog(L); }
    }
    for (let i = landed.length - 1; i >= 0; i--) { landed[i].a -= dt * 1.6; if (landed[i].a <= 0) landed.splice(i, 1); }

    // --- 花火 ---
    if (fx.fwBusy > 0) fx.fwBusy -= dt;
    for (let i = rockets.length - 1; i >= 0; i--) {
      const r = rockets[i];
      if (r.delay > 0) { r.delay -= dt; continue; }
      r.age += dt;
      const k = Math.min(1, r.age / r.dur), e = 1 - (1 - k) * (1 - k);
      r.x = lerp(r.sx, r.tx, e) + Math.sin(r.age * 18) * 3 * U * (1 - k); r.y = lerp(r.sy, r.ty, e);
      addSpark(r.x, r.y, rnd(-8, 8) * U, rnd(8, 30) * U, 0.4, 1.5 * U, r.c, 30);
      if (k >= 1) { rockets.splice(i, 1); explode(r.x, r.y, r.big, r.c); }
    }
    for (let i = fw.length - 1; i >= 0; i--) {
      const p = fw[i];
      p.age += dt;
      if (p.age >= p.life) { fw[i] = fw[fw.length - 1]; fw.pop(); continue; }
      p.vx -= p.vx * p.drag * dt; p.vy += (p.grav - p.vy * p.drag) * dt;
      p.x += p.vx * dt; p.y += p.vy * dt;
    }
    if (fx.flash > 0) fx.flash = Math.max(0, fx.flash - dt * 0.7);

    // --- 流れ星・虹・精霊・色炎 ---
    if (fx.meteorT > 0) fx.meteorT -= dt;
    for (let i = shoots.length - 1; i >= 0; i--) {
      const s = shoots[i];
      if (s.delay > 0) { s.delay -= dt; continue; }
      s.age += dt;
      if (s.age >= s.dur) shoots.splice(i, 1);
    }
    if (fx.rain) { fx.rain.age += dt; if (fx.rain.age > 26) fx.rain = null; }
    if (fx.flameT > 0) { fx.flameT -= dt; if (fx.flameT <= 0) fx.flame = null; }
    updSpirit(dt);

    // --- 時間でのレア抽選（火が燃えている間、10秒ごと） ---
    ambT += dt;
    if (ambT >= 10) { ambT = 0; rollRare('amb'); }

    // --- 音 ---
    crackT -= dt;
    if (crackT <= 0) { crackT = rnd(0.02, 0.5) / (0.25 + I * 1.6); if (lit > 0.2) Sound.crackle(0.5 + I); }
    Sound.level(Math.min(1, I), lit);

    updHud();
    saveT += dt;
    if (saveT > 5) { saveT = 0; if (dirty) save(); }
  }

  function updSpirit(dt) {
    const sp = fx.spirit;
    if (!sp) return;
    sp.age += dt;
    const a = sp.age;
    if (sp.kind === 'fire') {
      const gy = baseY + 30 * U, R = 96 * U, tOut = 1.1, tPlay = 13, tIn = 1.3;
      if (a < tOut) {
        const k = a / tOut, e = 1 - Math.pow(1 - k, 3);
        sp.x = cx + R * e; sp.y = lerp(baseY - flameH * 0.6, gy, e) - Math.sin(k * Math.PI) * 60 * U; sp.s = 0.3 + 0.7 * e;
      } else if (a < tOut + tPlay) {
        const t = a - tOut;
        sp.x = cx + Math.cos(t * 0.85) * R; sp.y = gy - Math.abs(Math.sin(t * 3.4)) * 26 * U; sp.s = 1;
        sp.ex = sp.x; sp.ey = sp.y;
        if (Math.random() < dt * 6) addSpark(sp.x, sp.y - 20 * U, rnd(-20, 20) * U, -rnd(20, 60) * U, rnd(0.5, 1), 1.6 * U, [255, 210, 120], -10);
      } else if (a < tOut + tPlay + tIn) {
        const k = (a - tOut - tPlay) / tIn;
        sp.x = lerp(sp.ex, cx, k); sp.y = lerp(sp.ey, baseY - 24 * U, k) - Math.sin(k * Math.PI) * 70 * U; sp.s = 1 - 0.75 * k;
      } else {
        emberBurst(cx, baseY - 24 * U, 22, 1.1); flare = 0.55; Sound.crackle(2); fx.spirit = null;
      }
    } else {
      const hx = cx - 100 * U, hy = baseY - 30 * U;
      if (a < 3) {
        const k = a / 3, e = k * k * (3 - 2 * k);
        sp.x = lerp(W * 0.16, hx, e) + Math.sin(a * 2.2) * 14 * U * (1 - e); sp.y = lerp(H * 0.2, hy, e); sp.al = Math.min(1, k * 3);
      } else if (a < 14) {
        sp.x = hx + Math.sin(a * 0.9) * 8 * U; sp.y = hy + Math.sin(a * 1.6) * 5 * U; sp.al = 1;
      } else if (a < 17) {
        const k = (a - 14) / 3, e = k * k;
        sp.x = hx - 34 * U * e + Math.sin(a * 2.2) * 14 * U * e; sp.y = lerp(hy, H * 0.12, e); sp.al = 1 - k;
      } else { fx.spirit = null; return; }
      if (Math.random() < dt * 9) addSpark(sp.x + rnd(-12, 12) * U, sp.y + rnd(-14, 10) * U, rnd(-6, 6) * U, rnd(4, 18) * U, rnd(0.6, 1.3), rnd(1, 1.9) * U, [205, 240, 255], 10);
    }
  }

  // ================= 描画 =================
  const SLOTS = [
    { x: 1,   y: -20, a: -1.32, l: 58, at: 62 },
    { x: -13, y: -10, a: -0.92, l: 62, at: 26 },
    { x: 14,  y: -11, a: 0.95,  l: 60, at: 44 },
    { x: -20, y: 5,   a: -0.3,  l: 74, at: -10 },
    { x: 21,  y: 6,   a: 0.28,  l: 72, at: -10 },
    { x: 0,   y: 12,  a: 0.04,  l: 84, at: 10 },
  ];
  const STONES = (() => {
    const R = seeded(5), out = [];
    for (let i = 0; i < 11; i++) out.push({ a: (i / 11) * TAU + 0.18, rx: 11 + R() * 5, ry: 7 + R() * 3, sh: 0.8 + R() * 0.4 });
    return out.sort((p, q) => Math.sin(p.a) - Math.sin(q.a));
  })();

  function drawLog(x, y, ang, len, th, freshness, alpha, glow) {
    g.save(); g.translate(x, y); g.rotate(ang); g.globalAlpha = alpha;
    g.fillStyle = rgb(mix([34, 22, 15], [128, 88, 52], freshness));
    rr(-len / 2, -th / 2, len, th, th * 0.4); g.fill();
    g.strokeStyle = freshness > 0.5 ? 'rgba(60,36,18,.55)' : 'rgba(0,0,0,.5)'; g.lineWidth = Math.max(1, U);
    g.beginPath();
    g.moveTo(-len * 0.38, -th * 0.2); g.lineTo(len * 0.26, -th * 0.2);
    g.moveTo(-len * 0.3, th * 0.18); g.lineTo(len * 0.32, th * 0.18);
    g.stroke();
    g.fillStyle = rgb(mix([70, 44, 28], [214, 172, 116], freshness));
    g.beginPath(); g.ellipse(len / 2 - th * 0.2, 0, th * 0.2, th * 0.42, 0, 0, TAU); g.fill();
    if (glow > 0) { // 下側が赤く熾る
      g.globalCompositeOperation = 'lighter';
      const gr = g.createLinearGradient(0, -th / 2, 0, th / 2);
      gr.addColorStop(0, 'rgba(255,110,20,0)'); gr.addColorStop(1, 'rgba(255,110,20,1)');
      g.globalAlpha = alpha * glow * (0.7 + 0.3 * Math.sin(T * 3 + x));
      g.fillStyle = gr; rr(-len / 2, -th / 2, len, th, th * 0.4); g.fill();
    }
    g.restore();
  }

  function drawStones(front, pal) {
    const base = mix([38, 36, 40], [124, 120, 112], dayness);
    for (const s of STONES) {
      const sy = Math.sin(s.a);
      if ((sy > 0.05) !== front) continue;
      const x = cx + Math.cos(s.a) * 68 * U, y = baseY + 10 * U + sy * 18 * U;
      g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1;
      g.fillStyle = rgb([base[0] * s.sh, base[1] * s.sh, base[2] * s.sh]);
      g.beginPath(); g.ellipse(x, y, s.rx * U, s.ry * U, 0, 0, TAU); g.fill();
      g.globalCompositeOperation = 'lighter';
      g.globalAlpha = (0.1 + 0.3 * Math.min(1, I)) * flick * (0.3 + 0.7 * lit) * (front ? 0.45 : 1);
      g.fillStyle = rgb(pal.glow);
      g.beginPath(); g.ellipse(x, y - s.ry * U * 0.25, s.rx * U * 0.85, s.ry * U * 0.6, 0, 0, TAU); g.fill();
    }
    g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1;
  }

  function drawSpirit() {
    const sp = fx.spirit;
    if (!sp || sp.s <= 0.02 || sp.al <= 0.02) return;
    const fire = sp.kind === 'fire';
    const s = 15 * U * sp.s, x = sp.x, y = sp.y + Math.sin(T * 5) * 1.5 * U;
    g.globalCompositeOperation = 'lighter';
    blob(fire ? [255, 170, 60] : [150, 220, 255], x, y - s * 0.3, s * 3.6, 0.5 * sp.al);
    g.globalCompositeOperation = 'source-over';
    g.save(); g.translate(x, y); g.globalAlpha = sp.al;
    const wob = Math.sin(T * 8 + 1) * (fire ? 0.35 : 0.1) * s, tip = -s * (fire ? 2.1 : 1.75);
    g.beginPath();
    g.moveTo(wob, tip);
    g.bezierCurveTo(s * 0.55, -s * 1.1, s * 1.05, -s * 0.35, s, s * 0.2);
    g.bezierCurveTo(s * 0.95, s, -s * 0.95, s, -s, s * 0.2);
    g.bezierCurveTo(-s * 1.05, -s * 0.35, -s * 0.55, -s * 1.1, wob, tip);
    const gr = g.createRadialGradient(0, s * 0.25, s * 0.1, 0, 0, s * 1.9);
    if (fire) { gr.addColorStop(0, '#fff7cf'); gr.addColorStop(0.45, '#ffc04a'); gr.addColorStop(1, '#ff5a12'); }
    else { gr.addColorStop(0, 'rgba(255,255,255,.98)'); gr.addColorStop(0.5, 'rgba(196,236,255,.94)'); gr.addColorStop(1, 'rgba(96,172,240,.88)'); }
    g.fillStyle = gr; g.fill();
    if (!fire) { g.fillStyle = 'rgba(255,255,255,.85)'; g.beginPath(); g.ellipse(-s * 0.45, -s * 0.4, s * 0.13, s * 0.26, 0.5, 0, TAU); g.fill(); }
    const ink = fire ? '#4a1e08' : '#1d4a78';
    const blink = Math.sin(T * 1.7 + 0.5) > 0.985 ? 0.15 : 1;
    g.fillStyle = ink;
    for (const d of [-1, 1]) { g.beginPath(); g.ellipse(d * s * 0.34, s * 0.12, s * 0.11, s * 0.17 * blink, 0, 0, TAU); g.fill(); }
    g.fillStyle = fire ? 'rgba(255,80,50,.5)' : 'rgba(255,150,170,.6)';
    for (const d of [-1, 1]) { g.beginPath(); g.arc(d * s * 0.6, s * 0.4, s * 0.13, 0, TAU); g.fill(); }
    g.strokeStyle = ink; g.lineWidth = Math.max(1, s * 0.07); g.lineCap = 'round';
    g.beginPath(); g.arc(0, s * 0.3, s * 0.14, 0.15 * Math.PI, 0.85 * Math.PI); g.stroke();
    g.restore();
  }

  function draw() {
    g.setTransform(DPR, 0, 0, DPR, 0, 0);
    g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1;
    const day = dayness, night = 1 - day, pal = PAL[palName()], Ic = Math.min(1, I);

    // --- 空 ---
    if (day < 1) { g.fillStyle = skyN; g.fillRect(0, 0, W, H); }
    if (day > 0) { g.globalAlpha = day; g.fillStyle = skyD; g.fillRect(0, 0, W, H); g.globalAlpha = 1; }
    if (night > 0.02) {
      g.fillStyle = '#fff';
      for (const s of stars) { g.globalAlpha = night * s.a * (0.55 + 0.45 * Math.sin(T * s.sp + s.ph)); g.fillRect(s.x, s.y, s.r, s.r); }
      const mx = W * 0.78, my = Math.max(H * 0.15, 96);
      g.globalCompositeOperation = 'lighter';
      blob([190, 205, 255], mx, my, 64 * U, 0.2 * night);
      g.globalCompositeOperation = 'source-over';
      g.globalAlpha = night; g.drawImage(moonCv, mx - moonR, my - moonR, moonR * 2, moonR * 2);
    }
    if (day > 0.02) {
      g.globalCompositeOperation = 'lighter';
      blob([255, 206, 150], W * 0.22, horizon - 44 * U, 170 * U, 0.38 * day);
      blob([255, 244, 214], W * 0.22, horizon - 44 * U, 34 * U, 0.85 * day);
      g.globalCompositeOperation = 'source-over';
      const cs = sprite([255, 255, 255]);
      g.globalAlpha = 0.42 * day;
      for (const c of clouds) g.drawImage(cs, c.x - c.w, c.y - c.h, c.w * 2, c.h * 2);
    }
    if (fx.rain) { // 虹
      const a = clamp(fx.rain.age / 3, 0, 1) * clamp((26 - fx.rain.age) / 4, 0, 1) * (0.25 + 0.75 * day);
      const cols = ['255,80,80', '255,160,60', '255,230,90', '110,220,120', '90,170,255', '120,110,240', '180,110,230'];
      const lw = 4.6 * U, R0 = W * 0.6;
      g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1; g.lineWidth = lw + 0.6; g.lineCap = 'butt';
      cols.forEach((c, i) => {
        g.strokeStyle = `rgba(${c},${0.42 * a})`;
        g.beginPath(); g.arc(W * 0.5, horizon + 40 * U, R0 - i * lw, Math.PI, TAU); g.stroke();
      });
    }
    g.globalCompositeOperation = 'lighter'; g.lineCap = 'round';
    for (const s of shoots) { // 流れ星
      if (s.delay > 0) continue;
      const k = s.age / s.dur, a = Math.sin(k * Math.PI) * (0.35 + 0.65 * night);
      const hx = s.x + s.vx * s.age, hy = s.y + s.vy * s.age, sp = Math.hypot(s.vx, s.vy);
      const len = s.len * Math.min(1, k * 3), tx = hx - (s.vx / sp) * len, ty = hy - (s.vy / sp) * len;
      const gr = g.createLinearGradient(hx, hy, tx, ty);
      gr.addColorStop(0, `rgba(255,255,255,${a})`); gr.addColorStop(1, 'rgba(160,190,255,0)');
      g.globalAlpha = 1; g.strokeStyle = gr; g.lineWidth = 2 * U;
      g.beginPath(); g.moveTo(hx, hy); g.lineTo(tx, ty); g.stroke();
      blob([220, 232, 255], hx, hy, 7 * U, a);
    }

    // --- 山・森・地面 ---
    g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1;
    if (day < 1) g.drawImage(layerN, 0, 0, W, H);
    if (day > 0) { g.globalAlpha = day; g.drawImage(layerD, 0, 0, W, H); g.globalAlpha = 1; }

    // --- 火の明かり ---
    g.globalCompositeOperation = 'lighter';
    g.save(); g.translate(cx, baseY + 12 * U); g.scale(1, 0.3);
    blob(pal.glow, 0, 0, (130 + 250 * Ic) * U, (0.16 + 0.34 * Ic) * flick * (1 - 0.55 * day) * (0.35 + 0.65 * lit));
    g.restore();
    blob(pal.glow, cx, baseY - flameH * 0.45, (90 + 230 * Ic) * U, (0.06 + 0.2 * Ic) * flick * lit * (1 - 0.5 * day));

    drawStones(false, pal);

    // --- 熾と薪 ---
    g.globalCompositeOperation = 'lighter';
    g.save(); g.translate(cx, baseY + 8 * U); g.scale(1, 0.34);
    const pulse = 0.78 + 0.22 * Math.sin(T * 2.1);
    blob([255, 80, 20], 0, 0, 74 * U, (0.5 + 0.3 * lit) * pulse);
    blob([255, 190, 80], 0, 0, 38 * U, (0.22 + 0.5 * lit) * pulse);
    g.restore();
    g.globalCompositeOperation = 'source-over';
    for (const s of SLOTS) {
      const a = clamp((S.fuel - s.at) / 6, 0, 1);
      if (a > 0) drawLog(cx + s.x * U, baseY + s.y * U, s.a, s.l * U, 13 * U, 0, a, 0.25 + 0.5 * lit);
    }
    for (const L of landed) drawLog(L.x, L.y, L.rot, 58 * U, 13 * U, L.a, L.a, 0);

    // --- 煙 ---
    const smokeC = mix([150, 150, 162], [238, 238, 238], day);
    beginBuf(sBuf);
    for (const p of smokes) { const k = p.age / p.life; blob(smokeC, p.x, p.y, p.size * (1 + k * 2.4), Math.sin(k * Math.PI) * (0.06 + 0.07 * day)); }
    endBuf(sBuf, 'source-over', 1);

    // --- 炎（加算で描きためてから、下地 → 加算の2度重ね。朝の明るい空でも色が残る） ---
    const dim = 0.78 - 0.34 * Ic; // 大きい火ほど粒が重なるので1粒を淡く
    beginBuf(fBuf);
    g.globalCompositeOperation = 'lighter';
    for (const p of flames) {
      const k = p.age / p.life, a = (k < 0.12 ? k / 0.12 : 1) * (1 - k) * dim, s = p.size * (1 - 0.7 * k), c = PAL[p.pal];
      blob(c.edge, p.x, p.y, s, a * 0.5);
      blob(c.core, p.x, p.y + s * 0.12, s * 0.5, a * (1 - k) * 0.6);
    }
    endBuf(fBuf, 'source-over', 0.22 + 0.5 * day);
    endBuf(fBuf, 'lighter', 1 - 0.3 * day);

    drawStones(true, pal);

    // --- 火の粉 ---
    g.globalCompositeOperation = 'lighter';
    for (const p of sparks) {
      const k = p.age / p.life;
      blob(p.c, p.x, p.y, p.size * 2.2, (1 - k) * (0.65 + 0.35 * Math.sin(T * 22 + p.seed)));
    }
    g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1;

    drawSpirit();

    for (const L of flying) { const k = L.t / L.dur, sc = 1.25 - 0.35 * k; drawLog(L.x, L.y, L.rot, 58 * U * sc, 13 * U * sc, 1, 1, 0); }

    // --- 花火 ---
    g.globalCompositeOperation = 'lighter';
    if (fx.flash > 0) blob(fx.flashC, fx.flashX, fx.flashY, W * 0.9, fx.flash);
    for (const r of rockets) { if (r.delay > 0) continue; blob(r.c, r.x, r.y, 9 * U, 0.7); blob([255, 255, 240], r.x, r.y, 4 * U, 1); }
    g.lineCap = 'round';
    for (let i = 0; i < fw.length; i++) {
      const p = fw[i], k = p.age / p.life;
      let a = Math.pow(1 - k, 1.4);
      if (p.tw || k > 0.55) a *= 0.55 + 0.45 * Math.sin(T * 34 + i * 1.7);
      g.globalAlpha = 1; g.strokeStyle = rgba(p.c, a.toFixed(3)); g.lineWidth = (p.big ? 2.4 : 2) * U;
      g.beginPath(); g.moveTo(p.x - p.vx * 0.1, p.y - p.vy * 0.1); g.lineTo(p.x, p.y); g.stroke();
      blob(p.c, p.x, p.y, (p.big ? 5 : 4) * U, a);
    }
    g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1;
  }

  // ================= 画面まわり（DOM） =================
  const elStage = $('stage'), elCount = $('count'), elRing = $('ring'), elToast = $('toast'), elHint = $('hint');
  const ICON = {
    moon: '<svg viewBox="0 0 24 24"><path d="M20 14.5A8 8 0 1 1 9.5 4a6.5 6.5 0 0 0 10.5 10.5z"/></svg>',
    sun: '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="4"/><path d="M12 3v2M12 19v2M3 12h2M19 12h2M5.6 5.6 7 7M17 17l1.4 1.4M5.6 18.4 7 17M17 7l1.4-1.4"/></svg>',
    on: '<svg viewBox="0 0 24 24"><path d="M4 9.5v5h3.5L12 18V6L7.5 9.5z"/><path d="M15.5 9a4 4 0 0 1 0 6M18 6.5a7.5 7.5 0 0 1 0 11"/></svg>',
    off: '<svg viewBox="0 0 24 24"><path d="M4 9.5v5h3.5L12 18V6L7.5 9.5z"/><path d="M16 9.5l5 5M21 9.5l-5 5"/></svg>',
  };
  const SKY_LABEL = { auto: '自動', night: '夜', day: '朝' };

  function skyTarget() {
    if (S.set.sky === 'night') return 0;
    if (S.set.sky === 'day') return 1;
    const h = new Date().getHours();
    return h >= 5 && h < 17 ? 1 : 0; // 自動: 5:00〜16:59 を朝とする
  }

  let lastStage = '', lastCount = -1, lastRing = -1;
  function updHud() {
    const st = stageOf(S.fuel);
    if (st !== lastStage) { lastStage = st; elStage.textContent = st; }
    if (S.totalLogs !== lastCount) { lastCount = S.totalLogs; elCount.textContent = `くべた薪 ${S.totalLogs.toLocaleString('ja-JP')}本`; }
    const ring = Math.round(S.fuel * 2);
    if (ring !== lastRing) { lastRing = ring; elRing.style.strokeDashoffset = (295.31 * (1 - ring / 200)).toFixed(1); }
  }

  let toastTimer = 0;
  function toast(stars, title, sub, ms) {
    const b = elToast.querySelector('b');
    b.textContent = '';
    if (stars) { const sp = document.createElement('span'); sp.textContent = stars; b.appendChild(sp); }
    b.appendChild(document.createTextNode(title));
    elToast.querySelector('small').textContent = sub || '';
    elToast.classList.add('show'); app.classList.add('toasting');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { elToast.classList.remove('show'); app.classList.remove('toasting'); }, ms || 4000);
  }
  function hideHint() { elHint.classList.add('gone'); }

  function syncButtons() {
    $('icoSound').innerHTML = S.set.sound ? ICON.on : ICON.off;
    $('lblSound').textContent = S.set.sound ? '音・ON' : '音・OFF';
    $('btnSound').classList.toggle('active', S.set.sound);
    $('icoSky').innerHTML = skyTarget() ? ICON.sun : ICON.moon;
    $('lblSky').textContent = '空・' + SKY_LABEL[S.set.sky];
    const found = RARES.filter((r) => S.col[r.id]).length;
    $('lblBook').textContent = `図鑑 ${found}/${RARES.length}`;
    $('dot').classList.toggle('on', !!S.unseen);
  }
  function setSound(v) { S.set.sound = v; Sound.set(v); save(); syncButtons(); }
  function setSky(m) { S.set.sky = m; dayTarget = skyTarget(); save(); syncButtons(); }

  // --- 薪をくべる操作（タップ／長押しで連続） ---
  const btnFeed = $('btnFeed');
  let holdDelay = 0, holdTimer = 0;
  function startFeed(e, fromBtn) {
    if (e.button > 0) return;
    e.preventDefault();
    Sound.unlock();
    const px = fromBtn ? cx : e.clientX - app.getBoundingClientRect().left;
    throwLog(px);
    endFeed();
    if (fromBtn) btnFeed.classList.add('down');
    holdDelay = setTimeout(() => { holdTimer = setInterval(() => throwLog(px + rnd(-20, 20)), 250); }, 380);
  }
  function endFeed() { clearTimeout(holdDelay); clearInterval(holdTimer); btnFeed.classList.remove('down'); }
  cv.addEventListener('pointerdown', (e) => startFeed(e, false));
  btnFeed.addEventListener('pointerdown', (e) => startFeed(e, true));
  for (const ev of ['pointerup', 'pointercancel', 'blur']) window.addEventListener(ev, endFeed);
  window.addEventListener('keydown', (e) => {
    if ((e.key === ' ' || e.key === 'Enter') && !e.repeat && !app.classList.contains('sheet')) { e.preventDefault(); Sound.unlock(); throwLog(cx); }
  });

  $('btnSound').addEventListener('click', () => {
    setSound(!S.set.sound);
    toast('', S.set.sound ? '音 ON' : '音 OFF', S.set.sound ? '消音モードのときは鳴りません' : '', 1800);
  });
  $('btnSky').addEventListener('click', () => {
    const order = ['auto', 'night', 'day'];
    setSky(order[(order.indexOf(S.set.sky) + 1) % 3]);
    toast('', '空：' + SKY_LABEL[S.set.sky], S.set.sky === 'auto' ? '時刻に合わせて朝と夜が変わります' : '', 1800);
  });

  // --- 図鑑・ログ・設定 ---
  const fmtDate = (t) => { const d = new Date(t), p = (n) => String(n).padStart(2, '0'); return `${d.getMonth() + 1}/${d.getDate()} ${p(d.getHours())}:${p(d.getMinutes())}`; };
  const fmtDay = (t) => { const d = new Date(t); return `${d.getFullYear()}/${d.getMonth() + 1}/${d.getDate()}`; };
  const fmtDur = (s) => { s = Math.floor(s); const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60); return h ? `${h}時間${m}分` : m ? `${m}分` : `${s}秒`; };
  const orbCss = (r) => (r.orb
    ? `background:radial-gradient(circle at 50% 32%,${r.orb[0]},${r.orb[1]} 55%,${r.orb[2]});box-shadow:0 0 14px ${r.orb[1]}66`
    : 'background:conic-gradient(#ff5a5a,#ffb23c,#ffe66a,#5fd86f,#5aa8ff,#9a6aff,#ff5a5a);box-shadow:0 0 14px #ffffff55');
  const el = (tag, cls, text) => { const n = document.createElement(tag); if (cls) n.className = cls; if (text != null) n.textContent = text; return n; };

  let sheetTab = 'book', selId = null, wipeArm = 0;
  function openSheet(tab) { sheetTab = tab; endFeed(); app.classList.add('sheet'); $('sheet').setAttribute('aria-hidden', 'false'); renderSheet(); $('sheetBody').scrollTop = 0; }
  function closeSheet() { app.classList.remove('sheet'); $('sheet').setAttribute('aria-hidden', 'true'); }
  function renderSheet() {
    for (const t of ['book', 'log', 'set']) $('pane' + t[0].toUpperCase() + t.slice(1)).classList.toggle('on', t === sheetTab);
    document.querySelectorAll('#tabs [data-tab]').forEach((b) => b.classList.toggle('on', b.dataset.tab === sheetTab));
    if (sheetTab === 'book') { renderBook(); if (S.unseen) { S.unseen = false; save(); syncButtons(); } }
    else if (sheetTab === 'log') renderLog();
    else renderSet();
  }
  function renderBook() {
    const pane = $('paneBook'); pane.textContent = '';
    const found = RARES.filter((r) => S.col[r.id]).length;
    const h = el('h2', '', '図鑑'); h.appendChild(el('small', '', `${found} / ${RARES.length}`)); pane.appendChild(h);
    const grid = el('div', 'grid'), d = el('div'); d.id = 'detail';
    const show = () => {
      grid.querySelectorAll('.card').forEach((c) => c.classList.toggle('sel', c.dataset.id === selId));
      d.textContent = '';
      const r = RMAP[selId], c = r && S.col[r.id];
      if (!r) { d.appendChild(el('small', '', 'カードをタップすると、説明が見られます。')); return; }
      const b = el('b', '', c ? r.name : '？？？'); b.appendChild(el('span', 'st', '★'.repeat(r.star)));
      d.append(b, el('div', '', c ? r.desc : r.hint));
      if (c) d.appendChild(el('small', '', `はじめて出会った日：${fmtDate(c.first)}　／　${c.n}回`));
    };
    for (const r of RARES) {
      const c = S.col[r.id];
      const card = el('button', 'card' + (c ? '' : ' lock')); card.dataset.id = r.id;
      const orb = el('span', 'orb', c ? r.glyph : '？'); orb.style.cssText = orbCss(r);
      const meta = el('span', 'ct'); meta.append(el('i', 'st', '★'.repeat(r.star)), document.createTextNode(c ? ` ${c.n}回` : ''));
      card.append(orb, el('span', 'nm', c ? r.name : '？？？'), meta);
      card.addEventListener('click', () => { selId = r.id; show(); });
      grid.appendChild(card);
    }
    pane.append(grid, d);
    show();
  }
  function renderLog() {
    const pane = $('paneLog'); pane.textContent = '';
    pane.appendChild(el('h2', '', 'できごとログ'));
    if (!S.log.length) { pane.appendChild(el('div', 'empty', 'まだ何も起きていません。\n火を育てて、気長に待ちましょう。')); pane.lastChild.style.whiteSpace = 'pre-line'; return; }
    for (const e of S.log) {
      const r = RMAP[e.id]; if (!r) continue;
      const row = el('div', 'logrow');
      row.append(el('time', '', fmtDate(e.t)), el('span', 'st', '★'.repeat(r.star)), el('span', '', r.name));
      pane.appendChild(row);
    }
  }
  function renderSet() {
    const pane = $('paneSet'); pane.textContent = '';
    pane.appendChild(el('h2', '', '設定'));
    const row = (label, ctl) => { const r = el('div', 'row'); r.append(el('span', '', label), ctl); pane.appendChild(r); };
    const tog = (v, fn) => { const b = el('button', 'tog' + (v ? ' on' : '')); b.setAttribute('role', 'switch'); b.setAttribute('aria-checked', String(v)); b.addEventListener('click', () => { fn(!v); renderSet(); }); return b; };
    row('音（焚き火の音・効果音）', tog(S.set.sound, setSound));
    if (navigator.vibrate) row('振動', tog(S.set.vib, (v) => { S.set.vib = v; save(); }));
    const seg = el('div', 'seg');
    for (const m of ['auto', 'night', 'day']) {
      const b = el('button', S.set.sky === m ? 'on' : '', SKY_LABEL[m]);
      b.addEventListener('click', () => { setSky(m); renderSet(); });
      seg.appendChild(b);
    }
    row('空', seg);
    pane.appendChild(el('h2', '', 'きろく')).style.marginTop = '20px';
    const dl = el('dl', 'stats');
    const stat = (k, v) => dl.append(el('dt', '', k), el('dd', '', v));
    stat('くべた薪', `${S.totalLogs.toLocaleString('ja-JP')}本`);
    stat('火を燃やした時間', fmtDur(S.burnSec));
    stat('見つけた出来事', `${RARES.filter((r) => S.col[r.id]).length} / ${RARES.length}`);
    stat('はじめて火をおこした日', fmtDay(S.firstDay));
    pane.appendChild(dl);
    const armed = Date.now() < wipeArm;
    const wipe = el('button', 'danger' + (armed ? ' arm' : ''), armed ? '本当に消す（もう一度タップ）' : '記録をすべて消す');
    wipe.addEventListener('click', () => {
      if (Date.now() < wipeArm) {
        const keep = S.set; S = fresh(); S.set = keep; wipeArm = 0; selId = null; lastCount = -1;
        fx.flame = null; fx.flameT = 0; save(); syncButtons(); renderSet(); toast('', '記録を消しました', '', 2000);
      } else { wipeArm = Date.now() + 4000; renderSet(); setTimeout(() => { if (sheetTab === 'set' && app.classList.contains('sheet')) renderSet(); }, 4100); }
    });
    pane.appendChild(wipe);
    pane.appendChild(el('p', 'note', '課金・広告・通信は一切ありません。記録はこの端末のブラウザ内（localStorage）にだけ保存されます。'));
  }
  $('btnBook').addEventListener('click', () => openSheet('book'));
  $('btnSet').addEventListener('click', () => openSheet('set'));
  $('btnClose').addEventListener('click', closeSheet);
  $('backdrop').addEventListener('click', closeSheet);
  document.querySelectorAll('#tabs [data-tab]').forEach((b) => b.addEventListener('click', () => { sheetTab = b.dataset.tab; renderSheet(); $('sheetBody').scrollTop = 0; }));
  { // つまみを下にスワイプで閉じる
    let y0 = null;
    const grab = $('grab');
    grab.addEventListener('pointerdown', (e) => { y0 = e.clientY; });
    grab.addEventListener('pointerup', (e) => { if (y0 != null && e.clientY - y0 > 30) closeSheet(); y0 = null; });
    grab.addEventListener('pointercancel', () => { y0 = null; });
  }

  // --- スマホ向けの抑止（スクロール・拡大・長押しメニュー） ---
  document.addEventListener('touchmove', (e) => { if (!e.target.closest || !e.target.closest('#sheetBody')) e.preventDefault(); }, { passive: false });
  for (const ev of ['contextmenu', 'gesturestart', 'dblclick']) document.addEventListener(ev, (e) => e.preventDefault());

  // --- 中断と再開 ---
  let hiddenAt = 0;
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) { hiddenAt = Date.now(); endFeed(); save(); Sound.pause(true); }
    else { if (hiddenAt) decayAway((Date.now() - hiddenAt) / 1000); hiddenAt = 0; lastT = 0; Sound.pause(false); }
  });
  window.addEventListener('pagehide', save);
  let resizeT = 0;
  window.addEventListener('resize', () => { clearTimeout(resizeT); resizeT = setTimeout(resize, 120); });

  // ================= 起動 =================
  let lastT = 0;
  function frame(now) {
    requestAnimationFrame(frame);
    const dt = lastT ? Math.min(0.05, (now - lastT) / 1000) : 0.016;
    lastT = now; T += dt;
    update(dt); draw();
  }

  resize();
  dayTarget = skyTarget(); dayness = dayTarget;
  I = clamp(Math.pow(S.fuel / 100, 0.8) * 0.92, 0, 1);
  syncButtons(); updHud();
  if (S.totalLogs > 0) {
    hideHint();
    if (awaySec > 120) toast('', 'おかえりなさい', S.fuel < 5 ? '熾火が、まだ赤く残っています' : '火はまだ燃えています', 3600);
  }
  if (DEBUG) { // ?debug を付けると演出をコンソールから試せる（記録には残らない）
    window.takibi = { fx: (id) => trigger(id, false), fuel: (n) => { S.fuel = clamp(n, 0, 100); }, sky: setSky, ids: RARES.map((r) => r.id), state: () => S, pick: pickRare };
  }
  requestAnimationFrame(frame);
})();
