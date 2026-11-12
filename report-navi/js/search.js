/* =========================================================
   レポート構成ナビ  search.js
   ---------------------------------------------------------
   文献検索（どちらもAPIキー・登録不要、ブラウザから直接呼び出し）
   ・Semantic Scholar Academic Graph API … 理系・全分野
   ・CiNii Research OpenSearch API       … 文系・日本語
   送信するのは検索語と絞り込み条件だけです。
   ========================================================= */
(function (root) {
  'use strict';

  const S2_URL = 'https://api.semanticscholar.org/graph/v1/paper/search';
  const S2_FIELDS = 'title,authors,year,venue,journal,externalIds,publicationTypes,citationCount,openAccessPdf,abstract,url';
  const CINII_URL = 'https://cir.nii.ac.jp/opensearch/';
  const CINII_KINDS = ['all', 'articles', 'books', 'dissertations'];
  const PAGE = 20;

  const S2_TYPE_LABEL = {
    Review: 'レビュー論文', MetaAnalysis: 'メタ分析', Book: '図書', BookSection: '図書の章', Conference: '会議論文',
    CaseReport: '症例報告', ClinicalTrial: '臨床試験', Dataset: 'データセット',
    Editorial: '論説', LettersAndComments: 'レター', News: 'ニュース'
  };
  const S2_TYPE_ORDER = ['Review', 'MetaAnalysis', 'Book', 'BookSection', 'Conference', 'CaseReport', 'ClinicalTrial', 'Dataset', 'Editorial', 'LettersAndComments', 'News'];
  const CINII_TYPE_LABEL = { Article: '論文', Book: '図書', Dissertation: '博士論文', Dataset: '研究データ', Project: '研究プロジェクト' };

  function SearchError(kind, status) {
    this.name = 'SearchError';
    this.kind = kind; /* busy | offline | http | parse */
    this.status = status || 0;
    this.message = 'search failed: ' + kind + (status ? ' ' + status : '');
  }
  SearchError.prototype = Object.create(Error.prototype);

  function abortError() {
    try { return new DOMException('Aborted', 'AbortError'); } catch (e) {
      const x = new Error('Aborted'); x.name = 'AbortError'; return x;
    }
  }

  function sleep(ms, signal) {
    return new Promise(function (resolve, reject) {
      if (signal && signal.aborted) { reject(abortError()); return; }
      const t = setTimeout(resolve, ms);
      if (signal) signal.addEventListener('abort', function () { clearTimeout(t); reject(abortError()); }, { once: true });
    });
  }

  function isOffline() {
    return typeof navigator !== 'undefined' && navigator.onLine === false;
  }

  function str(x) { return String(x == null ? '' : x).replace(/\s+/g, ' ').trim(); }
  function year4(x) { return /^\d{4}$/.test(str(x)); }

  function plainText(x) {
    return str(String(x == null ? '' : x).replace(/<[^>]*>/g, ' ')
      .replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&quot;/g, '"').replace(/&#0?39;|&apos;/g, "'").replace(/&amp;/g, '&'));
  }

  function yearRange(from, to) {
    const f = year4(from) ? str(from) : '';
    const t = year4(to) ? str(to) : '';
    if (f && t) return (f <= t ? f + '-' + t : t + '-' + f);
    if (f) return f + '-';
    if (t) return '-' + t;
    return '';
  }

  /* ---------- Semantic Scholar ---------- */

  function normalizeS2(p) {
    p = p || {};
    const j = p.journal || {};
    const ids = p.externalIds || {};
    const types = Array.isArray(p.publicationTypes) ? p.publicationTypes : [];
    let kind = '論文';
    for (let i = 0; i < S2_TYPE_ORDER.length; i++) {
      if (types.indexOf(S2_TYPE_ORDER[i]) !== -1) { kind = S2_TYPE_LABEL[S2_TYPE_ORDER[i]]; break; }
    }
    const url = str(p.url) || (p.paperId ? 'https://www.semanticscholar.org/paper/' + p.paperId : '');
    const arxiv = ids.ArXiv ? 'https://arxiv.org/abs/' + ids.ArXiv : '';
    return {
      source: 's2',
      sourceId: str(p.paperId),
      type: types.indexOf('Book') !== -1 ? 'book' : 'article',
      kind: kind,
      authors: (Array.isArray(p.authors) ? p.authors : []).map(function (a) { return str(a && a.name); }).filter(Boolean),
      year: p.year ? String(p.year) : '',
      title: str(p.title) || '（タイトルなし）',
      container: str(p.venue) || str(j.name),
      volume: str(j.volume),
      issue: '',
      pages: str(j.pages),
      publisher: '',
      doi: str(ids.DOI),
      url: url,
      pdf: str(p.openAccessPdf && p.openAccessPdf.url) || arxiv,
      cited: typeof p.citationCount === 'number' ? p.citationCount : null,
      abstract: plainText(p.abstract)
    };
  }

  const cache = new Map();
  let s2Last = 0;

  /* o: { q, offset, yearFrom, yearTo, oa }  ctx: { signal, onRetry } */
  async function searchS2(o, ctx) {
    ctx = ctx || {};
    const qs = new URLSearchParams();
    qs.set('query', str(o.q).replace(/-/g, ' '));
    qs.set('offset', String(o.offset || 0));
    qs.set('limit', String(PAGE));
    qs.set('fields', S2_FIELDS);
    const yr = yearRange(o.yearFrom, o.yearTo);
    if (yr) qs.set('year', yr);
    let url = S2_URL + '?' + qs.toString();
    if (o.oa) url += '&openAccessPdf';
    if (cache.has(url)) return cache.get(url);

    /* キーなしの共有枠は混みやすい（429）。間隔を空けて自動で数回やり直す */
    const waits = [0, 2000, 4000, 8000];
    let last = null;
    for (let i = 0; i < waits.length; i++) {
      if (i > 0) {
        if (ctx.onRetry) ctx.onRetry({ attempt: i + 1, max: waits.length });
        await sleep(waits[i], ctx.signal);
      }
      const gap = 1100 - (Date.now() - s2Last);
      if (gap > 0) await sleep(gap, ctx.signal);
      s2Last = Date.now();
      let res;
      try {
        res = await fetch(url, { signal: ctx.signal });
      } catch (e) {
        if (e && e.name === 'AbortError') throw e;
        /* 429 の応答はブラウザからは「通信エラー」に見えることがある */
        if (isOffline()) throw new SearchError('offline');
        last = new SearchError('busy');
        continue;
      }
      if (res.status === 429 || res.status >= 500) { last = new SearchError('busy', res.status); continue; }
      if (!res.ok) throw new SearchError('http', res.status);
      let json;
      try { json = await res.json(); } catch (e) { throw new SearchError('parse'); }
      const out = {
        items: (Array.isArray(json.data) ? json.data : []).map(normalizeS2),
        total: Number(json.total) || 0,
        next: typeof json.next === 'number' ? json.next : null
      };
      cache.set(url, out);
      return out;
    }
    throw last || new SearchError('busy');
  }

  /* ---------- CiNii Research ---------- */

  function list(x) { return Array.isArray(x) ? x : (x == null ? [] : [x]); }

  const JA_RE = /[\u3040-\u30ff\u3400-\u4dbf\u4e00-\u9fff]/;

  /* CiNii は同じ著者を「漢字」と「ローマ字」の2通りで返すことがある。
     和文とローマ字が同じ人数ずつ並んでいるときは、和文だけを残す */
  function ciniiAuthors(raw) {
    const names = list(raw).map(str).filter(Boolean);
    const ja = names.filter(function (n) { return JA_RE.test(n); });
    if (ja.length && ja.length * 2 === names.length) return ja;
    return names;
  }

  function normalizeCinii(it) {
    it = it || {};
    const ids = list(it['dc:identifier']);
    let doi = '';
    ids.forEach(function (d) { if (d && d['@type'] === 'cir:DOI' && !doi) doi = str(d['@value']); });
    const dcType = str(it['dc:type']);
    let type = 'other';
    if (dcType === 'Article') type = 'article';
    else if (dcType === 'Book') type = 'book';
    else if (dcType === 'Dissertation') type = 'thesis';
    const date = str(it['prism:publicationDate']) || str(it['dc:date']);
    const ym = date.match(/\d{4}/);
    const start = str(it['prism:startingPage']);
    const end = str(it['prism:endingPage']);
    let pages = str(it['prism:pageRange']);
    if (!pages && start) pages = end && end !== start ? start + '-' + end : start;
    const publisher = str(it['dc:publisher']).replace(/^[^:：]{1,20}\s[:：]\s/, '');
    const link = it.link && it.link['@id'] ? str(it.link['@id']) : str(it['@id']);
    const src = list(it['dc:source']);
    return {
      source: 'cinii',
      sourceId: str(it['@id']) || link,
      type: type,
      kind: CINII_TYPE_LABEL[dcType] || 'その他',
      authors: ciniiAuthors(it['dc:creator']),
      year: ym ? ym[0] : '',
      title: plainText(it.title) || '（タイトルなし）',
      container: str(it['prism:publicationName']).replace(/\s+[\/=]\s+.*$/, ''),
      volume: str(it['prism:volume']),
      issue: str(it['prism:number']),
      pages: pages,
      publisher: publisher,
      degree: str(it['ndl:degreeName']),
      doi: doi,
      url: link,
      pdf: '',
      fulltext: src.length && src[0] && src[0]['@id'] ? str(src[0]['@id']) : '',
      cited: null,
      abstract: plainText(it.description)
    };
  }

  /* o: { q, offset(1始まり), yearFrom, yearTo, kind, sort, fulltext }  ctx: { signal } */
  async function searchCinii(o, ctx) {
    ctx = ctx || {};
    const kind = CINII_KINDS.indexOf(o.kind) !== -1 ? o.kind : 'all';
    const start = o.offset || 1;
    const qs = new URLSearchParams();
    qs.set('q', str(o.q));
    qs.set('format', 'json');
    qs.set('count', String(PAGE));
    qs.set('start', String(start));
    if (year4(o.yearFrom)) qs.set('from', str(o.yearFrom));
    if (year4(o.yearTo)) qs.set('until', str(o.yearTo));
    if (o.sort === '0' || o.sort === '1') qs.set('sortorder', o.sort);
    if (o.fulltext) qs.set('hasLinkToFullText', 'true');
    const url = CINII_URL + kind + '?' + qs.toString();
    if (cache.has(url)) return cache.get(url);

    let last = null;
    for (let i = 0; i < 2; i++) {
      if (i > 0) await sleep(1500, ctx.signal);
      let res;
      try {
        res = await fetch(url, { signal: ctx.signal });
      } catch (e) {
        if (e && e.name === 'AbortError') throw e;
        if (isOffline()) throw new SearchError('offline');
        last = new SearchError('busy');
        continue;
      }
      if (res.status === 429 || res.status >= 500) { last = new SearchError('busy', res.status); continue; }
      if (!res.ok) throw new SearchError('http', res.status);
      let json;
      try { json = await res.json(); } catch (e) { throw new SearchError('parse'); }
      const items = list(json.items).map(normalizeCinii);
      const total = Number(json['opensearch:totalResults']) || 0;
      const from = Number(json['opensearch:startIndex']) || start;
      const nextStart = from + items.length;
      const out = { items: items, total: total, next: items.length && nextStart <= total ? nextStart : null };
      cache.set(url, out);
      return out;
    }
    throw last || new SearchError('busy');
  }

  /* 検索元のサイトで同じ語を開くためのURL（APIが混んでいるときの逃げ道） */
  function siteUrl(source, q) {
    const e = encodeURIComponent(str(q));
    return source === 's2' ? 'https://www.semanticscholar.org/search?q=' + e : 'https://cir.nii.ac.jp/all?q=' + e;
  }

  const api = {
    searchS2: searchS2,
    searchCinii: searchCinii,
    normalizeS2: normalizeS2,
    normalizeCinii: normalizeCinii,
    siteUrl: siteUrl,
    SearchError: SearchError,
    PAGE: PAGE
  };

  root.RN_SEARCH = api;
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
