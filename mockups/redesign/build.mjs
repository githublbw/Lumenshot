// Lumenshot icon system — 10 proposals.
// Generates one self-contained HTML board per concept, 1600 × 1180.
import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const outDir = join(here, "html");
mkdirSync(outDir, { recursive: true });

/* ------------------------------------------------------------------ */
/* Base geometry — a 24px grid, drawn once, reinterpreted by each set. */
/* ------------------------------------------------------------------ */
const BASE = {
  logo: `<circle cx="12" cy="12" r="9.4"/><path d="m14.31 8 5.42 9.39"/><path d="M9.69 8h11.48"/><path d="m7.38 12 5.74-9.94"/><path d="M9.69 16 3.95 6.06"/><path d="M14.31 16H2.83"/><path d="m16.62 12-5.74 9.94"/>`,
  region: `<path d="M3.5 9V6.5a3 3 0 0 1 3-3H9"/><path d="M15 3.5h2.5a3 3 0 0 1 3 3V9"/><path d="M20.5 15v2.5a3 3 0 0 1-3 3H15"/><path d="M9 20.5H6.5a3 3 0 0 1-3-3V15"/><circle cx="12" cy="12" r="2.4"/>`,
  window: `<rect x="3" y="4.5" width="18" height="15" rx="2.5"/><path d="M3 9h18"/><circle cx="6.1" cy="6.75" r=".55" fill="currentColor" stroke="none"/><circle cx="8.1" cy="6.75" r=".55" fill="currentColor" stroke="none"/><circle cx="10.1" cy="6.75" r=".55" fill="currentColor" stroke="none"/><rect x="7" y="12" width="10" height="4.8" rx="1"/>`,
  screen: `<rect x="2.5" y="4" width="19" height="13" rx="2.5"/><path d="M12 17v3.5"/><path d="M8 20.5h8"/>`,
  rect: `<rect x="3.5" y="6" width="17" height="12" rx="2.5"/>`,
  circle: `<circle cx="12" cy="12" r="8.5"/>`,
  arrow: `<path d="M7 17 17 7"/><path d="M8.5 7H17v8.5"/>`,
  type: `<path d="M5 6.5v-2h14v2"/><path d="M12 4.5v15"/><path d="M9 19.5h6"/>`,
  mosaic: `<circle cx="9.2" cy="12" r="5.7"/><circle cx="14.8" cy="12" r="5.7"/>`,
  grid: `<rect x="3.5" y="3.5" width="17" height="17" rx="2.5"/><path d="M3.5 9.17h17M3.5 14.83h17M9.17 3.5v17M14.83 3.5v17"/>`,
  undo: `<path d="M9 14 4 9l5-5"/><path d="M4 9h10.5a5 5 0 0 1 0 10H10"/>`,
  redo: `<path d="m15 14 5-5-5-5"/><path d="M20 9H9.5a5 5 0 0 0 0 10H14"/>`,
  pin: `<path d="M12 17v4.5"/><path d="M9 10.6V7a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v3.6a2 2 0 0 0 .59 1.42l1.82 1.82A1 1 0 0 1 17 16H7a1 1 0 0 1-.41-1.76l1.82-1.82A2 2 0 0 0 9 10.6Z"/>`,
  copy: `<rect x="8.5" y="8.5" width="12" height="12" rx="2.2"/><path d="M15.5 8.5V5.7a2.2 2.2 0 0 0-2.2-2.2H5.7a2.2 2.2 0 0 0-2.2 2.2v7.6a2.2 2.2 0 0 0 2.2 2.2h2.8"/>`,
  save: `<path d="M12 3.5v11.5"/><path d="m7.5 10.5 4.5 4.5 4.5-4.5"/><path d="M4 19.5h16"/>`,
  menu: `<path d="M4 7h16M4 12h16M4 17h10"/>`,
};

const CLOSED = new Set(["logo", "region", "window", "screen", "rect", "circle", "type", "mosaic", "grid", "pin", "copy"]);

const ICON_LABELS = {
  logo: "主标识", region: "区域截图", window: "窗口截图", screen: "整屏截图",
  rect: "矩形", circle: "椭圆", arrow: "箭头", type: "文字",
  mosaic: "马赛克", grid: "网格", undo: "撤销", redo: "重做",
  pin: "置顶", copy: "复制", save: "保存", menu: "菜单",
};
const ICON_ORDER = ["logo", "region", "window", "screen", "rect", "circle", "arrow", "type",
  "mosaic", "grid", "undo", "redo", "pin", "copy", "save", "menu"];

/* ------------------------------------------------------------------ */
/* SVG helpers                                                        */
/* ------------------------------------------------------------------ */
function svgEl(inner, o = {}) {
  const {
    size = 24, sw = 1.5, caps = "round", join = "round", stroke = "currentColor",
    fill = "none", cls = "ic", style = "", extra = "",
  } = o;
  return `<svg class="${cls}" viewBox="0 0 24 24" width="${size}" height="${size}" fill="${fill}" stroke="${stroke}" stroke-width="${sw}" stroke-linecap="${caps}" stroke-linejoin="${join}" style="${style}" ${extra}>${inner}</svg>`;
}

const plain = (sw = 1.5, caps = "round", join = "round") =>
  (name, size) => svgEl(BASE[name], { size, sw, caps, join });

/* Duotone: outline over a soft silhouette. */
function duotoneIcon(name, size) {
  const under = CLOSED.has(name)
    ? svgEl(BASE[name], { size, fill: "#7C8AA0", stroke: "none", cls: "under", extra: 'fill-opacity=".34"' })
    : svgEl(BASE[name], { size, sw: 5.4, cls: "under", stroke: "#7C8AA0", extra: 'stroke-opacity=".26"' });
  return `<span class="stack" style="width:${size}px;height:${size}px">${under}${svgEl(BASE[name], { size })}</span>`;
}

/* Bento: square caps, tight joins, one 2.4px mint cube as a signature. */
function bentoIcon(name, size) {
  const cube = `<rect x="19.3" y="19.3" width="2.6" height="2.6" rx=".3" fill="currentColor" stroke="none"/>`;
  return svgEl(BASE[name] + cube, { size, sw: 2.3, caps: "square", join: "miter" });
}

/* Viewfinder: brackets around the glyph + coordinate tick. */
function hudGlyph(name, size) {
  return `<span class="hud"><i></i><i></i><i></i><i></i>${svgEl(BASE[name], { size, sw: 1.3 })}</span>`;
}

/* Pixel: 12 × 12 maps, rendered on a 2px unit grid. */
const PIXELS = {
  logo: [
    "............",
    "...######...",
    "..#......#..",
    ".#...##...#.",
    ".#..####..#.",
    "#..##..##..#",
    "#..#....#..#",
    "#..##..##..#",
    ".#..####..#.",
    ".#...##...#.",
    "..#......#..",
    "...######...",
  ],
  region: [
    "###......###",
    "#..........#",
    "#..........#",
    "............",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    "............",
    "#..........#",
    "#..........#",
    "###......###",
  ],
  window: [
    "############",
    "#..........#",
    "############",
    "#..........#",
    "#..........#",
    "#..........#",
    "#...####...#",
    "#...####...#",
    "#..........#",
    "#..........#",
    "#..........#",
    "############",
  ],
  screen: [
    "############",
    "#..........#",
    "#..........#",
    "#..........#",
    "#..........#",
    "#..........#",
    "############",
    ".....##.....",
    ".....##.....",
    "............",
    "...######...",
    "............",
  ],
  rect: [
    "............",
    "............",
    ".##########.",
    ".#........#.",
    ".#........#.",
    ".#........#.",
    ".#........#.",
    ".#........#.",
    ".#........#.",
    ".##########.",
    "............",
    "............",
  ],
  circle: [
    "............",
    "....####....",
    "..##....##..",
    ".#........#.",
    ".#........#.",
    "#..........#",
    "#..........#",
    ".#........#.",
    ".#........#.",
    "..##....##..",
    "....####....",
    "............",
  ],
  arrow: [
    "............",
    "........####",
    ".......#####",
    "......##..#.",
    ".....##..#..",
    "....##..#...",
    "...#..#.....",
    "..#..#......",
    ".##..#......",
    ".#..........",
    "............",
    "............",
  ],
  type: [
    "............",
    ".##########.",
    ".#...##...#.",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    "............",
    "............",
  ],
  mosaic: [
    "............",
    "..#####.....",
    "..#...#.....",
    "..#..#####..",
    "..####...#..",
    ".....#####..",
    ".....#...#..",
    ".....#...#..",
    ".....#####..",
    "............",
    "............",
    "............",
  ],
  grid: [
    "............",
    ".##########.",
    ".#..#..#..#.",
    ".#..#..#..#.",
    "###########.",
    ".#..#..#..#.",
    ".#..#..#..#.",
    "###########.",
    ".#..#..#..#.",
    ".#..#..#..#.",
    ".##########.",
    "............",
  ],
  undo: [
    "............",
    "..#.........",
    ".#..........",
    "############",
    ".#........#.",
    "..#......#..",
    ".........#..",
    ".........#..",
    ".........#..",
    "..........#.",
    "..........#.",
    "............",
  ],
  redo: [
    "............",
    ".........#..",
    "..........#.",
    "############",
    ".#........#.",
    "..#......#..",
    "..#.........",
    "..#.........",
    "..#.........",
    ".#..........",
    ".#..........",
    "............",
  ],
  pin: [
    "............",
    "....####....",
    "...######...",
    "...######...",
    "...######...",
    "....####....",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    "............",
    "............",
  ],
  copy: [
    "............",
    ".########...",
    ".#......#...",
    ".#..#####...",
    ".#..#...#...",
    ".#..#...#...",
    ".#..#...#...",
    ".#..#...#...",
    ".#####...#...",
    ".....#...#...",
    ".....#####...",
    "............",
  ],
  save: [
    "............",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    ".....##.....",
    "..#..##..#..",
    "...#.##.#...",
    "....####....",
    ".....##.....",
    "............",
    ".##########.",
    "............",
  ],
  menu: [
    "............",
    "............",
    ".##########.",
    "............",
    "............",
    "............",
    ".##########.",
    "............",
    "............",
    "............",
    ".##########.",
    "............",
  ],
};

function pixelIcon(name, size) {
  const rows = PIXELS[name] || [];
  const rects = [];
  rows.forEach((row, y) => {
    [...row.padEnd(12, ".")].slice(0, 12).forEach((ch, x) => {
      if (ch === "#") rects.push(`<rect x="${x * 2}" y="${y * 2}" width="2" height="2"/>`);
    });
  });
  return `<svg class="ic pixel" viewBox="0 0 24 24" width="${size}" height="${size}" fill="currentColor" shape-rendering="crispEdges">${rects.join("")}</svg>`;
}

/* Blueprint: same lines, with nodes and a technical title block. */
const NODES = {
  logo: [[12, 2.6], [12, 21.4], [3.9, 7.3], [20.1, 7.3], [3.9, 16.7], [20.1, 16.7], [12, 12]],
  region: [[3.5, 3.5], [20.5, 3.5], [3.5, 20.5], [20.5, 20.5], [12, 12]],
  window: [[3, 4.5], [21, 4.5], [3, 19.5], [21, 19.5]],
  screen: [[2.5, 4], [21.5, 4], [2.5, 17], [21.5, 17], [12, 20.5]],
  rect: [[3.5, 6], [20.5, 6], [3.5, 18], [20.5, 18]],
  circle: [[12, 3.5], [20.5, 12], [12, 20.5], [3.5, 12], [12, 12]],
  arrow: [[7, 17], [17, 7], [8.5, 7], [17, 15.5]],
  type: [[5, 4.5], [19, 4.5], [12, 4.5], [12, 19.5], [9, 19.5], [15, 19.5]],
  mosaic: [[9.2, 6.3], [14.8, 6.3], [9.2, 17.7], [14.8, 17.7], [12, 12]],
  grid: [[3.5, 3.5], [20.5, 3.5], [3.5, 20.5], [20.5, 20.5], [12, 12]],
  undo: [[4, 9], [9, 4], [9, 14], [14.5, 19], [10, 19]],
  redo: [[20, 9], [15, 4], [15, 14], [9.5, 19], [14, 19]],
  pin: [[12, 21.5], [9, 10.6], [15, 10.6], [7, 16], [17, 16]],
  copy: [[8.5, 8.5], [20.5, 8.5], [8.5, 20.5], [20.5, 20.5], [3.5, 3.5], [15.5, 3.5], [3.5, 14.5]],
  save: [[12, 3.5], [12, 15], [7.5, 10.5], [16.5, 10.5], [4, 19.5], [20, 19.5]],
  menu: [[4, 7], [20, 7], [4, 12], [20, 12], [4, 17], [14, 17]],
};

function blueprintIcon(name, size) {
  const dots = (NODES[name] || [])
    .map(([x, y]) => `<circle class="node" cx="${x}" cy="${y}" r="1.15"/>`)
    .join("");
  return svgEl(BASE[name] + dots, { size, sw: 1.15, caps: "butt" });
}

/* Chrome: gradient stroke + specular glow (defined in page defs). */
function chromeIcon(name, size) {
  return svgEl(BASE[name], { size, sw: 1.7, stroke: "url(#chromeGrad)", cls: "ic chrome" });
}

/* ------------------------------------------------------------------ */
/* Concepts                                                           */
/* ------------------------------------------------------------------ */
const concepts = [
  {
    id: "01", heroSpec: "24 grid / 1.50 px stroke", slug: "filament", cn: "光纤", en: "Filament",
    tagline: "把噪声减到零，只留下线的重量。",
    desc: "参考 21st.dev 上最克制的 shadcn 组件语言：去掉渐变与阴影，只留下 1.5px 等重线、24 网格和精准圆角。所有端点对齐像素中心，14px 下仍然清晰。",
    accent: "#A3F0CE", accent2: "#E9E9EE", glow: "rgba(163,240,206,.10)", glow2: "rgba(120,140,255,.05)",
    palette: ["#A3F0CE", "#E9E9EE", "#9A9AA4", "#0B0C0E"],
    specs: [["网格", "24 × 24"], ["描边", "1.50 px"], ["端点", "Round"], ["圆角", "2.5 px"], ["最小尺寸", "14 px"], ["视觉修正", "光学"]],
    keywords: ["极简", "等重", "光学", "无渐变"],
    icon: plain(1.5),
    tile: (n, i) => svgEl(BASE[n], { size: 34, sw: 1.5 }),
    css: `
.c01 .tile{border-radius:18px;background:rgba(255,255,255,.018);border:1px solid rgba(255,255,255,.055)}
.c01 .tile:hover{border-color:rgba(163,240,206,.18);background:rgba(163,240,206,.025)}
.c01 .dock-btn.active{background:rgba(163,240,206,.09);color:var(--accent)}
.c01 .mark{background:rgba(163,240,206,.08);border-color:rgba(163,240,206,.16)}
.c01 .empty-orb{background:rgba(163,240,206,.055);border:1px solid rgba(163,240,206,.14);box-shadow:0 0 60px rgba(163,240,206,.08)}
.c01 .state.on{background:rgba(163,240,206,.10);color:var(--accent);border-color:rgba(163,240,206,.22)}
`,
  },
  {
    id: "02", heroSpec: "24 grid / mint line + slate fill", slug: "duotone", cn: "叠影", en: "Duotone",
    tagline: "一线一影，先看见，再读懂。",
    desc: "同一条 1.5px 薄荷轮廓下垫一层 34% 灰蓝填充：冷色负责体积，薄荷负责识别。像 21st.dev 的 duotone 插画，但克制到可以进入工具栏。",
    accent: "#A3F0CE", accent2: "#DFFFF2", glow: "rgba(163,240,206,.12)", glow2: "rgba(124,138,160,.07)",
    palette: ["#A3F0CE", "#7C8AA0", "#DFFFF2", "#0B0C0E"],
    specs: [["网格", "24 × 24"], ["描边", "1.50 px"], ["填充", "34 % 灰蓝"], ["端点", "Round"], ["层数", "2 层"], ["圆角", "2.5 px"]],
    keywords: ["双色", "柔和", "层次", "描边+填充"],
    icon: duotoneIcon,
    tile: (n) => duotoneIcon(n, 34),
    css: `
.c02 .tile{border-radius:18px;background:linear-gradient(165deg,rgba(163,240,206,.075),rgba(124,138,160,.028));border:1px solid rgba(163,240,206,.10)}
.c02 .tile-ic{color:#C7F8E2}
.c02 .dock-btn.active{background:linear-gradient(180deg,rgba(163,240,206,.20),rgba(163,240,206,.06));color:var(--accent)}
.c02 .mark{background:linear-gradient(160deg,rgba(163,240,206,.18),rgba(163,240,206,.05));border-color:rgba(163,240,206,.22)}
.c02 .empty-orb{background:linear-gradient(160deg,rgba(163,240,206,.12),rgba(163,240,206,.03));border:1px solid rgba(163,240,206,.18)}
.c02 .state.on{background:linear-gradient(180deg,rgba(163,240,206,.20),rgba(163,240,206,.06));color:var(--accent);border-color:rgba(163,240,206,.26)}
`,
  },
  {
    id: "03", heroSpec: "24 grid / 2.00 px stroke", slug: "prism", cn: "棱镜", en: "Prism",
    tagline: "光从左上角穿过工具栏。",
    desc: "每枚图标坐在一块 32px 的磨砂玻璃片上：1px 渐变描边、顶部高光、24px 背景模糊。图形本身 2px 纯白，只有激活时才点亮为薄荷。",
    accent: "#A3F0CE", accent2: "#7FD4FF", glow: "rgba(127,212,255,.13)", glow2: "rgba(196,167,255,.11)",
    palette: ["#A3F0CE", "#7FD4FF", "#C4A7FF", "#161A20"],
    specs: [["玻璃片", "32 px"], ["描边", "2.00 px"], ["模糊", "24 px"], ["高光", "1 px"], ["渐变边框", "140°"], ["圆角", "20 px"]],
    keywords: ["玻璃", "渐变边框", "高光", "通透"],
    icon: (n, s) => svgEl(BASE[n], { size: s, sw: 2 }),
    tile: (n) => svgEl(BASE[n], { size: 34, sw: 2 }),
    css: `
.c03 .tile{border-radius:20px;border:1px solid transparent;
  background:linear-gradient(150deg,rgba(163,240,206,.11),rgba(255,255,255,.018) 45%,rgba(127,212,255,.07)) padding-box,
             linear-gradient(145deg,rgba(163,240,206,.55),rgba(255,255,255,.06) 42%,rgba(196,167,255,.45)) border-box;
  box-shadow:inset 0 1px 0 rgba(255,255,255,.14),0 14px 30px rgba(0,0,0,.3);backdrop-filter:blur(10px)}
.c03 .tile-ic{color:#F2FFFA}
.c03 .mark{background:linear-gradient(150deg,rgba(163,240,206,.22),rgba(127,212,255,.08));border:1px solid rgba(255,255,255,.2);box-shadow:inset 0 1px 0 rgba(255,255,255,.3)}
.c03 .dock{background:rgba(24,29,36,.55);border:1px solid rgba(255,255,255,.12);backdrop-filter:blur(30px) saturate(1.35);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.14),0 20px 50px rgba(0,0,0,.5)}
.c03 .dock-btn.active{background:linear-gradient(180deg,rgba(163,240,206,.24),rgba(127,212,255,.08));color:#E7FFF7;
  box-shadow:inset 0 1px 0 rgba(255,255,255,.28),0 0 20px rgba(163,240,206,.2)}
.c03 .empty-orb{background:linear-gradient(150deg,rgba(163,240,206,.16),rgba(127,212,255,.05));border:1px solid rgba(255,255,255,.16);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.25),0 0 70px rgba(127,212,255,.14)}
.c03 .detail-art{background:linear-gradient(150deg,rgba(163,240,206,.13),rgba(127,212,255,.04));border-color:rgba(255,255,255,.12)}
.c03 .state.on{background:linear-gradient(180deg,rgba(163,240,206,.24),rgba(127,212,255,.08));color:#E7FFF7;border-color:rgba(255,255,255,.25)}
`,
  },
  {
    id: "04", heroSpec: "24 grid / 2.30 px stroke", slug: "bento", cn: "积木", en: "Bento",
    tagline: "切一个 45° 角，让工具咔哒归位。",
    desc: "图标由 2.3px 方头线与 2px 圆角矩形构成，右上角统一 45° 切角；激活态是一块实心薄荷积木。灵感来自 21st.dev 的 bento 网格组件。",
    accent: "#B8F5D8", accent2: "#FFD166", glow: "rgba(184,245,216,.10)", glow2: "rgba(255,209,102,.06)",
    palette: ["#B8F5D8", "#FFD166", "#E8E8EC", "#0E0F11"],
    specs: [["网格", "24 × 24"], ["描边", "2.30 px"], ["端点", "Square"], ["切角", "45°"], ["圆角", "2 px"], ["激活", "实心"]],
    keywords: ["切角", "方头", "模块", "实心激活"],
    icon: bentoIcon,
    tile: (n) => bentoIcon(n, 34),
    css: `
.c04 .tile{border-radius:3px;background:#0E0F11;border:1px solid rgba(255,255,255,.075);
  clip-path:polygon(0 0,calc(100% - 20px) 0,100% 20px,100% 100%,0 100%)}
.c04 .tile-ic{color:#D4FBE6}
.c04 .mark{border-radius:3px;clip-path:polygon(0 0,calc(100% - 8px) 0,100% 8px,100% 100%,0 100%);
  background:rgba(184,245,216,.10);border-color:rgba(184,245,216,.20)}
.c04 .dock{border-radius:10px;background:#0E0F11;border:1px solid rgba(255,255,255,.09)}
.c04 .dock-btn{border-radius:3px}
.c04 .dock-btn.active{background:var(--accent);color:#08110D;
  clip-path:polygon(0 0,calc(100% - 9px) 0,100% 9px,100% 100%,0 100%)}
.c04 .empty-orb{border-radius:4px;clip-path:polygon(0 0,calc(100% - 14px) 0,100% 14px,100% 100%,0 100%);
  background:rgba(184,245,216,.10);border:1px solid rgba(184,245,216,.22);box-shadow:0 0 60px rgba(184,245,216,.08)}
.c04 .app-top{border-radius:6px}
.c04 .detail-art{border-radius:4px;clip-path:polygon(0 0,calc(100% - 24px) 0,100% 24px,100% 100%,0 100%);background:#0E0F11}
.c04 .state.on{background:var(--accent);color:#08110D;border-color:transparent}
`,
  },
  {
    id: "05", heroSpec: "24 grid / 1.15 px stroke", slug: "viewfinder", cn: "取景器", en: "Viewfinder",
    tagline: "每枚图标，都是一个取景框。",
    desc: "每枚图标都是一个微型取景框：四角括号 + 2px 刻度 + 点阵底纹，激活时括号闭合。让截图工具第一次拥有“测量”的气质。",
    accent: "#C7F76E", accent2: "#6CFFC2", glow: "rgba(199,247,110,.10)", glow2: "rgba(108,255,194,.06)",
    palette: ["#C7F76E", "#6CFFC2", "#E8FFE0", "#0B0D0A"],
    specs: [["描边", "1.15 px"], ["括号", "8 px"], ["刻度", "2 px"], ["点阵", "6 px"], ["强调", "Lime"], ["圆角", "10 px"]],
    keywords: ["取景框", "刻度", "点阵", "技术"],
    icon: (n, s) => svgEl(BASE[n], { size: s, sw: 1.35 }),
    tile: (n, i) => `<span class="coord">X${String(i + 1).padStart(2, "0")}</span>${hudGlyph(n, 34)}`,
    css: `
.c05 .tile{border-radius:10px;border:1px solid rgba(199,247,110,.10);
  background-color:#0B0D0A;background-image:radial-gradient(rgba(199,247,110,.10) 1px,transparent 1px);background-size:7px 7px}
.c05 .tile-ic{color:#DBFFB0}
.c05 .tile-no{display:none}
.c05 .hud{position:relative;display:grid;place-items:center;width:100%;height:100%}
.c05 .hud i{position:absolute;width:9px;height:9px;border-color:rgba(199,247,110,.55);border-style:solid;border-width:0}
.c05 .hud i:nth-child(1){top:13px;left:13px;border-top-width:1px;border-left-width:1px}
.c05 .hud i:nth-child(2){top:13px;right:13px;border-top-width:1px;border-right-width:1px}
.c05 .hud i:nth-child(3){bottom:13px;left:13px;border-bottom-width:1px;border-left-width:1px}
.c05 .hud i:nth-child(4){bottom:13px;right:13px;border-bottom-width:1px;border-right-width:1px}
.c05 .coord{position:absolute;top:9px;left:12px;font-family:"JetBrains Mono",monospace;font-size:8px;letter-spacing:.08em;color:rgba(199,247,110,.45)}
.c05 .mark{border-radius:4px;background:rgba(199,247,110,.07);border-color:rgba(199,247,110,.25)}
.c05 .dock{background:rgba(11,13,10,.94);border:1px solid rgba(199,247,110,.16);border-radius:8px}
.c05 .dock-btn{border-radius:4px}
.c05 .dock-btn.active{background:rgba(199,247,110,.13);color:var(--accent);box-shadow:inset 0 0 0 1px rgba(199,247,110,.28)}
.c05 .sep{background:repeating-linear-gradient(180deg,rgba(199,247,110,.45) 0 2px,transparent 2px 4px)}
.c05 .empty-orb{border-radius:6px;background:rgba(199,247,110,.06);border:1px solid rgba(199,247,110,.2)}
.c05 .detail-art{background-image:radial-gradient(rgba(199,247,110,.10) 1px,transparent 1px);background-size:7px 7px;border-color:rgba(199,247,110,.12)}
.c05 .state.on{background:rgba(199,247,110,.13);color:var(--accent);border-color:rgba(199,247,110,.3)}
`,
  },
  {
    id: "06", heroSpec: "24 grid / 2.40 px stroke", slug: "clay", cn: "陶土", en: "Clay",
    tagline: "把边缘揉圆，像一块温热的陶。",
    desc: "所有端点 2.4px、圆头，圆角半径拉到 3.5，图形带一点手绘的不对称；点缀色换成暖杏，在深色里更有体温。",
    accent: "#FFD9B0", accent2: "#A3F0CE", glow: "rgba(255,217,176,.11)", glow2: "rgba(163,240,206,.05)",
    palette: ["#FFD9B0", "#A3F0CE", "#F6E7D8", "#14110F"],
    specs: [["描边", "2.40 px"], ["圆角", "3.5 px"], ["端点", "Round"], ["点缀", "暖杏"], ["触感", "柔和"], ["网格", "24 × 24"]],
    keywords: ["圆润", "暖色", "手作", "亲和"],
    icon: plain(2.4),
    tile: (n) => svgEl(BASE[n], { size: 34, sw: 2.4 }),
    css: `
.c06 .tile{border-radius:26px;background:linear-gradient(180deg,#171411,#100E0C);border:1px solid rgba(255,217,176,.10);
  box-shadow:inset 0 1px 0 rgba(255,217,176,.10),0 14px 30px rgba(0,0,0,.32)}
.c06 .tile-ic{color:#F8D9B8}
.c06 .mark{background:rgba(255,217,176,.10);border-color:rgba(255,217,176,.18);border-radius:10px}
.c06 .dock{border-radius:999px;background:#14110F;border:1px solid rgba(255,217,176,.11);
  box-shadow:inset 0 1px 0 rgba(255,217,176,.10),0 18px 36px rgba(0,0,0,.4)}
.c06 .dock-btn{border-radius:999px}
.c06 .dock-btn.active{background:var(--accent);color:#241605;box-shadow:0 6px 18px rgba(255,217,176,.18)}
.c06 .empty-orb{border-radius:999px;background:rgba(255,217,176,.09);border:1px solid rgba(255,217,176,.18);box-shadow:0 0 60px rgba(255,217,176,.08)}
.c06 .detail-art{border-radius:26px;background:linear-gradient(180deg,#171411,#100E0C);border-color:rgba(255,217,176,.10)}
.c06 .state.on{background:var(--accent);color:#241605;border-color:transparent}
`,
  },
  {
    id: "07", heroSpec: "24 grid / gradient stroke", slug: "extruded", cn: "浮雕", en: "Extruded",
    tagline: "给线条一道顶光和一粒投影。",
    desc: "描边使用银白→薄荷的线性渐变，叠 1px 深色投影与 8px 冷光，像从金属面板上铣出来；按钮带内高光，像真的能按下去。",
    accent: "#DFFFF2", accent2: "#A3F0CE", glow: "rgba(223,255,242,.10)", glow2: "rgba(163,240,206,.07)",
    palette: ["#DFFFF2", "#A3F0CE", "#8A949C", "#16181B"],
    specs: [["描边", "渐变"], ["投影", "1 px"], ["冷光", "8 px"], ["内高光", "1 px"], ["按钮", "凸起"], ["圆角", "18 px"]],
    keywords: ["立体", "顶光", "投影", "硬件感"],
    icon: (n, s) => svgEl(BASE[n], { size: s, stroke: "url(#extrudeGrad)", sw: 1.45, cls: "ic glow" }),
    tile: (n) => svgEl(BASE[n], { size: 34, stroke: "url(#extrudeGrad)", sw: 1.45, cls: "ic glow" }),
    css: `
.c07 .tile{border-radius:18px;background:linear-gradient(180deg,#141619,#0B0C0E);border:1px solid rgba(255,255,255,.07);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.14),0 12px 26px rgba(0,0,0,.42)}
.c07 .tile-ic{filter:drop-shadow(0 1px 0 rgba(0,0,0,.9)) drop-shadow(0 0 7px rgba(163,240,206,.22))}
.c07 .mark{background:linear-gradient(180deg,#1C2024,#0E1012);border-color:rgba(255,255,255,.14);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.18),0 4px 10px rgba(0,0,0,.4)}
.c07 .dock{background:linear-gradient(180deg,#17191D,#0E1012);border:1px solid rgba(255,255,255,.09);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.16),0 20px 42px rgba(0,0,0,.5)}
.c07 .dock-btn{border-radius:9px}
.c07 .dock-btn.active{background:linear-gradient(180deg,rgba(163,240,206,.22),rgba(163,240,206,.05));color:#D9FFEF;
  box-shadow:inset 0 1px 0 rgba(255,255,255,.24),0 0 18px rgba(163,240,206,.14)}
.c07 .empty-orb{background:linear-gradient(180deg,#171A1E,#0C0E10);border:1px solid rgba(255,255,255,.12);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.2),0 20px 50px rgba(0,0,0,.45),0 0 44px rgba(163,240,206,.08)}
.c07 .detail-art{background:linear-gradient(180deg,#141619,#0B0C0E);border-color:rgba(255,255,255,.08)}
.c07 .state.on{background:linear-gradient(180deg,rgba(163,240,206,.22),rgba(163,240,206,.05));color:#D9FFEF;border-color:rgba(255,255,255,.16)}
`,
  },
  {
    id: "08", heroSpec: "12 grid / 2 px cell", slug: "pixel", cn: "像素", en: "Pixel",
    tagline: "12×12 的方块，一点游戏机的浪漫。",
    desc: "12×12 方块对齐 2px 网格，不做抗锯齿；激活态加一条扫描线。像 21st.dev 的 pixel 组件，但配色与间距保持产品级克制。",
    accent: "#9BFFC8", accent2: "#C6FFE0", glow: "rgba(155,255,200,.10)", glow2: "rgba(155,255,200,.05)",
    palette: ["#9BFFC8", "#C6FFE0", "#5E7A6B", "#0A0C0B"],
    specs: [["网格", "12 × 12"], ["单元", "2 px"], ["抗锯齿", "Off"], ["扫描线", "2 px"], ["字体", "Mono"], ["圆角", "8 px"]],
    keywords: ["像素", "等宽", "扫描线", "复古未来"],
    icon: pixelIcon,
    tile: (n) => pixelIcon(n, 34),
    css: `
.c08 .tile{border-radius:8px;background-color:#0A0C0B;border:1px solid rgba(155,255,200,.12);
  background-image:repeating-linear-gradient(0deg,rgba(155,255,200,.035) 0 1px,transparent 1px 4px)}
.c08 .tile-ic{color:#A8FFD2}
.c08 .tile-label{font-family:"JetBrains Mono","Noto Sans CJK SC",monospace;letter-spacing:.02em}
.c08 .mark{border-radius:2px;background:rgba(155,255,200,.08);border-color:rgba(155,255,200,.22)}
.c08 .dock{background:#0A0C0B;border:1px solid rgba(155,255,200,.16);border-radius:9px}
.c08 .dock-btn{border-radius:3px;font-family:"JetBrains Mono","Noto Sans CJK SC",monospace}
.c08 .dock-btn.icon{width:34px}
.c08 .dock-btn.active{background:rgba(155,255,200,.14);color:#C6FFE0;box-shadow:inset 0 0 0 1px rgba(155,255,200,.35)}
.c08 .empty-orb{border-radius:4px;background:#0A0C0B;border:1px solid rgba(155,255,200,.24);
  box-shadow:inset 0 0 0 1px rgba(155,255,200,.06),0 0 54px rgba(155,255,200,.08);
  background-image:repeating-linear-gradient(0deg,rgba(155,255,200,.05) 0 2px,transparent 2px 5px)}
.c08 .detail-art{border-radius:8px;background:#0A0C0B;background-image:repeating-linear-gradient(0deg,rgba(155,255,200,.035) 0 1px,transparent 1px 4px);border-color:rgba(155,255,200,.12)}
.c08 .state.on{background:rgba(155,255,200,.14);color:#C6FFE0;border-color:rgba(155,255,200,.35)}
`,
  },
  {
    id: "09", heroSpec: "24 grid / 1.15 px + nodes", slug: "blueprint", cn: "蓝图", en: "Blueprint",
    tagline: "用节点和连线，画一张可执行的图纸。",
    desc: "每条线的端点都是一颗节点，内部铺 6px 点阵，配十字准星与刻度。把“精确截取”翻译成可以被看见的工程语言。",
    accent: "#7FE7FF", accent2: "#A3F0CE", glow: "rgba(127,231,255,.12)", glow2: "rgba(127,231,255,.05)",
    palette: ["#7FE7FF", "#A3F0CE", "#9BEEFF", "#081013"],
    specs: [["描边", "1.15 px"], ["节点", "2.5 px"], ["点阵", "6 px"], ["准星", "4 ×"], ["气质", "工程"], ["圆角", "14 px"]],
    keywords: ["节点", "蓝图", "准星", "精确"],
    icon: blueprintIcon,
    tile: (n) => blueprintIcon(n, 34),
    css: `
.c09 .tile{border-radius:14px;background-color:#081013;border:1px solid rgba(127,231,255,.12);
  background-image:radial-gradient(rgba(127,231,255,.10) 1px,transparent 1px);background-size:8px 8px}
.c09 .tile-ic{color:#9BEEFF}
.c09 .node{fill:#7FE7FF;stroke:#081013;stroke-width:.7}
.c09 .mark{border-radius:4px;background:rgba(127,231,255,.07);border-color:rgba(127,231,255,.25)}
.c09 .dock{background:rgba(7,15,18,.94);border:1px solid rgba(127,231,255,.18);
  background-image:radial-gradient(rgba(127,231,255,.07) 1px,transparent 1px);background-size:8px 8px}
.c09 .dock-btn{border-radius:5px}
.c09 .dock-btn.active{background:rgba(127,231,255,.13);color:#C9F7FF;box-shadow:inset 0 0 0 1px rgba(127,231,255,.3)}
.c09 .sep{background:repeating-linear-gradient(180deg,rgba(127,231,255,.4) 0 2px,transparent 2px 4px)}
.c09 .empty-orb{border-radius:8px;background:rgba(127,231,255,.05);border:1px solid rgba(127,231,255,.2);
  box-shadow:0 0 60px rgba(127,231,255,.09)}
.c09 .detail-art{background-color:#081013;background-image:radial-gradient(rgba(127,231,255,.10) 1px,transparent 1px);background-size:8px 8px;border-color:rgba(127,231,255,.12)}
.c09 .state.on{background:rgba(127,231,255,.13);color:#C9F7FF;border-color:rgba(127,231,255,.32)}
`,
  },
  {
    id: "10", heroSpec: "24 grid / metal gradient", slug: "chrome", cn: "液态金属", en: "Chrome",
    tagline: "让图标像液态金属一样反光。",
    desc: "描边用银白→薄荷→深灰的金属渐变，叠 1px 高光与 10px 冷光；激活态整枚变成镜面。灵感来自 21st.dev 的 shiny button 与金属卡片。",
    accent: "#DFFFF2", accent2: "#A3F0CE", glow: "rgba(223,255,242,.11)", glow2: "rgba(130,150,160,.08)",
    palette: ["#DFFFF2", "#A3F0CE", "#7E8A93", "#14171A"],
    specs: [["金属渐变", "4 段"], ["高光", "1 px"], ["冷光", "10 px"], ["激活", "镜面"], ["反射", "对角"], ["圆角", "20 px"]],
    keywords: ["金属", "镜面", "冷光", "奢华"],
    icon: chromeIcon,
    tile: (n) => chromeIcon(n, 34),
    css: `
.c10 .tile{border-radius:20px;border:1px solid rgba(255,255,255,.10);
  background:radial-gradient(120% 100% at 30% 0%,#1B1E22,#0A0B0D 62%);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.16),0 16px 34px rgba(0,0,0,.48)}
.c10 .tile-ic{filter:drop-shadow(0 1px 1px rgba(0,0,0,.85)) drop-shadow(0 0 7px rgba(163,240,206,.25))}
.c10 .mark{background:linear-gradient(180deg,#1D2125,#0C0E10);border-color:rgba(255,255,255,.18);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.25),0 4px 10px rgba(0,0,0,.45)}
.c10 .dock{background:linear-gradient(180deg,#1A1D21,#0D0F11 55%,#08090B);border:1px solid rgba(255,255,255,.12);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.2),0 22px 44px rgba(0,0,0,.55)}
.c10 .dock-btn.active{background:linear-gradient(180deg,rgba(223,255,242,.22),rgba(163,240,206,.05));color:#EAFFF7;
  box-shadow:inset 0 1px 0 rgba(255,255,255,.35),0 0 18px rgba(163,240,206,.2)}
.c10 .empty-orb{background:radial-gradient(100% 100% at 30% 0%,#20252A,#0A0B0D);
  border:1px solid rgba(255,255,255,.16);box-shadow:inset 0 1px 0 rgba(255,255,255,.25),0 0 60px rgba(163,240,206,.1)}
.c10 .detail-art{background:radial-gradient(120% 100% at 30% 0%,#1B1E22,#0A0B0D 62%);border-color:rgba(255,255,255,.10)}
.c10 .state.on{background:linear-gradient(180deg,rgba(223,255,242,.22),rgba(163,240,206,.05));color:#EAFFF7;border-color:rgba(255,255,255,.3)}
`,
  },
];

/* ------------------------------------------------------------------ */
/* Signature detail — one enlarged glyph + three craft notes         */
/* ------------------------------------------------------------------ */
const DETAILS = {
  "01": { icon: "region", tag: "Optical Alignment", notes: [
    "端点落点对齐 24 网格中心，不靠目测",
    "2.5px 圆角让每个转角保持同样的呼吸",
    "1.5px 等重，缩到 14px 仍不糊成一团"] },
  "02": { icon: "mosaic", tag: "Two Channels", notes: [
    "34% 灰蓝填充提供体积，不抢描边识别度",
    "开放路径改用 26% 灰蓝粗线垫底，避免填充变形",
    "同色双通道，让图标在暗色界面先被看见"] },
  "03": { icon: "region", tag: "Glass Edge", notes: [
    "1px 渐变边框勾出玻璃的厚度",
    "顶部高光把光源固定在左上角",
    "24px 背景模糊只作用于容器，不糊图形"] },
  "04": { icon: "pin", tag: "Cut Corner", notes: [
    "右上角 45° 切角是整套系统的统一签名",
    "方头端点与 2px 圆角让图标更“硬”",
    "激活态换成实心积木，状态一眼可辨"] },
  "05": { icon: "region", tag: "Measurement", notes: [
    "四角括号定义了每一枚图标的视窗",
    "2px 刻度提供“可测量”的心理暗示",
    "激活时括号收紧、颜色点亮，像完成对焦"] },
  "06": { icon: "screen", tag: "Soft Touch", notes: [
    "2.4px 圆头线，触感比 1.5px 更软",
    "3.5px 大圆角削掉机械感",
    "暖杏只做点缀，品牌薄荷仍留在系统里"] },
  "07": { icon: "save", tag: "Top Light", notes: [
    "渐变方向固定为左上到右下，模拟顶光",
    "1px 深色投影让线条真正离开面板",
    "8px 冷光只在深色背景上成立，浅色需重调"] },
  "08": { icon: "logo", tag: "Pixel Grid", notes: [
    "所有边缘对齐 2px 单元，不做半像素",
    "关闭抗锯齿，保留清晰的像素颗粒",
    "扫描线只出现在容器，永远不进图形"] },
  "09": { icon: "arrow", tag: "Node & Line", notes: [
    "每条线的端点都是一颗 2.5px 节点",
    "6px 点阵当作图纸底纹，暗示坐标",
    "激活态增加十字准星，强化“取景”语义"] },
  "10": { icon: "logo", tag: "Specular", notes: [
    "四段金属渐变制造出体积与反射",
    "高光放在 28% 处最接近真实镜面",
    "10px 冷光让金属在纯黑上不发脏"] },
};

/* ------------------------------------------------------------------ */
/* Page shell                                                         */
/* ------------------------------------------------------------------ */
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
svg{display:block;overflow:visible}
.board{position:relative;width:1600px;height:1180px;padding:42px 48px 34px;color:#EDEDF0;overflow:hidden;
  background:#08090B;--accent:#A3F0CE}
.board::before{content:"";position:absolute;inset:0;pointer-events:none;
  background:radial-gradient(720px 460px at 8% -6%,var(--glow),transparent 68%),
             radial-gradient(760px 520px at 104% 106%,var(--glow2),transparent 70%);}
.board::after{content:"";position:absolute;inset:0;pointer-events:none;opacity:.045;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='180' height='180'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='.8' numOctaves='3'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E")}
.board > *{position:relative;z-index:1}
.stack{position:relative;display:inline-grid;place-items:center}
.stack>svg{grid-area:1/1}

.head{display:flex;justify-content:space-between;align-items:flex-start;gap:56px;height:158px}
.eyebrow{display:flex;align-items:center;gap:9px;font-family:"JetBrains Mono",monospace;font-size:10px;
  letter-spacing:.16em;color:#7C7C86;text-transform:uppercase;margin-bottom:13px}
.eyebrow .dot{width:5px;height:5px;border-radius:50%;background:var(--accent);box-shadow:0 0 10px var(--accent)}
.eyebrow b{color:#C9C9D0;font-weight:500}
.titleline{display:flex;align-items:baseline;gap:16px;margin-bottom:10px}
.idx{font-family:"JetBrains Mono",monospace;font-size:13px;letter-spacing:.08em;color:var(--accent);
  border:1px solid color-mix(in srgb,var(--accent) 30%,transparent);border-radius:5px;padding:3px 7px;
  transform:translateY(-12px)}
h1{font-size:58px;line-height:1;font-weight:600;letter-spacing:-.035em;color:#F6F6F8}
.en{font-family:"Inter",sans-serif;font-size:23px;font-weight:400;letter-spacing:.02em;color:#77777F}
.tagline{font-size:17px;line-height:1.5;color:#B0B0B8;font-weight:400;letter-spacing:.005em}
.head-right{width:520px;padding-top:6px}
.desc{font-size:12.5px;line-height:1.85;color:#8C8C96;letter-spacing:.01em}
.palette{display:flex;gap:18px;margin-top:19px}
.sw{display:flex;align-items:center;gap:8px;font-family:"JetBrains Mono",monospace;font-size:9.5px;color:#8E8E98;letter-spacing:.04em}
.sw i{width:12px;height:12px;border-radius:3px;display:block;box-shadow:inset 0 0 0 1px rgba(255,255,255,.16)}

.main{display:grid;grid-template-columns:800px 1fr;gap:28px;height:638px;margin-top:6px}
.app-col{display:flex;flex-direction:column;gap:12px}
.mini-label{display:flex;align-items:center;justify-content:space-between;font-family:"JetBrains Mono",monospace;
  font-size:9px;letter-spacing:.16em;color:#6F6F78;text-transform:uppercase}
.mini-label span:last-child{color:#55555E}
.app-scale{width:800px;height:600px;position:relative}
.app-scale .app{transform:scale(1.31);transform-origin:top left}

/* App */
.app{position:relative;width:610px;height:446px;border-radius:16px;overflow:hidden;color:#EDEDF0;
  background:radial-gradient(120% 95% at 50% 0%,#111318 0%,#0A0B0E 56%,#08090B 100%);
  border:1px solid rgba(255,255,255,.085);
  box-shadow:0 44px 100px rgba(0,0,0,.62),inset 0 1px 0 rgba(255,255,255,.055)}
.app-top{position:absolute;left:14px;right:14px;top:12px;height:34px;display:flex;align-items:center;gap:8px;
  padding:0 9px 0 9px;border-radius:12px;background:rgba(20,21,25,.74);border:1px solid rgba(255,255,255,.065);
  backdrop-filter:blur(18px)}
.brand{display:flex;align-items:center;gap:8px}
.brand-name{font-size:11.5px;font-weight:500;letter-spacing:.01em;color:#E4E4EA}
.mark{width:24px;height:24px;border-radius:8px;display:grid;place-items:center;color:var(--accent);
  background:rgba(163,240,206,.08);border:1px solid rgba(163,240,206,.16)}
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
.empty-orb{width:54px;height:54px;border-radius:17px;display:grid;place-items:center;color:var(--accent);
  background:rgba(163,240,206,.055);border:1px solid rgba(163,240,206,.14);margin-bottom:5px}
.empty h3{font-size:16.5px;font-weight:500;letter-spacing:.01em;color:#EDEDF2}
.empty p{font-size:10.5px;color:#7E7E88;display:flex;align-items:center;gap:4px}
kbd{font-family:"Inter",sans-serif;font-size:9px;color:#9D9DA7;background:rgba(255,255,255,.045);
  border:1px solid rgba(255,255,255,.085);border-radius:4px;padding:1.5px 5px;line-height:1.2}
kbd + kbd{margin-left:1px}

.dock{position:absolute;left:50%;bottom:12px;transform:translateX(-50%);height:46px;display:flex;align-items:center;
  gap:2px;padding:0 7px;border-radius:14px;background:rgba(18,19,23,.9);border:1px solid rgba(255,255,255,.07);
  backdrop-filter:blur(20px);box-shadow:0 18px 40px rgba(0,0,0,.5);white-space:nowrap}
.dock-btn{height:32px;min-width:32px;padding:0 7px;border:0;border-radius:9px;background:transparent;color:#A7A7B0;
  display:flex;align-items:center;justify-content:center;gap:5px;font-family:inherit;font-size:10.5px}
.dock-btn.labeled{padding:0 9px 0 8px}
.dock-btn.icon{width:32px;padding:0}
.dock-btn.active{color:var(--accent);background:rgba(163,240,206,.09)}
.sep{width:1px;height:16px;background:rgba(255,255,255,.08);margin:0 5px;flex:none}

/* Side column */
.side{display:flex;flex-direction:column;gap:12px;min-width:0}
.hero{position:relative;display:flex;align-items:center;gap:22px;height:158px;padding:0 24px;border-radius:20px;
  background:linear-gradient(120deg,rgba(255,255,255,.035),rgba(255,255,255,.008));
  border:1px solid rgba(255,255,255,.06);overflow:hidden}
.hero::after{content:"";position:absolute;right:-50px;top:-90px;width:250px;height:250px;border-radius:50%;
  background:radial-gradient(circle,var(--glow),transparent 68%)}
.hero-tile{width:104px;height:104px;border-radius:28px;display:grid;place-items:center;flex:none;
  color:var(--accent);background:linear-gradient(150deg,rgba(255,255,255,.07),rgba(255,255,255,.015));
  border:1px solid rgba(255,255,255,.10);box-shadow:inset 0 1px 0 rgba(255,255,255,.14),0 18px 36px rgba(0,0,0,.35)}
.hero-meta{display:flex;flex-direction:column;gap:6px;min-width:0}
.hero-meta .kicker{font-family:"JetBrains Mono",monospace;font-size:9px;letter-spacing:.16em;color:#6E6E77}
.hero-meta b{font-size:19px;font-weight:500;letter-spacing:-.01em;color:#F1F1F5}
.hero-meta em{font-style:normal;font-family:"JetBrains Mono",monospace;font-size:10px;color:#7E7E88;letter-spacing:.02em}
.hero-no{position:absolute;right:20px;bottom:2px;font-size:80px;line-height:1;font-weight:600;color:rgba(255,255,255,.035);letter-spacing:-.05em}

.specs{display:grid;grid-template-columns:1fr 1fr;gap:0 24px;padding:6px 4px}
.spec{display:flex;align-items:center;justify-content:space-between;gap:12px;height:33px;
  border-bottom:1px solid rgba(255,255,255,.045)}
.spec span{font-size:10.5px;color:#7E7E88;letter-spacing:.02em}
.spec b{font-family:"JetBrains Mono",monospace;font-size:10.5px;font-weight:400;color:#CFCFD6;letter-spacing:.02em}

.states{display:flex;gap:9px;align-items:stretch;height:64px}
.state{flex:1;display:flex;align-items:center;justify-content:center;gap:8px;border-radius:12px;
  background:rgba(255,255,255,.022);border:1px solid rgba(255,255,255,.055);color:#9B9BA5;
  font-size:10px;letter-spacing:.04em}
.state.hover{background:rgba(255,255,255,.05);color:#D6D6DD}
.state.on{color:var(--accent);background:rgba(163,240,206,.10);border-color:rgba(163,240,206,.22)}
.state em{font-style:normal;font-family:"JetBrains Mono",monospace;font-size:8.5px;color:#66666F;letter-spacing:.08em}

.keywords{display:flex;gap:8px;flex-wrap:wrap}
.kw{font-size:10px;letter-spacing:.06em;color:#8A8A94;padding:5px 10px;border-radius:999px;
  border:1px solid rgba(255,255,255,.07);background:rgba(255,255,255,.018)}
.kw:first-child{color:var(--accent);border-color:color-mix(in srgb,var(--accent) 24%,transparent);
  background:color-mix(in srgb,var(--accent) 7%,transparent)}

/* Icon sheet */
.sheet{margin-top:20px;display:flex;flex-direction:column;gap:11px}
.sheet-head{display:flex;align-items:center;justify-content:space-between}
.sheet-head .t{display:flex;align-items:baseline;gap:10px}
.sheet-head b{font-size:13px;font-weight:500;color:#E2E2E8;letter-spacing:.02em}
.sheet-head span{font-family:"JetBrains Mono",monospace;font-size:9px;letter-spacing:.14em;color:#6D6D76;text-transform:uppercase}
.grid16{display:grid;grid-template-columns:repeat(8,1fr);gap:10px}
.tile{position:relative;height:108px;display:flex;flex-direction:column;align-items:center;justify-content:center;
  gap:9px;border-radius:18px;background:rgba(255,255,255,.018);border:1px solid rgba(255,255,255,.055);
  transition:background .2s,border-color .2s}
.tile-no{position:absolute;top:8px;left:10px;font-family:"JetBrains Mono",monospace;font-size:8px;
  letter-spacing:.1em;color:rgba(255,255,255,.16)}
.tile-ic{display:grid;place-items:center;height:38px;color:#C9C9D2}
.tile-label{font-size:10px;color:#8B8B95;letter-spacing:.05em}
.tile:hover .tile-ic{color:var(--accent)}

.foot{display:flex;align-items:center;justify-content:space-between;margin-top:12px;
  font-family:"JetBrains Mono",monospace;font-size:8.5px;letter-spacing:.16em;color:#4E4E57;text-transform:uppercase}
.foot .mid{color:#5C5C66}
.foot span{white-space:nowrap}
.foot{gap:24px}

.detail{display:flex;gap:20px;height:220px;padding:18px;border-radius:18px;
  background:linear-gradient(120deg,rgba(255,255,255,.032),rgba(255,255,255,.008));
  border:1px solid rgba(255,255,255,.055);overflow:hidden}
.detail-art{position:relative;width:150px;height:150px;flex:none;border-radius:18px;display:grid;place-items:center;
  background:rgba(255,255,255,.022);border:1px solid rgba(255,255,255,.06)}
.detail-ic{color:var(--accent);display:grid;place-items:center;filter:drop-shadow(0 0 18px var(--glow))}
.detail-tag{position:absolute;bottom:8px;left:0;right:0;text-align:center;font-family:"JetBrains Mono",monospace;
  font-size:7px;letter-spacing:.13em;color:#5E5E68;text-transform:uppercase}
.detail-copy{display:flex;flex-direction:column;justify-content:center;gap:10px;min-width:0}
.detail-copy .kicker{font-family:"JetBrains Mono",monospace;font-size:8.5px;letter-spacing:.16em;color:#6E6E77;
  text-transform:uppercase;margin-bottom:1px}
.note{display:flex;gap:9px;align-items:flex-start;font-size:10.5px;line-height:1.5;color:#9A9AA4}
.note em{font-style:normal;font-family:"JetBrains Mono",monospace;font-size:8px;color:var(--accent);
  opacity:.65;padding-top:2px;flex:none}
`;

function page(c) {
  const tiles = ICON_ORDER.map((n, i) => `
      <div class="tile">
        <span class="tile-no">${String(i + 1).padStart(2, "0")}</span>
        <div class="tile-ic">${c.tile(n, i)}</div>
        <span class="tile-label">${ICON_LABELS[n]}</span>
      </div>`).join("");

  const palette = c.palette.map(hex => `<span class="sw"><i style="background:${hex}"></i>${hex.toUpperCase()}</span>`).join("");
  const specs = c.specs.map(([k, v]) => `<div class="spec"><span>${k}</span><b>${v}</b></div>`).join("");
  const keywords = c.keywords.map(k => `<span class="kw">${k}</span>`).join("");
  const stateIcon = c.icon("rect", 16);

  return `<!doctype html>
<html lang="zh-CN">
<head><meta charset="utf-8"><title>${c.id} · ${c.cn} ${c.en}</title><style>${CSS}${c.css}</style></head>
<body>
<div class="board ${"c" + c.id}" style="--accent:${c.accent};--accent2:${c.accent2};--glow:${c.glow};--glow2:${c.glow2}">
  <svg width="0" height="0" style="position:absolute" aria-hidden="true"><defs>
    <linearGradient id="extrudeGrad" gradientUnits="userSpaceOnUse" x1="1" y1="1" x2="23" y2="23">
      <stop offset="0" stop-color="#FFFFFF"/><stop offset=".42" stop-color="#DFFFF2"/>
      <stop offset=".72" stop-color="#9FE8C8"/><stop offset="1" stop-color="#6E8A80"/>
    </linearGradient>
    <linearGradient id="chromeGrad" gradientUnits="userSpaceOnUse" x1="1" y1="1" x2="23" y2="23">
      <stop offset="0" stop-color="#FFFFFF"/><stop offset=".28" stop-color="#E9FFF7"/>
      <stop offset=".52" stop-color="#A3F0CE"/><stop offset=".74" stop-color="#DDFFF2"/>
      <stop offset="1" stop-color="#67737A"/>
    </linearGradient>
  </defs></svg>

  <header class="head">
    <div>
      <div class="eyebrow"><span class="dot"></span>Lumenshot · Icon System <b>/ 2026</b></div>
      <div class="titleline"><span class="idx">${c.id}</span><h1>${c.cn}</h1><span class="en">${c.en}</span></div>
      <p class="tagline">${c.tagline}</p>
    </div>
    <div class="head-right">
      <p class="desc">${c.desc}</p>
      <div class="palette">${palette}</div>
    </div>
  </header>

  <main class="main">
    <section class="app-col">
      <div class="mini-label"><span>In Context / 真实界面</span><span>610 × 446</span></div>
      <div class="app-scale">
        <div class="app">
          <div class="app-top">
            <div class="brand"><span class="mark">${c.icon("logo", 15)}</span><span class="brand-name">Lumenshot</span></div>
            <span class="status"><i></i>就绪</span>
            <span class="grow"></span>
            <span class="app-ghost">${c.icon("menu", 14)}</span>
            <span class="vsep"></span>
            <div class="wctl"><span class="wc-min"></span><span class="wc-max"></span><span class="wc-close"></span></div>
          </div>
          <div class="stage">
            <div class="empty">
              <div class="empty-orb">${c.icon("region", 27)}</div>
              <h3>还没有截图</h3>
              <p>选择一种方式开始，或按 <kbd>Ctrl</kbd><kbd>Alt</kbd><kbd>A</kbd></p>
            </div>
          </div>
          <div class="dock">
            <button class="dock-btn labeled active">${c.icon("region", 15)}<span>区域</span></button>
            <button class="dock-btn labeled">${c.icon("window", 15)}<span>窗口</span></button>
            <button class="dock-btn labeled">${c.icon("screen", 15)}<span>整屏</span></button>
            <span class="sep"></span>
            <button class="dock-btn icon">${c.icon("rect", 17)}</button>
            <button class="dock-btn icon">${c.icon("circle", 17)}</button>
            <button class="dock-btn icon">${c.icon("arrow", 17)}</button>
            <button class="dock-btn icon">${c.icon("type", 17)}</button>
            <button class="dock-btn icon">${c.icon("mosaic", 17)}</button>
            <button class="dock-btn icon">${c.icon("grid", 17)}</button>
            <span class="sep"></span>
            <button class="dock-btn icon">${c.icon("undo", 17)}</button>
            <button class="dock-btn icon">${c.icon("redo", 17)}</button>
          </div>
        </div>
      </div>
    </section>

    <aside class="side">
      <div class="hero">
        <div class="hero-tile">${c.icon("logo", 66)}</div>
        <div class="hero-meta">
          <span class="kicker">Primary Mark</span>
          <b>光圈 · Aperture</b>
          <em>${c.heroSpec}</em>
        </div>
        <span class="hero-no">${c.id}</span>
      </div>
      <div class="specs">${specs}</div>
      <div class="states">
        <span class="state"><em>01</em>${stateIcon}默认</span>
        <span class="state hover"><em>02</em>${stateIcon}悬停</span>
        <span class="state on"><em>03</em>${stateIcon}激活</span>
      </div>
      <div class="keywords">${keywords}</div>
      <div class="detail">
        <div class="detail-art">
          <span class="detail-ic">${c.icon(DETAILS[c.id].icon, 88)}</span>
          <span class="detail-tag">${DETAILS[c.id].tag}</span>
        </div>
        <div class="detail-copy">
          <span class="kicker">Signature / 细节</span>
          ${DETAILS[c.id].notes.map((n, i) => `<div class="note"><em>0${i + 1}</em><span>${n}</span></div>`).join("")}
        </div>
      </div>
    </aside>
  </main>

  <section class="sheet">
    <div class="sheet-head">
      <div class="t"><b>完整图标组</b><span>16 icons / 24 × 24 grid</span></div>
      <span>02 尺寸：34 px 展示 · 15–17 px 实用</span>
    </div>
    <div class="grid16">${tiles}</div>
  </section>

  <footer class="foot">
    <span>Lumenshot · Icon System / 10 Directions</span>
    <span class="mid">Designed for dark UI · Mint ${c.accent.toUpperCase()}</span>
    <span>${c.id} / 10 · ${c.en}</span>
  </footer>
</div>
</body></html>`;
}

for (const c of concepts) {
  writeFileSync(join(outDir, `${c.id}-${c.slug}.html`), page(c), "utf8");
}
console.log(`wrote ${concepts.length} boards to ${outDir}`);

/* ------------------------------------------------------------------ */
/* Overview sheet — all ten directions at a glance                    */
/* ------------------------------------------------------------------ */
const OVERVIEW_CSS = `
@font-face{font-family:"Inter";src:url("../fonts/Inter-400.ttf") format("truetype");font-weight:400}
@font-face{font-family:"Inter";src:url("../fonts/Inter-500.ttf") format("truetype");font-weight:500}
@font-face{font-family:"Inter";src:url("../fonts/Inter-600.ttf") format("truetype");font-weight:600}
@font-face{font-family:"JetBrains Mono";src:url("../fonts/JetBrainsMono-400.ttf") format("truetype");font-weight:400}
*{box-sizing:border-box;margin:0;padding:0}
html,body{width:1600px;height:980px;overflow:hidden;background:#050608}
body{font-family:"Inter","Noto Sans CJK SC",system-ui,sans-serif;-webkit-font-smoothing:antialiased}
.overview{position:relative;width:1600px;height:980px;padding:38px 40px 26px;background:#08090B;color:#EDEDF0;overflow:hidden}
.overview::before{content:"";position:absolute;inset:0;background:
  radial-gradient(700px 400px at 6% -4%,rgba(163,240,206,.09),transparent 68%),
  radial-gradient(700px 400px at 98% 104%,rgba(127,212,255,.07),transparent 70%);pointer-events:none}
.overview::after{content:"";position:absolute;inset:0;opacity:.04;pointer-events:none;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='180' height='180'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='.8' numOctaves='3'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E")}
.overview>*{position:relative;z-index:1}
.stack{position:relative;display:inline-grid;place-items:center}
.stack>svg{grid-area:1/1}
.node{fill:currentColor;stroke:none}
.ohead{display:flex;align-items:flex-end;justify-content:space-between;margin-bottom:24px}
.ohead .left{display:flex;flex-direction:column;gap:10px}
.oeyebrow{display:flex;align-items:center;gap:9px;font-family:"JetBrains Mono",monospace;font-size:10px;
  letter-spacing:.16em;color:#7C7C86;text-transform:uppercase}
.oeyebrow i{width:5px;height:5px;border-radius:50%;background:#A3F0CE;box-shadow:0 0 10px #A3F0CE}
.otitle{display:flex;align-items:baseline;gap:16px}
.otitle h1{font-size:44px;line-height:1;font-weight:600;letter-spacing:-.035em}
.otitle span{font-size:18px;color:#77777F;letter-spacing:.02em}
.osub{font-size:12px;color:#8C8C96;letter-spacing:.01em;max-width:540px;line-height:1.7}
.ometa{font-family:"JetBrains Mono",monospace;font-size:9px;letter-spacing:.14em;color:#5E5E68;text-align:right;line-height:2}
.ogrid{display:grid;grid-template-columns:repeat(5,1fr);gap:14px}
.ocard{position:relative;height:356px;padding:17px 17px 15px;border-radius:18px;overflow:hidden;
  background:linear-gradient(165deg,rgba(255,255,255,.035),rgba(255,255,255,.008));
  border:1px solid rgba(255,255,255,.06);display:flex;flex-direction:column}
.ocard::before{content:"";position:absolute;right:-40px;top:-70px;width:180px;height:180px;border-radius:50%;
  background:radial-gradient(circle,var(--og),transparent 68%)}
.ocard .otop{display:flex;align-items:center;justify-content:space-between}
.onum{font-family:"JetBrains Mono",monospace;font-size:9px;letter-spacing:.1em;color:var(--oa);
  border:1px solid color-mix(in srgb,var(--oa) 28%,transparent);border-radius:4px;padding:2px 5px}
.odots{display:flex;gap:4px}
.odots i{width:7px;height:7px;border-radius:2px;display:block;box-shadow:inset 0 0 0 1px rgba(255,255,255,.15)}
.ohero{height:104px;display:grid;place-items:center;color:var(--oa);margin:2px 0 2px}
.ohero .ic{filter:drop-shadow(0 0 14px var(--og))}
.oname{font-size:17px;font-weight:500;letter-spacing:-.01em;color:#F2F2F6;display:flex;align-items:baseline;gap:7px}
.oname em{font-style:normal;font-size:11px;color:#77777F;font-weight:400;letter-spacing:.02em}
.otag{font-size:10px;line-height:1.65;color:#8B8B95;margin-top:5px;min-height:32px}
.ostrip{display:flex;align-items:center;gap:10px;margin-top:auto;padding-top:12px;border-top:1px solid rgba(255,255,255,.055);color:#B9B9C2}
.ostrip .ic{opacity:.9}
.ofoot{display:flex;justify-content:space-between;margin-top:20px;font-family:"JetBrains Mono",monospace;
  font-size:8.5px;letter-spacing:.16em;color:#4E4E57;text-transform:uppercase}
`;

function overviewPage() {
  const cards = concepts.map(c => `
    <article class="ocard c${c.id}" style="--oa:${c.accent};--og:${c.glow}">
      <div class="otop">
        <span class="onum">${c.id}</span>
        <span class="odots">${c.palette.slice(0, 3).map(h => `<i style="background:${h}"></i>`).join("")}</span>
      </div>
      <div class="ohero">${c.icon("logo", 54)}</div>
      <div class="oname">${c.cn}<em>${c.en}</em></div>
      <p class="otag">${c.tagline}</p>
      <div class="ostrip">${["region", "window", "rect", "arrow", "type", "save"].map(n => c.icon(n, 17)).join("")}</div>
    </article>`).join("");

  return `<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8"><title>Lumenshot · 10 Icon Directions</title>
<style>${OVERVIEW_CSS}</style></head>
<body><div class="overview">
  <svg width="0" height="0" style="position:absolute" aria-hidden="true"><defs>
    <linearGradient id="extrudeGrad" gradientUnits="userSpaceOnUse" x1="1" y1="1" x2="23" y2="23">
      <stop offset="0" stop-color="#FFFFFF"/><stop offset=".42" stop-color="#DFFFF2"/>
      <stop offset=".72" stop-color="#9FE8C8"/><stop offset="1" stop-color="#6E8A80"/>
    </linearGradient>
    <linearGradient id="chromeGrad" gradientUnits="userSpaceOnUse" x1="1" y1="1" x2="23" y2="23">
      <stop offset="0" stop-color="#FFFFFF"/><stop offset=".28" stop-color="#E9FFF7"/>
      <stop offset=".52" stop-color="#A3F0CE"/><stop offset=".74" stop-color="#DDFFF2"/>
      <stop offset="1" stop-color="#67737A"/>
    </linearGradient>
  </defs></svg>
  <header class="ohead">
    <div class="left">
      <div class="oeyebrow"><i></i>Lumenshot · Icon System / 2026</div>
      <div class="otitle"><h1>十套图标语言</h1><span>Ten Icon Directions</span></div>
      <p class="osub">同一套功能、同一个 24px 网格，用十种材质与构造重新回答“截图工具应该长什么样”。每套都包含 16 枚图标与真实界面上下文。</p>
    </div>
    <div class="ometa">DARK UI · MINT ANCHOR<br>1600 × 1180 / EACH BOARD<br>10 / 10 DIRECTIONS</div>
  </header>
  <div class="ogrid">${cards}</div>
  <footer class="ofoot">
    <span>Lumenshot · Icon System / 10 Directions</span>
    <span>01 Filament · 02 Duotone · 03 Prism · 04 Bento · 05 Viewfinder · 06 Clay · 07 Extruded · 08 Pixel · 09 Blueprint · 10 Chrome</span>
    <span>2026</span>
  </footer>
</div></body></html>`;
}

writeFileSync(join(outDir, "overview.html"), overviewPage(), "utf8");
console.log("wrote overview");
