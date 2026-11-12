/* =========================================================
   レポート構成ナビ  cite.js
   ---------------------------------------------------------
   文献データを「引用形式の雛形」に整えます。
   取得できなかった項目は 〔巻(号)〕 のように 〔 〕 で残すので、
   原典を見て埋めてください。
   ========================================================= */
(function (root) {
  'use strict';

  const STYLES = [
    { id: 'ja', label: '日本語・一般', desc: '著者（年）「題」『誌名』巻(号)，頁．人文・社会系で広く使われる形', numbered: false },
    { id: 'apa', label: 'APA 第7版', desc: '著者 (年). 題. 誌名, 巻(号), 頁. 心理・教育・社会科学系', numbered: false },
    { id: 'sist02', label: 'SIST02', desc: '著者. 題. 誌名. 年, vol., no., p. 理工系の番号方式', numbered: true },
    { id: 'ieee', label: 'IEEE', desc: '[1] 著者, "題," 誌名, vol., no., pp., 年. 工学・情報系の番号方式', numbered: true }
  ];

  const TYPE_LABEL = { article: '論文', book: '図書', thesis: '学位論文', other: 'その他・Web' };

  const JA_RE = /[぀-ヿ㐀-䶿一-鿿々〆ヶ]/;
  function hasJa(s) { return JA_RE.test(String(s || '')); }

  const PARTICLES = ['van', 'von', 'de', 'der', 'den', 'del', 'della', 'di', 'da', 'dos', 'das', 'du', 'la', 'le', 'bin', 'ibn', 'al', 'el', 'ter', 'ten'];
  const ORG_RE = /\b(university|institute|organization|organisation|association|committee|ministry|council|society|agency|bank|department|consortium|collaboration|corporation|foundation|centre|center|office|network|group|team|commission|bureau|inc|ltd|unesco|oecd)\b/i;

  function ph(label) { return '〔' + label + '〕'; }
  function s(x) { return String(x == null ? '' : x).replace(/\s+/g, ' ').trim(); }

  function titleCase(w) {
    return w.split('-').map(function (p) { return p ? p.charAt(0).toUpperCase() + p.slice(1).toLowerCase() : p; }).join('-');
  }

  /* 著者名を「姓・名」に分ける。確実にしたいときは「姓, 名」と書く */
  function parseName(raw) {
    let t = String(raw == null ? '' : raw).replace(/　/g, ' ').replace(/\s+/g, ' ').trim();
    t = t.replace(/\s+\d{3,}$/, '');
    if (!t) return null;
    const ja = hasJa(t);
    if (/[,，]/.test(t)) {
      const parts = t.split(/[,，]/);
      const family = parts[0].trim();
      let given = parts.slice(1).join(' ').trim();
      if (/^[.\-‐・\s]*$/.test(given)) given = '';
      if (!family) return given ? { family: given, given: '', ja: ja, literal: false } : null;
      return { family: family, given: ja ? given.replace(/\s+/g, '') : given, ja: ja, literal: false };
    }
    if (ja) {
      const p = t.split(' ');
      return { family: p[0], given: p.slice(1).join(''), ja: true, literal: false };
    }
    if (ORG_RE.test(t)) return { family: t, given: '', ja: false, literal: true };
    const w = t.split(' ');
    if (w.length === 1) return { family: t, given: '', ja: false, literal: false };
    const isCaps = function (x) { return x.length >= 3 && /^[A-ZÀ-Þ'’\-]+$/.test(x) && !/^[IVX]+$/.test(x); };
    if (isCaps(w[0]) && !w.slice(1).every(isCaps)) {
      return { family: titleCase(w[0]), given: w.slice(1).join(' '), ja: false, literal: false };
    }
    if (isCaps(w[w.length - 1]) && !w.slice(0, -1).every(isCaps)) {
      return { family: titleCase(w[w.length - 1]), given: w.slice(0, -1).join(' '), ja: false, literal: false };
    }
    const tail = w[w.length - 1];
    if (/^[A-Z]{1,2}$/.test(tail) && /^[A-Z][a-z]/.test(w[0])) {
      return { family: w.slice(0, -1).join(' '), given: tail.split('').join(' '), ja: false, literal: false };
    }
    let i = w.length - 1;
    while (i - 1 > 0 && PARTICLES.indexOf(w[i - 1]) !== -1) i--;
    return { family: w.slice(i).join(' '), given: w.slice(0, i).join(' '), ja: false, literal: false };
  }

  function initials(given) {
    return String(given || '').split(/\s+/).filter(Boolean).map(function (word) {
      return word.split('-').map(function (p) {
        const c = p.replace(/[^A-Za-zÀ-ÿ]/g, '').charAt(0);
        return c ? c.toUpperCase() + '.' : '';
      }).filter(Boolean).join('-');
    }).filter(Boolean).join(' ');
  }

  function people(ref) {
    return (ref.authors || []).map(parseName).filter(Boolean);
  }

  /* 表記のバリエーション */
  function jaFull(p, sep) { return p.family + (p.given ? (sep || '') + p.given : ''); }
  function familyInitials(p) {
    if (p.literal || !p.given) return p.family;
    return p.family + ', ' + initials(p.given);
  }
  function familyGiven(p) {
    if (p.literal || !p.given) return p.family;
    return p.family + ', ' + p.given;
  }
  function initialsFamily(p) {
    if (p.literal || !p.given) return p.family;
    return initials(p.given) + ' ' + p.family;
  }

  function isJaRef(ref) {
    return hasJa(ref.title) || hasJa((ref.authors || []).join(''));
  }

  function cleanTitle(ref) {
    let t = s(ref.title);
    if (!t) return ph('タイトル');
    if (hasJa(t)) return t.replace(/\s+[:：]\s+/g, '：');
    return t.replace(/\s+:\s+/g, ': ').replace(/[.。]$/, '');
  }

  function pagesOf(ref, dash) {
    let p = s(ref.pages);
    if (!p) return '';
    p = p.replace(/^(pp?\.?|頁)\s*/i, '');
    const m = p.match(/^(\S+?)\s*[-‐‑‒–—―~〜～]\s*(\S+)$/);
    if (m) {
      if (m[1] === m[2]) return m[1];
      return m[1] + dash + m[2];
    }
    return p;
  }
  function isRange(p) { return /[-–]/.test(p); }

  function volIssue(ref) {
    const vol = s(ref.volume);
    const iss = s(ref.issue);
    if (vol && iss) return vol + '(' + iss + ')';
    if (vol) return vol;
    if (iss) return '(' + iss + ')';
    return '';
  }

  function doiUrl(ref) {
    const d = s(ref.doi).replace(/^https?:\/\/(dx\.)?doi\.org\//i, '').replace(/^doi:\s*/i, '');
    return d ? 'https://doi.org/' + d : '';
  }
  function doiBare(ref) {
    return doiUrl(ref).replace('https://doi.org/', '');
  }

  function yearOf(ref) { return s(ref.year) || ph('年'); }

  /* 欧文の題名を引用符でくくる（? や ! で終わる題名にはカンマを足さない） */
  function quoted(title) {
    return /[?？!！]$/.test(title) ? '"' + title + '"' : '"' + title + ',"';
  }

  function endDot(text, dot) {
    const t = text.replace(/\s+$/, '');
    if (/[.．。?？!！]$/.test(t)) return t;
    return t + dot;
  }

  function accessed(ref) {
    return s(ref.accessed) || new Date().toISOString().slice(0, 10);
  }

  function degreeJa(ref) {
    const d = s(ref.degree);
    if (/修士/.test(d)) return '修士論文';
    return '博士論文';
  }
  function degreeEn(ref) {
    return /修士|master/i.test(s(ref.degree)) ? "Master's thesis" : 'Doctoral dissertation';
  }

  /* ---------- 日本語・一般 ---------- */

  function fmtJa(ref) {
    const ps = people(ref);
    const ja = isJaRef(ref);
    const title = cleanTitle(ref);
    const y = yearOf(ref);
    const type = ref.type || 'article';
    const doi = doiUrl(ref);
    let text;
    if (ja) {
      let a;
      if (!ps.length) a = ph('著者');
      else if (ps.length <= 3) a = ps.map(function (p) { return p.ja ? jaFull(p) : familyInitials(p); }).join('・');
      else a = (ps[0].ja ? jaFull(ps[0]) : familyInitials(ps[0])) + 'ほか';
      const head = a + '（' + y + '）';
      if (type === 'book') {
        text = head + '『' + title + '』' + (s(ref.publisher) || ph('出版社')) + '．';
      } else if (type === 'thesis') {
        text = head + '「' + title + '」' + (s(ref.publisher) || ph('大学名')) + degreeJa(ref) + '．';
      } else if (type === 'other') {
        const where = s(ref.container) || s(ref.publisher);
        text = head + '「' + title + '」' + (where ? where + '．' : '');
        const url = s(ref.url);
        if (!doi && url) text += url + '（' + accessed(ref) + ' 閲覧）';
      } else {
        const p = pagesOf(ref, '–');
        text = head + '「' + title + '」『' + (s(ref.container) || ph('雑誌名')) + '』' + (volIssue(ref) || ph('巻(号)')) + '，' +
          (p ? (isRange(p) ? 'pp. ' : 'p. ') + p : ph('ページ')) + '．';
      }
    } else {
      let a;
      const names = ps.map(familyInitials);
      if (!names.length) a = ph('著者');
      else if (names.length === 1) a = names[0];
      else if (names.length === 2) a = names[0] + ' and ' + names[1];
      else if (names.length === 3) a = names[0] + ', ' + names[1] + ' and ' + names[2];
      else a = names[0] + ' et al.';
      const head = a + ' (' + y + ')';
      if (type === 'book') {
        text = head + ' ' + title + ', ' + endDot(s(ref.publisher) || ph('出版社'), '.');
      } else if (type === 'thesis') {
        text = head + ' ' + quoted(title) + ' ' + degreeEn(ref) + ', ' + endDot(s(ref.publisher) || ph('大学名'), '.');
      } else if (type === 'other') {
        const where = s(ref.container) || s(ref.publisher);
        text = head + ' ' + (where ? quoted(title) + ' ' + endDot(where, '.') : '"' + endDot(title, '.') + '"');
        const url = s(ref.url);
        if (!doi && url) text += ' ' + url + ' (accessed ' + accessed(ref) + ').';
      } else {
        const p = pagesOf(ref, '–');
        text = head + ' ' + quoted(title) + ' ' + (s(ref.container) || ph('雑誌名')) + ', ' + (volIssue(ref) || ph('巻(号)')) + ', ' +
          (p ? (isRange(p) ? 'pp. ' : 'p. ') + p : ph('ページ')) + '.';
      }
    }
    if (doi) text += ' ' + doi;
    return text;
  }

  function intextJa(ref) {
    const ps = people(ref);
    const y = yearOf(ref);
    if (isJaRef(ref)) {
      let a;
      if (!ps.length) a = ph('著者');
      else if (ps.length === 1) a = ps[0].family;
      else if (ps.length === 2) a = ps[0].family + '・' + ps[1].family;
      else a = ps[0].family + 'ほか';
      return '（' + a + '，' + y + '，p. ' + ph('頁') + '）';
    }
    let b;
    if (!ps.length) b = ph('著者');
    else if (ps.length === 1) b = ps[0].family;
    else if (ps.length === 2) b = ps[0].family + ' and ' + ps[1].family;
    else b = ps[0].family + ' et al.';
    return '(' + b + ', ' + y + ', p. ' + ph('頁') + ')';
  }

  /* ---------- APA 第7版 ---------- */

  function fmtApa(ref) {
    const ps = people(ref);
    const ja = isJaRef(ref);
    const title = cleanTitle(ref);
    const y = yearOf(ref);
    const type = ref.type || 'article';
    const doi = doiUrl(ref);
    const url = s(ref.url);
    let text;
    if (ja) {
      const a = ps.length ? ps.map(function (p) { return p.ja ? jaFull(p, ' ') : familyInitials(p); }).join('・') : ph('著者');
      const head = a + ' (' + y + '). ';
      if (type === 'book') {
        text = head + title + ' ' + (s(ref.publisher) || ph('出版社'));
      } else if (type === 'thesis') {
        text = head + title + ' ' + (s(ref.publisher) || ph('大学名')) + degreeJa(ref);
      } else if (type === 'other') {
        const where = s(ref.container) || s(ref.publisher);
        text = head + title + (where ? ' ' + where : '');
        if (!doi && url) text += ' ' + url;
      } else {
        const p = pagesOf(ref, '–');
        text = head + title + ' ' + (s(ref.container) || ph('雑誌名')) + ', ' + (volIssue(ref) || ph('巻(号)')) + ', ' + (p || ph('ページ')) + '.';
      }
    } else {
      const names = ps.map(familyInitials);
      let a;
      if (!names.length) a = ph('著者');
      else if (names.length === 1) a = names[0];
      else if (names.length <= 20) a = names.slice(0, -1).join(', ') + ', & ' + names[names.length - 1];
      else a = names.slice(0, 19).join(', ') + ', . . . ' + names[names.length - 1];
      const head = endDot(a + ' (' + y + ')', '.') + ' ';
      if (type === 'book') {
        text = head + endDot(title, '.') + ' ' + endDot(s(ref.publisher) || ph('出版社'), '.');
      } else if (type === 'thesis') {
        text = head + title + ' [' + degreeEn(ref) + ', ' + (s(ref.publisher) || ph('大学名')) + '].';
      } else if (type === 'other') {
        const where = s(ref.container) || s(ref.publisher);
        text = head + endDot(title, '.') + (where ? ' ' + endDot(where, '.') : '');
        if (!doi && url) text += ' ' + url;
      } else {
        const p = pagesOf(ref, '–');
        text = head + endDot(title, '.') + ' ' + (s(ref.container) || ph('雑誌名')) + ', ' + (volIssue(ref) || ph('巻(号)')) + ', ' + (p || ph('ページ')) + '.';
      }
    }
    if (doi) text += ' ' + doi;
    return text;
  }

  function intextApa(ref) {
    const ps = people(ref);
    const y = yearOf(ref);
    if (isJaRef(ref)) {
      let a;
      if (!ps.length) a = ph('著者');
      else if (ps.length === 1) a = ps[0].family;
      else if (ps.length === 2) a = ps[0].family + '・' + ps[1].family;
      else a = ps[0].family + '他';
      return '（' + a + ', ' + y + '）';
    }
    let b;
    if (!ps.length) b = ph('著者');
    else if (ps.length === 1) b = ps[0].family;
    else if (ps.length === 2) b = ps[0].family + ' & ' + ps[1].family;
    else b = ps[0].family + ' et al.';
    return '(' + b + ', ' + y + ')';
  }

  /* ---------- SIST02 ---------- */

  function fmtSist(ref) {
    const ps = people(ref);
    const ja = isJaRef(ref);
    const title = cleanTitle(ref);
    const y = yearOf(ref);
    const type = ref.type || 'article';
    const doi = doiUrl(ref);
    const url = s(ref.url);
    const names = ps.map(function (p) { return p.ja ? jaFull(p) : familyGiven(p); });
    let a;
    if (!names.length) a = ph('著者');
    else if (names.length <= 3) a = names.join('; ');
    else a = names[0] + (ja ? ', ほか' : ', et al');
    const head = endDot(a, '.') + ' ' + endDot(title, '.') + ' ';
    let text;
    if (type === 'book') {
      text = head + (s(ref.publisher) || ph('出版社')) + ', ' + y + '.';
    } else if (type === 'thesis') {
      text = head + (s(ref.publisher) || ph('大学名')) + ', ' + y + ', ' + (ja ? degreeJa(ref) : degreeEn(ref)) + '.';
    } else if (type === 'other') {
      const where = s(ref.container) || s(ref.publisher);
      text = head + (where ? endDot(where, '.') + ' ' : '') + y + '.';
      if (!doi && url) text += ' ' + url + ', (参照 ' + accessed(ref) + ').';
    } else {
      const p = pagesOf(ref, '-');
      const vol = s(ref.volume);
      const iss = s(ref.issue);
      const bits = [y];
      if (vol) bits.push('vol. ' + vol);
      if (iss) bits.push('no. ' + iss);
      if (!vol && !iss) bits.push(ph('巻・号'));
      bits.push(p ? 'p. ' + p : ph('ページ'));
      text = head + endDot(s(ref.container) || ph('雑誌名'), '.') + ' ' + bits.join(', ') + '.';
    }
    if (doi) text += ' ' + doi;
    return text;
  }

  /* ---------- IEEE ---------- */

  function fmtIeee(ref) {
    const ps = people(ref);
    const ja = isJaRef(ref);
    const title = cleanTitle(ref);
    const y = yearOf(ref);
    const type = ref.type || 'article';
    const doi = doiBare(ref);
    const url = s(ref.url);
    let a;
    if (!ps.length) {
      a = ph('著者');
    } else if (ja) {
      const jn = ps.map(function (p) { return p.ja ? jaFull(p) : initialsFamily(p); });
      a = jn.length > 6 ? jn[0] + 'ほか' : jn.join('，');
    } else {
      const en = ps.map(initialsFamily);
      if (en.length === 1) a = en[0];
      else if (en.length === 2) a = en[0] + ' and ' + en[1];
      else if (en.length <= 6) a = en.slice(0, -1).join(', ') + ', and ' + en[en.length - 1];
      else a = en[0] + ' et al.';
    }
    let text;
    if (type === 'book') {
      text = a + ', ' + endDot(title, '.') + ' ' + (s(ref.publisher) || ph('出版社')) + ', ' + y + '.';
    } else if (type === 'thesis') {
      text = a + ', ' + quoted(title) + ' ' + (ja ? degreeJa(ref) : (/Master/.test(degreeEn(ref)) ? 'M.S. thesis' : 'Ph.D. dissertation')) + ', ' + (s(ref.publisher) || ph('大学名')) + ', ' + y + '.';
    } else if (type === 'other') {
      const where = s(ref.container) || s(ref.publisher);
      text = a + ', ' + quoted(title) + ' ' + (where ? where + ', ' : '') + y + '.';
      if (!doi && url) text += ' [Online]. Available: ' + url;
    } else {
      const p = pagesOf(ref, '–');
      const vol = s(ref.volume);
      const iss = s(ref.issue);
      const bits = [s(ref.container) || ph('雑誌名')];
      if (vol) bits.push('vol. ' + vol);
      if (iss) bits.push('no. ' + iss);
      if (!vol && !iss) bits.push(ph('巻・号'));
      bits.push(p ? (isRange(p) ? 'pp. ' : 'p. ') + p : ph('ページ'));
      bits.push(y);
      text = a + ', ' + quoted(title) + ' ' + bits.join(', ') + (doi ? ', doi: ' + doi : '') + '.';
      return text;
    }
    if (doi) text += ' doi: ' + doi + '.';
    return text;
  }

  /* ---------- 公開関数 ---------- */

  /* 文献1件を整える。n は番号方式（SIST02・IEEE）での通し番号 */
  function format(ref, style, n) {
    const num = n || 1;
    if (style === 'apa') return { text: fmtApa(ref), intext: intextApa(ref) };
    if (style === 'sist02') return { text: num + ') ' + fmtSist(ref), intext: num + ')' };
    if (style === 'ieee') return { text: '[' + num + '] ' + fmtIeee(ref), intext: '[' + num + ']' };
    return { text: fmtJa(ref), intext: intextJa(ref) };
  }

  /* 原典で確かめたい項目 */
  function missing(ref) {
    const out = [];
    const type = ref.type || 'article';
    if (!(ref.authors || []).some(function (a) { return s(a); })) out.push('著者');
    if (!s(ref.year)) out.push('出版年');
    if (!s(ref.title)) out.push('タイトル');
    if (type === 'article') {
      if (!s(ref.container)) out.push('雑誌名');
      if (!s(ref.volume) && !s(ref.issue)) out.push('巻・号');
      else if (!s(ref.issue) && ref.source === 's2') out.push('号（この検索元からは取得できません）');
      if (!s(ref.pages)) out.push('ページ');
    } else if (type === 'book') {
      if (!s(ref.publisher)) out.push('出版社');
    } else if (type === 'thesis') {
      if (!s(ref.publisher)) out.push('大学名');
    }
    return out;
  }

  function sortKey(ref) {
    const ps = people(ref);
    const first = ps[0] ? ps[0].family : s(ref.title);
    return (hasJa(first) ? '1' : '0') + first.toLowerCase();
  }

  /* 並べ替え：order が 'author' なら著者名順（欧文→和文）、それ以外は追加順 */
  function sortRefs(refs, order) {
    const list = refs.slice();
    if (order !== 'author') return list;
    return list.sort(function (a, b) {
      const ka = sortKey(a);
      const kb = sortKey(b);
      if (ka[0] !== kb[0]) return ka < kb ? -1 : 1;
      const c = ka.localeCompare(kb, 'ja');
      if (c !== 0) return c;
      return s(a.year).localeCompare(s(b.year));
    });
  }

  function styleDef(id) {
    for (let i = 0; i < STYLES.length; i++) if (STYLES[i].id === id) return STYLES[i];
    return STYLES[0];
  }

  /* 並び順の既定：番号方式は追加順、著者年方式は著者名順 */
  function resolveOrder(style, order) {
    if (order === 'added' || order === 'author') return order;
    return styleDef(style).numbered ? 'added' : 'author';
  }

  /* 「松岡 2019」のような短い表示名 */
  function shortLabel(ref) {
    const ps = people(ref);
    let a;
    if (!ps.length) a = s(ref.title).slice(0, 12) || '（著者なし）';
    else if (ps.length === 1) a = ps[0].family;
    else if (ps.length === 2) a = ps[0].family + '・' + ps[1].family;
    else a = ps[0].family + (ps[0].ja ? 'ほか' : ' et al.');
    return a + ' ' + (s(ref.year) || '年不明');
  }

  const api = {
    STYLES: STYLES,
    TYPE_LABEL: TYPE_LABEL,
    parseName: parseName,
    initials: initials,
    format: format,
    missing: missing,
    sortRefs: sortRefs,
    styleDef: styleDef,
    resolveOrder: resolveOrder,
    shortLabel: shortLabel,
    hasJa: hasJa
  };

  root.RN_CITE = api;
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
