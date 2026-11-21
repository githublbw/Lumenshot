import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import A from "./concepts-a.mjs";
import B from "./concepts-b.mjs";
import C from "./concepts-c.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const outDir = join(here, "html");
mkdirSync(outDir, { recursive: true });

const concepts = [...A, ...B, ...C];
const ORDER = ["logo", "region", "window", "screen", "rect", "circle", "arrow", "type",
  "mosaic", "grid", "undo", "redo", "pin", "copy", "save", "menu"];
const LABELS = {
  logo: "主标识", region: "区域截图", window: "窗口截图", screen: "整屏截图",
  rect: "矩形", circle: "椭圆", arrow: "箭头", type: "文字",
  mosaic: "马赛克", grid: "网格", undo: "撤销", redo: "重做",
  pin: "置顶", copy: "复制", save: "保存", menu: "菜单",
};

function icon(c, name, size) {
  const a = c.svg || {};
  return `<svg class="ic" viewBox="0 0 24 24" width="${size}" height="${size}" `
    + `fill="${a.fill || "none"}" stroke="${a.stroke || "currentColor"}" `
    + `stroke-width="${a.sw ?? 1.5}" stroke-linecap="${a.caps || "round"}" `
    + `stroke-linejoin="${a.join || "round"}">${c.icons[name]}</svg>`;
}

const CSS = `
@font-face{font-family:"Inter";src:url("../fonts/Inter-400.ttf") format("truetype");font-weight:400}
@font-face{font-family:"Inter";src:url("../fonts/Inter-500.ttf") format("truetype");font-weight:500}
@font-face{font-family:"Inter";src:url("../fonts/Inter-600.ttf") format("truetype");font-weight:600}
@font-face{font-family:"Inter";src:url("../fonts/Inter-700.ttf") format("truetype");font-weight:700}
@font-face{font-family:"JetBrains Mono";src:url("../fonts/JetBrainsMono-400.ttf") format("truetype");font-weight:400}
@font-face{font-family:"JetBrains Mono";src:url("../fonts/JetBrainsMono-500.ttf") format("truetype");font-weight:500}
*{box-sizing:border-box;margin:0;padding:0}
html,body{width:1600px;height:1180px;overflow:hidden;background:#050608}
body{font-family:"Inter","Noto Sans CJK SC",system-ui,sans-serif;-webkit-font-smoothing:antialiased;text-rendering:geometricPrecision}
.board{position:relative;width:1600px;height:1180px;padding:42px 48px 34px;color:#EDEDF0;overflow:hidden;background:#08090B;--accent:#A3F0CE}
.board::before{content:"";position:absolute;inset:0;pointer-events:none;background:
  radial-gradient(720px 440px at 8% -6%,rgba(163,240,206,.085),transparent 68%),
  radial-gradient(720px 480px at 104% 108%,rgba(127,170,255,.055),transparent 70%)}
.board::after{content:"";position:absolute;inset:0;pointer-events:none;opacity:.04;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='180' height='180'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='.8' numOctaves='3'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E")}
.board>*{position:relative;z-index:1}
svg{display:block;overflow:visible}

.head{display:flex;justify-content:space-between;align-items:flex-start;gap:56px;height:158px}
.eyebrow{display:flex;align-items:center;gap:9px;font-family:"JetBrains Mono",monospace;font-size:10px;letter-spacing:.16em;color:#7C7C86;text-transform:uppercase;margin-bottom:13px}
.eyebrow .dot{width:5px;height:5px;border-radius:50%;background:var(--accent);box-shadow:0 0 10px var(--accent)}
.eyebrow b{color:#C9C9D0;font-weight:500}
.titleline{display:flex;align-items:baseline;gap:16px;margin-bottom:11px}
.idx{font-family:"JetBrains Mono",monospace;font-size:13px;letter-spacing:.08em;color:var(--accent);border:1px solid rgba(163,240,206,.30);border-radius:5px;padding:3px 7px;transform:translateY(-11px)}
h1{font-size:54px;line-height:1;font-weight:600;letter-spacing:-.035em;color:#F6F6F8}
.en{font-size:22px;font-weight:400;letter-spacing:.02em;color:#77777F}
.tagline{font-size:16.5px;line-height:1.5;color:#B0B0B8;letter-spacing:.005em}
.grammar{margin-top:9px;font-family:"JetBrains Mono",monospace;font-size:9.5px;letter-spacing:.12em;color:#6F6F78;text-transform:uppercase}
.head-right{width:512px;padding-top:4px}
.desc{font-size:12.5px;line-height:1.85;color:#8C8C96;letter-spacing:.01em}
.palette{display:flex;align-items:center;gap:16px;margin-top:14px}
.palette .plabel{font-family:"JetBrains Mono",monospace;font-size:8.5px;letter-spacing:.14em;color:#5E5E68;text-transform:uppercase}
.sw{display:flex;align-items:center;gap:7px;font-family:"JetBrains Mono",monospace;font-size:9px;color:#8E8E98;letter-spacing:.04em}
.sw i{width:11px;height:11px;border-radius:3px;display:block;box-shadow:inset 0 0 0 1px rgba(255,255,255,.16)}
.kwrow{display:flex;gap:7px;margin-top:13px;flex-wrap:wrap}
.kw{font-size:9.5px;letter-spacing:.06em;color:#8A8A94;padding:4px 9px;border-radius:999px;border:1px solid rgba(255,255,255,.07);background:rgba(255,255,255,.018)}
.kw:first-child{color:var(--accent);border-color:rgba(163,240,206,.24);background:rgba(163,240,206,.07)}

.main{display:grid;grid-template-columns:800px 1fr;gap:28px;height:638px;margin-top:6px}
.app-col{display:flex;flex-direction:column;gap:12px}
.mini-label{display:flex;align-items:center;justify-content:space-between;font-family:"JetBrains Mono",monospace;font-size:9px;letter-spacing:.16em;color:#6F6F78;text-transform:uppercase}
.mini-label span:last-child{color:#55555E}
.app-scale{width:800px;height:600px;position:relative}
.app-scale .app{transform:scale(1.31);transform-origin:top left}
.app{position:relative;width:610px;height:446px;border-radius:16px;overflow:hidden;color:#EDEDF0;background:radial-gradient(120% 95% at 50% 0%,#111318 0%,#0A0B0E 56%,#08090B 100%);border:1px solid rgba(255,255,255,.085);box-shadow:0 44px 100px rgba(0,0,0,.62),inset 0 1px 0 rgba(255,255,255,.055)}
.app-top{position:absolute;left:14px;right:14px;top:12px;height:34px;display:flex;align-items:center;gap:8px;padding:0 9px;border-radius:12px;background:rgba(20,21,25,.74);border:1px solid rgba(255,255,255,.065);backdrop-filter:blur(18px)}
.brand{display:flex;align-items:center;gap:8px}
.brand-name{font-size:11.5px;font-weight:500;letter-spacing:.01em;color:#E4E4EA}
.mark{width:24px;height:24px;border-radius:8px;display:grid;place-items:center;color:var(--accent);background:rgba(163,240,206,.08);border:1px solid rgba(163,240,206,.16)}
.status{display:flex;align-items:center;gap:5px;font-size:9.5px;color:#7E7E88;margin-left:5px}
.status i{width:5px;height:5px;border-radius:50%;background:var(--accent);box-shadow:0 0 7px var(--accent);opacity:.85}
.grow{flex:1}
.app-ghost{width:26px;height:26px;border-radius:7px;display:grid;place-items:center;color:#8D8D97;background:transparent;border:0}
.vsep{width:1px;height:14px;background:rgba(255,255,255,.08);margin:0 3px}
.wctl{display:flex;align-items:center;gap:12px;padding-right:3px}
.wctl span{display:block;position:relative;width:11px;height:11px;opacity:.7}
.wc-min::before{content:"";position:absolute;left:1px;right:1px;top:5px;height:1px;background:#B6B6C0}
.wc-max::before{content:"";position:absolute;inset:1px;border:1px solid #B6B6C0;border-radius:1.5px}
.wc-close::before,.wc-close::after{content:"";position:absolute;left:1px;right:1px;top:5px;height:1px;background:#B6B6C0;transform:rotate(45deg)}
.wc-close::after{transform:rotate(-45deg)}
.stage{position:absolute;left:14px;right:14px;top:58px;bottom:76px;display:grid;place-items:center}
.empty{display:flex;flex-direction:column;align-items:center;gap:9px;text-align:center}
.empty-orb{width:54px;height:54px;border-radius:17px;display:grid;place-items:center;color:var(--accent);background:rgba(163,240,206,.055);border:1px solid rgba(163,240,206,.14);margin-bottom:5px}
.empty h3{font-size:16.5px;font-weight:500;letter-spacing:.01em;color:#EDEDF2}
.empty p{font-size:10.5px;color:#7E7E88;display:flex;align-items:center;gap:4px}
kbd{font-family:"Inter",sans-serif;font-size:9px;color:#9D9DA7;background:rgba(255,255,255,.045);border:1px solid rgba(255,255,255,.085);border-radius:4px;padding:1.5px 5px;line-height:1.2}
kbd+kbd{margin-left:1px}
.dock{position:absolute;left:50%;bottom:12px;transform:translateX(-50%);height:46px;display:flex;align-items:center;gap:2px;padding:0 7px;border-radius:14px;background:rgba(18,19,23,.9);border:1px solid rgba(255,255,255,.07);backdrop-filter:blur(20px);box-shadow:0 18px 40px rgba(0,0,0,.5);white-space:nowrap}
.dock-btn{height:32px;min-width:32px;padding:0 7px;border:0;border-radius:9px;background:transparent;color:#A7A7B0;display:flex;align-items:center;justify-content:center;gap:5px;font-family:inherit;font-size:10.5px}
.dock-btn.labeled{padding:0 9px 0 8px}
.dock-btn.icon{width:32px;padding:0}
.dock-btn.active{color:var(--accent);background:rgba(163,240,206,.09)}
.sep{width:1px;height:16px;background:rgba(255,255,255,.08);margin:0 5px;flex:none}

.side{display:flex;flex-direction:column;gap:11px;min-width:0}
.hero{position:relative;display:flex;align-items:center;gap:22px;height:150px;padding:0 24px;border-radius:20px;background:linear-gradient(120deg,rgba(255,255,255,.035),rgba(255,255,255,.008));border:1px solid rgba(255,255,255,.06);overflow:hidden}
.hero::after{content:"";position:absolute;right:-50px;top:-90px;width:240px;height:240px;border-radius:50%;background:radial-gradient(circle,rgba(163,240,206,.10),transparent 68%)}
.hero-tile{width:100px;height:100px;border-radius:26px;display:grid;place-items:center;flex:none;color:var(--accent);background:linear-gradient(150deg,rgba(255,255,255,.07),rgba(255,255,255,.015));border:1px solid rgba(255,255,255,.10);box-shadow:inset 0 1px 0 rgba(255,255,255,.14),0 18px 36px rgba(0,0,0,.35)}
.hero-meta{display:flex;flex-direction:column;gap:6px;min-width:0}
.hero-meta .kicker{font-family:"JetBrains Mono",monospace;font-size:9px;letter-spacing:.16em;color:#6E6E77;text-transform:uppercase}
.hero-meta b{font-size:18px;font-weight:500;letter-spacing:-.01em;color:#F1F1F5}
.hero-meta em{font-style:normal;font-family:"JetBrains Mono",monospace;font-size:9.5px;color:#7E7E88;letter-spacing:.02em;line-height:1.7}
.hero-no{position:absolute;right:20px;bottom:2px;font-size:76px;line-height:1;font-weight:600;color:rgba(255,255,255,.035);letter-spacing:-.05em}
.specs{display:grid;grid-template-columns:1fr 1fr;gap:0 24px;padding:2px 4px}
.spec{display:flex;align-items:center;justify-content:space-between;gap:12px;height:32px;border-bottom:1px solid rgba(255,255,255,.045)}
.spec span{font-size:10.5px;color:#7E7E88;letter-spacing:.02em}
.spec b{font-family:"JetBrains Mono",monospace;font-size:10.5px;font-weight:400;color:#CFCFD6;letter-spacing:.02em}

.sil{display:flex;flex-direction:column;gap:6px}
.sil-head{display:flex;justify-content:space-between;font-family:"JetBrains Mono",monospace;font-size:8.5px;letter-spacing:.14em;color:#6A6A73;text-transform:uppercase}
.sil-head span:last-child{color:#4F4F58}
.sil-row{height:31px;border-radius:8px;display:flex;align-items:center;justify-content:space-around;padding:0 8px}
.sil-dark{background:#000;color:#fff}
.sil-light{background:#F3F3EE;color:#08090A}
.detail{display:flex;gap:18px;height:216px;padding:17px;border-radius:18px;background:linear-gradient(120deg,rgba(255,255,255,.03),rgba(255,255,255,.008));border:1px solid rgba(255,255,255,.055);overflow:hidden}
.detail-art{position:relative;width:142px;height:142px;flex:none;border-radius:16px;display:grid;place-items:center;background:rgba(255,255,255,.022);border:1px solid rgba(255,255,255,.06)}
.detail-ic{color:var(--accent);display:grid;place-items:center;filter:drop-shadow(0 0 16px rgba(163,240,206,.16))}
.detail-tag{position:absolute;bottom:7px;left:0;right:0;text-align:center;font-family:"JetBrains Mono",monospace;font-size:7px;letter-spacing:.13em;color:#5E5E68;text-transform:uppercase}
.detail-copy{display:flex;flex-direction:column;justify-content:center;gap:9px;min-width:0}
.detail-copy .kicker{font-family:"JetBrains Mono",monospace;font-size:8.5px;letter-spacing:.16em;color:#6E6E77;text-transform:uppercase;margin-bottom:1px}
.note{display:flex;gap:8px;align-items:flex-start;font-size:10.2px;line-height:1.5;color:#9A9AA4}
.note em{font-style:normal;font-family:"JetBrains Mono",monospace;font-size:8px;color:var(--accent);opacity:.65;padding-top:2px;flex:none}

.sheet{margin-top:20px;display:flex;flex-direction:column;gap:11px}
.sheet-head{display:flex;align-items:center;justify-content:space-between}
.sheet-head .t{display:flex;align-items:baseline;gap:10px}
.sheet-head b{font-size:13px;font-weight:500;color:#E2E2E8;letter-spacing:.02em}
.sheet-head span{font-family:"JetBrains Mono",monospace;font-size:9px;letter-spacing:.14em;color:#6D6D76;text-transform:uppercase}
.grid16{display:grid;grid-template-columns:repeat(8,1fr);gap:10px}
.tile{position:relative;height:108px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:9px;border-radius:18px;background:rgba(255,255,255,.018);border:1px solid rgba(255,255,255,.055)}
.tile-no{position:absolute;top:8px;left:10px;font-family:"JetBrains Mono",monospace;font-size:8px;letter-spacing:.1em;color:rgba(255,255,255,.16)}
.tile-ic{display:grid;place-items:center;height:38px;color:#D2D2DA}
.tile-label{font-size:10px;color:#8B8B95;letter-spacing:.05em}
.foot{display:flex;align-items:center;justify-content:space-between;gap:24px;margin-top:12px;font-family:"JetBrains Mono",monospace;font-size:8.5px;letter-spacing:.16em;color:#4E4E57;text-transform:uppercase}
.foot .mid{color:#5C5C66}
.foot span{white-space:nowrap}
`;

function page(c) {
  const tiles = ORDER.map((n, i) => `
      <div class="tile"><span class="tile-no">${String(i + 1).padStart(2, "0")}</span>
        <div class="tile-ic">${icon(c, n, 34)}</div>
        <span class="tile-label">${LABELS[n]}</span>
      </div>`).join("");
  const specs = c.specs.map(([k, v]) => `<div class="spec"><span>${k}</span><b>${v}</b></div>`).join("");
  const kws = c.keywords.map(k => `<span class="kw">${k}</span>`).join("");
  const palette = ["#A3F0CE", "#EDEDF0", "#8A8A93", "#0B0C0E"]
    .map(hex => `<span class="sw"><i style="background:${hex}"></i>${hex.toUpperCase()}</span>`).join("");
  const silIcons = ["logo", "region", "rect", "arrow", "type", "save"].map(n => icon(c, n, 18)).join("");

  return `<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8"><title>${c.id} · ${c.cn} ${c.en}</title><style>${CSS}</style></head>
<body><div class="board c${c.id}">
  <header class="head">
    <div>
      <div class="eyebrow"><span class="dot"></span>Lumenshot · Icon Redesign <b>/ 2026</b></div>
      <div class="titleline"><span class="idx">${c.id}</span><h1>${c.cn}</h1><span class="en">${c.en}</span></div>
      <p class="tagline">${c.tagline}</p>
      <p class="grammar">${c.grammar}</p>
    </div>
    <div class="head-right">
      <p class="desc">${c.desc}</p>
      <div class="palette"><span class="plabel">共用色板</span>${palette}</div>
      <div class="kwrow">${kws}</div>
    </div>
  </header>

  <main class="main">
    <section class="app-col">
      <div class="mini-label"><span>In Context / 真实界面</span><span>610 × 446</span></div>
      <div class="app-scale"><div class="app">
        <div class="app-top">
          <div class="brand"><span class="mark">${icon(c, "logo", 15)}</span><span class="brand-name">Lumenshot</span></div>
          <span class="status"><i></i>就绪</span><span class="grow"></span>
          <span class="app-ghost">${icon(c, "menu", 14)}</span><span class="vsep"></span>
          <div class="wctl"><span class="wc-min"></span><span class="wc-max"></span><span class="wc-close"></span></div>
        </div>
        <div class="stage"><div class="empty">
          <div class="empty-orb">${icon(c, "region", 27)}</div>
          <h3>还没有截图</h3>
          <p>选择一种方式开始，或按 <kbd>Ctrl</kbd><kbd>Alt</kbd><kbd>A</kbd></p>
        </div></div>
        <div class="dock">
          <button class="dock-btn labeled active">${icon(c, "region", 15)}<span>区域</span></button>
          <button class="dock-btn labeled">${icon(c, "window", 15)}<span>窗口</span></button>
          <button class="dock-btn labeled">${icon(c, "screen", 15)}<span>整屏</span></button>
          <span class="sep"></span>
          <button class="dock-btn icon">${icon(c, "rect", 17)}</button>
          <button class="dock-btn icon">${icon(c, "circle", 17)}</button>
          <button class="dock-btn icon">${icon(c, "arrow", 17)}</button>
          <button class="dock-btn icon">${icon(c, "type", 17)}</button>
          <button class="dock-btn icon">${icon(c, "mosaic", 17)}</button>
          <button class="dock-btn icon">${icon(c, "grid", 17)}</button>
          <span class="sep"></span>
          <button class="dock-btn icon">${icon(c, "undo", 17)}</button>
          <button class="dock-btn icon">${icon(c, "redo", 17)}</button>
        </div>
      </div></div>
    </section>

    <aside class="side">
      <div class="hero">
        <div class="hero-tile">${icon(c, c.signature, 66)}</div>
        <div class="hero-meta"><span class="kicker">Construction Study</span><b>${c.signatureCn}</b><em>${c.signatureSub}</em></div>
        <span class="hero-no">${c.id}</span>
      </div>
      <div class="specs">${specs}</div>
      <div class="sil">
        <div class="sil-head"><span>Silhouette / 剪影测试</span><span>去掉颜色与材质后仍能区分</span></div>
        <div class="sil-row sil-dark">${silIcons}</div>
        <div class="sil-row sil-light">${silIcons}</div>
      </div>
      <div class="detail">
        <div class="detail-art"><span class="detail-ic">${icon(c, c.signature, 84)}</span><span class="detail-tag">${c.signatureEn}</span></div>
        <div class="detail-copy"><span class="kicker">造形规则 / Rules</span>
          ${c.rules.map((r, i) => `<div class="note"><em>0${i + 1}</em><span>${r}</span></div>`).join("")}
        </div>
      </div>
    </aside>
  </main>

  <section class="sheet">
    <div class="sheet-head"><div class="t"><b>完整图标组</b><span>16 icons / one construction language</span></div><span>34 px 展示 · 15–17 px 实用</span></div>
    <div class="grid16">${tiles}</div>
  </section>

  <footer class="foot">
    <span>Lumenshot · Icon Redesign / 10 Constructions</span>
    <span class="mid">Same UI · Same palette · Different geometry</span>
    <span>${c.id} / 10 · ${c.en}</span>
  </footer>
</div></body></html>`;
}

for (const c of concepts) writeFileSync(join(outDir, `${c.id}-${c.slug}.html`), page(c), "utf8");
console.log(`wrote ${concepts.length} boards`);

/* 00 · Overview ----------------------------------------------------- */
const OV_CSS = `
@font-face{font-family:"Inter";src:url("../fonts/Inter-400.ttf") format("truetype");font-weight:400}
@font-face{font-family:"Inter";src:url("../fonts/Inter-500.ttf") format("truetype");font-weight:500}
@font-face{font-family:"Inter";src:url("../fonts/Inter-600.ttf") format("truetype");font-weight:600}
@font-face{font-family:"Inter";src:url("../fonts/Inter-700.ttf") format("truetype");font-weight:700}
@font-face{font-family:"JetBrains Mono";src:url("../fonts/JetBrainsMono-400.ttf") format("truetype");font-weight:400}
@font-face{font-family:"JetBrains Mono";src:url("../fonts/JetBrainsMono-500.ttf") format("truetype");font-weight:500}
*{box-sizing:border-box;margin:0;padding:0}
html,body{width:1600px;height:1180px;overflow:hidden;background:#050608}
body{font-family:"Inter","Noto Sans CJK SC",system-ui,sans-serif;-webkit-font-smoothing:antialiased;text-rendering:geometricPrecision}
svg{display:block;overflow:visible}
.ov{position:relative;width:1600px;height:1180px;padding:40px 48px 28px;background:#08090B;color:#EDEDF0;overflow:hidden;--accent:#A3F0CE}
.ov::before{content:"";position:absolute;inset:0;pointer-events:none;background:
  radial-gradient(760px 460px at 6% -8%,rgba(163,240,206,.09),transparent 68%),
  radial-gradient(720px 480px at 104% 108%,rgba(127,170,255,.05),transparent 70%)}
.ov::after{content:"";position:absolute;inset:0;pointer-events:none;opacity:.035;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='180' height='180'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='.8' numOctaves='3'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E")}
.ov>*{position:relative;z-index:1}
.eyebrow{display:flex;align-items:center;gap:9px;font-family:"JetBrains Mono",monospace;font-size:10px;letter-spacing:.17em;color:#7C7C86;text-transform:uppercase}
.eyebrow .dot{width:5px;height:5px;border-radius:50%;background:var(--accent);box-shadow:0 0 10px var(--accent)}
.ov-head{display:flex;justify-content:space-between;align-items:flex-end;height:126px;padding-bottom:18px;border-bottom:1px solid rgba(255,255,255,.07)}
.titleline{display:flex;align-items:baseline;gap:15px;margin-top:12px}
h1{font-size:44px;line-height:1;font-weight:600;letter-spacing:-.035em;color:#F6F6F8}
.titleline .en{font-size:18px;font-weight:400;color:#77777F;letter-spacing:.02em}
.sub{margin-top:10px;font-size:12.5px;line-height:1.6;color:#8C8C96;letter-spacing:.01em}
.legend{display:flex;gap:38px;padding-bottom:2px}
.legend div{display:flex;flex-direction:column;align-items:flex-end;gap:3px}
.legend b{font-family:"JetBrains Mono",monospace;font-size:26px;font-weight:500;color:#DFF9EC;letter-spacing:-.02em}
.legend span{font-size:9.5px;letter-spacing:.08em;color:#6F6F78}
.ov-grid{display:grid;grid-template-columns:repeat(5,1fr);grid-auto-rows:1fr;gap:14px;height:878px;margin-top:18px}
.card{display:flex;flex-direction:column;min-width:0;padding:15px 15px 12px;background:linear-gradient(180deg,#0E0F13,#0B0C0F);border:1px solid rgba(255,255,255,.075);border-radius:13px;overflow:hidden}
.c-head{display:flex;align-items:baseline;gap:7px;min-width:0}
.c-head .num{font-family:"JetBrains Mono",monospace;font-size:8.5px;color:var(--accent);border:1px solid rgba(163,240,206,.28);border-radius:4px;padding:1.5px 4px;transform:translateY(-2px)}
.c-head h2{font-size:20px;font-weight:600;letter-spacing:-.015em;color:#F2F2F5;white-space:nowrap}
.c-head .en{font-size:8.5px;color:#6F6F78;letter-spacing:.03em;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.c-tag{margin-top:6px;height:32px;font-size:10.5px;line-height:1.5;color:#8E8E98;letter-spacing:.005em}
.c-hero{display:flex;align-items:center;gap:10px;margin-top:9px;height:78px}
.c-hero .tile{width:70px;height:70px;flex:0 0 70px;display:flex;align-items:center;justify-content:center;color:var(--accent);border:1px solid rgba(255,255,255,.07);border-radius:11px;background:radial-gradient(circle at 50% 42%,rgba(163,240,206,.10),rgba(255,255,255,.012) 68%)}
.c-hero .gram{font-family:"JetBrains Mono",monospace;font-size:8px;line-height:1.6;letter-spacing:.10em;color:#6F6F78;text-transform:uppercase;min-width:0;overflow:hidden}
.c-icons{display:grid;grid-template-columns:repeat(8,1fr);gap:3px;margin-top:9px;color:#D8D8DE}
.c-icons svg{margin:auto}
.c-rules{margin-top:10px;display:flex;flex-direction:column;gap:4px}
.c-rules div{display:flex;gap:5px;font-size:9px;line-height:1.35;color:#8A8A94;letter-spacing:.005em}
.c-rules em{font-family:"JetBrains Mono",monospace;font-style:normal;font-size:7.5px;color:#5E8F7B;padding-top:1px;flex:0 0 12px}
.c-sil{margin-top:9px}
.c-sil .lbl{display:block;font-family:"JetBrains Mono",monospace;font-size:6.5px;letter-spacing:.13em;color:#5D5D67;text-transform:uppercase;margin-bottom:3px}
.sil-b{display:flex;align-items:center;justify-content:space-between;height:19px;padding:0 8px;border-radius:4px;margin-top:3px}
.sil-dark{background:#000;color:#F4F4F4}
.sil-light{background:#E9E9E6;color:#08090B}
.c-kws{margin-top:auto;display:flex;flex-wrap:wrap;gap:4px;padding-top:9px}
.c-kws span{font-size:8px;letter-spacing:.05em;color:#9A9AA4;padding:2px 6px;border:1px solid rgba(255,255,255,.075);border-radius:20px}
.c-meta{display:flex;justify-content:space-between;margin-top:7px;padding-top:7px;border-top:1px dashed rgba(255,255,255,.07);font-family:"JetBrains Mono",monospace;font-size:7.5px;letter-spacing:.05em;color:#5D5D67}
.ov-foot{display:flex;justify-content:space-between;align-items:center;height:24px;margin-top:11px;font-family:"JetBrains Mono",monospace;font-size:8.5px;letter-spacing:.13em;color:#5D5D67;text-transform:uppercase}
.ov-foot .mid{color:#77777F}
`;

function overviewPage() {
  const SIL = ["logo", "region", "rect", "arrow", "type", "save"];
  const cards = concepts.map((c) => `
    <article class="card">
      <div class="c-head"><span class="num">${c.id}</span><h2>${c.cn}</h2><span class="en">${c.en}</span></div>
      <p class="c-tag">${c.tagline}</p>
      <div class="c-hero"><div class="tile">${icon(c, c.signature, 54)}</div><div class="gram">${c.grammar}</div></div>
      <div class="c-icons">${ORDER.map((n) => icon(c, n, 18)).join("")}</div>
      <div class="c-rules">${c.rules.map((r, i) => `<div><em>0${i + 1}</em><span>${r}</span></div>`).join("")}</div>
      <div class="c-sil"><span class="lbl">Silhouette / 剪影</span>
        <div class="sil-b sil-dark">${SIL.map((n) => icon(c, n, 13)).join("")}</div>
        <div class="sil-b sil-light">${SIL.map((n) => icon(c, n, 13)).join("")}</div>
      </div>
      <div class="c-kws">${c.keywords.map((k) => `<span>${k}</span>`).join("")}</div>
      <div class="c-meta"><span>${c.specs[1][0]} ${c.specs[1][1]}</span><span>MIN ${c.specs[5][1]}</span></div>
    </article>`).join("");
  return `<!doctype html><html><head><meta charset="utf-8"><style>${OV_CSS}</style></head>
<body><div class="ov">
  <header class="ov-head">
    <div>
      <div class="eyebrow"><span class="dot"></span>Lumenshot · Icon Redesign / 2026</div>
      <div class="titleline"><h1>10 种造形语言</h1><span class="en">10 Constructions</span></div>
      <p class="sub">同一个界面、同一套色板、同一组 16 枚功能，只改变几何构造。每套都经过黑白剪影测试，去掉颜色后仍能彼此区分。</p>
    </div>
    <div class="legend">
      <div><b>16</b><span>每套图标</span></div>
      <div><b>24</b><span>统一网格 / px</span></div>
      <div><b>10</b><span>造形语法</span></div>
    </div>
  </header>
  <main class="ov-grid">${cards}</main>
  <footer class="ov-foot"><span>Lumenshot · Icon Redesign</span><span class="mid">Same UI · Same palette · Different geometry</span><span>01–10 / 2026</span></footer>
</div></body></html>`;
}
writeFileSync(join(outDir, "00-overview.html"), overviewPage(), "utf8");
console.log("wrote 00-overview");
