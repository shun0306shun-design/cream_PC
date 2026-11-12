/* =========================================================
   レポート構成ナビ  app.js
   ---------------------------------------------------------
   ・素の JavaScript だけで動きます（ビルド不要）
   ・データはブラウザの localStorage（キー：reportNavi.v1）に保存
   ・外部と通信するのは「文献検索」を押したときだけです
   ========================================================= */
(function () {
  'use strict';

  const D = window.RN_DATA;
  const C = window.RN_CITE;
  const S = window.RN_SEARCH;

  const STORE_KEY = 'reportNavi.v1';
  const STEPS = [
    { id: 'input', label: '課題', desc: '課題を整理する' },
    { id: 'rq', label: '問い', desc: '問いを立てる' },
    { id: 'outline', label: '構成', desc: '構成をつくる' },
    { id: 'search', label: '検索', desc: '文献を探す' },
    { id: 'cite', label: '引用', desc: '引用を整える' }
  ];
  const PLEDGE = '※これは構成と要点のメモです。本文ではありません。本文は必ず自分の言葉で書き、引用には出典を付けてください。';

  /* ---------- 小道具 ---------- */

  function $(sel, el) { return (el || document).querySelector(sel); }
  function $$(sel, el) { return Array.prototype.slice.call((el || document).querySelectorAll(sel)); }
  function esc(x) {
    return String(x == null ? '' : x).replace(/[&<>"']/g, function (c) {
      return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c];
    });
  }
  function uid(prefix) { return (prefix || '') + Date.now().toString(36) + Math.random().toString(36).slice(2, 7); }
  function num(n) { return Number(n || 0).toLocaleString('ja-JP'); }
  function text(x) { return String(x == null ? '' : x); }
  function clamp(n, lo, hi) { return Math.min(hi, Math.max(lo, n)); }
  function markPh(html) { return html.replace(/〔[^〕]*〕/g, function (m) { return '<mark class="ph">' + m + '</mark>'; }); }
  function safeUrl(u) {
    try {
      const x = new URL(String(u));
      return (x.protocol === 'https:' || x.protocol === 'http:') ? x.href : '';
    } catch (e) { return ''; }
  }
  function extLink(url, label, cls) {
    const u = safeUrl(url);
    if (!u) return '';
    return '<a class="' + (cls || '') + '" href="' + esc(u) + '" target="_blank" rel="noopener noreferrer">' + label + '</a>';
  }

  /* ---------- 保存データ ---------- */

  function blankProject() {
    const now = Date.now();
    return {
      id: uid('p_'),
      createdAt: now,
      updatedAt: now,
      input: { theme: '', prompt: '', length: 2000, type: 'essay' },
      rq: { fields: {}, question: '', thesis: '', checks: {} },
      outline: { type: null, length: null, points: null, sections: [] },
      search: { tab: 's2', qS2: '', qCinii: '', yearFrom: '', yearTo: '', oa: false, kind: 'all', sort: '', fulltext: false },
      refs: [],
      cite: { style: 'ja', order: 'auto' }
    };
  }

  function normalizeRef(x) {
    x = x && typeof x === 'object' ? x : {};
    return {
      id: typeof x.id === 'string' && x.id ? x.id : uid('r_'),
      source: ['s2', 'cinii', 'manual'].indexOf(x.source) !== -1 ? x.source : 'manual',
      sourceId: text(x.sourceId),
      type: C.TYPE_LABEL[x.type] ? x.type : 'article',
      authors: Array.isArray(x.authors) ? x.authors.map(text).filter(function (a) { return a.trim(); }) : [],
      year: text(x.year),
      title: text(x.title),
      container: text(x.container),
      volume: text(x.volume),
      issue: text(x.issue),
      pages: text(x.pages),
      publisher: text(x.publisher),
      degree: text(x.degree),
      doi: text(x.doi),
      url: text(x.url),
      pdf: text(x.pdf),
      note: text(x.note),
      addedAt: Number(x.addedAt) || Date.now()
    };
  }

  function normalizeProject(x) {
    const p = blankProject();
    if (!x || typeof x !== 'object') return p;
    if (typeof x.id === 'string' && x.id) p.id = x.id;
    p.createdAt = Number(x.createdAt) || p.createdAt;
    p.updatedAt = Number(x.updatedAt) || p.updatedAt;
    const i = x.input || {};
    p.input.theme = text(i.theme);
    p.input.prompt = text(i.prompt);
    p.input.length = Number(i.length) || 2000;
    p.input.type = D.TYPES.some(function (t) { return t.id === i.type; }) ? i.type : 'essay';
    const r = x.rq || {};
    if (r.fields && typeof r.fields === 'object') {
      Object.keys(r.fields).forEach(function (k) { p.rq.fields[k] = text(r.fields[k]); });
    }
    p.rq.question = text(r.question);
    p.rq.thesis = text(r.thesis);
    if (r.checks && typeof r.checks === 'object') {
      Object.keys(r.checks).forEach(function (k) { p.rq.checks[k] = !!r.checks[k]; });
    }
    p.refs = Array.isArray(x.refs) ? x.refs.map(normalizeRef) : [];
    const refIds = p.refs.map(function (f) { return f.id; });
    const o = x.outline || {};
    if (Array.isArray(o.sections)) {
      const seen = {};
      p.outline.type = D.TYPES.some(function (t) { return t.id === o.type; }) ? o.type : null;
      p.outline.length = Number(o.length) || null;
      p.outline.points = Number(o.points) || null;
      p.outline.sections = o.sections.filter(function (sec) { return sec && typeof sec === 'object'; }).map(function (sec) {
        let key = text(sec.key) || uid('custom_');
        if (seen[key]) key = uid('custom_');
        seen[key] = true;
        return {
          key: key,
          part: ['intro', 'body', 'concl'].indexOf(sec.part) !== -1 ? sec.part : 'body',
          title: text(sec.title),
          titleEdited: !!sec.titleEdited,
          chars: Math.max(0, Math.round(Number(sec.chars) || 0)),
          memo: text(sec.memo),
          refs: Array.isArray(sec.refs) ? sec.refs.filter(function (id) { return refIds.indexOf(id) !== -1; }) : []
        };
      });
    }
    const s = x.search || {};
    p.search.tab = s.tab === 'cinii' ? 'cinii' : 's2';
    p.search.qS2 = text(s.qS2);
    p.search.qCinii = text(s.qCinii);
    p.search.yearFrom = text(s.yearFrom);
    p.search.yearTo = text(s.yearTo);
    p.search.oa = !!s.oa;
    p.search.kind = ['all', 'articles', 'books', 'dissertations'].indexOf(s.kind) !== -1 ? s.kind : 'all';
    p.search.sort = s.sort === '0' || s.sort === '1' ? s.sort : '';
    p.search.fulltext = !!s.fulltext;
    const c = x.cite || {};
    p.cite.style = C.STYLES.some(function (st) { return st.id === c.style; }) ? c.style : 'ja';
    p.cite.order = ['auto', 'added', 'author'].indexOf(c.order) !== -1 ? c.order : 'auto';
    return p;
  }

  let storageOk = true;

  function loadState() {
    let raw = null;
    let data = null;
    try { raw = window.localStorage.getItem(STORE_KEY); } catch (e) { storageOk = false; }
    if (raw) {
      try { data = JSON.parse(raw); } catch (e) {
        /* 壊れたデータは別名で残して、新しく始める */
        try { window.localStorage.setItem(STORE_KEY + '.broken', raw); } catch (e2) { /* 何もしない */ }
      }
    }
    const st = { v: 1, currentId: null, projects: {}, ui: { step: 'input', theme: 'auto' } };
    if (data && typeof data === 'object' && data.projects && typeof data.projects === 'object') {
      Object.keys(data.projects).forEach(function (id) {
        const p = normalizeProject(data.projects[id]);
        st.projects[p.id] = p;
      });
      if (typeof data.currentId === 'string' && st.projects[data.currentId]) st.currentId = data.currentId;
      if (data.ui && typeof data.ui === 'object') {
        if (STEPS.some(function (s) { return s.id === data.ui.step; })) st.ui.step = data.ui.step;
        if (['auto', 'light', 'dark'].indexOf(data.ui.theme) !== -1) st.ui.theme = data.ui.theme;
      }
    }
    if (!st.currentId) {
      const ids = Object.keys(st.projects);
      if (ids.length) {
        st.currentId = ids.sort(function (a, b) { return st.projects[b].updatedAt - st.projects[a].updatedAt; })[0];
      } else {
        const p = blankProject();
        st.projects[p.id] = p;
        st.currentId = p.id;
      }
    }
    return st;
  }

  const state = loadState();
  let saveTimer = null;

  function saveNow() {
    clearTimeout(saveTimer);
    saveTimer = null;
    try {
      window.localStorage.setItem(STORE_KEY, JSON.stringify(state));
      storageOk = true;
    } catch (e) {
      storageOk = false;
    }
    $('#storageWarn').hidden = storageOk;
  }
  function save() {
    clearTimeout(saveTimer);
    saveTimer = setTimeout(saveNow, 250);
  }
  function cur() { return state.projects[state.currentId]; }
  function touch() { cur().updatedAt = Date.now(); save(); }
  function lengthOf(p) { return clamp(Math.round(Number(p.input.length) || 2000), 200, 100000); }
  function typeLabel(id) {
    const t = D.TYPES.filter(function (x) { return x.id === id; })[0];
    return t ? t.label : '';
  }

  function setPath(obj, path, val) {
    const ks = path.split('.');
    let o = obj;
    for (let i = 0; i < ks.length - 1; i++) {
      if (o[ks[i]] == null || typeof o[ks[i]] !== 'object') o[ks[i]] = {};
      o = o[ks[i]];
    }
    o[ks[ks.length - 1]] = val;
  }

  /* ---------- お知らせ・コピー・保存 ---------- */

  let toastTimer = null;
  function toast(msg) {
    const el = $('#toast');
    el.textContent = msg;
    el.classList.add('is-show');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(function () { el.classList.remove('is-show'); }, 2600);
  }

  function copyText(str, okMsg) {
    function fallback() {
      const ta = document.createElement('textarea');
      ta.value = str;
      ta.setAttribute('readonly', '');
      ta.style.position = 'fixed';
      ta.style.top = '-1000px';
      document.body.appendChild(ta);
      ta.select();
      let ok = false;
      try { ok = document.execCommand('copy'); } catch (e) { ok = false; }
      document.body.removeChild(ta);
      toast(ok ? okMsg : 'コピーできませんでした。文字を選択してコピーしてください。');
    }
    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(str).then(function () { toast(okMsg); }, fallback);
    } else {
      fallback();
    }
  }

  function download(filename, content, mime) {
    const blob = new Blob([content], { type: mime || 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(function () { URL.revokeObjectURL(url); }, 2000);
  }

  function fileSafe(name) {
    return (name || '').replace(/[\\/:*?"<>|\s]+/g, '_').slice(0, 30) || 'report';
  }

  function openDialog(id) {
    const d = $('#' + id);
    if (d.showModal) { if (!d.open) d.showModal(); } else { d.setAttribute('open', ''); }
  }
  function closeDialogs() {
    $$('dialog').forEach(function (d) {
      if (d.close && d.open) d.close(); else d.removeAttribute('open');
    });
  }

  function applyTheme() {
    const t = state.ui.theme;
    if (t === 'light' || t === 'dark') document.documentElement.setAttribute('data-theme', t);
    else document.documentElement.removeAttribute('data-theme');
  }

  /* ---------- 共通の枠 ---------- */

  function projectName(p) { return p.input.theme.trim() || '無題のレポート'; }

  function renderProjectSelect() {
    const list = Object.keys(state.projects).map(function (id) { return state.projects[id]; })
      .sort(function (a, b) { return b.createdAt - a.createdAt; });
    $('#projectSelect').innerHTML = list.map(function (p) {
      return '<option value="' + esc(p.id) + '"' + (p.id === state.currentId ? ' selected' : '') + '>' + esc(projectName(p)) + '</option>';
    }).join('');
  }

  function renderSteps() {
    const p = cur();
    const done = {
      input: !!p.input.theme.trim(),
      rq: !!p.rq.question.trim(),
      outline: p.outline.sections.length > 0,
      search: false,
      cite: p.refs.length > 0
    };
    $('#steps').innerHTML = STEPS.map(function (s, i) {
      const on = state.ui.step === s.id;
      return '<button type="button" class="step-btn' + (on ? ' is-current' : '') + (done[s.id] ? ' is-done' : '') + '" data-act="go" data-step="' + s.id + '"' + (on ? ' aria-current="step"' : '') + '>' +
        '<span class="step-num" aria-hidden="true">' + (i + 1) + '</span>' +
        '<span class="step-text"><span class="step-label">' + s.label + '</span><span class="step-desc">' + s.desc + '</span></span>' +
        (s.id === 'cite' && p.refs.length ? '<span class="step-count" aria-label="保存した文献 ' + p.refs.length + '件">' + p.refs.length + '</span>' : '') +
        '</button>';
    }).join('');
  }

  let softTimer = null;
  function refreshChromeSoon() {
    clearTimeout(softTimer);
    softTimer = setTimeout(function () { renderProjectSelect(); renderSteps(); }, 400);
  }

  function head(n, title, lead) {
    return '<header class="step-head"><p class="step-kicker">STEP ' + n + ' / 5</p><h2 tabindex="-1">' + title + '</h2><p class="lead">' + lead + '</p></header>';
  }
  function foot(prev, next) {
    return '<div class="step-foot">' +
      (prev ? '<button type="button" class="btn ghost" data-act="go" data-step="' + prev[0] + '">← ' + prev[1] + '</button>' : '<span></span>') +
      (next ? '<button type="button" class="btn primary" data-act="go" data-step="' + next[0] + '">' + next[1] + ' →</button>' : '') +
      '</div>';
  }

  const RENDER = {};

  function go(step, opts) {
    if (!RENDER[step]) step = 'input';
    state.ui.step = step;
    save();
    STEPS.forEach(function (s) { $('#step-' + s.id).hidden = s.id !== step; });
    RENDER[step]();
    renderSteps();
    if (!opts || !opts.keep) {
      window.scrollTo(0, 0);
      const h = $('#step-' + step + ' h2');
      if (h && opts && opts.focus) h.focus({ preventScroll: true });
    }
  }

  /* ---------- STEP 1：課題 ---------- */

  function directivesHtml(p) {
    const info = D.readPrompt(p.input.prompt);
    if (!p.input.prompt.trim()) return '';
    let html = '<div class="hint-box"><p class="hint-title">課題文から読み取れる指示</p>';
    if (info.directives.length) {
      html += '<ul class="dir-list">' + info.directives.map(function (d) {
        return '<li><b>' + esc(d.word) + '</b><span>' + esc(d.tip) + '</span></li>';
      }).join('') + '</ul>';
    } else {
      html += '<p class="muted small">「論じなさい」「比較しなさい」などの指示語は見つかりませんでした。何を求められているかを、自分の言葉で一文にしてみましょう。</p>';
    }
    if (info.length) {
      html += '<p class="dir-length">字数の指定：<b>' + num(info.length) + '字</b>' + (info.lengthNote ? '　<span class="muted">' + esc(info.lengthNote) + '</span>' : '') +
        (info.length !== Number(p.input.length) ? ' <button type="button" class="btn small" data-act="setLength" data-val="' + info.length + '">字数に反映</button>' : '') + '</p>';
    }
    return html + '</div>';
  }

  RENDER.input = function () {
    const p = cur();
    const i = p.input;
    $('#step-input').innerHTML =
      head(1, '課題を整理する', '出された課題をそのまま入力します。ここを起点に、問い・構成・検索語を組み立てます。') +
      '<div class="card">' +
        '<label class="field"><span class="label">テーマ</span>' +
          '<input type="text" data-bind="input.theme" value="' + esc(i.theme) + '" placeholder="例：教育格差" maxlength="100" autocomplete="off"></label>' +
        '<label class="field"><span class="label">課題文 <small>配られた指示をそのまま貼り付け</small></span>' +
          '<textarea data-bind="input.prompt" rows="5" placeholder="例：授業で扱った内容を踏まえ、日本の教育格差について自分の考えを2000字程度で論じなさい。参考文献を2点以上挙げること。">' + esc(i.prompt) + '</textarea></label>' +
        '<div id="directives">' + directivesHtml(p) + '</div>' +
        '<div class="field"><label class="label" for="lengthInput">字数</label>' +
          '<div class="length-row"><span class="unit-input"><input id="lengthInput" type="number" inputmode="numeric" min="200" max="100000" step="100" data-bind="input.length" data-num value="' + esc(i.length) + '"><span>字</span></span>' +
          '<span class="chips">' + [800, 1200, 2000, 4000, 8000].map(function (n) {
            return '<button type="button" class="chip" data-act="setLength" data-val="' + n + '">' + num(n) + '</button>';
          }).join('') + '</span></div></div>' +
        '<fieldset class="field"><legend class="label">レポートの種類</legend><div class="type-grid">' +
          D.TYPES.map(function (t) {
            return '<label class="type-opt"><input type="radio" name="rtype" value="' + t.id + '" data-bind="input.type"' + (i.type === t.id ? ' checked' : '') + '>' +
              '<span class="type-body"><b>' + t.label + '</b><small>' + t.desc + '</small></span></label>';
          }).join('') +
        '</div></fieldset>' +
      '</div>' +
      foot(null, ['rq', '問いを立てる']);
  };

  /* ---------- STEP 2：問い ---------- */

  function candidatesHtml(p) {
    return D.buildCandidates(p).map(function (c, i) {
      return '<li class="cand"><div class="cand-main"><span class="tag">' + esc(c.label) + '</span>' +
        '<p class="cand-text">' + markPh(esc(c.text)) + '</p><p class="cand-hint">' + esc(c.hint) + '</p></div>' +
        '<button type="button" class="btn small" data-act="useCandidate" data-i="' + i + '">この問いを使う</button></li>';
    }).join('');
  }

  function checksHtml(p) {
    const list = D.checkQuestion(p.rq.question, p);
    if (!list.length) return '<p class="muted small">問いを入力すると、形のチェックがここに出ます。</p>';
    const icon = { ok: '✓', tip: '！', info: 'i' };
    return '<ul class="checks">' + list.map(function (c) {
      return '<li class="check-' + c.level + '"><span class="check-icon" aria-hidden="true">' + icon[c.level] + '</span><span>' + esc(c.text) + '</span></li>';
    }).join('') + '</ul>';
  }

  RENDER.rq = function () {
    const p = cur();
    const def = D.rqDef(p.input.type);
    const f = p.rq.fields;
    const selfChecks = [
      ['evidence', '文献やデータで、答えを支えられそうだ'],
      ['fit', '課題文の指示と、授業の範囲に合っている'],
      ['own', '自分が知りたいと思える問いになっている']
    ];
    $('#step-rq').innerHTML =
      head(2, '問いを立てる', 'テーマは「話題」、問いは「答えを出す疑問文」です。問いが決まると、構成も検索語も決まります。') +
      '<div class="card">' +
        '<h3><span class="h-num">1</span>絞り込む材料をメモする</h3>' +
        '<p class="note">テーマ：<b>' + (p.input.theme.trim() ? esc(p.input.theme) : '<span class="muted">未入力（STEP 1 で入力）</span>') + '</b>　／　種類：' + esc(typeLabel(p.input.type)) + '　分かる範囲だけで構いません。</p>' +
        '<div class="grid2">' + def.fields.map(function (fd) {
          return '<label class="field"><span class="label">' + esc(fd.label) + '</span><input type="text" data-bind="rq.fields.' + fd.key + '" value="' + esc(f[fd.key] || '') + '" placeholder="' + esc(fd.ph) + '" autocomplete="off"></label>';
        }).join('') + '</div>' +
      '</div>' +
      '<div class="card">' +
        '<h3><span class="h-num">2</span>問いの型から候補を選ぶ</h3>' +
        '<p class="note">型にあてはめただけの下書きです。選んだあと、必ず自分の言葉に直してください。<mark class="ph">〔 〕</mark>は未入力の材料です。</p>' +
        '<ul class="cands" id="candidates">' + candidatesHtml(p) + '</ul>' +
      '</div>' +
      '<div class="card">' +
        '<h3><span class="h-num">3</span>自分の問いと、仮の答え</h3>' +
        '<label class="field"><span class="label">自分の問い（リサーチクエスチョン）</span>' +
          '<textarea id="rqQuestion" data-bind="rq.question" rows="3" placeholder="例：家庭の経済状況に注目すると、日本の公立中学生の学力差の背景にはどのような要因があるのか">' + esc(p.rq.question) + '</textarea></label>' +
        '<div id="rqChecks">' + checksHtml(p) + '</div>' +
        '<label class="field"><span class="label">' + esc(def.thesisLabel) + ' <small>調べたあとで変わって構いません</small></span>' +
          '<textarea data-bind="rq.thesis" rows="2" placeholder="' + esc(def.thesisPh) + '">' + esc(p.rq.thesis) + '</textarea></label>' +
        '<fieldset class="field self-checks"><legend class="label">自分で確かめる</legend>' +
          selfChecks.map(function (c) {
            return '<label class="check-row"><input type="checkbox" data-bind="rq.checks.' + c[0] + '"' + (p.rq.checks[c[0]] ? ' checked' : '') + '><span>' + c[1] + '</span></label>';
          }).join('') +
        '</fieldset>' +
      '</div>' +
      foot(['input', '課題'], ['outline', '構成をつくる']);
  };

  /* ---------- STEP 3：構成 ---------- */

  function sectionCtx(p, sec) {
    return { type: p.outline.type || p.input.type, T: p.input.theme, Q: p.rq.question, H: p.rq.thesis, f: p.rq.fields };
  }

  /* 構成を（作り）直す。メモと文献の割り当ては、同じ役割のセクションに引き継ぐ */
  function generateOutline(p) {
    const type = p.input.type;
    const N = lengthOf(p);
    const points = D.hasPoints(type) ? clamp(p.outline.points || D.defaultPoints(type, N), 1, 4) : 0;
    const planList = D.plan(type, points, N);
    const old = {};
    p.outline.sections.forEach(function (s) { old[s.key] = s; });
    const planKeys = planList.map(function (pl) { return pl.key; });
    /* 新しい型に居場所がないセクションは、内容があれば「追加セクション」として残す */
    const keep = p.outline.sections.filter(function (s) {
      if (planKeys.indexOf(s.key) !== -1) return false;
      return /^custom_/.test(s.key) || s.memo.trim() || s.refs.length;
    }).map(function (s) {
      return { key: /^custom_/.test(s.key) ? s.key : uid('custom_'), part: 'body', title: s.title, titleEdited: true, chars: s.chars, memo: s.memo, refs: s.refs };
    });
    const keepTotal = keep.reduce(function (a, s) { return a + s.chars; }, 0);
    const chars = D.allocate(Math.max(N - keepTotal, Math.round(N * 0.5)), planList);
    const secs = planList.map(function (pl, i) {
      const o = old[pl.key];
      return {
        key: pl.key,
        part: pl.part,
        title: o && o.titleEdited && p.outline.type === type ? o.title : D.sectionTitle(pl.key, type),
        titleEdited: !!(o && o.titleEdited && p.outline.type === type),
        chars: chars[i],
        memo: o ? o.memo : '',
        refs: o ? o.refs : []
      };
    });
    let at = secs.length;
    for (let i = 0; i < secs.length; i++) { if (secs[i].part === 'concl') { at = i; break; } }
    Array.prototype.splice.apply(secs, [at, 0].concat(keep));
    p.outline = { type: type, length: N, points: points, sections: secs };
  }

  /* 字数だけを配分し直す（並び・題名・メモはそのまま） */
  function reallocate(p) {
    const type = p.outline.type || p.input.type;
    const N = lengthOf(p);
    const planList = D.plan(type, p.outline.points || 0, N);
    const ratio = {};
    planList.forEach(function (pl) { ratio[pl.key] = pl.ratio; });
    const std = p.outline.sections.filter(function (s) { return ratio[s.key] != null; });
    const others = p.outline.sections.filter(function (s) { return ratio[s.key] == null; });
    const otherTotal = others.reduce(function (a, s) { return a + s.chars; }, 0);
    const sum = std.reduce(function (a, s) { return a + ratio[s.key]; }, 0) || 1;
    const chars = D.allocate(Math.max(N - otherTotal, 0), std.map(function (s) { return { key: s.key, part: s.part, ratio: ratio[s.key] / sum }; }));
    std.forEach(function (s, i) { s.chars = chars[i]; });
    p.outline.length = N;
  }

  function segClass(sec) { return sec.key === 'counter' ? 'counter' : sec.part; }

  function distHtml(p) {
    const secs = p.outline.sections;
    const N = lengthOf(p);
    const total = secs.reduce(function (a, s) { return a + s.chars; }, 0);
    const segs = secs.map(function (s) {
      return '<span class="dist-seg seg-' + segClass(s) + '" style="flex-grow:' + Math.max(s.chars, 1) + '" title="' + esc(s.title) + '：' + num(s.chars) + '字"></span>';
    }).join('');
    const legend = ['intro', 'body', 'concl'].map(function (k) {
      const c = secs.filter(function (s) { return s.part === k; }).reduce(function (a, s) { return a + s.chars; }, 0);
      const pct = total ? Math.round(c / total * 100) : 0;
      return '<li><i class="dot seg-' + k + '" aria-hidden="true"></i>' + D.PART[k] + ' <b>' + num(c) + '</b>字 <span class="muted">' + pct + '%</span></li>';
    }).join('');
    const diff = total - N;
    const note = diff === 0 ? 'ちょうど' : (diff > 0 ? num(diff) + '字オーバー' : 'あと' + num(-diff) + '字');
    return '<div class="dist" role="img" aria-label="字数配分のバー">' + segs + '</div>' +
      '<ul class="dist-legend">' + legend + '</ul>' +
      '<p class="dist-total' + (diff === 0 ? '' : ' is-off') + '">目安の合計 <b>' + num(total) + '</b> / ' + num(N) + '字<span>（' + note + '）</span></p>';
  }

  function refChipsHtml(p, sec) {
    const chosen = sec.refs.map(function (id) { return p.refs.filter(function (r) { return r.id === id; })[0]; }).filter(Boolean);
    if (!chosen.length) return '<span class="muted small">まだ選んでいません</span>';
    return chosen.map(function (r) {
      return '<span class="ref-chip">' + esc(C.shortLabel(r)) + '<button type="button" data-act="unsetSecRef" data-key="' + esc(sec.key) + '" data-id="' + esc(r.id) + '" aria-label="' + esc(C.shortLabel(r)) + ' を外す">×</button></span>';
    }).join('');
  }

  function memoWarnText(sec) {
    const n = sec.memo.replace(/\s/g, '').length;
    const limit = Math.max(120, Math.round(sec.chars * 0.4));
    if (n <= limit) return '';
    return 'メモが長くなってきました（' + num(n) + '字）。ここは要点だけにとどめ、本文は提出用のファイルで、自分の言葉で書きましょう。';
  }

  function sectionHtml(p, sec, idx, count) {
    const info = D.sectionInfo(sec.key, sectionCtx(p, sec));
    const k = esc(sec.key);
    const warn = memoWarnText(sec);
    return '<li class="sec part-' + sec.part + ' seg-' + segClass(sec) + '" data-key="' + k + '">' +
      '<div class="sec-head">' +
        '<span class="part-badge">' + D.PART[sec.part] + '</span>' +
        '<input class="sec-title" type="text" value="' + esc(sec.title) + '" data-sec="' + k + '" data-field="title" aria-label="セクション名" autocomplete="off">' +
        '<label class="sec-chars"><span>目安</span><input type="number" inputmode="numeric" min="0" step="10" value="' + sec.chars + '" data-sec="' + k + '" data-field="chars" aria-label="目安字数"><span>字</span></label>' +
      '</div>' +
      '<div class="sec-guide"><p class="mini-title">何を書くか</p><ul>' +
        info.guide.map(function (g) {
          return '<li><span class="gtag g-' + g.kind + '">' + esc(g.tag) + '</span><span>' + esc(g.text) + '</span></li>';
        }).join('') +
      '</ul></div>' +
      '<details class="starters"><summary>書き出しの型を見る</summary><ul>' +
        info.starters.map(function (s) { return '<li>' + esc(s) + '</li>'; }).join('') +
        '</ul><p class="note">文の入り口の型です。中身（〜の部分）は自分で考えて書きます。</p></details>' +
      '<label class="field memo"><span class="mini-title">要点メモ <small>箇条書きで。本文はここに書かない</small></span>' +
        '<textarea rows="3" data-sec="' + k + '" data-field="memo" placeholder="・&#10;・">' + esc(sec.memo) + '</textarea></label>' +
      '<p class="memo-warn"' + (warn ? '' : ' hidden') + '>' + esc(warn) + '</p>' +
      '<div class="sec-refs"><p class="mini-title">根拠に使う文献</p><div class="ref-chips">' + refChipsHtml(p, sec) + '</div>' +
        (p.refs.length
          ? '<details class="ref-pick"><summary>文献を選ぶ</summary><ul>' + p.refs.map(function (r) {
            return '<li><label class="check-row"><input type="checkbox" data-secref="' + k + '" value="' + esc(r.id) + '"' + (sec.refs.indexOf(r.id) !== -1 ? ' checked' : '') + '><span>' + esc(C.shortLabel(r)) + '　<span class="muted">' + esc(r.title.slice(0, 40)) + '</span></span></label></li>';
          }).join('') + '</ul></details>'
          : '<p class="muted small">STEP 4 で文献を保存すると、ここで割り当てられます。</p>') +
      '</div>' +
      '<div class="sec-tools">' +
        '<button type="button" class="btn tiny ghost" data-act="secMove" data-key="' + k + '" data-d="-1"' + (idx === 0 ? ' disabled' : '') + '>↑ 上へ</button>' +
        '<button type="button" class="btn tiny ghost" data-act="secMove" data-key="' + k + '" data-d="1"' + (idx === count - 1 ? ' disabled' : '') + '>↓ 下へ</button>' +
        '<button type="button" class="btn tiny ghost danger" data-act="secDel" data-key="' + k + '">削除</button>' +
      '</div>' +
    '</li>';
  }

  function sectionsHtml(p) {
    const secs = p.outline.sections;
    return secs.map(function (s, i) { return sectionHtml(p, s, i, secs.length); }).join('');
  }

  RENDER.outline = function () {
    const p = cur();
    if (!p.outline.sections.length) { generateOutline(p); touch(); }
    const o = p.outline;
    const def = D.rqDef(p.input.type);
    const N = lengthOf(p);
    let stale = '';
    if (o.type !== p.input.type) {
      stale = '<div class="notice"><p>レポートの種類が「' + esc(typeLabel(p.input.type)) + '」に変わっています。</p><button type="button" class="btn small" data-act="regen">この種類でつくり直す</button></div>';
    } else if (o.length !== N) {
      stale = '<div class="notice"><p>字数が ' + num(N) + '字に変わっています。</p><button type="button" class="btn small" data-act="realloc">字数を配分し直す</button></div>';
    }
    $('#step-outline').innerHTML =
      head(3, '構成をつくる', '序論 → 本論（主張＋根拠＋反論）→ 結論の骨組みです。各欄には要点だけをメモし、本文は自分の言葉で書きます。') +
      '<div class="recap"><dl>' +
        '<div><dt>問い</dt><dd>' + (p.rq.question.trim() ? esc(p.rq.question) : '<span class="muted">まだ決まっていません</span>') + '</dd></div>' +
        '<div><dt>' + esc(def.thesisLabel.replace(/（.*$/, '')) + '</dt><dd>' + (p.rq.thesis.trim() ? esc(p.rq.thesis) : '<span class="muted">まだ決まっていません</span>') + '</dd></div>' +
      '</dl><button type="button" class="link-btn" data-act="go" data-step="rq">STEP 2 で直す</button></div>' +
      stale +
      '<div class="card dist-card">' +
        '<div class="toolbar">' +
          (D.hasPoints(o.type) ? '<div class="stepper"><span class="label">本論の' + (o.type === 'review' ? '評価点' : '論点') + '</span>' +
            '<button type="button" class="icon-btn" data-act="points" data-d="-1" aria-label="減らす"' + (o.points <= 1 ? ' disabled' : '') + '>−</button>' +
            '<output aria-live="polite">' + o.points + '</output>' +
            '<button type="button" class="icon-btn" data-act="points" data-d="1" aria-label="増やす"' + (o.points >= 4 ? ' disabled' : '') + '>＋</button></div>' : '<span class="label">' + esc(typeLabel(o.type)) + 'レポートの型</span>') +
          '<span class="toolbar-gap"></span>' +
          '<button type="button" class="btn small" data-act="realloc">字数を配分し直す</button>' +
          '<button type="button" class="btn small" data-act="regen">つくり直す</button>' +
        '</div>' +
        '<div id="dist">' + distHtml(p) + '</div>' +
      '</div>' +
      '<ol class="sections" id="sections">' + sectionsHtml(p) + '</ol>' +
      '<p class="add-row"><button type="button" class="btn" data-act="secAdd">＋ セクションを追加</button></p>' +
      '<div class="card export">' +
        '<h3>構成メモを持ち出す</h3>' +
        '<p class="note">出力されるのは見出し・書く内容の要点・自分のメモ・文献リストの雛形です。本文は含まれません。</p>' +
        '<div class="btn-row">' +
          '<button type="button" class="btn primary" data-act="copyOutline">構成メモをコピー</button>' +
          '<button type="button" class="btn" data-act="saveOutline">テキストで保存</button>' +
          '<button type="button" class="btn" data-act="printOutline">印刷</button>' +
        '</div>' +
      '</div>' +
      foot(['rq', '問い'], ['search', '文献を探す']);
  };

  function findSec(key) {
    return cur().outline.sections.filter(function (s) { return s.key === key; })[0];
  }

  function bibliographyLines(p) {
    const style = p.cite.style;
    const list = C.sortRefs(p.refs, C.resolveOrder(style, p.cite.order));
    return list.map(function (r, i) { return C.format(r, style, i + 1).text; });
  }

  function outlineText(p) {
    const def = D.rqDef(p.input.type);
    const L = [];
    L.push('【レポート構成メモ】');
    L.push(PLEDGE);
    L.push('');
    L.push('テーマ：' + (p.input.theme.trim() || '（未入力）'));
    L.push('種類：' + typeLabel(p.outline.type || p.input.type) + '　／　字数：' + num(lengthOf(p)) + '字');
    if (p.rq.question.trim()) L.push('問い：' + p.rq.question.trim());
    if (p.rq.thesis.trim()) L.push(def.thesisLabel.replace(/（.*$/, '') + '：' + p.rq.thesis.trim());
    L.push('');
    p.outline.sections.forEach(function (sec) {
      const info = D.sectionInfo(sec.key, sectionCtx(p, sec));
      L.push('■ ' + sec.title + '（目安 ' + num(sec.chars) + '字）');
      L.push('  何を書くか');
      info.guide.forEach(function (g) { L.push('  ・［' + g.tag + '］' + g.text); });
      if (sec.memo.trim()) {
        L.push('  要点メモ');
        sec.memo.split('\n').forEach(function (line) { if (line.trim()) L.push('    ' + line.trim()); });
      }
      const labels = sec.refs.map(function (id) { return p.refs.filter(function (r) { return r.id === id; })[0]; }).filter(Boolean).map(C.shortLabel);
      if (labels.length) L.push('  使う文献：' + labels.join('／'));
      L.push('');
    });
    if (p.refs.length) {
      L.push('■ 参考文献（' + C.styleDef(p.cite.style).label + 'の雛形。〔 〕と巻・号・ページは原典で確認）');
      bibliographyLines(p).forEach(function (line) { L.push('  ' + line); });
      L.push('');
    }
    return L.join('\n');
  }

  function printOutline(p) {
    const def = D.rqDef(p.input.type);
    let html = '<h1>レポート構成メモ</h1><p class="print-pledge">' + esc(PLEDGE) + '</p>' +
      '<table class="print-meta"><tr><th>テーマ</th><td>' + esc(p.input.theme || '（未入力）') + '</td></tr>' +
      '<tr><th>種類・字数</th><td>' + esc(typeLabel(p.outline.type || p.input.type)) + '・' + num(lengthOf(p)) + '字</td></tr>' +
      '<tr><th>問い</th><td>' + esc(p.rq.question || '（未入力）') + '</td></tr>' +
      '<tr><th>' + esc(def.thesisLabel.replace(/（.*$/, '')) + '</th><td>' + esc(p.rq.thesis || '（未入力）') + '</td></tr></table>';
    p.outline.sections.forEach(function (sec) {
      const info = D.sectionInfo(sec.key, sectionCtx(p, sec));
      const labels = sec.refs.map(function (id) { return p.refs.filter(function (r) { return r.id === id; })[0]; }).filter(Boolean).map(C.shortLabel);
      html += '<section class="print-sec"><h2>' + esc(sec.title) + '<span>目安 ' + num(sec.chars) + '字</span></h2><ul>' +
        info.guide.map(function (g) { return '<li><b>' + esc(g.tag) + '</b>　' + esc(g.text) + '</li>'; }).join('') + '</ul>' +
        (sec.memo.trim() ? '<p class="print-memo"><b>要点メモ</b><br>' + esc(sec.memo.trim()).replace(/\n/g, '<br>') + '</p>' : '') +
        (labels.length ? '<p class="print-refs"><b>使う文献</b>　' + esc(labels.join('／')) + '</p>' : '') + '</section>';
    });
    if (p.refs.length) {
      html += '<section class="print-sec"><h2>参考文献<span>' + esc(C.styleDef(p.cite.style).label) + 'の雛形・要確認</span></h2><ul class="print-bib">' +
        bibliographyLines(p).map(function (line) { return '<li>' + esc(line) + '</li>'; }).join('') + '</ul></section>';
    }
    $('#printArea').innerHTML = html;
    window.print();
  }

  /* ---------- STEP 4：文献検索 ---------- */

  const RESULTS = {};
  let searchCtl = null;
  let searchBox = null; /* いま検索中の結果置き場 */

  /* 進行中の検索を止める。止めた側の表示が「検索中」のまま残らないようにする */
  function abortSearch() {
    if (searchCtl) searchCtl.abort();
    if (searchBox && searchBox.status === 'loading') searchBox.status = searchBox.items.length ? 'done' : 'idle';
    searchCtl = null;
    searchBox = null;
  }

  function resultsOf(tab) {
    const id = state.currentId;
    if (!RESULTS[id]) RESULTS[id] = {};
    if (!RESULTS[id][tab]) RESULTS[id][tab] = { status: 'idle', items: [], total: 0, next: null, params: null, error: null, retry: null };
    return RESULTS[id][tab];
  }

  function isAdded(p, it) {
    return p.refs.some(function (r) { return r.source === it.source && r.sourceId && r.sourceId === it.sourceId; });
  }

  function resultHtml(p, it, i) {
    const added = isAdded(p, it);
    const names = it.authors.map(function (a) {
      const n = C.parseName(a);
      return n && n.ja ? n.family + (n.given ? ' ' + n.given : '') : a;
    });
    const authors = names.length ? names.slice(0, 4).join('、') + (names.length > 4 ? ' ほか' + (names.length - 4) + '名' : '') : '著者不明';
    let vi = '';
    if (it.volume && it.issue) vi = it.volume + '(' + it.issue + ')';
    else if (it.volume) vi = it.volume;
    else if (it.issue) vi = '(' + it.issue + ')';
    const venue = [it.container || (it.type !== 'article' ? it.publisher : ''), vi].filter(Boolean).join(' ');
    const url = safeUrl(it.url);
    const full = it.fulltext && it.fulltext !== it.url ? extLink(it.fulltext, '本文・所蔵先 ↗', 'tag link') : '';
    const short = it.abstract && it.abstract.length < 60;
    return '<li class="result">' +
      '<div class="result-body">' +
        '<h3 class="result-title">' + (url ? '<a href="' + esc(url) + '" target="_blank" rel="noopener noreferrer">' + esc(it.title) + '</a>' : esc(it.title)) + '</h3>' +
        '<p class="result-meta"><span>' + esc(authors) + '</span><span>' + esc(it.year || '年不明') + '</span>' + (venue ? '<span>' + esc(venue) + '</span>' : '') + '</p>' +
        '<p class="result-tags"><span class="tag">' + esc(it.kind) + '</span>' +
          (it.cited != null ? '<span class="tag plain">被引用 ' + num(it.cited) + '</span>' : '') +
          extLink(it.pdf, '無料PDF ↗', 'tag link') +
          (it.doi ? extLink('https://doi.org/' + it.doi, 'DOI ↗', 'tag link') : '') +
          full +
          (short ? '<span class="tag plain">' + esc(it.abstract) + '</span>' : '') +
        '</p>' +
        (it.abstract && !short ? '<details class="abstract"><summary>要旨を見る</summary><p>' + esc(it.abstract) + '</p>' +
          '<p class="note">要旨は「読む文献を選ぶ」ためのものです。レポートには写さず、本文を読んで自分の言葉でまとめましょう。</p></details>' : '') +
      '</div>' +
      '<button type="button" class="btn small add-btn' + (added ? ' is-added' : '') + '" data-act="addRef" data-i="' + i + '"' + (added ? ' disabled' : '') + '>' + (added ? '追加済み ✓' : '＋ 文献リストへ') + '</button>' +
    '</li>';
  }

  function statusHtml(p, tab, R) {
    const src = tab === 's2' ? 'Semantic Scholar' : 'CiNii Research';
    if (R.status === 'loading') {
      return '<p class="status is-loading"><span class="spinner" aria-hidden="true"></span><span>' +
        (R.retry ? src + ' が混み合っています。自動で再試行中（' + R.retry.attempt + '/' + R.retry.max + '）…' : src + ' を検索しています…') +
        '</span><button type="button" class="link-btn" data-act="cancelSearch">中止</button></p>';
    }
    if (R.status === 'error') {
      const q = R.params ? R.params.q : '';
      const kind = R.error && R.error.kind;
      let msg;
      if (kind === 'offline') msg = 'インターネットにつながっていないようです。接続を確認して、もう一度検索してください。';
      else if (kind === 'http') msg = '検索条件を受け付けてもらえませんでした（エラー ' + (R.error.status || '') + '）。検索語や絞り込みを変えて、もう一度お試しください。';
      else if (kind === 'parse') msg = '検索結果を読み取れませんでした。少し時間をおいて、もう一度お試しください。';
      else if (tab === 's2') msg = 'Semantic Scholar が混み合っていて、結果を取得できませんでした（キーなしで使える共有枠の制限です）。少し時間をおいて「もう一度検索」を押してください。';
      else msg = 'CiNii Research から結果を取得できませんでした。少し時間をおいて、もう一度お試しください。';
      return '<div class="status is-error"><p>' + esc(msg) + '</p><div class="btn-row">' +
        '<button type="button" class="btn small primary" data-act="searchRetry">もう一度検索</button>' +
        (tab === 's2' && kind !== 'offline' ? '<button type="button" class="btn small" data-act="toCinii">CiNii Research で同じ語を検索</button>' : '') +
        extLink(S.siteUrl(tab, q), src + ' のサイトで開く ↗', 'btn small ghost') +
        '</div></div>';
    }
    if (R.status === 'done') {
      if (!R.items.length) {
        return '<div class="status is-empty"><p><b>見つかりませんでした。</b></p><ul>' +
          '<li>語を減らす（2〜3語に）、または別の言い方にする</li>' +
          (tab === 's2' ? '<li>英語のキーワードにする（例：educational inequality）</li><li>日本語の文献は「文系・日本語」タブで探す</li>' : '<li>年や種別の絞り込みを外す</li>') +
          '</ul></div>';
      }
      return '<p class="status">' + num(R.total) + ' 件中 ' + num(R.items.length) + ' 件を表示　<span class="muted">「' + esc(R.params.q) + '」</span></p>';
    }
    const tips = tab === 's2'
      ? ['英語のキーワードのほうが多く見つかります（例：educational inequality Japan）', '語は2〜4個が目安。多すぎると見つかりません', '「被引用」が多い文献は、その分野でよく参照されている目安になります']
      : ['日本語の論文・本・博士論文をまとめて探せます', '語をスペースで区切ると、すべてを含むものに絞られます', '本文へのリンクがない文献は、大学図書館の蔵書検索や取り寄せ（ILL）で入手できます'];
    return '<ul class="tips">' + tips.map(function (t) { return '<li>' + t + '</li>'; }).join('') + '</ul>';
  }

  function renderResults() {
    const p = cur();
    const tab = p.search.tab;
    const R = resultsOf(tab);
    const box = $('#searchStatus');
    if (!box) return;
    box.innerHTML = statusHtml(p, tab, R);
    $('#results').innerHTML = R.items.map(function (it, i) { return resultHtml(p, it, i); }).join('');
    $('#searchMore').innerHTML = (R.status === 'done' && R.next != null && R.items.length)
      ? '<button type="button" class="btn" data-act="searchMore">さらに ' + S.PAGE + ' 件を読み込む</button>' : '';
  }

  function runSearch(more) {
    const p = cur();
    const s = p.search;
    const tab = s.tab;
    const R = resultsOf(tab);
    let params;
    if (more && R.params && R.next != null) {
      params = Object.assign({}, R.params, { offset: R.next });
    } else {
      const q = (tab === 's2' ? s.qS2 : s.qCinii).trim();
      if (!q) {
        toast('検索語を入力してください');
        const inp = $('#searchQ');
        if (inp) inp.focus();
        return;
      }
      params = { q: q, yearFrom: s.yearFrom, yearTo: s.yearTo, oa: s.oa, kind: s.kind, sort: s.sort, fulltext: s.fulltext, offset: tab === 's2' ? 0 : 1 };
      R.items = [];
      R.total = 0;
      R.next = null;
      more = false;
    }
    abortSearch();
    const ctl = new AbortController();
    searchCtl = ctl;
    searchBox = R;
    R.params = params;
    R.status = 'loading';
    R.error = null;
    R.retry = null;
    renderResults();
    if (!more) {
      /* スマホでは結果が画面の下に隠れるので、検索中の表示が見える位置まで送る */
      const box = $('#searchStatus');
      const rect = box.getBoundingClientRect();
      if (rect.top > window.innerHeight - 160) box.scrollIntoView({ block: 'center', behavior: 'smooth' });
    }
    const job = tab === 's2'
      ? S.searchS2(params, { signal: ctl.signal, onRetry: function (info) { R.retry = info; if (cur().search.tab === tab) renderResults(); } })
      : S.searchCinii(params, { signal: ctl.signal });
    job.then(function (out) {
      if (searchCtl !== ctl) return;
      searchCtl = null;
      searchBox = null;
      R.items = more ? R.items.concat(out.items) : out.items;
      R.total = out.total;
      R.next = out.next;
      R.status = 'done';
      renderResults();
    }, function (err) {
      if (err && err.name === 'AbortError') return;
      if (searchCtl !== ctl) return;
      searchCtl = null;
      searchBox = null;
      R.status = more && R.items.length ? 'done' : 'error';
      R.error = err;
      renderResults();
      if (R.status === 'done') toast('続きを読み込めませんでした。少し待ってからもう一度お試しください。');
    });
  }

  function cancelSearch() {
    abortSearch();
    renderResults();
  }

  RENDER.search = function () {
    const p = cur();
    const s = p.search;
    const tab = s.tab;
    const isS2 = tab === 's2';
    const sug = D.suggestQueries(p);
    const tabs = [
      ['s2', '理系・全分野', 'Semantic Scholar'],
      ['cinii', '文系・日本語', 'CiNii Research']
    ];
    const kinds = [['all', 'すべて'], ['articles', '論文'], ['books', '本'], ['dissertations', '博士論文']];
    const sorts = [['', '関連度順'], ['0', '新しい順'], ['1', '古い順']];
    $('#step-search').innerHTML =
      head(4, '文献を探す', '主張の根拠になる文献を探して、文献リストに保存します。検索語と条件だけが検索先に送られます。') +
      '<div class="tabs" role="tablist" aria-label="検索先">' + tabs.map(function (t) {
        return '<button type="button" role="tab" class="tab' + (tab === t[0] ? ' is-on' : '') + '" aria-selected="' + (tab === t[0]) + '" data-act="tab" data-tab="' + t[0] + '"><b>' + t[1] + '</b><small>' + t[2] + '</small></button>';
      }).join('') + '</div>' +
      '<div class="card search-card" role="tabpanel">' +
        '<form class="search-form" data-form="search" autocomplete="off">' +
          '<div class="search-row">' +
            '<label class="sr-only" for="searchQ">検索語</label>' +
            '<input type="search" id="searchQ" data-bind="search.' + (isS2 ? 'qS2' : 'qCinii') + '" value="' + esc(isS2 ? s.qS2 : s.qCinii) + '" placeholder="' + (isS2 ? '英語のキーワード（例：educational inequality Japan）' : 'キーワード（例：教育格差 家庭環境）') + '" enterkeyhint="search">' +
            '<button type="submit" class="btn primary">検索</button>' +
          '</div>' +
          (sug.length ? '<p class="suggest"><span class="muted small">テーマから：</span>' + sug.map(function (q) {
            return '<button type="button" class="chip" data-act="suggest" data-q="' + esc(q) + '">' + esc(q) + '</button>';
          }).join('') + (isS2 ? '<span class="muted small">（英語に直すと結果が増えます）</span>' : '') + '</p>' : '') +
          '<details class="filters"' + ((s.yearFrom || s.yearTo || (isS2 ? s.oa : (s.kind !== 'all' || s.sort || s.fulltext))) ? ' open' : '') + '><summary>絞り込み</summary><div class="filter-grid">' +
            '<div class="field"><span class="label" id="yearLabel">出版年</span><div class="year-row" role="group" aria-labelledby="yearLabel">' +
              '<input type="number" inputmode="numeric" min="1800" max="2100" placeholder="2015" data-bind="search.yearFrom" value="' + esc(s.yearFrom) + '" aria-label="出版年（から）"><span>〜</span>' +
              '<input type="number" inputmode="numeric" min="1800" max="2100" placeholder="2025" data-bind="search.yearTo" value="' + esc(s.yearTo) + '" aria-label="出版年（まで）"></div></div>' +
            (isS2
              ? '<label class="check-row"><input type="checkbox" data-bind="search.oa"' + (s.oa ? ' checked' : '') + '><span>無料で読めるPDFがあるものだけ</span></label>'
              : '<label class="field"><span class="label">種別</span><select data-bind="search.kind">' + kinds.map(function (k) { return '<option value="' + k[0] + '"' + (s.kind === k[0] ? ' selected' : '') + '>' + k[1] + '</option>'; }).join('') + '</select></label>' +
                '<label class="field"><span class="label">並び順</span><select data-bind="search.sort">' + sorts.map(function (k) { return '<option value="' + k[0] + '"' + (s.sort === k[0] ? ' selected' : '') + '>' + k[1] + '</option>'; }).join('') + '</select></label>' +
                '<label class="check-row"><input type="checkbox" data-bind="search.fulltext"' + (s.fulltext ? ' checked' : '') + '><span>本文へのリンクがあるものだけ</span></label>') +
          '</div></details>' +
        '</form>' +
      '</div>' +
      '<div id="searchStatus" aria-live="polite"></div>' +
      '<ol class="results" id="results"></ol>' +
      '<div class="more-row" id="searchMore"></div>' +
      foot(['outline', '構成'], ['cite', '引用を整える（' + p.refs.length + '件）']);
    renderResults();
  };

  function addRef(i) {
    const p = cur();
    const R = resultsOf(p.search.tab);
    const it = R.items[i];
    if (!it || isAdded(p, it)) return false;
    p.refs.push(normalizeRef({
      source: it.source, sourceId: it.sourceId, type: it.type, authors: it.authors, year: it.year, title: it.title,
      container: it.container, volume: it.volume, issue: it.issue, pages: it.pages, publisher: it.publisher,
      degree: it.degree, doi: it.doi, url: it.url, pdf: it.pdf
    }));
    touch();
    return true;
  }

  /* ---------- STEP 5：引用 ---------- */

  let refNums = {};

  function orderedRefs(p) {
    return C.sortRefs(p.refs, C.resolveOrder(p.cite.style, p.cite.order));
  }

  function refOutHtml(p, r) {
    const f = C.format(r, p.cite.style, refNums[r.id] || 1);
    const miss = C.missing(r);
    return '<div class="ref-out"><p class="ref-text">' + markPh(esc(f.text)) + '</p>' +
        '<button type="button" class="btn small" data-act="copyRef" data-id="' + esc(r.id) + '">コピー</button></div>' +
      '<div class="ref-out sub"><p class="ref-intext"><span class="mini-title">本文中</span><span class="intext">' + markPh(esc(f.intext)) + '</span></p>' +
        '<button type="button" class="btn small ghost" data-act="copyIntext" data-id="' + esc(r.id) + '">コピー</button></div>' +
      (miss.length ? '<p class="ref-missing"><b>原典で確認：</b>' + esc(miss.join('、')) + '</p>' : '');
  }

  function refHtml(p, r, open) {
    const id = esc(r.id);
    function inp(field, label, ph, cls) {
      return '<label class="field ' + (cls || '') + '"><span class="label">' + label + '</span><input type="text" data-ref="' + id + '" data-field="' + field + '" value="' + esc(r[field]) + '" placeholder="' + (ph || '') + '" autocomplete="off"></label>';
    }
    const link = safeUrl(r.url) || (r.doi ? safeUrl('https://doi.org/' + r.doi) : '');
    return '<li class="ref" data-id="' + id + '">' +
      '<div class="ref-outs">' + refOutHtml(p, r) + '</div>' +
      '<details class="ref-edit"' + (open ? ' open' : '') + '><summary>書誌情報を直す</summary><div class="ref-form">' +
        '<label class="field"><span class="label">種類</span><select data-ref="' + id + '" data-field="type">' +
          Object.keys(C.TYPE_LABEL).map(function (k) { return '<option value="' + k + '"' + (r.type === k ? ' selected' : '') + '>' + C.TYPE_LABEL[k] + '</option>'; }).join('') + '</select></label>' +
        inp('year', '出版年', '2024') +
        '<label class="field wide"><span class="label">著者 <small>1行に1人。「姓, 名」と書くと確実です</small></span><textarea rows="' + clamp(r.authors.length, 2, 6) + '" data-ref="' + id + '" data-field="authors" placeholder="松岡, 亮二">' + esc(r.authors.join('\n')) + '</textarea></label>' +
        inp('title', 'タイトル', '', 'wide') +
        inp('container', '雑誌名・掲載先', '', 'wide') +
        inp('volume', '巻', '') + inp('issue', '号', '') + inp('pages', 'ページ', '36-43') +
        inp('publisher', '出版社・大学名', '') +
        inp('doi', 'DOI', '10.xxxx/xxxxx', 'wide') + inp('url', 'URL', 'https://', 'wide') +
      '</div></details>' +
      '<label class="field memo"><span class="mini-title">メモ <small>この文献から使う要点とページ。自分の言葉で</small></span>' +
        '<textarea rows="2" data-ref="' + id + '" data-field="note" placeholder="例：p. 45 の表2。世帯年収と学力の関係を示すデータ">' + esc(r.note) + '</textarea></label>' +
      '<div class="ref-tools">' + (link ? '<a class="link-btn" href="' + esc(link) + '" target="_blank" rel="noopener noreferrer">元のページを開く ↗</a>' : '<span></span>') +
        '<button type="button" class="btn tiny ghost danger" data-act="delRef" data-id="' + id + '">削除</button></div>' +
    '</li>';
  }

  let openRefId = null;

  RENDER.cite = function () {
    const p = cur();
    const style = p.cite.style;
    const list = orderedRefs(p);
    refNums = {};
    list.forEach(function (r, i) { refNums[r.id] = i + 1; });
    const order = C.resolveOrder(style, p.cite.order);
    $('#step-cite').innerHTML =
      head(5, '引用を整える', '保存した文献を、引用形式の雛形に整えます。巻・号・ページや著者名の表記は、必ず原典で確かめてください。') +
      '<div class="card">' +
        '<fieldset class="field"><legend class="label">書式 <small>授業や学部で指定があれば、そちらを優先</small></legend><div class="style-grid">' +
          C.STYLES.map(function (st) {
            return '<label class="type-opt"><input type="radio" name="citeStyle" value="' + st.id + '" data-bind="cite.style"' + (style === st.id ? ' checked' : '') + '><span class="type-body"><b>' + st.label + '</b><small>' + esc(st.desc) + '</small></span></label>';
          }).join('') + '</div></fieldset>' +
        '<label class="field inline"><span class="label">並び順</span><select data-bind="cite.order">' +
          [['auto', '書式に合わせる'], ['added', '追加した順'], ['author', '著者名順']].map(function (o) {
            return '<option value="' + o[0] + '"' + (p.cite.order === o[0] ? ' selected' : '') + '>' + o[1] + '</option>';
          }).join('') + '</select></label>' +
        (order === 'author' && list.some(function (r) { return C.hasJa(r.authors.join('')); }) ? '<p class="note">日本語の著者名は読み（五十音）で並べられないため、提出前に並びを確認してください。</p>' : '') +
      '</div>' +
      (list.length
        ? '<ol class="refs" id="refs">' + list.map(function (r) { return refHtml(p, r, r.id === openRefId); }).join('') + '</ol>' +
          '<div class="card export"><h3>参考文献リスト</h3><p class="note"><mark class="ph">〔 〕</mark>は取得できなかった項目です。原典を見て埋めてから提出してください。</p><div class="btn-row">' +
            '<button type="button" class="btn primary" data-act="copyAll">リストをすべてコピー</button>' +
            '<button type="button" class="btn" data-act="addManual">＋ 手入力で追加</button></div></div>'
        : '<div class="empty"><p><b>まだ文献が保存されていません。</b></p><p class="muted">STEP 4 の検索結果で「＋ 文献リストへ」を押すと、ここに並びます。図書館で見つけた本などは手入力でも追加できます。</p><div class="btn-row">' +
            '<button type="button" class="btn primary" data-act="go" data-step="search">文献を探す</button>' +
            '<button type="button" class="btn" data-act="addManual">＋ 手入力で追加</button></div></div>') +
      '<div class="card rules-card"><h3>引用するときの約束</h3><ul>' +
        '<li>そのまま使う文は「 」でくくり、出典とページを示す。</li>' +
        '<li>言い換え・要約でも、出典は必ず付ける。</li>' +
        '<li>要旨だけで引用せず、本文を読んで確かめる。</li></ul>' +
        '<button type="button" class="link-btn" data-act="openRules">くわしく見る</button></div>' +
      foot(['search', '文献を探す'], null);
    openRefId = null;
  };

  function findRef(id) {
    return cur().refs.filter(function (r) { return r.id === id; })[0];
  }

  function refreshRefCard(id) {
    const r = findRef(id);
    const li = $$('#refs .ref').filter(function (el) { return el.getAttribute('data-id') === id; })[0];
    if (r && li) $('.ref-outs', li).innerHTML = refOutHtml(cur(), r);
  }

  /* ---------- 操作（クリック） ---------- */

  function switchProject(id) {
    if (!state.projects[id]) return;
    abortSearch();
    state.currentId = id;
    renderProjectSelect();
    go(state.ui.step);
  }

  const ACT = {
    go: function (el) { go(el.getAttribute('data-step'), { focus: true }); },

    newProject: function () {
      const p = blankProject();
      state.projects[p.id] = p;
      state.currentId = p.id;
      abortSearch();
      renderProjectSelect();
      go('input');
      toast('新しいレポートを作りました');
      const first = $('#step-input input[type="text"]');
      if (first) first.focus();
    },

    openMenu: function () {
      $$('input[name="uiTheme"]').forEach(function (r) { r.checked = r.value === state.ui.theme; });
      openDialog('menuDialog');
    },
    openRules: function () { openDialog('rulesDialog'); },
    closeDialog: function () { closeDialogs(); },

    exportData: function () {
      saveNow();
      const data = { app: 'report-navi', v: 1, exportedAt: new Date().toISOString(), currentId: state.currentId, projects: state.projects };
      download('report-navi-backup-' + new Date().toISOString().slice(0, 10) + '.json', JSON.stringify(data, null, 2), 'application/json');
      toast('バックアップを書き出しました');
    },
    importData: function () { $('#importFile').click(); },

    deleteProject: function () {
      const p = cur();
      if (!window.confirm('「' + projectName(p) + '」を削除します。元に戻せません。よろしいですか？')) return;
      delete state.projects[p.id];
      delete RESULTS[p.id];
      const ids = Object.keys(state.projects);
      if (ids.length) {
        state.currentId = ids.sort(function (a, b) { return state.projects[b].updatedAt - state.projects[a].updatedAt; })[0];
      } else {
        const np = blankProject();
        state.projects[np.id] = np;
        state.currentId = np.id;
      }
      saveNow();
      closeDialogs();
      renderProjectSelect();
      go('input');
      toast('削除しました');
    },

    setLength: function (el) {
      const n = parseInt(el.getAttribute('data-val'), 10);
      if (!n) return;
      cur().input.length = n;
      touch();
      const inp = $('#lengthInput');
      if (inp) inp.value = n;
      const box = $('#directives');
      if (box) box.innerHTML = directivesHtml(cur());
    },

    useCandidate: function (el) {
      const p = cur();
      const c = D.buildCandidates(p)[parseInt(el.getAttribute('data-i'), 10)];
      if (!c) return;
      if (p.rq.question.trim() && p.rq.question.trim() !== c.text && !window.confirm('いま書いてある問いを、この候補で置き換えますか？')) return;
      p.rq.question = c.text;
      touch();
      const ta = $('#rqQuestion');
      ta.value = c.text;
      $('#rqChecks').innerHTML = checksHtml(p);
      renderSteps();
      ta.focus();
      ta.scrollIntoView({ block: 'center', behavior: 'smooth' });
      toast('候補を入れました。自分の言葉に直しましょう');
    },

    regen: function () {
      const p = cur();
      if (p.outline.sections.length && !window.confirm('構成を初期の形に戻します（目安字数と並び順がリセットされます）。メモと文献の割り当ては残ります。よろしいですか？')) return;
      generateOutline(p);
      touch();
      RENDER.outline();
      renderSteps();
      toast('構成をつくり直しました');
    },

    realloc: function () {
      reallocate(cur());
      touch();
      RENDER.outline();
      toast('字数を配分し直しました');
    },

    points: function (el) {
      const p = cur();
      const o = p.outline;
      const next = clamp((o.points || 1) + parseInt(el.getAttribute('data-d'), 10), 1, 4);
      if (next === o.points) return;
      o.points = next;
      generateOutline(p);
      touch();
      RENDER.outline();
    },

    secMove: function (el) {
      const secs = cur().outline.sections;
      const key = el.getAttribute('data-key');
      const d = parseInt(el.getAttribute('data-d'), 10);
      const i = secs.map(function (s) { return s.key; }).indexOf(key);
      const j = i + d;
      if (i < 0 || j < 0 || j >= secs.length) return;
      const tmp = secs[i];
      secs[i] = secs[j];
      secs[j] = tmp;
      touch();
      $('#sections').innerHTML = sectionsHtml(cur());
      $('#dist').innerHTML = distHtml(cur());
      const moved = $$('#sections .sec').filter(function (li) { return li.getAttribute('data-key') === key; })[0];
      if (moved) {
        const btn = $('[data-act="secMove"][data-d="' + d + '"]:not([disabled])', moved) || $('[data-act="secMove"]:not([disabled])', moved);
        if (btn) btn.focus();
      }
    },

    secDel: function (el) {
      const p = cur();
      const key = el.getAttribute('data-key');
      const sec = findSec(key);
      if (!sec) return;
      if (!window.confirm('「' + sec.title + '」を削除します。' + (sec.memo.trim() ? '書いたメモも消えます。' : '') + 'よろしいですか？')) return;
      p.outline.sections = p.outline.sections.filter(function (s) { return s.key !== key; });
      touch();
      $('#sections').innerHTML = sectionsHtml(p);
      $('#dist').innerHTML = distHtml(p);
    },

    secAdd: function () {
      const p = cur();
      const secs = p.outline.sections;
      const N = lengthOf(p);
      const sec = { key: uid('custom_'), part: 'body', title: '本論：追加セクション', titleEdited: true, chars: Math.max(50, Math.round(N * 0.1 / 50) * 50), memo: '', refs: [] };
      let at = secs.length;
      for (let i = 0; i < secs.length; i++) { if (secs[i].part === 'concl') { at = i; break; } }
      secs.splice(at, 0, sec);
      touch();
      $('#sections').innerHTML = sectionsHtml(p);
      $('#dist').innerHTML = distHtml(p);
      const li = $$('#sections .sec').filter(function (x) { return x.getAttribute('data-key') === sec.key; })[0];
      if (li) { const t = $('.sec-title', li); t.focus(); t.select(); }
    },

    unsetSecRef: function (el) {
      const sec = findSec(el.getAttribute('data-key'));
      if (!sec) return;
      const id = el.getAttribute('data-id');
      sec.refs = sec.refs.filter(function (x) { return x !== id; });
      touch();
      const li = el.closest('.sec');
      $('.ref-chips', li).innerHTML = refChipsHtml(cur(), sec);
      $$('input[data-secref]', li).forEach(function (cb) { if (cb.value === id) cb.checked = false; });
    },

    copyOutline: function () { copyText(outlineText(cur()), '構成メモをコピーしました'); },
    saveOutline: function () {
      download('構成メモ_' + fileSafe(cur().input.theme) + '.txt', outlineText(cur()));
      toast('テキストファイルを保存しました');
    },
    printOutline: function () { printOutline(cur()); },

    tab: function (el) {
      const p = cur();
      const t = el.getAttribute('data-tab') === 'cinii' ? 'cinii' : 's2';
      if (p.search.tab === t) return;
      p.search.tab = t;
      touch();
      RENDER.search();
      const on = $('.tab.is-on');
      if (on) on.focus();
    },

    suggest: function (el) {
      const p = cur();
      const q = el.getAttribute('data-q');
      if (p.search.tab === 's2') p.search.qS2 = q; else p.search.qCinii = q;
      touch();
      $('#searchQ').value = q;
      runSearch(false);
    },

    searchMore: function () { runSearch(true); },
    searchRetry: function () {
      const R = resultsOf(cur().search.tab);
      if (R.params && R.items.length) runSearch(true); else runSearch(false);
    },
    cancelSearch: cancelSearch,

    toCinii: function () {
      const p = cur();
      const R = resultsOf('s2');
      p.search.qCinii = (R.params && R.params.q) || p.search.qS2;
      p.search.tab = 'cinii';
      touch();
      RENDER.search();
      runSearch(false);
    },

    addRef: function (el) {
      if (!addRef(parseInt(el.getAttribute('data-i'), 10))) return;
      el.disabled = true;
      el.classList.add('is-added');
      el.textContent = '追加済み ✓';
      renderSteps();
      const next = $('#step-search .step-foot .primary');
      if (next) next.textContent = '引用を整える（' + cur().refs.length + '件） →';
      toast('文献リストに追加しました');
    },

    copyRef: function (el) {
      const r = findRef(el.getAttribute('data-id'));
      if (r) copyText(C.format(r, cur().cite.style, refNums[r.id] || 1).text, '引用の雛形をコピーしました');
    },
    copyIntext: function (el) {
      const r = findRef(el.getAttribute('data-id'));
      if (r) copyText(C.format(r, cur().cite.style, refNums[r.id] || 1).intext, '本文中の形をコピーしました');
    },
    copyAll: function () {
      copyText(bibliographyLines(cur()).join('\n'), '参考文献リストをコピーしました');
    },

    addManual: function () {
      const p = cur();
      const r = normalizeRef({ source: 'manual', type: 'book' });
      p.refs.push(r);
      touch();
      openRefId = r.id;
      RENDER.cite();
      renderSteps();
      const li = $$('#refs .ref').filter(function (x) { return x.getAttribute('data-id') === r.id; })[0];
      if (li) {
        li.scrollIntoView({ block: 'center' });
        const first = $('.ref-form input[data-field="year"]', li);
        if (first) first.focus();
      }
    },

    delRef: function (el) {
      const p = cur();
      const id = el.getAttribute('data-id');
      const r = findRef(id);
      if (!r) return;
      if (!window.confirm('「' + (r.title || C.shortLabel(r)) + '」を文献リストから削除しますか？')) return;
      p.refs = p.refs.filter(function (x) { return x.id !== id; });
      p.outline.sections.forEach(function (s) { s.refs = s.refs.filter(function (x) { return x !== id; }); });
      touch();
      RENDER.cite();
      renderSteps();
    }
  };

  document.addEventListener('click', function (e) {
    const el = e.target.closest ? e.target.closest('[data-act]') : null;
    if (el) {
      if (el.disabled) return;
      const fn = ACT[el.getAttribute('data-act')];
      if (fn) fn(el, e);
      return;
    }
    /* ダイアログの外側（背景）を押したら閉じる */
    if (e.target.tagName === 'DIALOG') closeDialogs();
  });

  document.addEventListener('submit', function (e) {
    if (e.target.getAttribute('data-form') === 'search') {
      e.preventDefault();
      runSearch(false);
    }
  });

  /* ---------- 入力の反映 ---------- */

  function afterBind(path) {
    const p = cur();
    if (path === 'input.theme') refreshChromeSoon();
    if (path === 'input.prompt' || path === 'input.length') {
      const box = $('#directives');
      if (box) box.innerHTML = directivesHtml(p);
    }
    if (path.indexOf('rq.fields.') === 0) {
      const ul = $('#candidates');
      if (ul) ul.innerHTML = candidatesHtml(p);
      const ck = $('#rqChecks');
      if (ck) ck.innerHTML = checksHtml(p);
    }
    if (path === 'rq.question') {
      const ck = $('#rqChecks');
      if (ck) ck.innerHTML = checksHtml(p);
      refreshChromeSoon();
    }
    if (path === 'cite.style' || path === 'cite.order') RENDER.cite();
  }

  function onInput(e) {
    const el = e.target;
    if (!el || !el.getAttribute) return;

    /* 文字入力は input、選択式（ラジオ・チェック・セレクト・ファイル）は change で1回だけ処理する */
    const textual = el.tagName === 'TEXTAREA' || (el.tagName === 'INPUT' && ['radio', 'checkbox', 'file'].indexOf(el.type) === -1);
    if (textual ? e.type !== 'input' : e.type !== 'change') return;

    if (el.id === 'projectSelect') { switchProject(el.value); return; }
    if (el.name === 'uiTheme') {
      if (el.checked) { state.ui.theme = el.value; applyTheme(); save(); }
      return;
    }
    if (el.id === 'importFile') { importFile(el); return; }

    const bind = el.getAttribute('data-bind');
    if (bind) {
      if (el.type === 'radio' && !el.checked) return;
      let val = el.type === 'checkbox' ? el.checked : el.value;
      if (el.hasAttribute('data-num')) { val = parseInt(val, 10); if (!isFinite(val)) val = 0; }
      setPath(cur(), bind, val);
      touch();
      afterBind(bind);
      return;
    }

    const secKey = el.getAttribute('data-sec');
    if (secKey) {
      const sec = findSec(secKey);
      if (!sec) return;
      const field = el.getAttribute('data-field');
      if (field === 'title') { sec.title = el.value; sec.titleEdited = true; }
      if (field === 'chars') {
        sec.chars = Math.max(0, parseInt(el.value, 10) || 0);
        $('#dist').innerHTML = distHtml(cur());
      }
      if (field === 'memo') sec.memo = el.value;
      if (field === 'memo' || field === 'chars') {
        const li = el.closest('.sec');
        const w = $('.memo-warn', li);
        const msg = memoWarnText(sec);
        w.textContent = msg;
        w.hidden = !msg;
      }
      touch();
      return;
    }

    const secRef = el.getAttribute('data-secref');
    if (secRef) {
      const sec = findSec(secRef);
      if (!sec) return;
      sec.refs = sec.refs.filter(function (x) { return x !== el.value; });
      if (el.checked) sec.refs.push(el.value);
      touch();
      $('.ref-chips', el.closest('.sec')).innerHTML = refChipsHtml(cur(), sec);
      return;
    }

    const refId = el.getAttribute('data-ref');
    if (refId) {
      const r = findRef(refId);
      if (!r) return;
      const field = el.getAttribute('data-field');
      if (field === 'authors') r.authors = el.value.split('\n').map(function (a) { return a.trim(); }).filter(Boolean);
      else r[field] = el.value;
      touch();
      if (field !== 'note') refreshRefCard(refId);
    }
  }

  document.addEventListener('input', onInput);
  document.addEventListener('change', onInput);

  function importFile(input) {
    const file = input.files && input.files[0];
    input.value = '';
    if (!file) return;
    const reader = new FileReader();
    reader.onload = function () {
      let data = null;
      try { data = JSON.parse(String(reader.result)); } catch (e) { data = null; }
      if (!data || typeof data !== 'object' || !data.projects || typeof data.projects !== 'object') {
        toast('読み込めませんでした。このアプリで書き出したJSONファイルを選んでください。');
        return;
      }
      const ids = Object.keys(data.projects);
      const dup = ids.filter(function (id) { return state.projects[id]; }).length;
      if (dup && !window.confirm('同じレポートが ' + dup + ' 件あります。ファイルの内容で上書きしますか？')) return;
      let last = null;
      ids.forEach(function (id) {
        const p = normalizeProject(data.projects[id]);
        state.projects[p.id] = p;
        last = p.id;
      });
      if (!ids.length) { toast('ファイルにレポートが入っていませんでした。'); return; }
      if (typeof data.currentId === 'string' && state.projects[data.currentId]) last = data.currentId;
      state.currentId = last;
      saveNow();
      closeDialogs();
      renderProjectSelect();
      go('input');
      toast(ids.length + ' 件のレポートを読み込みました');
    };
    reader.onerror = function () { toast('ファイルを読み込めませんでした。'); };
    reader.readAsText(file);
  }

  /* ---------- 起動 ---------- */

  window.addEventListener('pagehide', function () { if (saveTimer) saveNow(); });
  document.addEventListener('visibilitychange', function () { if (document.hidden && saveTimer) saveNow(); });

  applyTheme();
  $('#storageWarn').hidden = storageOk;
  renderProjectSelect();
  go(state.ui.step);
})();
