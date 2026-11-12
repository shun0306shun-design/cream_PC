/* =========================================================
   講座・集客ツール  app.js
   ---------------------------------------------------------
   ・素の JavaScript だけで動きます（外部との通信なし）
   ・データはすべてブラウザの localStorage に保存します
     キー：dotokoza_settings / dotokoza_prices / dotokoza_qa
           dotokoza_records / dotokoza_forms
   ========================================================= */
(function () {
  'use strict';

  const DATA = window.APP_DATA;
  const PREFIX = DATA.appId + '_';
  const STORE_NAMES = ['settings', 'prices', 'qa', 'records', 'forms'];
  const TAB_NAMES = ['price', 'course', 'flyer', 'qa', 'records', 'settings'];
  const FONT_SIZES = ['standard', 'large', 'xlarge'];
  const FIRM_STATUSES = ['contract', 'active', 'done']; /* 「月1件」の枠を使う状態 */
  const LOGO_LIMIT = 200 * 1024; /* ロゴ画像の保存サイズの上限（文字数） */
  const A4_W = 700;
  const A4_H = 1020;
  const A4_FIT_MAX = 1.36; /* チラシの文字を大きくする上限（倍率） */
  const A4_FIT_MIN = 0.52; /* 小さくする下限（倍率） */
  const WEEK = ['日', '月', '火', '水', '木', '金', '土'];
  const FULL_NUM = ['１', '２', '３', '４', '５', '６', '７', '８', '９'];

  let state = null;
  let currentTab = 'price';
  let storageOK = true;
  const memoryStore = {};
  let qaCat = 'all';
  const qaOpen = new Set();
  let editingQaId = null;
  let editingRecId = null;
  const recFilter = { status: 'all', type: 'all', sort: 'desc' };
  let toastTimer = 0;
  let formsTimer = 0;

  /* =========================================================
     小さな道具
     ========================================================= */
  const $ = (sel, root) => (root || document).querySelector(sel);
  const $$ = (sel, root) => Array.from((root || document).querySelectorAll(sel));

  function esc(v) {
    return String(v == null ? '' : v)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }
  function nl2br(v) {
    return esc(v).replace(/\r?\n/g, '<br>');
  }
  function str(v) {
    return typeof v === 'string' ? v : (v == null ? '' : String(v));
  }
  function clone(o) {
    return JSON.parse(JSON.stringify(o));
  }
  function yen(n) {
    return Math.round(Number(n) || 0).toLocaleString('ja-JP') + '円';
  }
  function clampInt(v, min, max, fallback) {
    let n = parseInt(v, 10);
    if (!isFinite(n)) n = (fallback == null ? min : fallback);
    return Math.min(max, Math.max(min, n));
  }
  function newId(prefix) {
    return prefix + '-' + Date.now().toString(36) + '-' + Math.random().toString(36).slice(2, 8);
  }
  function pad2(n) {
    return String(n).padStart(2, '0');
  }
  function unique(list) {
    return list.filter((v, i) => list.indexOf(v) === i);
  }

  /* ---------- 日付 ---------- */
  function todayISO() {
    const d = new Date();
    return d.getFullYear() + '-' + pad2(d.getMonth() + 1) + '-' + pad2(d.getDate());
  }
  function parseISO(s) {
    const m = /^(\d{4})-(\d{2})-(\d{2})$/.exec(s || '');
    if (!m) return null;
    const d = new Date(Number(m[1]), Number(m[2]) - 1, Number(m[3]));
    return isNaN(d.getTime()) ? null : d;
  }
  function dateJP(s, withYear) {
    const d = parseISO(s);
    if (!d) return '日付なし';
    return (withYear ? d.getFullYear() + '年' : '') + (d.getMonth() + 1) + '月' + d.getDate() + '日（' + WEEK[d.getDay()] + '）';
  }
  function todayJP() {
    const d = new Date();
    return d.getFullYear() + '年' + (d.getMonth() + 1) + '月' + d.getDate() + '日';
  }
  function todayWareki() {
    const d = new Date();
    const y = d.getFullYear() - 2018;
    return '令和' + (y === 1 ? '元' : y) + '年' + (d.getMonth() + 1) + '月' + d.getDate() + '日';
  }
  function monthJP(ym) {
    const m = /^(\d{4})-(\d{2})/.exec(ym || '');
    return m ? Number(m[1]) + '年' + Number(m[2]) + '月' : '';
  }
  function timeJP(t) {
    const m = /^(\d{1,2}):(\d{2})/.exec(t || '');
    if (!m) return '';
    const h = Number(m[1]);
    const min = Number(m[2]);
    return h + '時' + (min ? min + '分' : '');
  }

  /* =========================================================
     保存（localStorage）
     ========================================================= */
  function testStorage() {
    try {
      const k = PREFIX + '__test';
      window.localStorage.setItem(k, '1');
      window.localStorage.removeItem(k);
      return true;
    } catch (e) {
      return false;
    }
  }
  function loadKey(name) {
    try {
      const raw = storageOK ? window.localStorage.getItem(PREFIX + name) : memoryStore[name];
      if (raw == null) return null;
      return JSON.parse(raw);
    } catch (e) {
      return null;
    }
  }
  function saveKey(name, value) {
    const raw = JSON.stringify(value);
    if (!storageOK) {
      memoryStore[name] = raw;
      return false;
    }
    try {
      window.localStorage.setItem(PREFIX + name, raw);
      return true;
    } catch (e) {
      toast('保存できませんでした。データの量が多すぎる可能性があります。', 'error');
      return false;
    }
  }
  function saveAll() {
    STORE_NAMES.forEach((n) => saveKey(n, state[n]));
  }
  function removeAllKeys() {
    STORE_NAMES.forEach((n) => {
      delete memoryStore[n];
      if (storageOK) {
        try { window.localStorage.removeItem(PREFIX + n); } catch (e) { /* 何もしない */ }
      }
    });
  }

  /* ---------- 読み込んだデータを安全な形に整える ---------- */
  function isImageDataUrl(v) {
    return typeof v === 'string' && /^data:image\/(png|jpeg|gif|webp);base64,[A-Za-z0-9+/=]+$/.test(v);
  }
  function sanitizeSettings(s) {
    const d = clone(DATA.defaultSettings);
    if (!s || typeof s !== 'object') return d;
    d.shopName = str(s.shopName);
    d.contact = str(s.contact);
    d.address = str(s.address);
    d.logo = isImageDataUrl(s.logo) ? s.logo : '';
    d.fontSize = FONT_SIZES.includes(s.fontSize) ? s.fontSize : 'standard';
    return d;
  }
  function priceFieldMeta(group, key) {
    const g = DATA.priceFields.find((x) => x.group === group);
    const f = g && g.fields.find((x) => x.key === key);
    return f || {};
  }
  function sanitizePrices(p) {
    const d = clone(DATA.defaultPrices);
    if (!p || typeof p !== 'object') return d;
    Object.keys(d).forEach((group) => {
      Object.keys(d[group]).forEach((key) => {
        const v = p[group] ? p[group][key] : undefined;
        if (v === '' || v == null) return;
        const n = Math.round(Number(v));
        const meta = priceFieldMeta(group, key);
        const min = meta.min || 0;
        if (!isFinite(n) || n < min) return;
        if (meta.max != null && n > meta.max) return;
        d[group][key] = n;
      });
    });
    return d;
  }
  function sanitizeQA(list) {
    if (!Array.isArray(list)) return null;
    return list
      .filter((x) => x && typeof x === 'object' && str(x.q).trim())
      .map((x) => ({
        id: str(x.id) || newId('qa'),
        cat: str(x.cat).trim() || 'その他',
        q: str(x.q),
        a: str(x.a)
      }));
  }
  function sanitizeRecords(list) {
    if (!Array.isArray(list)) return null;
    const statusIds = DATA.statuses.map((s) => s.id);
    return list
      .filter((x) => x && typeof x === 'object' && str(x.name).trim())
      .map((x) => ({
        id: str(x.id) || newId('rec'),
        name: str(x.name),
        type: x.type === 'course' ? 'course' : 'web',
        date: parseISO(str(x.date)) ? str(x.date) : '',
        amount: Math.max(0, Math.round(Number(x.amount) || 0)),
        status: statusIds.includes(x.status) ? x.status : 'inquiry',
        memo: str(x.memo)
      }));
  }
  function sanitizeForms(f) {
    const d = clone(DATA.defaultForms);
    if (!f || typeof f !== 'object') return d;
    if (TAB_NAMES.includes(f.lastTab)) d.lastTab = f.lastTab;
    if (f.simMode === 'web' || f.simMode === 'course') d.simMode = f.simMode;

    const w = (f.simWeb && typeof f.simWeb === 'object') ? f.simWeb : {};
    d.simWeb = {
      client: str(w.client),
      pages: clampInt(w.pages, 1, 20, d.simWeb.pages),
      photo: !!w.photo,
      contactForm: !!w.contactForm,
      map: !!w.map,
      domain: w.domain === undefined ? d.simWeb.domain : !!w.domain,
      update: ['none', 'light', 'full'].includes(w.update) ? w.update : 'none'
    };

    const c = (f.simCourse && typeof f.simCourse === 'object') ? f.simCourse : {};
    d.simCourse = {
      client: str(c.client),
      style: c.style === 'private' ? 'private' : 'group',
      times: clampInt(c.times, 1, 12, d.simCourse.times),
      people: clampInt(c.people, 1, 30, d.simCourse.people),
      visit: !!c.visit,
      text: c.text === undefined ? d.simCourse.text : !!c.text
    };

    const k = (f.cur && typeof f.cur === 'object') ? f.cur : {};
    const themeIds = DATA.themes.map((t) => t.id);
    d.cur = {
      title: str(k.title),
      audience: DATA.audiences.some((a) => a.id === k.audience) ? k.audience : d.cur.audience,
      count: clampInt(k.count, 1, 8, d.cur.count),
      minutes: [60, 90, 120].includes(Number(k.minutes)) ? Number(k.minutes) : d.cur.minutes,
      themes: Array.isArray(k.themes) ? k.themes.filter((id) => themeIds.includes(id)) : d.cur.themes
    };

    const fl = (f.flyer && typeof f.flyer === 'object') ? f.flyer : {};
    d.flyer = {
      title: str(fl.title),
      when: str(fl.when),
      where: str(fl.where),
      fee: str(fl.fee),
      cap: str(fl.cap),
      apply: str(fl.apply),
      pr: str(fl.pr),
      tone: fl.tone === 'formal' ? 'formal' : 'soft'
    };
    return d;
  }

  function defaultState() {
    return {
      settings: clone(DATA.defaultSettings),
      prices: clone(DATA.defaultPrices),
      qa: DATA.defaultQA.map((x, i) => ({ id: 'qa-init-' + (i + 1), cat: x.cat, q: x.q, a: x.a })),
      records: [],
      forms: clone(DATA.defaultForms)
    };
  }
  function loadState() {
    const def = defaultState();
    state = {
      settings: sanitizeSettings(loadKey('settings')),
      prices: sanitizePrices(loadKey('prices')),
      qa: sanitizeQA(loadKey('qa')) || def.qa,
      records: sanitizeRecords(loadKey('records')) || [],
      forms: sanitizeForms(loadKey('forms'))
    };
  }
  function saveFormsSoon() {
    clearTimeout(formsTimer);
    formsTimer = setTimeout(() => saveKey('forms', state.forms), 300);
  }
  function flushForms() {
    clearTimeout(formsTimer);
    if (state) saveKey('forms', state.forms);
  }

  /* =========================================================
     画面の共通部品（お知らせ・確認・コピー・印刷）
     ========================================================= */
  function toast(message, kind) {
    const el = $('#toast');
    el.textContent = message;
    el.className = 'toast is-show' + (kind === 'error' ? ' is-error' : '');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { el.className = 'toast'; }, kind === 'error' ? 5000 : 3200);
  }

  function openDialog(dlg) {
    if (typeof dlg.showModal === 'function') {
      if (!dlg.open) dlg.showModal();
    } else {
      dlg.classList.add('is-fallback');
      dlg.setAttribute('open', '');
    }
  }
  function closeDialog(dlg) {
    if (typeof dlg.close === 'function') {
      if (dlg.open) dlg.close();
    } else {
      dlg.removeAttribute('open');
      dlg.dispatchEvent(new Event('close'));
    }
  }

  /* 「はい／やめる」をたずねる。結果を true / false で返す */
  function confirmDialog(opt) {
    return new Promise((resolve) => {
      const dlg = $('#dlg-confirm');
      const ok = $('#dlg-confirm-ok');
      const cancel = $('#dlg-confirm-cancel');
      $('#dlg-confirm-title').textContent = opt.title || '確認';
      $('#dlg-confirm-message').textContent = opt.message || '';
      ok.textContent = opt.okLabel || 'はい';
      cancel.textContent = opt.cancelLabel || 'やめる';
      ok.className = 'btn btn-primary' + (opt.danger ? ' btn-danger' : '');
      let result = false;
      const onOk = () => { result = true; closeDialog(dlg); };
      const onCancel = () => { result = false; closeDialog(dlg); };
      const onClose = () => {
        ok.removeEventListener('click', onOk);
        cancel.removeEventListener('click', onCancel);
        dlg.removeEventListener('close', onClose);
        resolve(result);
      };
      ok.addEventListener('click', onOk);
      cancel.addEventListener('click', onCancel);
      dlg.addEventListener('close', onClose);
      openDialog(dlg);
      (opt.danger ? cancel : ok).focus();
    });
  }

  function copyText(text, okMessage) {
    const done = () => toast(okMessage || 'コピーしました');
    const fallback = () => {
      const ta = document.createElement('textarea');
      ta.value = text;
      ta.setAttribute('readonly', '');
      ta.style.position = 'fixed';
      ta.style.top = '0';
      ta.style.left = '0';
      ta.style.opacity = '0';
      document.body.appendChild(ta);
      ta.focus();
      ta.select();
      let ok = false;
      try { ok = document.execCommand('copy'); } catch (e) { ok = false; }
      ta.remove();
      if (ok) done();
      else toast('コピーできませんでした。文章を長押しして、手でコピーしてください。', 'error');
    };
    if (navigator.clipboard && window.isSecureContext) {
      navigator.clipboard.writeText(text).then(done, fallback);
    } else {
      fallback();
    }
  }

  /* 印刷：えらんだ書類だけを #print-root に写してから印刷する */
  function printNode(node) {
    if (!node) return;
    if (typeof window.print !== 'function') {
      toast('このブラウザでは印刷できません。パソコンでお試しください。', 'error');
      return;
    }
    const root = $('#print-root');
    root.innerHTML = '';
    const copy = node.cloneNode(true);
    copy.removeAttribute('id');
    $$('[id]', copy).forEach((el) => el.removeAttribute('id'));
    root.appendChild(copy);
    toast('印刷の画面をひらきます');
    setTimeout(() => window.print(), 80);
  }

  /* ---------- 下に入れる店名・連絡先・ロゴ ---------- */
  function hasFooter() {
    const s = state.settings;
    return !!(s.shopName.trim() || s.contact.trim() || s.address.trim() || s.logo);
  }
  function footerHTML() {
    if (!hasFooter()) return '';
    const s = state.settings;
    let h = '<footer class="doc-footer">';
    if (s.logo) h += '<img class="doc-footer-logo" src="' + esc(s.logo) + '" alt="">';
    h += '<div class="doc-footer-text">';
    if (s.shopName.trim()) h += '<strong>' + esc(s.shopName.trim()) + '</strong>';
    if (s.contact.trim()) h += '<span>' + esc(s.contact.trim()) + '</span>';
    if (s.address.trim()) h += '<span>' + esc(s.address.trim()) + '</span>';
    h += '</div></footer>';
    return h;
  }
  function footerText() {
    const s = state.settings;
    return [s.shopName, s.contact, s.address].map((v) => v.trim()).filter(Boolean).join('\n');
  }

  /* ---------- 設定を画面全体に反映 ---------- */
  function applySettings() {
    document.documentElement.setAttribute('data-font', state.settings.fontSize);
    $('#header-shop').textContent = state.settings.shopName.trim();
    $$('[data-fontsize]').forEach((b) => {
      b.setAttribute('aria-pressed', String(b.dataset.fontsize === state.settings.fontSize));
    });
  }

  /* =========================================================
     タブの切り替え
     ========================================================= */
  const RENDER = {
    price: renderQuote,
    course: renderCurriculum,
    flyer: renderFlyer,
    qa: renderQA,
    records: renderRecords,
    settings: renderSettings
  };
  function showTab(name, keepScroll) {
    if (!RENDER[name]) name = 'price';
    currentTab = name;
    $$('.tab-panel').forEach((p) => { p.hidden = (p.id !== 'tab-' + name); });
    $$('.tab-btn').forEach((b) => {
      if (b.dataset.tab === name) b.setAttribute('aria-current', 'page');
      else b.removeAttribute('aria-current');
    });
    RENDER[name]();
    state.forms.lastTab = name;
    saveFormsSoon();
    if (!keepScroll) window.scrollTo(0, 0);
  }

  /* =========================================================
     入力欄と state.forms のやりとり
     ========================================================= */
  function setRadio(form, name, value) {
    $$('input[name="' + name + '"]', form).forEach((i) => { i.checked = (i.value === value); });
  }
  function getRadio(form, name, fallback) {
    const el = $('input[name="' + name + '"]:checked', form);
    return el ? el.value : fallback;
  }
  function fillForms() {
    const F = state.forms;

    const w = $('#form-sim-web');
    w.elements.client.value = F.simWeb.client;
    w.elements.pages.value = F.simWeb.pages;
    w.elements.photo.checked = F.simWeb.photo;
    w.elements.contactForm.checked = F.simWeb.contactForm;
    w.elements.map.checked = F.simWeb.map;
    w.elements.domain.checked = F.simWeb.domain;
    setRadio(w, 'update', F.simWeb.update);

    const c = $('#form-sim-course');
    c.elements.client.value = F.simCourse.client;
    setRadio(c, 'style', F.simCourse.style);
    c.elements.times.value = F.simCourse.times;
    c.elements.people.value = F.simCourse.people;
    c.elements.visit.checked = F.simCourse.visit;
    c.elements.text.checked = F.simCourse.text;

    const k = $('#form-cur');
    k.elements.title.value = F.cur.title;
    setRadio(k, 'audience', F.cur.audience);
    k.elements.count.value = F.cur.count;
    k.elements.minutes.value = String(F.cur.minutes);
    $$('input[name="themes"]', k).forEach((i) => { i.checked = F.cur.themes.includes(i.value); });

    const f = $('#form-flyer');
    ['title', 'when', 'where', 'fee', 'cap', 'apply', 'pr'].forEach((key) => {
      f.elements[key].value = F.flyer[key];
    });
    setRadio(f, 'tone', F.flyer.tone);
  }
  function readSimForms() {
    const w = $('#form-sim-web');
    state.forms.simWeb = {
      client: w.elements.client.value,
      pages: clampInt(w.elements.pages.value, 1, 20, 1),
      photo: w.elements.photo.checked,
      contactForm: w.elements.contactForm.checked,
      map: w.elements.map.checked,
      domain: w.elements.domain.checked,
      update: getRadio(w, 'update', 'none')
    };
    const c = $('#form-sim-course');
    state.forms.simCourse = {
      client: c.elements.client.value,
      style: getRadio(c, 'style', 'group'),
      times: clampInt(c.elements.times.value, 1, 12, 1),
      people: clampInt(c.elements.people.value, 1, 30, 1),
      visit: c.elements.visit.checked,
      text: c.elements.text.checked
    };
  }
  function readCurForm() {
    const k = $('#form-cur');
    state.forms.cur = {
      title: k.elements.title.value,
      audience: getRadio(k, 'audience', DATA.audiences[0].id),
      count: clampInt(k.elements.count.value, 1, 8, 1),
      minutes: Number(k.elements.minutes.value) || 90,
      themes: $$('input[name="themes"]:checked', k).map((i) => i.value)
    };
  }
  function readFlyerForm() {
    const f = $('#form-flyer');
    const out = {};
    ['title', 'when', 'where', 'fee', 'cap', 'apply', 'pr'].forEach((key) => {
      out[key] = f.elements[key].value;
    });
    out.tone = getRadio(f, 'tone', 'soft');
    state.forms.flyer = out;
  }

  /* =========================================================
     1. 料金シミュレーター
     ========================================================= */
  function buildQuote() {
    const P = state.prices;
    const notesCommon = [
      '表示の金額が、お支払いの総額（概算）です。',
      '内容のご相談のあと、正式なお見積りをお出しします。',
      'このお見積りの有効期限は、発行日から1か月です。'
    ];

    if (state.forms.simMode === 'course') {
      const f = state.forms.simCourse;
      const C = P.course;
      const isPrivate = f.style === 'private';
      const unit = isPrivate ? C.private : C.group;
      const styleName = isPrivate ? '個別講座（1人ずつ）' : 'グループ講座';
      const lesson = unit * f.people * f.times;
      const items = [{
        name: '受講料：' + styleName,
        detail: yen(unit) + ' × ' + f.people + '人 × ' + f.times + '回',
        amount: lesson
      }];
      if (C.discountRate > 0 && f.times >= C.discountFrom) {
        const off = Math.round(lesson * C.discountRate / 100);
        if (off > 0) {
          items.push({
            name: '回数割引',
            detail: C.discountFrom + '回以上で受講料を' + C.discountRate + '％引き',
            amount: -off
          });
        }
      }
      if (f.visit) {
        items.push({ name: '出張費', detail: yen(C.visit) + ' × ' + f.times + '回', amount: C.visit * f.times });
      }
      if (f.text) {
        items.push({ name: 'テキスト代（印刷した資料）', detail: yen(C.text) + ' × ' + f.people + '人', amount: C.text * f.people });
      }
      const total = items.reduce((s, it) => s + it.amount, 0);
      const perPerson = Math.ceil(total / f.people);
      const perOnce = Math.ceil(total / f.people / f.times);
      return {
        kind: 'course',
        client: f.client.trim(),
        subject: 'スマホ・AI講座（' + (isPrivate ? '個別' : 'グループ') + '・全' + f.times + '回・' + f.people + '名）',
        highlights: [
          { label: '合計', value: yen(total) },
          { label: '1人あたり', value: yen(perPerson) },
          { label: '1人・1回あたり', value: '約' + yen(perOnce) }
        ],
        sections: [{ title: '内訳', items: items, totalLabel: '合計', total: total }],
        notes: notesCommon.concat(['1人あたりの金額は、合計を人数で割り、1円未満を切り上げています。']),
        recordAmount: total,
        memo: '見積：' + styleName + ' 全' + f.times + '回・' + f.people + '名'
          + (f.visit ? '／出張あり' : '') + (f.text ? '／テキストあり' : '')
          + '。1人あたり ' + yen(perPerson)
      };
    }

    const f = state.forms.simWeb;
    const W = P.web;
    const initial = [{ name: '基本料金', detail: 'トップページ1枚・スマホ対応こみ', amount: W.base }];
    const monthly = [];
    const memo = ['全' + f.pages + 'ページ'];
    if (f.pages > 1) {
      initial.push({ name: '追加ページ', detail: yen(W.page) + ' × ' + (f.pages - 1) + 'ページ', amount: W.page * (f.pages - 1) });
    }
    if (f.photo) {
      initial.push({ name: '写真撮影', detail: 'お店に伺って撮影（1回）', amount: W.photo });
      memo.push('写真撮影');
    }
    if (f.contactForm) {
      initial.push({ name: 'お問い合わせフォーム', detail: '設置と動作の確認', amount: W.contactForm });
      memo.push('お問い合わせフォーム');
    }
    if (f.map) {
      initial.push({ name: 'Googleマップへのお店登録', detail: '登録の手続き代行', amount: W.map });
      memo.push('Googleマップ登録');
    }
    if (f.domain) {
      initial.push({ name: 'ドメイン・サーバー代行', detail: '手続きと最初の設定', amount: W.domainInit });
      monthly.push({ name: 'ドメイン・サーバー管理', detail: '実費こみ', amount: W.domainMonthly });
      memo.push('ドメイン・サーバー代行');
    }
    if (f.update === 'light') {
      monthly.push({ name: '更新代行 ライト', detail: '月1回までの書きかえ', amount: W.updateLight });
      memo.push('更新代行ライト');
    } else if (f.update === 'full') {
      monthly.push({ name: '更新代行 しっかり', detail: '月4回までの書きかえ', amount: W.updateFull });
      memo.push('更新代行しっかり');
    }
    const initialTotal = initial.reduce((s, it) => s + it.amount, 0);
    const monthlyTotal = monthly.reduce((s, it) => s + it.amount, 0);
    const sections = [{ title: '初期費用の内訳（最初の1回だけ）', items: initial, totalLabel: '初期費用の合計', total: initialTotal }];
    if (monthly.length) {
      sections.push({ title: '月額の内訳（毎月）', items: monthly, totalLabel: '月額の合計', total: monthlyTotal });
    } else {
      sections.push({ title: '月額の内訳（毎月）', items: [], empty: '毎月かかる費用はありません。' });
    }
    return {
      kind: 'web',
      client: f.client.trim(),
      subject: 'ホームページ制作（全' + f.pages + 'ページ）',
      highlights: [
        { label: '初期費用（最初の1回）', value: yen(initialTotal) },
        { label: '月額（毎月）', value: monthlyTotal ? yen(monthlyTotal) : 'なし' }
      ],
      sections: sections,
      notes: notesCommon,
      recordAmount: initialTotal,
      memo: '見積：' + memo.join('／') + '。月額 ' + (monthlyTotal ? yen(monthlyTotal) : 'なし')
    };
  }

  function quoteHTML(m) {
    let h = '<article class="doc doc-onepage quote-doc" id="quote-doc">';
    h += '<h3 class="doc-title">お見積書（概算）</h3>';
    h += '<p class="doc-date">発行日：' + esc(todayJP()) + '</p>';
    h += '<p class="quote-to">' + (m.client ? '<strong>' + esc(m.client) + '</strong>' : '<span class="quote-blank"></span>') + ' 様</p>';
    h += '<p class="quote-subject">件名：' + esc(m.subject) + '</p>';
    h += '<div class="quote-totals">';
    m.highlights.forEach((x) => {
      h += '<p class="quote-total"><span>' + esc(x.label) + '</span><strong>' + esc(x.value) + '</strong></p>';
    });
    h += '</div>';
    m.sections.forEach((sec) => {
      h += '<h4 class="doc-sub">' + esc(sec.title) + '</h4>';
      if (!sec.items.length) {
        h += '<p class="quote-none">' + esc(sec.empty || '') + '</p>';
        return;
      }
      h += '<table class="quote-table"><thead><tr><th scope="col">内容</th><th scope="col" class="num">金額</th></tr></thead><tbody>';
      sec.items.forEach((it) => {
        h += '<tr><td><span class="quote-name">' + esc(it.name) + '</span>'
          + (it.detail ? '<span class="quote-detail">' + esc(it.detail) + '</span>' : '')
          + '</td><td class="num">' + (it.amount < 0 ? '−' + esc(yen(-it.amount)) : esc(yen(it.amount))) + '</td></tr>';
      });
      h += '</tbody><tfoot><tr><th scope="row">' + esc(sec.totalLabel) + '</th><td class="num">' + esc(yen(sec.total)) + '</td></tr></tfoot></table>';
    });
    h += '<ul class="doc-notes">' + m.notes.map((n) => '<li>' + esc(n) + '</li>').join('') + '</ul>';
    h += footerHTML();
    h += '</article>';
    return h;
  }
  function quoteText(m) {
    const lines = ['【お見積書（概算）】', '発行日：' + todayJP(), (m.client || '　　　　') + ' 様', '件名：' + m.subject, ''];
    m.highlights.forEach((x) => lines.push('■ ' + x.label + '：' + x.value));
    m.sections.forEach((sec) => {
      lines.push('', '＜' + sec.title + '＞');
      if (!sec.items.length) {
        lines.push(sec.empty || '');
        return;
      }
      sec.items.forEach((it) => {
        lines.push('・' + it.name + (it.detail ? '（' + it.detail + '）' : '') + '：' + (it.amount < 0 ? '−' + yen(-it.amount) : yen(it.amount)));
      });
      lines.push(sec.totalLabel + '：' + yen(sec.total));
    });
    lines.push('');
    m.notes.forEach((n) => lines.push('※' + n));
    const ft = footerText();
    if (ft) lines.push('', '――――――――', ft);
    return lines.join('\n');
  }
  function renderQuote() {
    const mode = state.forms.simMode;
    $$('[data-sim]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.sim === mode)));
    $('#form-sim-web').hidden = (mode !== 'web');
    $('#form-sim-course').hidden = (mode !== 'course');
    $('#quote-out').innerHTML = quoteHTML(buildQuote());
  }

  /* =========================================================
     2. 講座カリキュラム生成（ひな形の組み合わせ）
     ========================================================= */
  function buildCurriculum() {
    const f = state.forms.cur;
    const aud = DATA.audiences.find((a) => a.id === f.audience) || DATA.audiences[0];
    const themes = DATA.themes.filter((t) => f.themes.includes(t.id));
    const n = clampInt(f.count, 1, 8, 4);
    if (!themes.length) return null;

    const sessions = [];
    if (n >= themes.length) {
      /* 回数がテーマ数以上：テーマごとに回を割りふる */
      const base = Math.floor(n / themes.length);
      const extra = n % themes.length;
      themes.forEach((t, i) => {
        const cnt = base + (i < extra ? 1 : 0);
        for (let j = 0; j < cnt; j++) {
          if (j < t.modules.length) {
            const m = t.modules[j];
            sessions.push({ theme: t, themeIndex: j, title: m.title, content: m.content.slice(), goal: m.goal, plus: m.plus || '', bring: t.bring.slice() });
          } else {
            const k = j - t.modules.length;
            const fl = DATA.fillers[k % DATA.fillers.length];
            const round = Math.floor(k / DATA.fillers.length);
            sessions.push({
              theme: t,
              themeIndex: j,
              title: t.short + '：' + fl.title + (round ? '（その' + (round + 1) + '）' : ''),
              content: fl.content.slice(),
              goal: fl.goal,
              plus: '',
              bring: t.bring.slice()
            });
          }
        }
      });
    } else {
      /* 回数がテーマ数より少ない：1回に複数のテーマをまとめる */
      const groups = [];
      for (let i = 0; i < n; i++) groups.push([]);
      themes.forEach((t, i) => groups[Math.floor(i * n / themes.length)].push(t));
      groups.forEach((g) => {
        if (g.length === 1) {
          const t = g[0];
          const m = t.modules[0];
          sessions.push({ theme: t, themeIndex: 0, title: m.title, content: m.content.slice(), goal: m.goal, plus: m.plus || '', bring: t.bring.slice() });
        } else {
          let content = [];
          let bring = [];
          g.forEach((t) => {
            content = content.concat(t.digest.content.map((c) => '【' + t.short + '】' + c));
            bring = bring.concat(t.bring);
          });
          sessions.push({
            theme: null,
            themeIndex: 0,
            title: g.map((t) => t.short).join('・') + ' まとめて体験',
            content: content,
            goal: g.map((t) => t.digest.goal).join('／'),
            plus: '',
            bring: bring
          });
        }
      });
    }

    /* 対象（だれ向けか）に合わせて、内容と持ち物を足す */
    sessions.forEach((s, i) => {
      let content = [];
      if (i === 0) content = content.concat(aud.intro);
      else if (aud.review) content.push(aud.review);
      content = content.concat(s.content);
      if (aud.usePlus && s.plus) content.push('もう一歩：' + s.plus);
      if (aud.useBiz && s.theme && s.theme.biz.length) {
        content.push('お店で活かす：' + s.theme.biz[s.themeIndex % s.theme.biz.length]);
      }
      if (n >= 2 && i === n - 1) content.push(DATA.curriculumClosing);
      s.no = i + 1;
      s.content = content;
      s.bring = unique(s.bring.concat(aud.bring));
    });

    const shorts = themes.map((t) => t.short);
    const autoTitle = aud.titlePrefix + (shorts.length > 3 ? 'スマホ活用' : shorts.join('・')) + '講座';
    const minutes = [60, 90, 120].includes(Number(f.minutes)) ? Number(f.minutes) : 90;
    return {
      title: f.title.trim() || autoTitle,
      audience: aud,
      count: n,
      minutes: minutes,
      themes: themes,
      sessions: sessions,
      notes: [
        aud.note,
        '1回 ' + minutes + '分です（途中で' + (minutes >= 90 ? '10分' : '5分') + 'ほどの休憩をふくみます）。',
        '参加される方のようすに合わせて、内容や順番を変えることがあります。'
      ]
    };
  }
  function curriculumHTML(c) {
    let h = '<article class="doc cur-doc" id="cur-doc">';
    h += '<h3 class="doc-title">' + esc(c.title) + '</h3>';
    h += '<ul class="doc-meta">';
    h += '<li><b>対象</b>' + esc(c.audience.name) + '</li>';
    h += '<li><b>回数</b>全' + c.count + '回</li>';
    h += '<li><b>1回の時間</b>' + c.minutes + '分</li>';
    h += '<li><b>テーマ</b>' + esc(c.themes.map((t) => t.short).join('、')) + '</li>';
    h += '</ul>';
    h += '<table class="cur-table"><thead><tr>'
      + '<th scope="col" class="c-no">回</th><th scope="col" class="c-title">タイトル</th><th scope="col" class="c-content">内容</th>'
      + '<th scope="col" class="c-bring">持ち物</th><th scope="col" class="c-goal">到達ゴール</th></tr></thead><tbody>';
    c.sessions.forEach((s) => {
      h += '<tr>';
      h += '<td class="c-no" data-label="回">第' + s.no + '回</td>';
      h += '<td class="c-title" data-label="タイトル">' + esc(s.title) + '</td>';
      h += '<td class="c-content" data-label="内容"><ul>' + s.content.map((x) => '<li>' + esc(x) + '</li>').join('') + '</ul></td>';
      h += '<td class="c-bring" data-label="持ち物"><ul>' + s.bring.map((x) => '<li>' + esc(x) + '</li>').join('') + '</ul></td>';
      h += '<td class="c-goal" data-label="到達ゴール">' + esc(s.goal) + '</td>';
      h += '</tr>';
    });
    h += '</tbody></table>';
    h += '<ul class="doc-notes">' + c.notes.map((x) => '<li>' + esc(x) + '</li>').join('') + '</ul>';
    h += footerHTML();
    h += '</article>';
    return h;
  }
  function curriculumText(c) {
    const lines = [
      '【' + c.title + '】',
      '対象：' + c.audience.name,
      '回数：全' + c.count + '回（1回 ' + c.minutes + '分）',
      'テーマ：' + c.themes.map((t) => t.short).join('、')
    ];
    c.sessions.forEach((s) => {
      lines.push('', '■ 第' + s.no + '回　' + s.title, '内容：');
      s.content.forEach((x) => lines.push('・' + x));
      lines.push('持ち物：' + s.bring.join('、'));
      lines.push('到達ゴール：' + s.goal);
    });
    lines.push('');
    c.notes.forEach((x) => lines.push('※' + x));
    const ft = footerText();
    if (ft) lines.push('', '――――――――', ft);
    return lines.join('\n');
  }
  function renderCurriculum() {
    const c = buildCurriculum();
    $('#cur-out').innerHTML = c
      ? curriculumHTML(c)
      : '<p class="notice notice-warn">テーマを1つ以上えらんでください。</p>';
  }

  /* =========================================================
     3. チラシ／案内文ジェネレーター
     ========================================================= */
  const FLYER_KEYS = ['when', 'where', 'fee', 'cap', 'apply'];
  function getFlyer() {
    const src = state.forms.flyer;
    const f = {};
    ['title', 'when', 'where', 'fee', 'cap', 'apply', 'pr'].forEach((k) => { f[k] = str(src[k]).trim(); });
    f.tone = DATA.tones[src.tone] ? src.tone : 'soft';
    f.titleShown = f.title || '（講座名）';
    return f;
  }
  function flyerRows(f, labels) {
    return FLYER_KEYS.filter((k) => f[k]).map((k) => ({ label: labels[k], value: f[k] }));
  }

  /* ① A4チラシ */
  function flyerA4HTML(f, T) {
    const rows = flyerRows(f, T.labels);
    let h = '<div class="fl-frame">';
    h += '<div class="fl-head">';
    h += '<p class="fl-band">' + esc(T.band) + '</p>';
    h += '<h4 class="fl-title">' + esc(f.titleShown) + '</h4>';
    h += '<p class="fl-catch">' + esc(T.catch) + '</p>';
    h += '</div>';
    if (f.pr) h += '<p class="fl-pr">' + nl2br(f.pr) + '</p>';
    if (rows.length) {
      h += '<dl class="fl-info">' + rows.map((r) => '<div class="fl-row"><dt>' + esc(r.label) + '</dt><dd>' + nl2br(r.value) + '</dd></div>').join('') + '</dl>';
    }
    h += '<div class="fl-text">';
    h += '<p class="fl-lead">' + esc(T.lead) + '</p>';
    h += '<p class="fl-closing">' + esc(T.closing) + '</p>';
    h += '</div>';
    h += footerHTML();
    h += '</div>';
    return h;
  }
  function flyerA4Text(f, T) {
    const lines = ['【' + T.band + '】', f.titleShown, T.catch];
    if (f.pr) lines.push('', f.pr);
    lines.push('');
    flyerRows(f, T.labels).forEach((r) => lines.push('■ ' + r.label + '：' + r.value));
    lines.push('', T.lead, T.closing);
    const ft = footerText();
    if (ft) lines.push('', '――――――――', ft);
    return lines.join('\n');
  }

  /* ② LINE・SNS用の短い告知文 */
  function flyerSnsText(f, T) {
    const lines = [T.snsOpen.replace('{title}', f.titleShown)];
    if (f.pr) lines.push(f.pr);
    const rows = flyerRows(f, T.labels);
    if (rows.length) {
      lines.push('');
      rows.forEach((r) => lines.push('■ ' + r.label + '：' + r.value));
    }
    lines.push('', T.snsClose);
    const ft = footerText();
    if (ft) lines.push('', ft);
    return lines.join('\n');
  }
  function flyerSnsHTML(f, T) {
    /* 画面と印刷ではロゴつきのフッターを出すので、本文には店名を重ねて入れない */
    const lines = [T.snsOpen.replace('{title}', f.titleShown)];
    if (f.pr) lines.push(f.pr);
    const rows = flyerRows(f, T.labels);
    if (rows.length) {
      lines.push('');
      rows.forEach((r) => lines.push('■ ' + r.label + '：' + r.value));
    }
    lines.push('', T.snsClose);
    const count = Array.from(flyerSnsText(f, T).replace(/\s/g, '')).length;
    return '<article class="doc doc-onepage sns-doc" id="sns-doc">'
      + '<pre class="doc-pre">' + esc(lines.join('\n')) + '</pre>'
      + footerHTML()
      + '</article>'
      + '<p class="doc-count">文字数：' + count + '文字（空白をのぞく。コピーすると店名・連絡先も入ります）</p>';
  }

  /* ③ 回覧板・掲示用の案内文 */
  function noticeDate(T) {
    return T.wareki ? todayWareki() : todayJP();
  }
  function flyerNoticeHTML(f, T) {
    const rows = flyerRows(f, T.noticeLabels);
    const shop = state.settings.shopName.trim();
    let h = '<article class="doc doc-onepage notice-doc" id="notice-doc">';
    h += '<p class="nt-date">' + esc(noticeDate(T)) + '</p>';
    h += '<p class="nt-to">' + esc(T.noticeTo) + '</p>';
    if (shop) h += '<p class="nt-from">' + esc(shop) + '</p>';
    h += '<h4 class="nt-title">' + esc(T.noticeTitle.replace('{title}', f.titleShown)) + '</h4>';
    h += '<p class="nt-body">' + T.noticeBody.map(esc).join('<br>') + '</p>';
    if (T.useKi) h += '<p class="nt-ki">記</p>';
    if (rows.length) {
      h += '<dl class="nt-list">' + rows.map((r, i) => '<div><dt>' + (T.useKi ? FULL_NUM[i] + '．' : '') + esc(r.label) + '</dt><dd>' + nl2br(r.value) + '</dd></div>').join('') + '</dl>';
    }
    if (f.pr) h += '<p class="nt-pr">' + nl2br(f.pr) + '</p>';
    h += '<p class="nt-close">' + esc(T.noticeClose) + '</p>';
    if (T.useKi) h += '<p class="nt-end">以上</p>';
    h += footerHTML();
    h += '</article>';
    return h;
  }
  function flyerNoticeText(f, T) {
    const shop = state.settings.shopName.trim();
    const lines = [noticeDate(T), T.noticeTo];
    if (shop) lines.push(shop);
    lines.push('', T.noticeTitle.replace('{title}', f.titleShown), '');
    T.noticeBody.forEach((x) => lines.push(x));
    lines.push('');
    if (T.useKi) lines.push('記', '');
    flyerRows(f, T.noticeLabels).forEach((r, i) => {
      lines.push((T.useKi ? FULL_NUM[i] + '．' : '■ ') + r.label + '：' + r.value);
    });
    if (f.pr) lines.push('', f.pr);
    lines.push('', T.noticeClose);
    if (T.useKi) lines.push('', '以上');
    const ft = footerText();
    if (ft) lines.push('', '（お問い合わせ）', ft);
    return lines.join('\n');
  }

  /* チラシを画面の幅に合わせて縮小し、文字は用紙におさまる一番大きいサイズにする */
  function fitA4() {
    const wrap = $('#flyer-a4-wrap');
    const page = $('#flyer-a4');
    if (!wrap || !page || !wrap.clientWidth) return;
    let fit = A4_FIT_MAX;
    page.style.setProperty('--fit', String(fit));
    while (page.scrollHeight > page.clientHeight + 1 && fit > A4_FIT_MIN) {
      fit = Math.round((fit - 0.03) * 100) / 100;
      page.style.setProperty('--fit', String(fit));
    }
    const scale = Math.min(1, wrap.clientWidth / A4_W);
    page.style.transform = 'scale(' + scale + ')';
    wrap.style.height = Math.ceil(A4_H * scale) + 'px';
  }
  function renderFlyer() {
    const f = getFlyer();
    const T = DATA.tones[f.tone];
    $('#flyer-a4').innerHTML = flyerA4HTML(f, T);
    $('#flyer-sns-out').innerHTML = flyerSnsHTML(f, T);
    $('#flyer-notice-out').innerHTML = flyerNoticeHTML(f, T);
    fitA4();
    /* ロゴ画像の読み込みが終わってから、もう一度大きさを合わせる */
    $$('img', $('#flyer-a4')).forEach((img) => {
      if (!img.complete) img.addEventListener('load', fitA4, { once: true });
    });
  }

  /* =========================================================
     4. つまずきQ&A
     ========================================================= */
  function norm(s) {
    return String(s).normalize('NFKC').toLowerCase()
      .replace(/[ァ-ヶ]/g, (c) => String.fromCharCode(c.charCodeAt(0) - 0x60));
  }
  function qaCategories() {
    const cats = DATA.qaCategories.slice();
    state.qa.forEach((x) => { if (!cats.includes(x.cat)) cats.push(x.cat); });
    return cats;
  }
  function renderQA() {
    const cats = qaCategories();
    if (qaCat !== 'all' && !cats.includes(qaCat)) qaCat = 'all';
    $('#qa-cats').innerHTML = [{ id: 'all', label: 'すべて' }]
      .concat(cats.map((c) => ({ id: c, label: c })))
      .map((c) => '<button type="button" class="chip" data-cat="' + esc(c.id) + '" aria-pressed="' + (qaCat === c.id) + '">' + esc(c.label) + '</button>')
      .join('');

    const words = norm($('#qa-search').value).split(/\s+/).filter(Boolean);
    const list = state.qa.filter((item) => {
      if (qaCat !== 'all' && item.cat !== qaCat) return false;
      const hay = norm(item.cat + ' ' + item.q + ' ' + item.a);
      return words.every((w) => {
        if (hay.includes(w)) return true;
        const alias = DATA.searchAliases[w];
        return !!alias && hay.includes(alias);
      });
    });

    $('#qa-count').textContent = list.length + '件 見つかりました（全' + state.qa.length + '件）';
    if (!list.length) {
      $('#qa-list').innerHTML = '<p class="empty">あてはまるQ&Aがありません。言葉を短くするか、カテゴリを「すべて」にしてみてください。</p>';
      return;
    }
    $('#qa-list').innerHTML = list.map((item) => {
      return '<details class="qa-item" data-id="' + esc(item.id) + '"' + (qaOpen.has(item.id) ? ' open' : '') + '>'
        + '<summary><span class="qa-cat">' + esc(item.cat) + '</span><span class="qa-q">Q. ' + esc(item.q) + '</span></summary>'
        + '<div class="qa-body"><p class="qa-a">A. ' + esc(item.a) + '</p>'
        + '<div class="actions">'
        + '<button type="button" class="btn" data-act="qa-edit">このQ&Aを直す</button>'
        + '<button type="button" class="btn btn-danger" data-act="qa-del">このQ&Aを削除する</button>'
        + '</div></div></details>';
    }).join('');
  }
  function openQaDialog(id) {
    const form = $('#form-qa');
    const item = id ? state.qa.find((x) => x.id === id) : null;
    editingQaId = item ? item.id : null;
    $('#dlg-qa-title').textContent = item ? 'Q&Aを直す' : 'Q&Aを追加する';
    const cats = qaCategories();
    const selected = item ? item.cat : (qaCat !== 'all' ? qaCat : 'その他');
    form.elements.cat.innerHTML = cats.map((c) => '<option value="' + esc(c) + '"' + (c === selected ? ' selected' : '') + '>' + esc(c) + '</option>').join('');
    form.elements.q.value = item ? item.q : '';
    form.elements.a.value = item ? item.a : '';
    $('#qa-error').hidden = true;
    openDialog($('#dlg-qa'));
    form.elements.q.focus();
  }
  function submitQa(e) {
    e.preventDefault();
    const form = $('#form-qa');
    const q = form.elements.q.value.trim();
    const a = form.elements.a.value.trim();
    const cat = form.elements.cat.value || 'その他';
    const err = $('#qa-error');
    if (!q || !a) {
      err.textContent = !q ? '質問を入れてください。' : '答えを入れてください。';
      err.hidden = false;
      (!q ? form.elements.q : form.elements.a).focus();
      return;
    }
    if (editingQaId) {
      const item = state.qa.find((x) => x.id === editingQaId);
      if (item) { item.cat = cat; item.q = q; item.a = a; }
    } else {
      const item = { id: newId('qa'), cat: cat, q: q, a: a };
      state.qa.unshift(item);
      qaOpen.add(item.id);
    }
    saveKey('qa', state.qa);
    closeDialog($('#dlg-qa'));
    renderQA();
    toast('保存しました');
  }
  function deleteQa(id) {
    const item = state.qa.find((x) => x.id === id);
    if (!item) return;
    confirmDialog({
      title: 'Q&Aを削除します',
      message: '「' + item.q + '」\n\nこのQ&Aを削除します。よろしいですか？',
      okLabel: '削除する',
      danger: true
    }).then((ok) => {
      if (!ok) return;
      state.qa = state.qa.filter((x) => x.id !== id);
      qaOpen.delete(id);
      saveKey('qa', state.qa);
      renderQA();
      toast('削除しました');
    });
  }

  /* =========================================================
     5. 予約・進捗メモ管理
     ========================================================= */
  function webLimit() {
    return Math.max(1, Number(state.prices.rules.webMonthlyLimit) || 1);
  }
  function typeLabel(type) {
    const t = DATA.recordTypes.find((x) => x.id === type);
    return t ? t.label : '';
  }
  function statusLabel(statusId, type) {
    const s = DATA.statuses.find((x) => x.id === statusId) || DATA.statuses[0];
    if (type === 'web' && s.labelWeb) return s.labelWeb;
    if (type === 'course' && s.labelCourse) return s.labelCourse;
    return s.label;
  }
  function statusOptionsHTML(type, selected) {
    return DATA.statuses.map((s) => '<option value="' + s.id + '"' + (s.id === selected ? ' selected' : '') + '>' + esc(statusLabel(s.id, type)) + '</option>').join('');
  }

  /* 「Web制作は月◯件まで」の確認。rec = { id, type, date, status } */
  function capWarning(rec) {
    if (rec.type !== 'web' || !parseISO(rec.date)) return null;
    const limit = webLimit();
    const ym = rec.date.slice(0, 7);
    const others = state.records.filter((r) => r.type === 'web' && r.id !== rec.id && r.date && r.date.slice(0, 7) === ym);
    const firm = others.filter((r) => FIRM_STATUSES.includes(r.status));
    const pending = others.filter((r) => !FIRM_STATUSES.includes(r.status));
    const names = (list) => list.map((r) => r.name + ' 様').join('、');
    if (firm.length >= limit) {
      return {
        level: 'over',
        text: monthJP(ym) + 'は、すでにWeb制作が' + firm.length + '件決まっています（' + names(firm) + '）。このままだと「月' + limit + '件まで」の上限を超えます。別の月にできないか確かめましょう。'
      };
    }
    if (firm.length + pending.length >= limit) {
      return {
        level: 'near',
        text: monthJP(ym) + 'は、問合せ・見積中のWeb制作がほかに' + pending.length + '件あります（' + names(pending) + '）。契約できるのは月' + limit + '件までです。'
      };
    }
    return null;
  }
  function renderCapBanner() {
    const limit = webLimit();
    const nowYM = todayISO().slice(0, 7);
    const map = {};
    state.records.forEach((r) => {
      if (r.type !== 'web' || !r.date) return;
      const ym = r.date.slice(0, 7);
      if (ym < nowYM) return;
      const o = map[ym] || (map[ym] = { firm: 0, pending: 0 });
      if (FIRM_STATUSES.includes(r.status)) o.firm++;
      else o.pending++;
    });
    const cur = map[nowYM] || { firm: 0, pending: 0 };
    let h = '<p class="notice notice-info">今月のWeb制作：決まっている案件 <strong>' + cur.firm + '件</strong>（上限は月' + limit + '件まで）</p>';
    Object.keys(map).sort().forEach((ym) => {
      const o = map[ym];
      if (o.firm > limit) {
        h += '<p class="notice notice-danger" role="alert">【上限をこえています】' + esc(monthJP(ym)) + '：Web制作が' + o.firm + '件あります。「月' + limit + '件まで」を超えているので、日程を見直しましょう。</p>';
      } else if (o.firm + o.pending > limit) {
        h += '<p class="notice notice-warn" role="alert">【上限に注意】' + esc(monthJP(ym)) + '：'
          + (o.firm >= limit
            ? 'すでに' + o.firm + '件が決まっています。問合せ・見積中の' + o.pending + '件は、別の月にできないか相談しましょう。'
            : '問合せ・見積中のWeb制作が' + o.pending + '件あります。契約できるのは月' + limit + '件までです。')
          + '</p>';
      }
    });
    $('#rec-cap').innerHTML = h;
  }
  function renderRecords() {
    renderCapBanner();
    const sel = $('#rec-filter-status');
    sel.innerHTML = '<option value="all">すべて</option>' + DATA.statuses.map((s) => '<option value="' + s.id + '">' + esc(s.label) + '</option>').join('');
    sel.value = recFilter.status;
    $('#rec-filter-type').value = recFilter.type;
    $('#rec-sort').value = recFilter.sort;

    const list = state.records
      .map((r, index) => ({ r: r, index: index }))
      .filter((x) => (recFilter.status === 'all' || x.r.status === recFilter.status) && (recFilter.type === 'all' || x.r.type === recFilter.type))
      .sort((a, b) => {
        if (!a.r.date && !b.r.date) return a.index - b.index;
        if (!a.r.date) return 1;
        if (!b.r.date) return -1;
        if (a.r.date === b.r.date) return a.index - b.index;
        const cmp = a.r.date < b.r.date ? -1 : 1;
        return recFilter.sort === 'asc' ? cmp : -cmp;
      })
      .map((x) => x.r);

    const sum = list.reduce((s, r) => s + r.amount, 0);
    $('#rec-count').textContent = '表示 ' + list.length + '件（全' + state.records.length + '件）／金額の合計 ' + yen(sum);

    if (!state.records.length) {
      $('#rec-list').innerHTML = '<p class="empty">まだ登録がありません。「＋ 新しく登録する」から、案件や講座の予約を入れてください。</p>';
      return;
    }
    if (!list.length) {
      $('#rec-list').innerHTML = '<p class="empty">この条件にあてはまる登録はありません。しぼりこみを「すべて」にしてみてください。</p>';
      return;
    }
    $('#rec-list').innerHTML = list.map((r) => {
      const sid = 'rec-st-' + esc(r.id);
      return '<article class="rec-item" data-id="' + esc(r.id) + '">'
        + '<div class="rec-head"><span class="badge badge-' + r.type + '">' + esc(typeLabel(r.type)) + '</span><h3 class="rec-name">' + esc(r.name) + '</h3></div>'
        + '<dl class="rec-meta">'
        + '<div><dt>日付</dt><dd>' + esc(dateJP(r.date, true)) + '</dd></div>'
        + '<div><dt>金額</dt><dd>' + esc(yen(r.amount)) + '</dd></div>'
        + '</dl>'
        + '<div class="field"><label for="' + sid + '">ステータス（進み具合）</label>'
        + '<select id="' + sid + '" data-act="rec-status">' + statusOptionsHTML(r.type, r.status) + '</select></div>'
        + (r.memo.trim() ? '<p class="rec-memo">' + esc(r.memo.trim()) + '</p>' : '')
        + '<div class="actions">'
        + '<button type="button" class="btn" data-act="rec-edit">内容を直す</button>'
        + '<button type="button" class="btn btn-danger" data-act="rec-del">削除する</button>'
        + '</div></article>';
    }).join('');
  }

  function readRecordForm() {
    const form = $('#form-record');
    return {
      id: editingRecId,
      name: form.elements.name.value.trim(),
      type: getRadio(form, 'type', 'web'),
      date: form.elements.date.value,
      amount: Math.max(0, Math.round(Number(form.elements.amount.value) || 0)),
      status: form.elements.status.value || 'inquiry',
      memo: form.elements.memo.value
    };
  }
  function updateRecordWarn() {
    const warn = capWarning(readRecordForm());
    const el = $('#rec-warn');
    el.hidden = !warn;
    el.textContent = warn ? warn.text : '';
    el.className = 'notice ' + (warn && warn.level === 'over' ? 'notice-danger' : 'notice-warn');
  }
  function openRecordDialog(id, preset) {
    const form = $('#form-record');
    const found = id ? state.records.find((x) => x.id === id) : null;
    editingRecId = found ? found.id : null;
    const r = found || Object.assign({ name: '', type: 'web', date: todayISO(), amount: '', status: 'inquiry', memo: '' }, preset || {});
    $('#dlg-record-title').textContent = found ? '登録の内容を直す' : '新しく登録する';
    form.elements.name.value = r.name;
    setRadio(form, 'type', r.type);
    form.elements.date.value = r.date;
    form.elements.amount.value = (r.amount === '' || r.amount == null) ? '' : r.amount;
    form.elements.status.innerHTML = statusOptionsHTML(r.type, r.status);
    form.elements.memo.value = r.memo;
    $('#rec-error').hidden = true;
    updateRecordWarn();
    openDialog($('#dlg-record'));
    form.elements.name.focus();
  }
  function submitRecord(e) {
    e.preventDefault();
    const form = $('#form-record');
    const rec = readRecordForm();
    const err = $('#rec-error');
    if (!rec.name) {
      err.textContent = 'お名前を入れてください。';
      err.hidden = false;
      form.elements.name.focus();
      return;
    }
    if (!parseISO(rec.date)) {
      err.textContent = '日付をえらんでください。';
      err.hidden = false;
      form.elements.date.focus();
      return;
    }
    err.hidden = true;
    const commit = () => {
      if (editingRecId) {
        const i = state.records.findIndex((x) => x.id === editingRecId);
        if (i >= 0) state.records[i] = rec;
      } else {
        rec.id = newId('rec');
        state.records.push(rec);
      }
      saveKey('records', state.records);
      closeDialog($('#dlg-record'));
      renderRecords();
      toast(currentTab === 'records' ? '保存しました' : '予約・進捗メモに保存しました');
    };
    const warn = capWarning(rec);
    if (warn && warn.level === 'over') {
      confirmDialog({
        title: '月の上限を超えそうです',
        message: warn.text + '\n\nこのまま保存しますか？',
        okLabel: 'このまま保存する',
        cancelLabel: 'もどって直す'
      }).then((ok) => { if (ok) commit(); });
    } else {
      commit();
    }
  }
  function deleteRecord(id) {
    const r = state.records.find((x) => x.id === id);
    if (!r) return;
    confirmDialog({
      title: '登録を削除します',
      message: '「' + r.name + '」（' + typeLabel(r.type) + '・' + dateJP(r.date, true) + '）\n\nこの登録を削除します。よろしいですか？',
      okLabel: '削除する',
      danger: true
    }).then((ok) => {
      if (!ok) return;
      state.records = state.records.filter((x) => x.id !== id);
      saveKey('records', state.records);
      renderRecords();
      toast('削除しました');
    });
  }
  function changeRecordStatus(id, status) {
    const r = state.records.find((x) => x.id === id);
    if (!r || !DATA.statuses.some((s) => s.id === status)) return;
    r.status = status;
    saveKey('records', state.records);
    renderRecords();
    const warn = capWarning(r);
    if (warn && warn.level === 'over' && FIRM_STATUSES.includes(status)) {
      toast('保存しました。' + monthJP(r.date) + 'はWeb制作の上限（月' + webLimit() + '件）を超えています。', 'error');
    } else {
      toast('ステータスを「' + statusLabel(status, r.type) + '」にして保存しました');
    }
  }

  /* =========================================================
     6. 設定
     ========================================================= */
  function renderLogoBox() {
    const img = $('#logo-preview');
    const logo = state.settings.logo;
    if (logo) {
      img.src = logo;
      img.hidden = false;
      $('#logo-empty').hidden = true;
      $('#logo-info').textContent = 'いまのロゴの大きさ：約' + Math.max(1, Math.round(logo.length / 1024)) + 'KB（上限200KB）';
    } else {
      img.removeAttribute('src');
      img.hidden = true;
      $('#logo-empty').hidden = false;
      $('#logo-info').textContent = '大きな画像は、自動で小さくしてから保存します（200KB以下）。';
    }
    $('#btn-logo-del').hidden = !logo;
  }
  function renderFooterPreview() {
    $('#footer-preview').innerHTML = footerHTML() || '<p class="empty">店名・連絡先・ロゴがすべて空なので、印刷物の下には何も入りません。</p>';
  }
  function renderSettings() {
    const s = state.settings;
    const form = $('#form-shop');
    form.elements.shopName.value = s.shopName;
    form.elements.contact.value = s.contact;
    form.elements.address.value = s.address;
    renderLogoBox();
    renderFooterPreview();
    $$('#price-fields input').forEach((input) => {
      input.value = state.prices[input.dataset.group][input.dataset.key];
      input.removeAttribute('aria-invalid');
    });
    $('#price-error').hidden = true;
  }
  function saveShop(showToast) {
    const form = $('#form-shop');
    state.settings.shopName = form.elements.shopName.value.trim();
    state.settings.contact = form.elements.contact.value.trim();
    state.settings.address = form.elements.address.value.trim();
    const ok = saveKey('settings', state.settings);
    applySettings();
    renderFooterPreview();
    if (showToast && (ok || !storageOK)) toast('お店の情報を保存しました');
  }

  /* ---------- ロゴ：自動で縮小して Base64 で保存 ---------- */
  function readFileAsDataURL(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => resolve(String(reader.result));
      reader.onerror = () => reject(reader.error || new Error('read'));
      reader.readAsDataURL(file);
    });
  }
  function loadImage(src) {
    return new Promise((resolve, reject) => {
      const img = new Image();
      img.onload = () => resolve(img);
      img.onerror = () => reject(new Error('image'));
      img.src = src;
    });
  }
  function shrinkLogo(file) {
    return readFileAsDataURL(file).then(loadImage).then((img) => {
      const w0 = img.naturalWidth || img.width;
      const h0 = img.naturalHeight || img.height;
      if (!w0 || !h0) throw new Error('size');
      const canvas = document.createElement('canvas');
      const ctx = canvas.getContext('2d');
      if (!ctx) throw new Error('canvas');
      const sides = [480, 360, 280, 200, 140, 100];
      let last = '';
      for (let i = 0; i < sides.length; i++) {
        const scale = Math.min(1, sides[i] / Math.max(w0, h0));
        const w = Math.max(1, Math.round(w0 * scale));
        const h = Math.max(1, Math.round(h0 * scale));
        canvas.width = w;
        canvas.height = h;
        ctx.imageSmoothingQuality = 'high';
        /* まずは透明な部分を残せるPNGで試す */
        ctx.clearRect(0, 0, w, h);
        ctx.drawImage(img, 0, 0, w, h);
        let url = canvas.toDataURL('image/png');
        if (url.length <= LOGO_LIMIT) return url;
        /* 大きすぎるときは、白い背景のJPEGにして画質を下げる */
        ctx.fillStyle = '#ffffff';
        ctx.fillRect(0, 0, w, h);
        ctx.drawImage(img, 0, 0, w, h);
        const qualities = [0.85, 0.7, 0.55];
        for (let q = 0; q < qualities.length; q++) {
          url = canvas.toDataURL('image/jpeg', qualities[q]);
          if (url.length <= LOGO_LIMIT) return url;
        }
        last = url;
      }
      return last;
    });
  }
  function handleLogoFile(file) {
    if (!file) return;
    shrinkLogo(file).then((url) => {
      if (!isImageDataUrl(url) || url.length > LOGO_LIMIT) throw new Error('logo');
      state.settings.logo = url;
      const ok = saveKey('settings', state.settings);
      renderLogoBox();
      renderFooterPreview();
      if (ok || !storageOK) toast('ロゴを保存しました（約' + Math.max(1, Math.round(url.length / 1024)) + 'KB）');
    }).catch(() => {
      toast('この画像は読み込めませんでした。JPEG か PNG の画像でお試しください。', 'error');
    });
  }
  function deleteLogo() {
    if (!state.settings.logo) return;
    confirmDialog({
      title: 'ロゴを消します',
      message: '登録されているロゴの画像を消します。よろしいですか？',
      okLabel: 'ロゴを消す',
      danger: true
    }).then((ok) => {
      if (!ok) return;
      state.settings.logo = '';
      saveKey('settings', state.settings);
      renderLogoBox();
      renderFooterPreview();
      toast('ロゴを消しました');
    });
  }

  /* ---------- 料金の単価 ---------- */
  function buildPriceFields() {
    $('#price-fields').innerHTML = DATA.priceFields.map((g) => {
      return '<fieldset class="price-group"><legend>' + esc(g.title) + '</legend>'
        + g.fields.map((f) => {
          const id = 'price-' + g.group + '-' + f.key;
          return '<div class="field"><label for="' + id + '">' + esc(f.label) + '</label>'
            + '<div class="unit-input"><input type="number" inputmode="numeric" step="1" min="' + (f.min || 0) + '"'
            + (f.max != null ? ' max="' + f.max + '"' : '')
            + ' id="' + id + '" data-group="' + g.group + '" data-key="' + f.key + '" data-label="' + esc(f.label) + '">'
            + '<span class="unit">' + esc(f.unit) + '</span></div></div>';
        }).join('')
        + '</fieldset>';
    }).join('');
  }
  function readPriceInput(input) {
    const raw = input.value.trim();
    const n = Number(raw);
    const min = Number(input.min) || 0;
    const max = input.max === '' ? Infinity : Number(input.max);
    if (raw === '' || !isFinite(n) || Math.round(n) !== n || n < min || n > max) return null;
    return n;
  }
  function savePrices(showToast) {
    const err = $('#price-error');
    let firstBad = null;
    const next = clone(state.prices);
    $$('#price-fields input').forEach((input) => {
      const n = readPriceInput(input);
      if (n === null) {
        input.setAttribute('aria-invalid', 'true');
        if (!firstBad) firstBad = input;
      } else {
        input.removeAttribute('aria-invalid');
        next[input.dataset.group][input.dataset.key] = n;
      }
    });
    if (firstBad) {
      const min = Number(firstBad.min) || 0;
      err.textContent = '「' + firstBad.dataset.label + '」には、' + min + '以上' + (firstBad.max !== '' ? firstBad.max + '以下' : '') + 'の整数を入れてください。';
      err.hidden = false;
      if (showToast) firstBad.focus();
      return false;
    }
    err.hidden = true;
    state.prices = next;
    const ok = saveKey('prices', state.prices);
    if (showToast && (ok || !storageOK)) toast('単価を保存しました');
    return true;
  }
  function resetPrices() {
    confirmDialog({
      title: '単価を初期値にもどします',
      message: '料金の単価と受付のルールを、最初の金額にもどします。よろしいですか？',
      okLabel: '初期値にもどす',
      danger: true
    }).then((ok) => {
      if (!ok) return;
      state.prices = clone(DATA.defaultPrices);
      saveKey('prices', state.prices);
      renderSettings();
      toast('単価を初期値にもどしました');
    });
  }

  /* ---------- 文字の大きさ ---------- */
  function setFontSize(size) {
    if (!FONT_SIZES.includes(size)) return;
    state.settings.fontSize = size;
    saveKey('settings', state.settings);
    applySettings();
    updateTabbarHeight();
    toast('文字の大きさを変えました');
  }

  /* ---------- バックアップ（JSONの書き出し・読み込み）とリセット ---------- */
  function exportData() {
    flushForms();
    const payload = {
      app: DATA.appId,
      version: DATA.version,
      exportedAt: new Date().toISOString(),
      data: {
        settings: state.settings,
        prices: state.prices,
        qa: state.qa,
        records: state.records,
        forms: state.forms
      }
    };
    try {
      const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'kouza-tool-backup-' + todayISO() + '.json';
      document.body.appendChild(a);
      a.click();
      a.remove();
      setTimeout(() => URL.revokeObjectURL(url), 2000);
      toast('データを書き出しました（ダウンロードの場所に保存されます）');
    } catch (e) {
      toast('書き出しができませんでした。', 'error');
    }
  }
  function refreshAll() {
    applySettings();
    fillForms();
    qaCat = 'all';
    qaOpen.clear();
    $('#qa-search').value = '';
    recFilter.status = 'all';
    recFilter.type = 'all';
    recFilter.sort = 'desc';
    updateTabbarHeight();
    showTab(currentTab, true);
  }
  function importFile(file) {
    if (!file) return;
    const bad = () => toast('読み込めませんでした。このツールで書き出したファイルか、確かめてください。', 'error');
    const reader = new FileReader();
    reader.onerror = bad;
    reader.onload = () => {
      let obj = null;
      try { obj = JSON.parse(String(reader.result)); } catch (e) { obj = null; }
      if (!obj || obj.app !== DATA.appId || !obj.data || typeof obj.data !== 'object') {
        bad();
        return;
      }
      confirmDialog({
        title: 'データを読み込みます',
        message: 'いまのデータは、読み込むファイルの内容に置きかわります。よろしいですか？',
        okLabel: '読み込む'
      }).then((ok) => {
        if (!ok) return;
        const d = obj.data;
        const def = defaultState();
        state = {
          settings: sanitizeSettings(d.settings),
          prices: sanitizePrices(d.prices),
          qa: sanitizeQA(d.qa) || def.qa,
          records: sanitizeRecords(d.records) || [],
          forms: sanitizeForms(d.forms)
        };
        clearTimeout(formsTimer);
        saveAll();
        refreshAll();
        toast('データを読み込みました');
      });
    };
    reader.readAsText(file);
  }
  function resetAll() {
    confirmDialog({
      title: '初期データにもどします',
      message: 'お店の情報・ロゴ・単価・Q&A・予約メモ・入力途中の内容が、すべて最初の状態にもどります。\n\n先に「データを書き出す」でバックアップを取っておくと安心です。本当にもどしますか？',
      okLabel: '初期データにもどす',
      danger: true
    }).then((ok) => {
      if (!ok) return;
      clearTimeout(formsTimer);
      removeAllKeys();
      state = defaultState();
      refreshAll();
      toast('初期データにもどしました');
    });
  }

  /* =========================================================
     画面の準備と、ボタンの動き
     ========================================================= */
  function buildStaticUI() {
    $('#cur-audiences').innerHTML = DATA.audiences.map((a) => {
      return '<label class="check"><input type="radio" name="audience" value="' + esc(a.id) + '"><span>' + esc(a.name) + '</span></label>';
    }).join('');
    $('#cur-themes').innerHTML = DATA.themes.map((t) => {
      return '<label class="check"><input type="checkbox" name="themes" value="' + esc(t.id) + '"><span>' + esc(t.name) + '</span></label>';
    }).join('');
    buildPriceFields();
  }

  function updateTabbarHeight() {
    const nav = $('.tabbar');
    if (nav) document.documentElement.style.setProperty('--tabbar-h', nav.offsetHeight + 'px');
  }

  function on(sel, type, handler) {
    const el = $(sel);
    if (el) el.addEventListener(type, handler);
  }

  function bindEvents() {
    /* タブ */
    $$('.tab-btn').forEach((b) => b.addEventListener('click', () => showTab(b.dataset.tab)));

    /* Enterキーでページが再読み込みされないようにする */
    ['#form-sim-web', '#form-sim-course', '#form-cur', '#form-flyer'].forEach((sel) => {
      on(sel, 'submit', (e) => e.preventDefault());
    });

    /* 「へらす／ふやす」ボタン */
    document.addEventListener('click', (e) => {
      const b = e.target.closest ? e.target.closest('.stepper-btn') : null;
      if (!b) return;
      const input = $('input', b.closest('.stepper'));
      const min = Number(input.min);
      const max = Number(input.max);
      const next = clampInt(input.value, min, max, min) + Number(b.dataset.step);
      input.value = Math.min(max, Math.max(min, next));
      input.dispatchEvent(new Event('input', { bubbles: true }));
    });
    /* 数の入力は、範囲の中におさめる */
    document.addEventListener('change', (e) => {
      const el = e.target;
      if (el && el.matches && el.matches('.stepper input')) {
        el.value = clampInt(el.value, Number(el.min), Number(el.max), Number(el.min));
      }
    }, true);

    /* ---- 1. 料金シミュレーター ---- */
    $$('[data-sim]').forEach((b) => b.addEventListener('click', () => {
      state.forms.simMode = b.dataset.sim;
      saveFormsSoon();
      renderQuote();
    }));
    const onSim = () => { readSimForms(); saveFormsSoon(); renderQuote(); };
    ['#form-sim-web', '#form-sim-course'].forEach((sel) => {
      on(sel, 'input', onSim);
      on(sel, 'change', onSim);
    });
    on('#btn-quote-print', 'click', () => printNode($('#quote-doc')));
    on('#btn-quote-copy', 'click', () => copyText(quoteText(buildQuote()), '見積もりの文章をコピーしました'));
    on('#btn-quote-record', 'click', () => {
      const m = buildQuote();
      openRecordDialog(null, { name: m.client, type: m.kind, date: todayISO(), amount: m.recordAmount, status: 'quote', memo: m.memo });
    });

    /* ---- 2. 講座カリキュラム ---- */
    const onCur = () => { readCurForm(); saveFormsSoon(); renderCurriculum(); };
    on('#form-cur', 'input', onCur);
    on('#form-cur', 'change', onCur);
    const needCurriculum = () => {
      const c = buildCurriculum();
      if (!c) toast('テーマを1つ以上えらんでください。', 'error');
      return c;
    };
    on('#btn-cur-print', 'click', () => { if (needCurriculum()) printNode($('#cur-doc')); });
    on('#btn-cur-copy', 'click', () => {
      const c = needCurriculum();
      if (c) copyText(curriculumText(c), 'カリキュラムの文章をコピーしました');
    });
    on('#btn-cur-flyer', 'click', () => {
      const c = needCurriculum();
      if (!c) return;
      state.forms.flyer.title = c.title;
      $('#form-flyer').elements.title.value = c.title;
      saveFormsSoon();
      showTab('flyer');
      toast('講座名をチラシに入れました');
    });

    /* ---- 3. チラシ・案内文 ---- */
    const onFlyer = () => { readFlyerForm(); saveFormsSoon(); renderFlyer(); };
    on('#form-flyer', 'input', onFlyer);
    on('#form-flyer', 'change', onFlyer);
    on('#btn-flyer-date', 'click', () => {
      const d = $('#fl-date').value;
      if (!parseISO(d)) {
        toast('日付をえらんでください。', 'error');
        $('#fl-date').focus();
        return;
      }
      const s = timeJP($('#fl-start').value);
      const e = timeJP($('#fl-end').value);
      let text = dateJP(d, false);
      if (s) text += ' ' + s + '〜' + e;
      $('#fl-when').value = text;
      onFlyer();
      toast('日時を入れました');
    });
    on('#btn-flyer-sample', 'click', () => {
      const apply = () => {
        const form = $('#form-flyer');
        Object.keys(DATA.flyerSample).forEach((k) => { form.elements[k].value = DATA.flyerSample[k]; });
        onFlyer();
        toast('記入例を入れました');
      };
      const f = getFlyer();
      const hasInput = ['title', 'when', 'where', 'fee', 'cap', 'apply', 'pr'].some((k) => f[k]);
      if (!hasInput) {
        apply();
        return;
      }
      confirmDialog({
        title: '記入例を入れます',
        message: 'いま入っている内容は、記入例に置きかわります。よろしいですか？',
        okLabel: '記入例を入れる'
      }).then((ok) => { if (ok) apply(); });
    });
    on('#btn-flyer-clear', 'click', () => {
      confirmDialog({
        title: '入力を空にします',
        message: 'チラシ・案内文の入力をすべて消します。よろしいですか？',
        okLabel: '空にする',
        danger: true
      }).then((ok) => {
        if (!ok) return;
        const form = $('#form-flyer');
        ['title', 'when', 'where', 'fee', 'cap', 'apply', 'pr'].forEach((k) => { form.elements[k].value = ''; });
        onFlyer();
        toast('入力を空にしました');
      });
    });
    const tone = () => DATA.tones[getFlyer().tone];
    on('#btn-a4-print', 'click', () => printNode($('#flyer-a4')));
    on('#btn-a4-copy', 'click', () => copyText(flyerA4Text(getFlyer(), tone()), 'チラシの文章をコピーしました'));
    on('#btn-sns-copy', 'click', () => copyText(flyerSnsText(getFlyer(), tone()), '告知文をコピーしました'));
    on('#btn-sns-print', 'click', () => printNode($('#sns-doc')));
    on('#btn-notice-print', 'click', () => printNode($('#notice-doc')));
    on('#btn-notice-copy', 'click', () => copyText(flyerNoticeText(getFlyer(), tone()), '案内文をコピーしました'));

    /* ---- 4. つまずきQ&A ---- */
    on('#qa-search', 'input', renderQA);
    on('#qa-cats', 'click', (e) => {
      const b = e.target.closest('.chip');
      if (!b) return;
      qaCat = b.dataset.cat;
      renderQA();
    });
    on('#btn-qa-add', 'click', () => openQaDialog(null));
    on('#qa-list', 'click', (e) => {
      const b = e.target.closest('[data-act]');
      if (!b) return;
      const id = b.closest('.qa-item').dataset.id;
      if (b.dataset.act === 'qa-edit') openQaDialog(id);
      if (b.dataset.act === 'qa-del') deleteQa(id);
    });
    $('#qa-list').addEventListener('toggle', (e) => {
      const d = e.target;
      if (!d.classList || !d.classList.contains('qa-item')) return;
      if (d.open) qaOpen.add(d.dataset.id);
      else qaOpen.delete(d.dataset.id);
    }, true);
    on('#form-qa', 'submit', submitQa);

    /* ---- 5. 予約・進捗メモ ---- */
    on('#btn-rec-add', 'click', () => openRecordDialog(null));
    on('#rec-filter-status', 'change', (e) => { recFilter.status = e.target.value; renderRecords(); });
    on('#rec-filter-type', 'change', (e) => { recFilter.type = e.target.value; renderRecords(); });
    on('#rec-sort', 'change', (e) => { recFilter.sort = e.target.value; renderRecords(); });
    on('#rec-list', 'click', (e) => {
      const b = e.target.closest('button[data-act]');
      if (!b) return;
      const id = b.closest('.rec-item').dataset.id;
      if (b.dataset.act === 'rec-edit') openRecordDialog(id);
      if (b.dataset.act === 'rec-del') deleteRecord(id);
    });
    on('#rec-list', 'change', (e) => {
      const sel = e.target;
      if (sel.dataset && sel.dataset.act === 'rec-status') {
        changeRecordStatus(sel.closest('.rec-item').dataset.id, sel.value);
      }
    });
    on('#form-record', 'submit', submitRecord);
    on('#form-record', 'change', (e) => {
      const form = $('#form-record');
      if (e.target.name === 'type') {
        form.elements.status.innerHTML = statusOptionsHTML(getRadio(form, 'type', 'web'), form.elements.status.value);
      }
      updateRecordWarn();
    });

    /* 入力の窓の「やめる」 */
    $$('[data-close]').forEach((b) => b.addEventListener('click', () => closeDialog(b.closest('dialog'))));

    /* ---- 6. 設定 ---- */
    on('#form-shop', 'submit', (e) => { e.preventDefault(); saveShop(true); });
    on('#form-shop', 'change', (e) => { if (e.target.type !== 'file') saveShop(true); });
    on('#btn-logo-pick', 'click', () => $('#set-logo').click());
    on('#set-logo', 'change', (e) => {
      handleLogoFile(e.target.files && e.target.files[0]);
      e.target.value = '';
    });
    on('#btn-logo-del', 'click', deleteLogo);
    $$('[data-fontsize]').forEach((b) => b.addEventListener('click', () => setFontSize(b.dataset.fontsize)));
    on('#form-prices', 'submit', (e) => { e.preventDefault(); savePrices(true); });
    on('#form-prices', 'change', () => { if (savePrices(false)) toast('単価を保存しました'); });
    on('#btn-price-default', 'click', resetPrices);
    on('#btn-export', 'click', exportData);
    on('#btn-import', 'click', () => $('#import-file').click());
    on('#import-file', 'change', (e) => {
      importFile(e.target.files && e.target.files[0]);
      e.target.value = '';
    });
    on('#btn-reset', 'click', resetAll);

    /* 画面の大きさが変わったとき */
    window.addEventListener('resize', () => {
      updateTabbarHeight();
      if (currentTab === 'flyer') fitA4();
    });
    if (window.ResizeObserver) {
      new ResizeObserver(updateTabbarHeight).observe($('.tabbar'));
    }
    /* 閉じる前に、入力途中の内容を保存 */
    window.addEventListener('pagehide', flushForms);
    document.addEventListener('visibilitychange', () => {
      if (document.visibilityState === 'hidden') flushForms();
    });
  }

  function init() {
    storageOK = testStorage();
    loadState();
    buildStaticUI();
    bindEvents();
    applySettings();
    fillForms();
    updateTabbarHeight();
    showTab(state.forms.lastTab, true);
    if (!storageOK) $('#storage-warning').hidden = false;
  }

  init();
})();
