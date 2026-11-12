/* =========================================================
   レポート構成ナビ  data.js
   ---------------------------------------------------------
   種類ごとの「問いの型」と「構成の型」をまとめたファイルです。
   ここにある文章はすべて「何を書くか」の案内で、
   レポートの本文そのものは作りません。
   ========================================================= */
(function (root) {
  'use strict';

  const TYPES = [
    { id: 'essay', label: '論述', desc: '主張を立て、根拠と反論への応答で論証する' },
    { id: 'experiment', label: '実験', desc: '目的・方法・結果・考察の順で報告する' },
    { id: 'review', label: '書評', desc: '本を要約し、根拠を示して評価する' },
    { id: 'survey', label: '調査', desc: '文献やデータを調べて整理し、考察する' }
  ];

  const PART = { intro: '序論', body: '本論', concl: '結論' };

  /* ---------- 課題文の読み解き ---------- */

  const DIRECTIVES = [
    { re: /論じ|論ぜよ|論述/, word: '論じる', tip: '自分の主張を立て、根拠と、反対意見への応答まで示す。' },
    { re: /述べ/, word: '述べる', tip: '自分の考えを、理由とともに筋道立てて書く。' },
    { re: /説明/, word: '説明する', tip: '定義・仕組み・理由を、知らない人にも分かる順で整理する（意見より正確さ）。' },
    { re: /比較|比べ|相違/, word: '比較する', tip: '観点をそろえて、共通点と相違点を並べる。' },
    { re: /考察/, word: '考察する', tip: '事実や結果から何が言えるかを、理由つきで示す。感想で終わらせない。' },
    { re: /分析/, word: '分析する', tip: '要素に分け、原因や関係を明らかにする。' },
    { re: /評価|批評|批判的|検討/, word: '評価・検討する', tip: '判断の基準を示し、良い点と問題点の両方を扱う。' },
    { re: /要約|まとめ(よ|なさい|て|る)/, word: '要約する', tip: '要点だけを短く、自分の言葉で。原文の言い回しを写さない。' },
    { re: /具体例|事例|実例/, word: '具体例を挙げる', tip: '具体例を少なくとも1つ入れ、出どころを示す。' },
    { re: /(自分|あなた|自身)の(意見|考え|見解|立場)/, word: '自分の意見', tip: '立場をはっきり示す。「〜と考える。なぜなら〜」の形にする。' },
    { re: /参考文献|文献|引用|出典/, word: '文献を使う', tip: '引用箇所に出典を示し、末尾に参考文献リストを付ける。' },
    { re: /授業|講義|教科書|配布資料/, word: '授業内容を踏まえる', tip: '授業で扱った概念や資料に、少なくとも一度は結びつける。' }
  ];

  function toHalf(s) {
    return String(s).replace(/[０-９]/g, function (c) { return String.fromCharCode(c.charCodeAt(0) - 0xfee0); }).replace(/[,，]/g, '');
  }

  /* 課題文から「指示語」と「字数」を読み取る */
  function readPrompt(text) {
    const t = String(text || '');
    const found = DIRECTIVES.filter(function (d) { return d.re.test(t); }).map(function (d) { return { word: d.word, tip: d.tip }; });
    let length = null;
    let lengthNote = '';
    const m = t.match(/([0-9０-９][0-9０-９,，]*)\s*(字|文字)\s*(以内|以上|程度|前後|以下)?/);
    if (m) {
      const n = parseInt(toHalf(m[1]), 10);
      if (n >= 100 && n <= 100000) {
        length = n;
        const q = m[3] || '';
        if (q === '以内' || q === '以下') lengthNote = '「' + q + '」＝超えないこと。8〜9割以上は書くのが目安。';
        else if (q === '以上') lengthNote = '「以上」＝下回らないこと。';
        else if (q === '程度' || q === '前後') lengthNote = '「' + q + '」＝おおむね±10%が目安。';
      }
    } else {
      const a = t.match(/A4[^0-9０-９]{0,8}([0-9０-９]+)\s*(枚|ページ|頁)/i);
      if (a) {
        const pages = parseInt(toHalf(a[1]), 10);
        if (pages >= 1 && pages <= 50) {
          length = pages * 1400;
          lengthNote = 'A4 ' + pages + '枚を、1枚＝約1,400字として換算した目安です（書式の指定を確認）。';
        }
      }
    }
    return { directives: found, length: length, lengthNote: lengthNote };
  }

  /* ---------- 問い（リサーチクエスチョン）の補助 ---------- */

  function v(s) { return String(s == null ? '' : s).trim(); }
  function slot(s, label) { return v(s) || '〔' + label + '〕'; }

  /* 「（範囲）の（対象）における」という前置き。空なら何も付けない */
  function where(f) {
    const sc = v(f.scope);
    const tg = v(f.target);
    if (sc && tg) return sc + 'の' + tg + 'における';
    if (sc) return sc + 'における';
    if (tg) return tg + 'における';
    return '';
  }

  const COMMON_FIELDS = [
    { key: 'target', label: '対象（誰・何について）', ph: '例：日本の公立中学生' },
    { key: 'scope', label: '範囲（時期・地域）', ph: '例：2010年代以降' },
    { key: 'angle', label: '観点（切り口）', ph: '例：家庭の経済状況' },
    { key: 'compare', label: '比べる相手（任意）', ph: '例：フィンランド' }
  ];

  const P_WHY = {
    label: '原因', hint: '「なぜ」を問う型。原因を2〜3個に分けて論じやすい。',
    make: function (c) { return slot(c.f.angle, '観点') + 'に注目すると、' + where(c.f) + c.T + 'の背景にはどのような要因があるのか'; }
  };
  const P_COMPARE = {
    label: '比較', hint: '2つを同じ観点で比べる型。違いの理由まで問うと深まる。',
    make: function (c) { return c.T + 'について、' + slot(c.f.target, '対象') + 'と' + slot(c.f.compare, '比べる相手') + 'では何がどう違い、その違いはなぜ生まれるのか'; }
  };

  const RQ = {
    essay: {
      fields: COMMON_FIELDS,
      thesisLabel: '仮の答え（主張）',
      thesisPh: '現時点の予想で構いません。例：◯◯の主な要因は△△である。なぜなら…',
      patterns: [
        P_WHY,
        {
          label: '影響', hint: '影響や結果を問う型。統計や調査結果で示しやすい。',
          make: function (c) { return (v(c.f.scope) ? v(c.f.scope) + 'において、' : '') + c.T + 'は、' + slot(c.f.target, '対象') + 'にどのような影響を与えているのか'; }
        },
        {
          label: '対応', hint: '「どうすべきか」を問う型。反対の立場を扱いやすい。',
          make: function (c) { return where(c.f) + c.T + 'に対して、どのような対応が、なぜ必要なのか'; }
        },
        P_COMPARE,
        {
          label: '評価', hint: '通説や現在の対策を吟味する型。',
          make: function (c) { return c.T + 'への現在の対策（または通説）は、' + slot(c.f.angle, '観点') + 'の点でどの程度妥当といえるか'; }
        },
        {
          label: '条件', hint: '解決や実現の条件を探る型。',
          make: function (c) { return 'どのような条件がそろえば、' + where(c.f) + c.T + 'は改善に向かうのか'; }
        }
      ]
    },
    survey: {
      fields: COMMON_FIELDS,
      thesisLabel: '見通し（分かりそうなこと）',
      thesisPh: '例：◯◯については見解が大きく2つに分かれていそうだ。',
      patterns: [
        {
          label: '実態', hint: '現状を調べる型。数字と出典をそろえて整理する。',
          make: function (c) { return where(c.f) + c.T + 'は、現在どのような状況にあるのか'; }
        },
        {
          label: '変化', hint: '時間による変化と、その要因を調べる型。',
          make: function (c) { return c.T + 'は、' + slot(c.f.scope, '時期') + 'にどのように変化してきたのか。その要因は何か'; }
        },
        {
          label: '論点整理', hint: '先行研究の見解の違いを整理する型（文献レビュー向き）。',
          make: function (c) { return c.T + 'について、先行研究ではどのような見解の違いがあるのか'; }
        },
        P_WHY,
        P_COMPARE,
        {
          label: '未解明', hint: 'まだ分かっていない点を探す型。',
          make: function (c) { return c.T + 'について、まだ十分に明らかになっていない点は何か'; }
        }
      ]
    },
    experiment: {
      fields: [
        { key: 'target', label: '試料・対象', ph: '例：銅線（直径0.5 mm）' },
        { key: 'iv', label: '変える条件（独立変数）', ph: '例：温度' },
        { key: 'dv', label: '測る量（従属変数）', ph: '例：電気抵抗' },
        { key: 'theory', label: '比べる理論値・文献値（任意）', ph: '例：抵抗温度係数の文献値' }
      ],
      thesisLabel: '仮説（予想される結果）',
      thesisPh: '例：温度を上げると抵抗はほぼ直線的に増える。理由は…',
      patterns: [
        {
          label: '関係', hint: '条件を変えたときの変化を問う、実験の基本型。',
          make: function (c) { return slot(c.f.target, '試料・対象') + 'において、' + slot(c.f.iv, '変える条件') + 'を変えると' + slot(c.f.dv, '測る量') + 'はどのように変化するのか'; }
        },
        {
          label: '法則', hint: '2つの量の関係式を確かめる型。グラフの形が根拠になる。',
          make: function (c) { return slot(c.f.iv, '変える条件') + 'と' + slot(c.f.dv, '測る量') + 'の間には、どのような関係（比例・反比例・指数など）が成り立つのか'; }
        },
        {
          label: '理論との比較', hint: '測定値と理論値・文献値を比べる型。ずれの原因が考察になる。',
          make: function (c) { return slot(c.f.dv, '測る量') + 'の測定値は、' + slot(c.f.theory, '理論値・文献値') + 'とどの程度一致するのか。ずれの原因は何か'; }
        },
        {
          label: '精度', hint: '方法そのものの妥当性・精度を問う型。',
          make: function (c) { return 'この方法で、' + slot(c.f.dv, '測る量') + 'はどの程度の精度で求められるのか'; }
        }
      ]
    },
    review: {
      fields: [
        { key: 'book', label: '書名・著者', ph: '例：松岡亮二『教育格差』' },
        { key: 'claim', label: '本の中心的な主張（自分の言葉で）', ph: '例：生まれによる格差は戦後一貫して存在する' },
        { key: 'angle', label: '評価の観点', ph: '例：データの示し方、論理の一貫性' },
        { key: 'compare', label: '比べる本・議論（任意）', ph: '例：授業で扱った◯◯の議論' }
      ],
      thesisLabel: '自分の評価（ひとことで）',
      thesisPh: '例：データの示し方は説得的だが、◯◯の視点が不足している。',
      patterns: [
        {
          label: '説得力', hint: '主張がどこまで成り立つかを吟味する、書評の基本型。',
          make: function (c) { return c.B + 'の主張「' + slot(c.f.claim, '中心的な主張') + '」は、' + slot(c.f.angle, '評価の観点') + 'の点でどの程度説得的か'; }
        },
        {
          label: '貢献', hint: 'この本の新しさ・意義を問う型。',
          make: function (c) { return c.B + 'は、この分野の議論に何を新しく加えたのか'; }
        },
        {
          label: '限界', hint: '扱われていない点・弱点を問う型。',
          make: function (c) { return c.B + 'が十分に扱っていない点は何か。それは主張の説得力にどう影響するのか'; }
        },
        {
          label: '比較', hint: '別の本や議論と比べる型。',
          make: function (c) { return c.B + 'と' + slot(c.f.compare, '比べる本・議論') + 'では、同じ問題への答えがどう違うのか'; }
        },
        {
          label: '応用', hint: '現在の問題にあてはめて考える型。',
          make: function (c) { return c.B + 'の議論は、現在の問題を考えるうえでどこまで有効か'; }
        }
      ]
    }
  };

  function rqDef(type) { return RQ[type] || RQ.essay; }

  function buildCandidates(project) {
    const def = rqDef(project.input.type);
    const f = (project.rq && project.rq.fields) || {};
    const T = slot(project.input.theme, 'テーマ');
    const B = v(f.book) || v(project.input.theme) || '〔書名〕';
    const c = { T: T, B: B, f: f };
    return def.patterns.map(function (p) {
      return { label: p.label, hint: p.hint, text: p.make(c) };
    });
  }

  /* 問いの自動チェック（機械的に分かる範囲だけ） */
  function checkQuestion(question, project) {
    const q = v(question);
    const out = [];
    if (!q) return out;
    const f = (project.rq && project.rq.fields) || {};
    const N = Number(project.input.length) || 0;

    if (/〔[^〕]*〕/.test(q)) out.push({ level: 'tip', text: '〔 〕の部分がまだ埋まっていません。自分の言葉に置き換えましょう。' });

    const isQuestion = /(か|？|\?)[。．.？?\s]*$/.test(q) || /か[。．]/.test(q);
    if (isQuestion) out.push({ level: 'ok', text: '疑問文になっています。結論で「答え」を書きやすい形です。' });
    else out.push({ level: 'tip', text: '「〜か」で終わる疑問文にすると、結論で答えやすくなります。' });

    const open = /(なぜ|どの|どう|何が|何を|何か|いかに|どれ|どんな)/.test(q);
    if (open) out.push({ level: 'ok', text: '「なぜ／どのように」型で、理由や仕組みを論じられます。' });
    else if (isQuestion) out.push({ level: 'tip', text: '「はい／いいえ」で終わる形です。「なぜ」「どの程度」「どのような条件で」を加えると、論じる中身が生まれます。' });

    const slots = Object.keys(f).map(function (k) { return v(f[k]); }).filter(function (s) { return s.length >= 2; });
    const usesSlot = slots.some(function (s) { return q.indexOf(s) !== -1; });
    if (q.length < 15) out.push({ level: 'tip', text: '短く、範囲が広い可能性があります。対象・時期・観点のどれかを入れて絞りましょう。' });
    else if (usesSlot) out.push({ level: 'ok', text: '対象や観点が入っていて、範囲が絞れています。' });
    else if (q.length < 28) out.push({ level: 'tip', text: '対象（誰・何）や範囲（時期・地域）が入ると、調べる量が現実的になります。' });

    if (/(について考える|について述べる|について調べる|とは何か)[。．\s]*$/.test(q)) {
      out.push({ level: 'tip', text: '「〜について」「〜とは何か」は説明で終わりがちです。何を明らかにしたいのかを具体的に。' });
    }

    if (N > 0) {
      let t;
      if (N < 1500) t = '字数' + N.toLocaleString('ja-JP') + '字なら、論点1〜2個で答えられる広さが目安です。';
      else if (N < 4000) t = '字数' + N.toLocaleString('ja-JP') + '字なら、論点2〜3個で答えられる広さが目安です。';
      else t = '字数' + N.toLocaleString('ja-JP') + '字なら、論点3〜4個と反論への応答まで扱える広さが目安です。';
      out.push({ level: 'info', text: t });
    }
    return out;
  }

  /* 検索語の候補（テーマと絞り込み材料の組み合わせ） */
  function suggestQueries(project) {
    const f = (project.rq && project.rq.fields) || {};
    const T = v(project.input.theme);
    const out = [];
    function add(s) { s = v(s); if (s && out.indexOf(s) === -1) out.push(s); }
    if (project.input.type === 'experiment') {
      if (v(f.iv) && v(f.dv)) add(v(f.dv) + ' ' + v(f.iv));
      if (v(f.target) && v(f.dv)) add(v(f.target) + ' ' + v(f.dv));
      add(T);
    } else if (project.input.type === 'review') {
      add(v(f.book).replace(/[『』「」]/g, ' '));
      add(T);
      if (T && v(f.angle)) add(T + ' ' + v(f.angle));
    } else {
      add(T);
      if (T && v(f.angle)) add(T + ' ' + v(f.angle));
      if (T && v(f.target)) add(T + ' ' + v(f.target));
      if (T && v(f.compare)) add(T + ' ' + v(f.compare));
    }
    return out.slice(0, 4);
  }

  /* ---------- 構成（アウトライン） ---------- */

  function hasPoints(type) { return type !== 'experiment'; }

  function defaultPoints(type, N) {
    if (!hasPoints(type)) return 0;
    if (type === 'review') return N < 1500 ? 1 : (N < 4000 ? 2 : 3);
    if (N < 1000) return 1;
    if (N < 3000) return 2;
    if (N < 6000) return 3;
    return 4;
  }

  /* 種類ごとのセクションの並びと字数の割合（合計は 1） */
  function plan(type, points, N) {
    const list = [];
    function add(key, part, ratio) { list.push({ key: key, part: part, ratio: ratio }); }
    const long = Number(N) >= 4000;
    const intro = long ? 0.12 : 0.15;
    const concl = long ? 0.10 : 0.12;
    let i;
    let counter;
    let rest;
    if (type === 'experiment') {
      add('intro', 'intro', 0.15);
      add('method', 'body', 0.15);
      add('result', 'body', 0.22);
      add('discussion', 'body', 0.25);
      add('counter', 'body', 0.13);
      add('concl', 'concl', 0.10);
    } else if (type === 'review') {
      counter = points >= 3 ? 0.13 : 0.15;
      rest = 1 - intro - concl - counter - 0.20;
      add('intro', 'intro', intro);
      add('summary', 'body', 0.20);
      for (i = 1; i <= points; i++) add('p' + i, 'body', rest / points);
      add('counter', 'body', counter);
      add('concl', 'concl', concl);
    } else if (type === 'survey') {
      counter = points >= 3 ? 0.10 : 0.13;
      const disc = points >= 3 ? 0.12 : 0.15;
      rest = 1 - intro - concl - counter - disc;
      add('intro', 'intro', intro);
      for (i = 1; i <= points; i++) add('p' + i, 'body', rest / points);
      add('counter', 'body', counter);
      add('discussion', 'body', disc);
      add('concl', 'concl', concl);
    } else {
      counter = [0.20, 0.18, 0.16, 0.15][Math.min(Math.max(points, 1), 4) - 1];
      rest = 1 - intro - concl - counter;
      add('intro', 'intro', intro);
      for (i = 1; i <= points; i++) add('p' + i, 'body', rest / points);
      add('counter', 'body', counter);
      add('concl', 'concl', concl);
    }
    return list;
  }

  /* 目安字数を割り振る。端数は「切り捨てた分が大きい順」に配り、合計をちょうど N にする */
  function allocate(N, planList) {
    N = Math.max(0, Math.round(Number(N) || 0));
    if (!planList.length) return [];
    const unit = N >= 2000 ? 50 : 10;
    const units = Math.floor(N / unit);
    const exact = planList.map(function (p) { return units * p.ratio; });
    const base = exact.map(function (x) { return Math.max(1, Math.floor(x + 1e-9)); });
    let left = units - base.reduce(function (a, b) { return a + b; }, 0);
    const order = exact.map(function (x, i) { return { i: i, frac: x - Math.floor(x + 1e-9), body: planList[i].part === 'body' ? 1 : 0 }; })
      .sort(function (a, b) { return (b.frac - a.frac) || (b.body - a.body) || (a.i - b.i); });
    let k = 0;
    while (left > 0) { base[order[k % order.length].i] += 1; left--; k++; }
    const chars = base.map(function (b) { return b * unit; });
    let big = 0;
    planList.forEach(function (p, i) { if (chars[i] > chars[big]) big = i; });
    const diff = N - chars.reduce(function (a, b) { return a + b; }, 0);
    chars[big] = Math.max(unit, chars[big] + diff);
    return chars;
  }

  function g(tag, kind, text) { return { tag: tag, kind: kind, text: text }; }

  const POINT_LEAD = [
    '最も強い理由を最初に置く',
    '1つ目とは別の角度（別のデータ・別の立場）から',
    '具体的な事例や比較で主張を補強する',
    '残る重要な論点、または実践・政策への含意'
  ];

  function sectionTitle(key, type, index) {
    if (key === 'intro') {
      if (type === 'experiment') return '序論：目的と背景';
      if (type === 'review') return '序論：本の紹介と問い';
      if (type === 'survey') return '序論：目的と調べ方';
      return '序論：背景と問い';
    }
    if (key === 'concl') {
      if (type === 'review') return '結論：総合評価';
      return '結論：問いへの答え';
    }
    if (key === 'counter') {
      if (type === 'experiment') return '本論：考察②（誤差と別の解釈）';
      if (type === 'review') return '本論：批判と限界';
      if (type === 'survey') return '本論：対立する見解と限界';
      return '本論：反論と応答';
    }
    if (key === 'method') return '本論：方法';
    if (key === 'result') return '本論：結果';
    if (key === 'discussion') return type === 'experiment' ? '本論：考察①（主張と根拠）' : '本論：考察';
    if (key === 'summary') return '本論：要約';
    if (/^p\d+$/.test(key)) {
      const n = index || parseInt(key.slice(1), 10);
      if (type === 'review') return '本論：評価点' + n;
      if (type === 'survey') return '本論：分かったこと' + n;
      return '本論：論点' + n;
    }
    return '追加セクション';
  }

  /* セクションごとの「何を書くか」と「書き出しの型」 */
  function sectionInfo(key, ctx) {
    const type = ctx.type;
    const T = v(ctx.T) || 'このテーマ';
    const Q = v(ctx.Q);
    const H = v(ctx.H);
    const f = ctx.f || {};
    const qLine = Q ? '問いを疑問文のまま明示する：「' + Q + '」' : '②で立てた問いを、疑問文のまま明示する';
    const aLine = Q ? '問い「' + Q + '」への答えを、はっきり一文で述べる' : '問いへの答えを、はっきり一文で述べる';
    let guide = [];
    let starters = [];

    if (key === 'intro') {
      if (type === 'experiment') {
        guide = [
          g('目的', 'plain', '何を明らかにする実験か。' + (Q ? '問い：「' + Q + '」' : '測る量と、確かめたい関係を一文で')),
          g('背景・原理', 'plain', '必要な理論や式を、記号の定義とともに簡潔に。教科書を写さず、出典を示す'),
          g('仮説', 'claim', H ? '予想を一文で予告する：「' + H + '」' : '予想される結果と、そう考える理由')
        ];
        starters = ['本実験の目的は、〜を明らかにすることである。', '〜は、次の式で表される（出典）。', '本実験では、〜と予想した。'];
      } else if (type === 'review') {
        guide = [
          g('書誌', 'plain', '著者・書名・出版社・出版年。著者の専門や立場をひとこと' + (v(f.book) ? '（対象：' + v(f.book) + '）' : '')),
          g('位置づけ', 'plain', 'この本がどんな問題に答えようとしているか'),
          g('問い', 'plain', qLine),
          g('評価の予告', 'claim', H ? '自分の評価を一文で予告する：「' + H + '」' : '自分の評価を一文で予告する')
        ];
        starters = ['本書は、〜を論じた著作である。', '本稿では、本書の主張を〜の観点から検討する。', '結論を先に述べれば、本書は〜と評価できる。'];
      } else if (type === 'survey') {
        guide = [
          g('背景', 'plain', T + 'を調べる理由。現状や、きっかけになった出来事・数字'),
          g('問い', 'plain', qLine),
          g('範囲と方法', 'plain', 'どの範囲を、どう調べたか（データベース・検索語・期間・選んだ基準）'),
          g('進め方', 'plain', '本論で整理する順番を一文で')
        ];
        starters = ['近年、〜が注目されている。', '本稿では、〜について文献をもとに整理する。', '調査には〜を用い、〜年以降の文献を対象とした。'];
      } else {
        guide = [
          g('背景', 'plain', T + 'がなぜ論じる価値のある問題なのかを1〜2文で（現状・数字・きっかけとなる出来事）'),
          g('問い', 'plain', qLine),
          g('主張の予告', 'claim', H ? '仮の答えを一文で予告する：「' + H + '」' : '問いへの自分の答え（主張）を一文で予告する'),
          g('進め方', 'plain', '本論で扱う論点の順番を一文で')
        ];
        starters = ['近年、〜が問題となっている。', '本稿では、〜という問いを検討する。', '結論を先に述べれば、〜である。', '以下ではまず〜を確認し、次に〜を検討する。'];
      }
    } else if (/^p\d+$/.test(key)) {
      const n = ctx.index || parseInt(key.slice(1), 10);
      const lead = POINT_LEAD[Math.min(n, POINT_LEAD.length) - 1];
      if (type === 'review') {
        guide = [
          g('主張', 'claim', '評価点その' + n + '（良い点・新しさ・説得力など）を一文で'),
          g('根拠', 'evidence', '本のどの箇所がそう言えるのか（ページを控える）。他の文献や事実とも照らし合わせる'),
          g('説明', 'plain', 'なぜその箇所が評価の理由になるのかを、自分の言葉でつなぐ')
        ];
        starters = ['本書の優れた点の第一は、〜である。', '著者は〜と述べている（p. 〇）。', 'この指摘は、〜という点で重要である。'];
      } else if (type === 'survey') {
        guide = [
          g('主張', 'claim', 'この節で分かったことを、一文でまとめる（' + lead + '）'),
          g('根拠', 'evidence', '文献・統計の内容を整理して示す。出典（著者・年・ページ）を必ず控える'),
          g('説明', 'plain', '複数の文献の共通点と違いを、自分で整理して述べる')
        ];
        starters = ['まず、〜について整理する。', '〜によれば、…である（著者, 年）。', '以上から、〜という傾向が読み取れる。'];
      } else {
        guide = [
          g('主張', 'claim', '問いへの答えを支える理由その' + n + 'を一文で（' + lead + '）'),
          g('根拠', 'evidence', '主張を支える事実・データ・研究結果を示す。出典（著者・年・ページ）を必ず控える'),
          g('説明', 'plain', 'その根拠がなぜ主張を支えるのかを、自分の言葉でつなぐ')
        ];
        starters = [['第一に', '第二に', '第三に', '第四に'][Math.min(n, 4) - 1] + '、〜である。', '〜によれば、…という（著者, 年）。', 'このことから、〜といえる。'];
      }
    } else if (key === 'counter') {
      if (type === 'experiment') {
        guide = [
          g('反論', 'counter', '結果や解釈を疑わせる要因を挙げる：誤差の原因、別の解釈の可能性'),
          g('応答', 'plain', 'その影響の大きさを見積もり、結論が揺らぐかどうかを検討する'),
          g('改善', 'plain', '測定法・回数・条件など、具体的な改善案')
        ];
        starters = ['誤差の要因として、〜が考えられる。', 'この影響は〜程度と見積もられる。', 'より精度を上げるには、〜が有効である。'];
      } else if (type === 'review') {
        guide = [
          g('反論', 'counter', '本書の弱点、扱われていない点、別の立場からの批判'),
          g('応答', 'plain', '著者ならどう答えるかを想定し、それでも残る問題を示す')
        ];
        starters = ['一方で、本書には〜という限界がある。', '著者の立場からは〜と反論できるだろう。', 'それでもなお、〜という問題は残る。'];
      } else if (type === 'survey') {
        guide = [
          g('反論', 'counter', '見解が分かれている点、データの限界、調べきれなかった範囲'),
          g('応答', 'plain', 'それを踏まえて、どこまでなら言えるのかを示す')
        ];
        starters = ['ただし、〜については見解が分かれている。', '〜という限界に注意が必要である。'];
      } else {
        guide = [
          g('反論', 'counter', '自分の主張に対する最も手ごわい反対意見・別の解釈を、公平に紹介する（できれば文献つきで）'),
          g('応答', 'plain', '認められる部分は認めたうえで、それでも主張が成り立つ理由を示す'),
          g('限定', 'plain', '必要なら主張の範囲を限定する（「〜の場合に限れば」など）')
        ];
        starters = ['確かに、〜という見方もある。', 'しかし、〜を踏まえると…', 'したがって、〜の範囲では本稿の主張は維持される。'];
      }
    } else if (key === 'concl') {
      if (type === 'experiment') {
        guide = [
          g('答え', 'claim', '目的に対応する結論を、数値を入れて簡潔に' + (Q ? '（問い：「' + Q + '」）' : '')),
          g('まとめ', 'plain', '仮説は支持されたか。理論値・文献値との一致の程度'),
          g('課題', 'plain', '残った課題を1〜2文で（新しいデータや考察は出さない）')
        ];
        starters = ['本実験により、〜であることが確かめられた。', '測定値は〜で、文献値との差は〜%であった。'];
      } else if (type === 'review') {
        guide = [
          g('答え', 'claim', aLine),
          g('まとめ', 'plain', '評価の要点を短く振り返る（新しい論点は出さない）'),
          g('課題', 'plain', 'この本を踏まえて次に考えたいこと、薦めたい読者')
        ];
        starters = ['以上から、本書は〜と評価できる。', '本書は、〜に関心のある読者に薦められる。'];
      } else {
        guide = [
          g('答え', 'claim', aLine),
          g('まとめ', 'plain', '本論の論点を短く振り返る（新しい論点や根拠は出さない）'),
          g('課題', 'plain', '扱えなかった点・今後の課題を1〜2文で')
        ];
        starters = ['本稿では、〜を検討した。', '以上から、〜と結論づけられる。', 'ただし、〜については今後の課題である。'];
      }
    } else if (key === 'method') {
      guide = [
        g('装置・試料', 'plain', '使った器具・試料を具体的に（型番・規格・濃度など）' + (v(f.target) ? '。対象：' + v(f.target) : '')),
        g('手順', 'plain', '他の人が再現できる細かさで、過去形・時系列で書く'),
        g('条件', 'plain', '変えた条件' + (v(f.iv) ? '（' + v(f.iv) + '）' : '') + 'と、そろえた条件。測定回数')
      ];
      starters = ['実験には〜を用いた。', 'まず〜を行い、次に〜を測定した。', '測定は各条件につき〜回行った。'];
    } else if (key === 'result') {
      guide = [
        g('データ', 'evidence', '表・グラフで示す（図表番号、タイトル、軸、単位、有効数字）'),
        g('記述', 'plain', '図表から読み取れる事実だけを文章で述べる（解釈は考察に回す）'),
        g('計算', 'plain', '代表的な計算過程と、誤差の見積もり')
      ];
      starters = ['測定結果を表1に示す。', '図1から、〜が増加するにつれて〜が…することが分かる。'];
    } else if (key === 'discussion') {
      if (type === 'experiment') {
        guide = [
          g('主張', 'claim', '結果から何が言えるか（仮説は支持されたか）'),
          g('根拠', 'evidence', 'データのどこがそれを示すか。' + (v(f.theory) ? v(f.theory) + 'との比較（差は何%か）' : '理論値・文献値との比較（差は何%か）')),
          g('説明', 'plain', 'なぜそうなったのかを、原理に基づいて説明する')
        ];
        starters = ['結果から、〜と考えられる。', '測定値は理論値より〜%大きかった。', 'これは〜によると考えられる。'];
      } else {
        guide = [
          g('主張', 'claim', H ? '調べた結果から自分が導く考え：「' + H + '」（調べて変わったなら書き直す）' : '調べた結果から、自分は何が言えると考えるか'),
          g('根拠', 'evidence', 'どの文献・データがその考えを支えるか'),
          g('説明', 'plain', '整理した内容と自分の考えを、理由でつなぐ')
        ];
        starters = ['以上の整理から、〜と考えられる。', 'その理由は、〜である。'];
      }
    } else if (key === 'summary') {
      guide = [
        g('要約', 'plain', '本の中心的な主張と全体の流れを、自分の言葉で短く（章ごとの丸写しにしない）' + (v(f.claim) ? '。中心的な主張：' + v(f.claim) : '')),
        g('引用', 'evidence', '重要な一節だけを「 」で直接引用し、ページを示す'),
        g('分量', 'plain', '要約は全体の2〜3割まで。主役は自分の評価')
      ];
      starters = ['本書は全〜章からなる。', '著者の中心的な主張は、〜という点にある。'];
    } else {
      guide = [
        g('主張', 'claim', 'このセクションで言いたいことを一文で'),
        g('根拠', 'evidence', '支える事実・データ・文献（出典を控える）'),
        g('説明', 'plain', '根拠と主張を、自分の言葉でつなぐ')
      ];
      starters = ['次に、〜について述べる。'];
    }
    return { guide: guide, starters: starters };
  }

  const api = {
    TYPES: TYPES,
    PART: PART,
    readPrompt: readPrompt,
    rqDef: rqDef,
    buildCandidates: buildCandidates,
    checkQuestion: checkQuestion,
    suggestQueries: suggestQueries,
    hasPoints: hasPoints,
    defaultPoints: defaultPoints,
    plan: plan,
    allocate: allocate,
    sectionTitle: sectionTitle,
    sectionInfo: sectionInfo
  };

  root.RN_DATA = api;
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
