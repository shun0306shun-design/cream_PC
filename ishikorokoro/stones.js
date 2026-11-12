/* 石ころころ — 石の図鑑データと描画（外部依存なし・通信なし） */
(function (global) {
  'use strict';

  const TAU = Math.PI * 2;
  const NS = 72; // 輪郭のサンプル数
  const COS = new Float32Array(NS), SIN = new Float32Array(NS);
  for (let i = 0; i < NS; i++) { COS[i] = Math.cos(i / NS * TAU); SIN[i] = Math.sin(i / NS * TAU); }

  const clamp = (v, a, b) => (v < a ? a : v > b ? b : v);

  function rng(seed) {
    let a = seed >>> 0;
    return function () {
      a = (a + 0x6D2B79F5) | 0;
      let t = Math.imul(a ^ (a >>> 15), 1 | a);
      t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }
  function hash(str) {
    let h = 2166136261;
    for (let i = 0; i < str.length; i++) { h ^= str.charCodeAt(i); h = Math.imul(h, 16777619); }
    return h >>> 0;
  }
  function rgba(hex, a) {
    const n = parseInt(hex.slice(1), 16);
    return 'rgba(' + (n >> 16) + ',' + ((n >> 8) & 255) + ',' + (n & 255) + ',' + a + ')';
  }

  /* ---------- 形 ---------- */
  // 平均半径 1 の輪郭 R と、接地計算用の支持関数 H（＝中心の高さ）を返す
  function buildShape(sd, seed, polish) {
    const rnd = rng(seed ^ 0x9e3779b9);
    const n = sd.n || 10;
    const keep = sd.keep == null ? 0.3 : sd.keep; // 磨いても残るでこぼこ
    const irr = sd.irr * (keep + (1 - keep) * (1 - polish));
    const ctrl = [];
    for (let i = 0; i < n; i++) ctrl.push(rnd() * 2 - 1);
    const rot = rnd() * TAU;
    const R = new Float32Array(NS);
    let sum = 0;
    for (let i = 0; i < NS; i++) {
      const th = i / NS * TAU;
      const f = ((th + rot) / TAU) * n;
      const i1 = Math.floor(f), t = f - i1;
      const p0 = ctrl[(i1 - 1 + n * 4) % n], p1 = ctrl[i1 % n], p2 = ctrl[(i1 + 1) % n], p3 = ctrl[(i1 + 2) % n];
      const noise = 0.5 * (2 * p1 + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
      let b = 1;
      if (sd.type === 'flat') {
        const ea = 1.2, eb = 0.72;
        b = ea * eb / Math.sqrt(Math.pow(eb * COS[i], 2) + Math.pow(ea * SIN[i], 2));
      } else if (sd.type === 'tri') {
        b = 1 + 0.15 * Math.cos(3 * (th + Math.PI / 2));
      }
      R[i] = b * (1 + noise * irr);
      sum += R[i];
    }
    const m = sum / NS;
    for (let i = 0; i < NS; i++) R[i] /= m;

    const Hs = new Float32Array(NS), dH = new Float32Array(NS);
    let hmin = 9, hmax = 0;
    for (let k = 0; k < NS; k++) {
      let best = 0;
      for (let j = 0; j < NS; j++) {
        let d = j - k; if (d < 0) d += NS;
        const v = R[j] * COS[d];
        if (v > best) best = v;
      }
      Hs[k] = best;
      if (best < hmin) hmin = best;
      if (best > hmax) hmax = best;
    }
    const dphi = TAU / NS, c2 = new Float32Array(NS), K = new Float32Array(NS);
    for (let k = 0; k < NS; k++) {
      const a = Hs[(k + NS - 1) % NS], b = Hs[(k + 1) % NS];
      dH[k] = (b - a) / (2 * dphi);
      c2[k] = Math.max(0, (a + b - 2 * Hs[k]) / dphi);
    }
    // K = かどの鋭さ（面がぺたんと着くときに、どれだけ勢いが逃げるか）
    for (let k = 0; k < NS; k++) K[k] = Math.max(c2[(k + NS - 1) % NS], c2[k], c2[(k + 1) % NS]);
    return { R: R, H: Hs, dH: dH, K: K, rough: hmax - hmin };
  }

  function sample(arr, phi) {
    let f = phi / TAU * NS;
    f -= Math.floor(f / NS) * NS;
    const i = f | 0, t = f - i;
    return arr[i % NS] * (1 - t) + arr[(i + 1) % NS] * t;
  }

  function tracePath(c, R) {
    c.beginPath();
    for (let i = 0; i < NS; i++) {
      const x = COS[i] * R[i], y = SIN[i] * R[i];
      if (i) c.lineTo(x, y); else c.moveTo(x, y);
    }
    c.closePath();
  }

  /* ---------- もよう部品（半径1の座標系で描く） ---------- */
  function base(c, inner, outer) {
    const g = c.createRadialGradient(0, 0, 0.1, 0, 0, 1.3);
    g.addColorStop(0, inner); g.addColorStop(1, outer);
    c.fillStyle = g; c.fillRect(-1.7, -1.7, 3.4, 3.4);
  }
  function speckle(c, rnd, n, size, colors, alpha) {
    for (let i = 0; i < n; i++) {
      const a = rnd() * TAU, d = Math.sqrt(rnd()) * 1.3, s = size * (0.4 + rnd() * 1.1);
      c.globalAlpha = alpha * (0.5 + rnd() * 0.5);
      c.fillStyle = colors[(rnd() * colors.length) | 0];
      c.beginPath();
      c.ellipse(Math.cos(a) * d, Math.sin(a) * d, s, s * (0.6 + rnd() * 0.5), rnd() * 3.14, 0, TAU);
      c.fill();
    }
    c.globalAlpha = 1;
  }
  function blotch(c, rnd, n, size, color, alpha) {
    for (let i = 0; i < n; i++) {
      const a = rnd() * TAU, d = Math.sqrt(rnd()), s = size * (0.6 + rnd() * 0.8);
      const x = Math.cos(a) * d, y = Math.sin(a) * d;
      const g = c.createRadialGradient(x, y, 0, x, y, s);
      g.addColorStop(0, rgba(color, alpha)); g.addColorStop(1, rgba(color, 0));
      c.fillStyle = g; c.beginPath(); c.arc(x, y, s, 0, TAU); c.fill();
    }
  }
  function stripes(c, rnd, n, colors, wMin, wMax, alpha) {
    c.save(); c.rotate(rnd() * TAU); c.lineCap = 'round';
    for (let i = 0; i < n; i++) {
      const y = -1.25 + (i + 0.5 + (rnd() - 0.5) * 0.6) / n * 2.5, bend = (rnd() - 0.5) * 0.5;
      c.lineWidth = wMin + rnd() * (wMax - wMin);
      c.strokeStyle = colors[i % colors.length];
      c.globalAlpha = alpha * (0.7 + rnd() * 0.3);
      c.beginPath(); c.moveTo(-1.6, y); c.quadraticCurveTo(0, y + bend, 1.6, y + (rnd() - 0.5) * 0.2); c.stroke();
    }
    c.restore();
  }
  function vein(c, rnd, color, w, alpha) {
    c.save(); c.rotate(rnd() * TAU);
    const y = (rnd() - 0.5) * 0.9;
    c.lineWidth = w; c.strokeStyle = color; c.globalAlpha = alpha; c.lineCap = 'round';
    c.beginPath(); c.moveTo(-1.6, y);
    c.bezierCurveTo(-0.5, y + (rnd() - 0.5) * 0.6, 0.5, y + (rnd() - 0.5) * 0.6, 1.6, y + (rnd() - 0.5) * 0.5);
    c.stroke(); c.restore();
  }
  function layers(c, rnd, n, color) {
    c.save(); c.rotate(rnd() * TAU); c.strokeStyle = color; c.lineCap = 'round';
    for (let i = 0; i < n; i++) {
      const y = -1.2 + (i + 0.5) / n * 2.4 + (rnd() - 0.5) * 0.08;
      c.globalAlpha = 0.18 + rnd() * 0.25; c.lineWidth = 0.02 + rnd() * 0.05;
      c.beginPath(); c.moveTo(-1.6, y);
      c.bezierCurveTo(-0.5, y + (rnd() - 0.5) * 0.12, 0.5, y + (rnd() - 0.5) * 0.12, 1.6, y);
      c.stroke();
    }
    c.restore();
  }
  function moss(c, rnd) {
    const a0 = rnd() * TAU, greens = ['#5c8a3c', '#78a548', '#4a7534', '#93bd5c'];
    c.globalAlpha = 0.92;
    for (let i = 0; i < 30; i++) {
      const a = a0 + (rnd() - 0.5) * 2.0, d = 0.55 + rnd() * 0.6, s = 0.1 + rnd() * 0.2;
      c.fillStyle = greens[(rnd() * 4) | 0];
      c.beginPath(); c.arc(Math.cos(a) * d, Math.sin(a) * d, s, 0, TAU); c.fill();
    }
    c.fillStyle = '#c3e088'; c.globalAlpha = 0.8;
    for (let i = 0; i < 26; i++) {
      const a = a0 + (rnd() - 0.5) * 1.9, d = 0.5 + rnd() * 0.6;
      c.beginPath(); c.arc(Math.cos(a) * d, Math.sin(a) * d, 0.02 + rnd() * 0.02, 0, TAU); c.fill();
    }
    c.globalAlpha = 1;
  }
  function holes(c, rnd, n) {
    c.lineWidth = 0.016;
    for (let i = 0; i < n; i++) {
      const a = rnd() * TAU, d = Math.sqrt(rnd()) * 1.1, s = 0.035 + rnd() * 0.075;
      const x = Math.cos(a) * d, y = Math.sin(a) * d;
      c.fillStyle = 'rgba(74,60,44,0.5)';
      c.beginPath(); c.ellipse(x, y, s, s * (0.6 + rnd() * 0.4), rnd() * 3, 0, TAU); c.fill();
      c.strokeStyle = 'rgba(255,250,240,0.4)';
      c.beginPath(); c.arc(x, y, s * 1.08, 0.2, 1.8); c.stroke();
    }
  }
  function cracks(c, rnd, n, color) {
    c.save();
    c.strokeStyle = color; c.lineWidth = 0.028; c.lineJoin = 'round'; c.lineCap = 'round'; c.globalAlpha = 0.55;
    for (let i = 0; i < n; i++) {
      const a = rnd() * TAU;
      let x = Math.cos(a) * 1.2, y = Math.sin(a) * 1.2, dir = a + Math.PI + (rnd() - 0.5) * 0.8;
      c.beginPath(); c.moveTo(x, y);
      const segs = 3 + ((rnd() * 3) | 0);
      for (let j = 0; j < segs; j++) {
        dir += (rnd() - 0.5) * 1.1;
        const l = 0.18 + rnd() * 0.25;
        x += Math.cos(dir) * l; y += Math.sin(dir) * l;
        c.lineTo(x, y);
      }
      c.stroke();
    }
    c.restore();
  }
  function patches(c, rnd, n) {
    for (let i = 0; i < n; i++) {
      const a = rnd() * TAU, d = rnd() * 0.9, x = Math.cos(a) * d, y = Math.sin(a) * d;
      const m = 3 + ((rnd() * 3) | 0), a1 = rnd() * TAU;
      c.beginPath();
      for (let j = 0; j < m; j++) {
        const aa = a1 + j / m * TAU + rnd() * 0.6, rr = 0.25 + rnd() * 0.35;
        c.lineTo(x + Math.cos(aa) * rr, y + Math.sin(aa) * rr);
      }
      c.closePath();
      c.fillStyle = rnd() < 0.5 ? 'rgba(255,245,230,0.15)' : 'rgba(30,20,10,0.17)';
      c.fill();
    }
  }
  function nori(c) {
    c.fillStyle = '#272c27'; c.globalAlpha = 0.93;
    c.beginPath();
    c.moveTo(-0.4, 1.6); c.lineTo(-0.4, 0.3); c.quadraticCurveTo(-0.4, 0.2, -0.3, 0.2);
    c.lineTo(0.3, 0.2); c.quadraticCurveTo(0.4, 0.2, 0.4, 0.3); c.lineTo(0.4, 1.6);
    c.closePath(); c.fill();
    c.globalAlpha = 1;
  }
  function star(c, x, y, s, alpha, color) {
    c.save(); c.translate(x, y); c.globalAlpha = alpha; c.fillStyle = color || '#fff';
    c.beginPath();
    for (let i = 0; i < 8; i++) {
      const a = i / 8 * TAU, r = (i % 2) ? s * 0.22 : s;
      c.lineTo(Math.cos(a) * r, Math.sin(a) * r);
    }
    c.closePath(); c.fill(); c.restore();
  }

  /* ---------- レアのもよう ---------- */
  function gem(col) {
    return function (c, rnd) {
      const g = c.createRadialGradient(-0.12, -0.15, 0.05, 0, 0, 1.3);
      g.addColorStop(0, col.hi); g.addColorStop(0.5, col.mid); g.addColorStop(1, col.lo);
      c.fillStyle = g; c.fillRect(-1.7, -1.7, 3.4, 3.4);
      const n = 7 + ((rnd() * 3) | 0), a0 = rnd() * TAU;
      const cx = (rnd() - 0.5) * 0.24, cy = (rnd() - 0.5) * 0.24, pts = [];
      for (let i = 0; i < n; i++) {
        const a = a0 + (i + (rnd() - 0.5) * 0.5) / n * TAU, d = 0.46 + rnd() * 0.2;
        pts.push([Math.cos(a) * d, Math.sin(a) * d]);
      }
      const lightA = () => 'rgba(255,255,255,' + (0.10 + rnd() * 0.2).toFixed(3) + ')';
      const darkA = () => 'rgba(10,0,40,' + (0.05 + rnd() * 0.13).toFixed(3) + ')';
      for (let i = 0; i < n; i++) {
        const p = pts[i], q = pts[(i + 1) % n];
        c.beginPath(); c.moveTo(cx, cy); c.lineTo(p[0], p[1]); c.lineTo(q[0], q[1]); c.closePath();
        c.fillStyle = (i % 2) ? lightA() : darkA(); c.fill();
        c.beginPath(); c.moveTo(p[0], p[1]); c.lineTo(q[0], q[1]); c.lineTo(q[0] * 3, q[1] * 3); c.lineTo(p[0] * 3, p[1] * 3); c.closePath();
        c.fillStyle = (i % 2) ? darkA() : lightA(); c.fill();
      }
      c.strokeStyle = col.line || 'rgba(255,255,255,0.42)'; c.lineWidth = 0.02; c.lineJoin = 'round';
      c.beginPath();
      for (let i = 0; i < n; i++) {
        const p = pts[i], q = pts[(i + 1) % n];
        c.moveTo(cx, cy); c.lineTo(p[0], p[1]); c.lineTo(q[0], q[1]);
        c.moveTo(p[0], p[1]); c.lineTo(p[0] * 3, p[1] * 3);
      }
      c.stroke();
      for (let i = 0; i < 3; i++) {
        const a = rnd() * TAU, d = rnd() * 0.7;
        star(c, Math.cos(a) * d, Math.sin(a) * d, 0.07 + rnd() * 0.08, 0.85);
      }
    };
  }
  function opal(c, rnd) {
    const g = c.createRadialGradient(-0.1, -0.15, 0.05, 0, 0, 1.3);
    g.addColorStop(0, '#ffffff'); g.addColorStop(0.55, '#d4e6f7'); g.addColorStop(1, '#8fb0d4');
    c.fillStyle = g; c.fillRect(-1.7, -1.7, 3.4, 3.4);
    const hues = [175, 200, 285, 320, 35, 130];
    for (let i = 0; i < 48; i++) {
      const a = rnd() * TAU, d = Math.sqrt(rnd()) * 1.05, x = Math.cos(a) * d, y = Math.sin(a) * d;
      const s = 0.06 + rnd() * 0.15, m = 3 + ((rnd() * 3) | 0), a1 = rnd() * TAU;
      c.beginPath();
      for (let j = 0; j < m; j++) {
        const aa = a1 + j / m * TAU + rnd() * 0.5, rr = s * (0.6 + rnd() * 0.6);
        c.lineTo(x + Math.cos(aa) * rr, y + Math.sin(aa) * rr);
      }
      c.closePath();
      c.fillStyle = 'hsla(' + hues[(rnd() * hues.length) | 0] + ',90%,' + ((58 + rnd() * 16) | 0) + '%,' + (0.35 + rnd() * 0.35).toFixed(2) + ')';
      c.fill();
    }
    const v = c.createRadialGradient(0.2, 0.25, 0.1, 0, 0, 1.2);
    v.addColorStop(0, 'rgba(255,255,255,0.3)'); v.addColorStop(1, 'rgba(255,255,255,0)');
    c.fillStyle = v; c.fillRect(-1.7, -1.7, 3.4, 3.4);
  }
  function goldSkin(c, rnd) {
    const g = c.createRadialGradient(-0.15, -0.2, 0.05, 0, 0, 1.3);
    g.addColorStop(0, '#fff6c2'); g.addColorStop(0.4, '#f3c53c'); g.addColorStop(0.8, '#cc8d14'); g.addColorStop(1, '#8f5d06');
    c.fillStyle = g; c.fillRect(-1.7, -1.7, 3.4, 3.4);
    for (let i = 0; i < 15; i++) {
      const a = rnd() * TAU, d = Math.sqrt(rnd()), x = Math.cos(a) * d, y = Math.sin(a) * d, s = 0.14 + rnd() * 0.2;
      const h = c.createRadialGradient(x - s * 0.3, y - s * 0.3, 0, x, y, s);
      h.addColorStop(0, 'rgba(255,248,196,0.6)'); h.addColorStop(1, 'rgba(255,220,100,0)');
      c.fillStyle = h; c.beginPath(); c.arc(x, y, s, 0, TAU); c.fill();
      c.strokeStyle = 'rgba(120,70,0,0.28)'; c.lineWidth = 0.025;
      c.beginPath(); c.arc(x, y, s * 0.95, 0.3, 1.9); c.stroke();
    }
    speckle(c, rnd, 44, 0.02, ['#fffbe0'], 0.9);
  }
  function fossilBase(c, rnd) {
    base(c, '#cbbda2', '#8c7c65');
    speckle(c, rnd, 50, 0.02, ['#6f604c', '#e6dbc3'], 0.4);
  }
  const F_LIGHT = '#efe3c6', F_DARK = '#5f4f3c';
  function ammonite(c, rnd) {
    fossilBase(c, rnd);
    c.save(); c.rotate(rnd() * TAU);
    const T = 2.7 * TAU, r1 = 0.8, r0 = 0.06, k = Math.log(r1 / r0) / T;
    const rad = (t) => r0 * Math.exp(k * t);
    c.beginPath();
    for (let t = T - TAU; t <= T + 0.001; t += 0.1) c.lineTo(Math.cos(t) * rad(t), Math.sin(t) * rad(t));
    c.closePath(); c.fillStyle = F_LIGHT; c.globalAlpha = 0.93; c.fill();
    c.globalAlpha = 1; c.strokeStyle = F_DARK; c.lineWidth = 0.045; c.lineJoin = 'round'; c.lineCap = 'round';
    c.beginPath();
    for (let t = 0; t <= T + 0.001; t += 0.08) c.lineTo(Math.cos(t) * rad(t), Math.sin(t) * rad(t));
    c.lineTo(Math.cos(T) * rad(T - TAU), Math.sin(T) * rad(T - TAU));
    c.stroke();
    c.lineWidth = 0.022; c.globalAlpha = 0.75; c.beginPath();
    for (let t = TAU * 0.9; t < T - 0.1; t += 0.21) {
      const ro = rad(t) * 0.95, ri = Math.max(rad(t - TAU), r0) * 1.1;
      c.moveTo(Math.cos(t - 0.1) * ri, Math.sin(t - 0.1) * ri);
      c.lineTo(Math.cos(t) * ro, Math.sin(t) * ro);
    }
    c.stroke(); c.restore();
  }
  function trilobite(c, rnd) {
    fossilBase(c, rnd);
    c.save(); c.rotate(rnd() * TAU); c.scale(0.95, 0.95);
    c.beginPath(); c.ellipse(0, 0.02, 0.5, 0.8, 0, 0, TAU);
    c.fillStyle = F_LIGHT; c.globalAlpha = 0.93; c.fill();
    c.globalAlpha = 1; c.strokeStyle = F_DARK; c.lineWidth = 0.04; c.lineCap = 'round'; c.stroke();
    c.lineWidth = 0.032; c.beginPath();
    c.moveTo(-0.47, -0.24); c.quadraticCurveTo(0, -0.4, 0.47, -0.24);
    c.moveTo(-0.16, -0.3); c.quadraticCurveTo(-0.18, 0.3, -0.04, 0.76);
    c.moveTo(0.16, -0.3); c.quadraticCurveTo(0.18, 0.3, 0.04, 0.76);
    c.stroke();
    c.lineWidth = 0.022; c.beginPath();
    for (let i = 0; i < 9; i++) {
      const y = -0.17 + i * 0.105, half = 0.5 * Math.sqrt(Math.max(0, 1 - Math.pow((y - 0.02) / 0.8, 2))) * 0.93;
      c.moveTo(-half, y + 0.035); c.quadraticCurveTo(0, y - 0.05, half, y + 0.035);
    }
    c.stroke();
    c.lineWidth = 0.028; c.beginPath(); c.ellipse(0, -0.5, 0.15, 0.17, 0, 0, TAU); c.stroke();
    c.fillStyle = F_DARK;
    c.beginPath(); c.ellipse(-0.3, -0.46, 0.06, 0.035, -0.5, 0, TAU); c.fill();
    c.beginPath(); c.ellipse(0.3, -0.46, 0.06, 0.035, 0.5, 0, TAU); c.fill();
    c.restore();
  }
  function leaf(c, rnd) {
    base(c, '#d6c9ad', '#9a8a70');
    speckle(c, rnd, 40, 0.02, ['#7a6a54', '#efe5cd'], 0.4);
    c.save(); c.rotate(rnd() * TAU);
    c.beginPath(); c.moveTo(0, -0.84);
    c.bezierCurveTo(0.56, -0.45, 0.5, 0.35, 0, 0.74);
    c.bezierCurveTo(-0.5, 0.35, -0.56, -0.45, 0, -0.84);
    c.fillStyle = 'rgba(70,52,34,0.62)'; c.fill();
    c.strokeStyle = 'rgba(52,38,24,0.8)'; c.lineWidth = 0.03; c.lineJoin = 'round'; c.stroke();
    c.strokeStyle = 'rgba(226,210,180,0.75)'; c.lineCap = 'round';
    c.lineWidth = 0.035; c.beginPath(); c.moveTo(0, -0.74); c.lineTo(0, 0.98); c.stroke();
    c.lineWidth = 0.02; c.beginPath();
    for (let i = 0; i < 6; i++) {
      const y = 0.5 - i * 0.2, w = 0.4 * Math.sin((i + 1.2) / 7.4 * Math.PI);
      c.moveTo(0, y); c.quadraticCurveTo(w * 0.55, y - 0.1, w, y - 0.24);
      c.moveTo(0, y); c.quadraticCurveTo(-w * 0.55, y - 0.1, -w, y - 0.24);
    }
    c.stroke(); c.restore();
  }
  function shell(c, rnd) {
    fossilBase(c, rnd);
    c.save(); c.rotate(rnd() * TAU);
    const hx = 0, hy = 0.52, RR = 1.02, n = 9, span = 2.3;
    const ang = (i) => -Math.PI / 2 + (i / n - 0.5) * span;
    c.beginPath(); c.moveTo(hx - 0.2, hy + 0.02); c.lineTo(hx - 0.12, hy - 0.1);
    c.lineTo(hx + Math.cos(ang(0)) * RR, hy + Math.sin(ang(0)) * RR);
    for (let i = 0; i < n; i++) {
      const am = ang(i + 0.5), a2 = ang(i + 1);
      c.quadraticCurveTo(hx + Math.cos(am) * RR * 1.09, hy + Math.sin(am) * RR * 1.09, hx + Math.cos(a2) * RR, hy + Math.sin(a2) * RR);
    }
    c.lineTo(hx + 0.12, hy - 0.1); c.lineTo(hx + 0.2, hy + 0.02); c.closePath();
    c.fillStyle = F_LIGHT; c.globalAlpha = 0.93; c.fill();
    c.globalAlpha = 1; c.strokeStyle = F_DARK; c.lineWidth = 0.04; c.lineJoin = 'round'; c.stroke();
    c.lineWidth = 0.022; c.beginPath();
    for (let i = 1; i < n; i++) { c.moveTo(hx, hy - 0.06); c.lineTo(hx + Math.cos(ang(i)) * RR * 0.99, hy + Math.sin(ang(i)) * RR * 0.99); }
    c.stroke();
    c.globalAlpha = 0.5; c.beginPath();
    c.arc(hx, hy, RR * 0.52, ang(0), ang(n)); c.stroke();
    c.beginPath(); c.arc(hx, hy, RR * 0.78, ang(0), ang(n)); c.stroke();
    c.restore();
  }
  function iron(c, rnd) {
    base(c, '#7b818c', '#2f3239');
    for (let d = 0; d < 3; d++) {
      c.save(); c.rotate(rnd() * 0.6 + d * TAU / 6);
      c.strokeStyle = 'rgba(214,224,238,0.24)'; c.lineWidth = 0.02; c.beginPath();
      for (let y = -1.4; y < 1.4; y += 0.09 + rnd() * 0.12) {
        const x0 = -1.4 + rnd() * 1.4, len = 0.5 + rnd() * 1.4;
        c.moveTo(x0, y); c.lineTo(x0 + len, y);
      }
      c.stroke(); c.restore();
    }
    blotch(c, rnd, 7, 0.26, '#000000', 0.34);
    blotch(c, rnd, 4, 0.24, '#9a5a2c', 0.36);
  }
  function chondrite(c, rnd) {
    base(c, '#5e5046', '#221d1a');
    blotch(c, rnd, 1, 0.5, '#9a9388', 0.55);
    speckle(c, rnd, 60, 0.035, ['#a39b90', '#8d8479', '#c6bdae'], 0.6);
    cracks(c, rnd, 4, '#0d0b0a');
    blotch(c, rnd, 3, 0.22, '#7a3f22', 0.3);
  }
  function pallasite(c, rnd) {
    base(c, '#aab0ba', '#4b4f57');
    for (let i = 0; i < 17; i++) {
      const a = rnd() * TAU, d = Math.sqrt(rnd()) * 0.95, x = Math.cos(a) * d, y = Math.sin(a) * d;
      const s = 0.11 + rnd() * 0.12, m = 5 + ((rnd() * 2) | 0), a1 = rnd() * TAU, pts = [];
      for (let j = 0; j < m; j++) {
        const aa = a1 + j / m * TAU, rr = s * (0.75 + rnd() * 0.45);
        pts.push([x + Math.cos(aa) * rr, y + Math.sin(aa) * rr]);
      }
      c.beginPath();
      for (let j = 0; j < m; j++) {
        const p = pts[j], q = pts[(j + 1) % m];
        const mx = (p[0] + q[0]) / 2, my = (p[1] + q[1]) / 2;
        if (!j) c.moveTo(mx, my);
        const r2 = pts[(j + 2) % m];
        c.quadraticCurveTo(q[0], q[1], (q[0] + r2[0]) / 2, (q[1] + r2[1]) / 2);
      }
      c.closePath();
      const g = c.createRadialGradient(x - s * 0.3, y - s * 0.3, 0, x, y, s * 1.2);
      g.addColorStop(0, '#f6f99a'); g.addColorStop(0.6, '#b4cc38'); g.addColorStop(1, '#6f8c1e');
      c.fillStyle = g; c.fill();
      c.strokeStyle = 'rgba(40,50,10,0.5)'; c.lineWidth = 0.018; c.stroke();
    }
  }

  /* ---------- 図鑑 ---------- */
  const CATS = [
    { id: 'common', label: 'ふつうの石', mark: '', hint: 'まだ出会っていない石。景色によって、出やすい石がちがうらしい。' },
    { id: 'gem', label: '宝石', mark: '宝石', hint: 'ころがすうちに、石が宝石に化けることがあるらしい。' },
    { id: 'fossil', label: '化石', mark: '化石', hint: '石の中から、大昔の生きものが出てくることがあるらしい。' },
    { id: 'meteor', label: '隕石', mark: '隕石', hint: '夜は、空から石がふってきやすいらしい。' },
    { id: 'gold', label: '黄金', mark: '黄金', hint: 'ごくまれに、黄金にかがやく石があるらしい。' }
  ];

  const BLOB = (irr, n) => ({ type: 'blob', irr: irr, n: n || 10 });
  const METEOR = { type: 'blob', irr: 0.2, n: 8, keep: 0.6 };
  const GEM_COVER = ['maru', 'kawara', 'shiro'], FOSSIL_COVER = ['suna', 'gotsu', 'maru'], GOLD_COVER = ['maru', 'gotsu', 'kuro'];

  const KINDS = [
    { id: 'maru', cat: 'common', name: 'まるい石', desc: 'どこにでもある、まるい石。にぎると、なぜか落ちつく。',
      shape: BLOB(0.13), gloss: [0.12, 0.42], edge: '#56544f', w: 10, likes: [],
      skin: (c, r) => { base(c, '#b3b0a8', '#7d7a73'); blotch(c, r, 3, 0.5, '#6a6761', 0.2); speckle(c, r, 70, 0.03, ['#5e5b55', '#d8d5cd'], 0.5); } },
    { id: 'hira', cat: 'common', name: 'ひらたい石', desc: '水切りにちょうどよさそうな、ひらたい石。ぱたん、ぱたんと転がる。',
      shape: { type: 'flat', irr: 0.07, n: 10 }, gloss: [0.12, 0.42], edge: '#5f594d', w: 8, likes: ['water', 'beach'],
      skin: (c, r) => { base(c, '#bcb5a7', '#888174'); speckle(c, r, 60, 0.025, ['#6e675b', '#e5dfd2'], 0.45); layers(c, r, 4, '#6e675b'); } },
    { id: 'shiro', cat: 'common', name: 'しろい石', desc: 'ほんのり光を通す、まっしろな石。',
      shape: BLOB(0.11), gloss: [0.2, 0.6], edge: '#9c998f', w: 7, likes: ['beach', 'water'],
      skin: (c, r) => { base(c, '#fdfcf8', '#d0cdc4'); vein(c, r, '#b4b9c1', 0.03, 0.5); vein(c, r, '#c4c0b6', 0.02, 0.5); speckle(c, r, 20, 0.02, ['#c6c4bd'], 0.5); } },
    { id: 'kuro', cat: 'common', name: 'くろい石', desc: 'ぬれたように黒い石。ころがすほど、つやが出る。',
      shape: BLOB(0.12), gloss: [0.25, 0.78], edge: '#131417', w: 7, likes: ['valley'],
      skin: (c, r) => { base(c, '#4c4f56', '#222327'); speckle(c, r, 40, 0.018, ['#8e929b'], 0.5); } },
    { id: 'shima', cat: 'common', name: 'しましまの石', desc: '長い年月がつくった、しまもよう。',
      shape: BLOB(0.12), gloss: [0.14, 0.46], edge: '#5c4a36', w: 6, likes: ['valley', 'slope'],
      skin: (c, r) => { base(c, '#c1a787', '#8d7456'); stripes(c, r, 6, ['#ecdcbe', '#6c553d', '#dbc6a2'], 0.08, 0.2, 0.85); } },
    { id: 'aka', cat: 'common', name: 'あかい石', desc: '夕やけをとじこめたような、あかい石。',
      shape: BLOB(0.13), gloss: [0.14, 0.5], edge: '#55241a', w: 6, likes: ['slope'],
      skin: (c, r) => { base(c, '#bd6850', '#7c3a2b'); blotch(c, r, 4, 0.45, '#5a281d', 0.32); vein(c, r, '#ebbfab', 0.025, 0.6); speckle(c, r, 30, 0.02, ['#e39d85', '#5a281d'], 0.4); } },
    { id: 'ao', cat: 'common', name: 'あおみどりの石', desc: '川底でひっそり光る、あおみどりの石。',
      shape: BLOB(0.11), gloss: [0.16, 0.55], edge: '#27504c', w: 6, likes: ['water'],
      skin: (c, r) => { base(c, '#78aea6', '#3d736e'); blotch(c, r, 5, 0.4, '#b0d8ce', 0.3); stripes(c, r, 3, ['#2c5c58'], 0.02, 0.05, 0.4); } },
    { id: 'koke', cat: 'common', name: 'こけむした石', desc: 'こけの帽子をかぶった石。ころがしても、こけは元気。',
      shape: BLOB(0.13), gloss: [0.1, 0.3], edge: '#4f5149', w: 6, likes: ['grass', 'water'],
      skin: (c, r) => { base(c, '#9a9c92', '#696b62'); speckle(c, r, 40, 0.025, ['#55574f'], 0.4); moss(c, r); } },
    { id: 'madara', cat: 'common', name: 'まだらの石', desc: '白・黒・うすももいろのつぶつぶ。みかげ石のなかま。',
      shape: BLOB(0.13), gloss: [0.14, 0.5], edge: '#6d675f', w: 7, likes: ['slope', 'valley'],
      skin: (c, r) => { base(c, '#dad4cb', '#a19a90'); speckle(c, r, 150, 0.035, ['#3c3a38', '#f6f2ec', '#dba79c', '#6f6a64'], 0.75); } },
    { id: 'karu', cat: 'common', name: 'かるい石', desc: 'あなだらけで軽い石。水に入れると、ぷかりと浮く。', float: true,
      shape: BLOB(0.16), gloss: [0.06, 0.16], edge: '#84796a', w: 5, likes: ['beach'],
      skin: (c, r) => { base(c, '#dcd3c5', '#aca190'); holes(c, r, 36); } },
    { id: 'suna', cat: 'common', name: 'すなの石', desc: '砂がかたまってできた石。うすい層がかさなっている。',
      shape: BLOB(0.12), gloss: [0.08, 0.26], edge: '#86683e', w: 6, likes: ['beach', 'valley'],
      skin: (c, r) => { base(c, '#e3c798', '#b18f5d'); layers(c, r, 9, '#8c6a40'); speckle(c, r, 60, 0.018, ['#f5e4bf', '#8c6a40'], 0.4); } },
    { id: 'kawara', cat: 'common', name: 'かわらの石', desc: '川にみがかれた石。白いすじが、ひとすじ。',
      shape: BLOB(0.08), gloss: [0.16, 0.5], edge: '#434e57', w: 8, likes: ['water'],
      skin: (c, r) => { base(c, '#96a3ad', '#5d6a74'); speckle(c, r, 30, 0.02, ['#4b565f'], 0.4); vein(c, r, '#f0f4f6', 0.09, 0.9); } },
    { id: 'gotsu', cat: 'common', name: 'ごつごつの石', desc: 'かどだらけの石。ころがすと、すこしずつ丸くなる。',
      shape: BLOB(0.27, 8), gloss: [0.08, 0.34], edge: '#463d33', w: 7, likes: ['slope', 'valley'],
      skin: (c, r) => { base(c, '#a09385', '#64594d'); patches(c, r, 10); cracks(c, r, 3, '#3a322a'); } },
    { id: 'onigiri', cat: 'common', name: 'おにぎり石', desc: 'どう見ても、おにぎり。のりまで付いている。',
      shape: { type: 'tri', irr: 0.05, n: 9 }, gloss: [0.14, 0.4], edge: '#8f8b81', w: 3, likes: ['grass'],
      skin: (c, r) => { base(c, '#f5f2ea', '#c8c4b9'); speckle(c, r, 40, 0.02, ['#b4b0a6'], 0.5); nori(c); } },

    { id: 'amethyst', cat: 'gem', name: 'アメジスト', desc: 'ころがすうちに、むらさきの光が目をさました。',
      covers: GEM_COVER, gloss: 0.92, edge: '#4b2a86', glow: '#b78cff',
      skin: gem({ hi: '#efe0ff', mid: '#a06be0', lo: '#5a2f9e' }) },
    { id: 'ruby', cat: 'gem', name: 'ルビー', desc: '石の奥で燃えていた、あかい火。',
      covers: GEM_COVER, gloss: 0.92, edge: '#6e0d24', glow: '#ff7d95',
      skin: gem({ hi: '#ffd9df', mid: '#e0385a', lo: '#8e1230' }) },
    { id: 'sapphire', cat: 'gem', name: 'サファイア', desc: '深い空の色を、そのまま固めたような石。',
      covers: GEM_COVER, gloss: 0.92, edge: '#142a70', glow: '#7da2ff',
      skin: gem({ hi: '#d9e6ff', mid: '#3f6fe0', lo: '#17358f' }) },
    { id: 'emerald', cat: 'gem', name: 'エメラルド', desc: '若葉より深い、みどりのかがやき。',
      covers: GEM_COVER, gloss: 0.92, edge: '#0a5033', glow: '#6fe3a8',
      skin: gem({ hi: '#d7ffe9', mid: '#27b673', lo: '#0d6a43' }) },
    { id: 'opal', cat: 'gem', name: 'オパール', desc: '見る向きで色が変わる、虹をのみこんだ石。',
      covers: GEM_COVER, gloss: 0.8, edge: '#6f8fb4', glow: '#c6ecff', skin: opal },
    { id: 'quartz', cat: 'gem', name: '水晶', desc: 'すきとおった、氷のような石。',
      covers: GEM_COVER, gloss: 0.95, edge: '#7d9cc4', glow: '#eaf5ff',
      skin: gem({ hi: '#ffffff', mid: '#e3f1ff', lo: '#a8c7ea', line: 'rgba(120,160,210,0.5)' }) },

    { id: 'ammonite', cat: 'fossil', name: 'アンモナイト', desc: 'ぐるぐる巻きの、大昔の海の生きもの。',
      covers: FOSSIL_COVER, gloss: 0.3, edge: '#5f5140', glow: '#ffe9b8', skin: ammonite },
    { id: 'trilobite', cat: 'fossil', name: '三葉虫', desc: '何億年も前の海の底を、歩いていたらしい。',
      covers: FOSSIL_COVER, gloss: 0.3, edge: '#5f5140', glow: '#ffe9b8', skin: trilobite },
    { id: 'leaf', cat: 'fossil', name: '木の葉の化石', desc: '葉っぱのすじまで、くっきり残っている。',
      covers: FOSSIL_COVER, gloss: 0.26, edge: '#6a5b46', glow: '#ffe9b8', skin: leaf },
    { id: 'shell', cat: 'fossil', name: '貝の化石', desc: 'ここも昔は、海の底だったのかもしれない。',
      covers: FOSSIL_COVER, gloss: 0.3, edge: '#5f5140', glow: '#ffe9b8', skin: shell },

    { id: 'iron', cat: 'meteor', name: '鉄隕石', desc: 'ずしりと重い、宇宙からきた鉄のかたまり。',
      shape: METEOR, gloss: [0.5, 0.7], edge: '#17191d', glow: '#ffb070', skin: iron },
    { id: 'chondrite', cat: 'meteor', name: '石質隕石', desc: '太陽系が生まれたころの、つぶつぶを抱えた石。',
      shape: METEOR, gloss: [0.14, 0.3], edge: '#120f0d', glow: '#ffb070', skin: chondrite },
    { id: 'pallasite', cat: 'meteor', name: 'パラサイト隕石', desc: '鉄の中に、きみどりの結晶がちりばめられている。',
      shape: METEOR, gloss: [0.6, 0.85], edge: '#2c2f35', glow: '#e4ff8a', skin: pallasite },

    { id: 'gold', cat: 'gold', name: '黄金の石', desc: 'ただの石だと思っていたら、中身はぜんぶ黄金だった。',
      covers: GOLD_COVER, gloss: 0.95, edge: '#7a4e04', glow: '#ffe37a', skin: goldSkin }
  ];

  const BY = {};
  KINDS.forEach((k) => { BY[k.id] = k; });

  const coverOf = (def, seed) => (def.covers ? BY[def.covers[seed % def.covers.length]] : null);

  /* ---------- 描画 ---------- */
  let scratch = null;

  // c の原点を石の中心として描く（陰影なし）。o = { r, polish, reveal }。形を返す。
  function draw(c, kind, seed, o) {
    const def = BY[kind], cover = coverOf(def, seed);
    const shp = buildShape(cover ? cover.shape : def.shape, seed, o.polish);
    const reveal = cover ? clamp(o.reveal, 0, 1) : 1;
    c.save();
    c.scale(o.r, o.r);
    tracePath(c, shp.R);
    c.save(); c.clip();
    if (cover && reveal < 1) { c.save(); cover.skin(c, rng(seed ^ 0x51ed270b)); c.restore(); }
    if (reveal > 0) {
      if (!cover || reveal >= 1) { c.save(); def.skin(c, rng(seed ^ 0x2545f491)); c.restore(); }
      else {
        const size = Math.ceil(o.r * 3) + 4;
        if (!scratch) scratch = document.createElement('canvas');
        scratch.width = scratch.height = size;
        const sc = scratch.getContext('2d');
        sc.translate(size / 2, size / 2); sc.scale(o.r, o.r);
        tracePath(sc, shp.R); sc.clip();
        def.skin(sc, rng(seed ^ 0x2545f491));
        c.globalAlpha = reveal;
        c.drawImage(scratch, -size / 2 / o.r, -size / 2 / o.r, size / o.r, size / o.r);
        c.globalAlpha = 1;
      }
    }
    c.restore();
    tracePath(c, shp.R);
    c.lineWidth = 0.045; c.lineJoin = 'round';
    if (cover && reveal < 1) { c.globalAlpha = 0.6 * (1 - reveal); c.strokeStyle = cover.edge; c.stroke(); }
    c.globalAlpha = 0.6 * reveal; c.strokeStyle = def.edge; c.stroke();
    c.restore();
    return shp;
  }

  function glossOf(kind, seed, polish, reveal) {
    const def = BY[kind], cover = coverOf(def, seed);
    const g = (d) => (Array.isArray(d.gloss) ? d.gloss[0] + (d.gloss[1] - d.gloss[0]) * polish : d.gloss);
    return cover ? g(cover) + (g(def) - g(cover)) * clamp(reveal, 0, 1) : g(def);
  }

  // 石だけが描かれたキャンバスに、固定方向の光と色かぶりをのせる
  function light(c, size, r, gloss, tint, hot) {
    const cx = size / 2, cy = size / 2;
    c.globalCompositeOperation = 'source-atop';
    let g = c.createLinearGradient(cx - r * 0.8, cy - r * 0.9, cx + r * 0.8, cy + r * 0.9);
    g.addColorStop(0, 'rgba(255,255,255,' + (0.16 + 0.22 * gloss).toFixed(3) + ')');
    g.addColorStop(0.42, 'rgba(255,255,255,0)');
    g.addColorStop(0.58, 'rgba(0,0,20,0)');
    g.addColorStop(1, 'rgba(0,0,20,0.4)');
    c.fillStyle = g; c.fillRect(0, 0, size, size);
    const hx = cx - r * 0.36, hy = cy - r * 0.42, hr = r * (0.22 + 0.34 * gloss);
    g = c.createRadialGradient(hx, hy, 0, hx, hy, hr);
    g.addColorStop(0, 'rgba(255,255,255,' + (0.12 + 0.78 * gloss).toFixed(3) + ')');
    g.addColorStop(1, 'rgba(255,255,255,0)');
    c.fillStyle = g; c.fillRect(0, 0, size, size);
    if (gloss > 0.6) {
      const k = (gloss - 0.6) / 0.4;
      c.fillStyle = 'rgba(255,255,255,' + (0.75 * k).toFixed(3) + ')';
      c.beginPath(); c.ellipse(cx - r * 0.42, cy - r * 0.5, r * 0.17, r * 0.075, -0.7, 0, TAU); c.fill();
      const bx = cx + r * 0.4, by = cy + r * 0.48;
      g = c.createRadialGradient(bx, by, 0, bx, by, r * 0.5);
      g.addColorStop(0, 'rgba(255,255,255,' + (0.3 * k).toFixed(3) + ')');
      g.addColorStop(1, 'rgba(255,255,255,0)');
      c.fillStyle = g; c.fillRect(0, 0, size, size);
    }
    if (hot > 0.01) { c.fillStyle = 'rgba(255,110,30,' + (hot * 0.55).toFixed(3) + ')'; c.fillRect(0, 0, size, size); }
    if (tint && tint[3] > 0.005) {
      c.fillStyle = 'rgba(' + (tint[0] | 0) + ',' + (tint[1] | 0) + ',' + (tint[2] | 0) + ',' + tint[3].toFixed(3) + ')';
      c.fillRect(0, 0, size, size);
    }
    c.globalCompositeOperation = 'source-over';
  }

  const canonSeed = (kind) => hash('ishi:' + kind);

  // 図鑑・お知らせ用の小さな絵（px 四方のキャンバス）
  function icon(kind, px, opts) {
    const cvs = document.createElement('canvas');
    cvs.width = cvs.height = px;
    const c = cvs.getContext('2d'), r = px * 0.33, seed = canonSeed(kind);
    c.translate(px / 2, px * 0.48);
    if (opts && opts.silhouette) {
      const def = BY[kind], cover = coverOf(def, seed);
      const shp = buildShape(cover ? cover.shape : def.shape, seed, 1);
      c.scale(r, r); tracePath(c, shp.R);
      c.fillStyle = 'rgba(86,64,42,0.2)'; c.fill();
      c.lineWidth = 0.05; c.setLineDash([0.14, 0.12]); c.strokeStyle = 'rgba(86,64,42,0.32)'; c.stroke();
      return cvs;
    }
    draw(c, kind, seed, { r: r, polish: 1, reveal: 1 });
    c.setTransform(1, 0, 0, 1, 0, px * 0.48 - px / 2);
    light(c, px, r, glossOf(kind, seed, 1, 1), null, 0);
    c.setTransform(1, 0, 0, 1, 0, 0);
    c.globalCompositeOperation = 'destination-over';
    c.fillStyle = 'rgba(40,26,10,0.2)';
    c.beginPath(); c.ellipse(px / 2, px * 0.48 + r * 1.02, r * 0.95, r * 0.2, 0, 0, TAU); c.fill();
    c.globalCompositeOperation = 'source-over';
    return cvs;
  }

  global.Stones = {
    CATS: CATS, KINDS: KINDS,
    get: (id) => BY[id],
    cat: (id) => CATS.find((x) => x.id === id),
    rng: rng, hash: hash, sample: sample, star: star,
    draw: draw, light: light, gloss: glossOf, icon: icon,
    sizeOf: (kind, seed) => 0.9 + rng(seed ^ 0x7f4a7c15)() * 0.22
  };
})(window);
