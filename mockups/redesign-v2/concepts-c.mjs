/* 08–10: pixel, inline, compass. */

/* 08 · Pixel -------------------------------------------------------- */
const U = 2;
const px = (x, y, w = 1, h = 1) => `<rect x="${x * U}" y="${y * U}" width="${w * U}" height="${h * U}"/>`;
const pix = (s) => `<g shape-rendering="crispEdges">${s}</g>`;

const c08 = {
  id: "08", slug: "pixel", cn: "像素", en: "Pixel",
  tagline: "把 24px 拆成 12×12 个方格，只允许整格落子。",
  desc: "所有图标都画在同一张 12×12 的像素网格上，每个格子 2px，边缘一律硬切。没有曲线、没有半像素，放大看是马赛克，缩小看反而更清楚。",
  grammar: "12 × 12 GRID / 2PX CELL / CRISP EDGE",
  rules: [
    "所有边缘必须落在 2px 网格线上，不允许半格",
    "不使用曲线与圆角，转角一律硬切",
    "单个连通块最小 1 格，避免缩小后消失",
  ],
  specs: [["网格", "12 × 12"], ["格宽", "2.0 px"], ["曲线", "0"], ["圆角", "0"], ["对齐", "整格"], ["最小尺寸", "14 px"]],
  keywords: ["像素", "网格", "硬边", "8-bit"],
  signature: "logo", signatureCn: "主标识 · Aperture", signatureSub: "pixel ring + centre block", signatureEn: "Pixel Ring",
  svg: { fill: "currentColor", stroke: "none", sw: 0 },
  icons: {
    logo: pix(`${px(4, 2, 4, 2)}${px(2, 4, 2, 4)}${px(8, 4, 2, 4)}${px(4, 8, 4, 2)}${px(3, 3, 2, 2)}${px(7, 3, 2, 2)}${px(3, 7, 2, 2)}${px(7, 7, 2, 2)}${px(5, 5, 2, 2)}`),
    region: pix(`${px(2, 2, 3, 2)}${px(2, 4, 2, 2)}${px(7, 2, 3, 2)}${px(8, 4, 2, 2)}${px(2, 7, 2, 2)}${px(2, 8, 3, 2)}${px(8, 7, 2, 2)}${px(7, 8, 3, 2)}${px(5, 5, 2, 2)}`),
    window: pix(`${px(2, 2, 8, 2)}${px(2, 5, 3, 5)}${px(6, 5, 4, 2)}${px(6, 8, 4, 2)}`),
    screen: pix(`${px(2, 2, 8, 2)}${px(2, 6, 8, 2)}${px(2, 4, 2, 2)}${px(8, 4, 2, 2)}${px(5, 4, 2, 2)}${px(5, 8, 2, 2)}${px(3, 10, 6, 1)}`),
    rect: pix(`${px(2, 2, 2, 2)}${px(8, 2, 2, 2)}${px(2, 8, 2, 2)}${px(8, 8, 2, 2)}${px(5, 2, 2, 2)}${px(2, 5, 2, 2)}${px(8, 5, 2, 2)}${px(5, 8, 2, 2)}`),
    circle: pix(`${px(4, 2, 4, 1)}${px(3, 3, 1, 1)}${px(8, 3, 1, 1)}${px(2, 4, 1, 4)}${px(9, 4, 1, 4)}${px(3, 8, 1, 1)}${px(8, 8, 1, 1)}${px(4, 9, 4, 1)}${px(5, 5, 2, 2)}`),
    arrow: pix(`${px(7, 1, 5, 1)}${px(8, 2, 4, 1)}${px(9, 3, 3, 1)}${px(10, 4, 2, 1)}${px(2, 9, 2, 2)}${px(3, 8, 2, 2)}${px(4, 7, 2, 2)}${px(5, 6, 2, 2)}${px(6, 5, 2, 2)}`),
    type: pix(`${px(2, 2, 8, 2)}${px(5, 4, 2, 6)}${px(4, 10, 4, 1)}`),
    mosaic: pix(`${px(2, 2, 5, 3)}${px(8, 2, 2, 5)}${px(2, 6, 3, 4)}${px(6, 6, 4, 2)}${px(6, 9, 4, 1)}`),
    grid: pix(`${px(2, 2, 1, 9)}${px(6, 2, 1, 9)}${px(10, 2, 1, 9)}${px(2, 2, 9, 1)}${px(2, 6, 9, 1)}${px(2, 10, 9, 1)}`),
    undo: pix(`${px(2, 5, 2, 2)}${px(3, 4, 1, 1)}${px(3, 7, 1, 1)}${px(4, 4, 5, 2)}${px(8, 6, 2, 2)}${px(5, 8, 4, 2)}`),
    redo: pix(`${px(8, 5, 2, 2)}${px(8, 4, 1, 1)}${px(8, 7, 1, 1)}${px(3, 4, 5, 2)}${px(2, 6, 2, 2)}${px(3, 8, 4, 2)}`),
    pin: pix(`${px(3, 2, 6, 1)}${px(2, 3, 1, 3)}${px(9, 3, 1, 3)}${px(3, 6, 6, 1)}${px(5, 4, 2, 2)}${px(4, 7, 4, 2)}${px(5, 9, 2, 2)}${px(5, 11, 2, 1)}`),
    copy: pix(`${px(2, 2, 5, 5)}${px(6, 6, 2, 2)}${px(9, 6, 2, 2)}${px(6, 9, 2, 2)}${px(9, 9, 2, 2)}`),
    save: pix(`${px(5, 2, 2, 5)}${px(3, 6, 6, 1)}${px(4, 7, 4, 1)}${px(5, 8, 2, 1)}${px(2, 10, 8, 2)}${px(2, 9, 1, 1)}${px(9, 9, 1, 1)}`),
    menu: pix(`${px(2, 3, 8, 2)}${px(2, 6, 8, 2)}${px(2, 9, 5, 2)}${px(9, 9, 1, 2)}`),
  },
};

/* 09 · Inline ------------------------------------------------------- */
const rr = (x, y, w, h, r = 2.5) => `<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="${r}"/>`;
const c09 = {
  id: "09", slug: "inline", cn: "双线", en: "Inline",
  tagline: "每根线都画两遍，中间留 1.6px 的呼吸。",
  desc: "所有图标都由内外两条平行轮廓组成，像刻字后的双线描边。外线负责占地，内线负责透气；即使缩到 15px，双线并成一条，仍比单线更有分量。",
  grammar: "DOUBLE CONTOUR / 1.6PX GAP / PARALLEL PAIR",
  rules: [
    "每枚图标至少有一组内外双轮廓",
    "双线间距固定 1.6px，沿所有方向等距",
    "内外线同宽 1.3px，不分主次",
  ],
  specs: [["网格", "24 × 24"], ["描边", "1.30 px"], ["双线距", "1.6 px"], ["轮廓", "2 层"], ["端点", "Round"], ["最小尺寸", "16 px"]],
  keywords: ["双线", "刻字", "内轮廓", "平行"],
  signature: "logo", signatureCn: "主标识 · Aperture", signatureSub: "double squircle + centre dot", signatureEn: "Double Frame",
  svg: { sw: 1.3, caps: "round", join: "round" },
  icons: {
    logo: `${rr(3, 3, 18, 18, 5.5)}${rr(5.6, 5.6, 12.8, 12.8, 3.4)}<circle cx="12" cy="12" r="2.2" fill="currentColor" stroke="none"/>`,
    region: `<path d="M3.5 8.3V3.5H8.3M15.7 3.5h4.8v4.8M20.5 15.7v4.8h-4.8M8.3 20.5H3.5v-4.8"/><path d="M6.2 10V6.2H10M14 6.2h3.8V10M17.8 14v3.8H14M10 17.8H6.2V14"/><circle cx="12" cy="12" r="1.9" fill="currentColor" stroke="none"/>`,
    window: `${rr(3, 4.5, 18, 15, 2.5)}${rr(5.2, 9.4, 13.6, 8.1, 1.4)}<path d="M3 9h18"/><circle cx="6.4" cy="6.8" r=".8" fill="currentColor" stroke="none"/><circle cx="8.8" cy="6.8" r=".8" fill="currentColor" stroke="none"/>`,
    screen: `${rr(2.5, 4, 19, 12.5, 2.5)}${rr(4.7, 6.2, 14.6, 8.1, 1.4)}<path d="M8.5 16.5v3.5M15.5 16.5v3.5M6.5 20h11M8.5 18.3h7"/>`,
    rect: `${rr(3.5, 6, 17, 12, 2.5)}${rr(6, 8.5, 12, 7, 1.2)}`,
    circle: `<circle cx="12" cy="12" r="8.5"/><circle cx="12" cy="12" r="5.8"/><circle cx="12" cy="12" r="1.8" fill="currentColor" stroke="none"/>`,
    arrow: `<path d="M4 20 20 4M11 4h9v9"/><path d="M6.6 19.4 19.4 6.6M13 6.6h6.4v6.4"/>`,
    type: `<path d="M4.5 6h15M12 6v13"/><path d="M7 8.4h10M12 8.4v8.4"/>`,
    mosaic: `${rr(3, 3, 8, 8, 1.5)}${rr(4.8, 4.8, 4.4, 4.4, .8)}${rr(13, 3, 8, 8, 1.5)}${rr(14.8, 4.8, 4.4, 4.4, .8)}${rr(3, 13, 8, 8, 1.5)}${rr(4.8, 14.8, 4.4, 4.4, .8)}${rr(13, 13, 8, 8, 1.5)}${rr(14.8, 14.8, 4.4, 4.4, .8)}`,
    grid: `${rr(3.5, 3.5, 17, 17, 2.5)}${rr(6, 6, 12, 12, 1.2)}<path d="M10.4 6v12M13.6 6v12M6 10.4h12M6 13.6h12"/>`,
    undo: `<path d="M9 14 4 9l5-5"/><path d="M4.5 9h10a5 5 0 0 1 5 5v2"/><path d="M9.4 11.6 7 9l2.4-2.6"/><path d="M6.6 11h8.4a2.8 2.8 0 0 1 2.8 2.8v2.2"/>`,
    redo: `<path d="M15 14l5-5-5-5"/><path d="M19.5 9h-10a5 5 0 0 0-5 5v2"/><path d="M14.6 11.6 17 9l-2.4-2.6"/><path d="M17.4 11H9a2.8 2.8 0 0 0-2.8 2.8v2.2"/>`,
    pin: `<path d="M12 21c0 0 6-6.4 6-11a6 6 0 1 0-12 0c0 4.6 6 11 6 11z"/><path d="M12 17.4c0 0 3.6-3.7 3.6-7.4a3.6 3.6 0 1 0-7.2 0c0 3.7 3.6 7.4 3.6 7.4z"/><circle cx="12" cy="10" r="1.2" fill="currentColor" stroke="none"/>`,
    copy: `${rr(3, 3, 10, 10, 2)}${rr(4.8, 4.8, 6.4, 6.4, 1.2)}${rr(11, 11, 10, 10, 2)}${rr(12.8, 12.8, 6.4, 6.4, 1.2)}`,
    save: `<path d="M12 3.5 18 10.5h-4v5h-4v-5H6z"/><path d="M12 6.9 14.8 10h-1.6v3.5h-2.4V10H9.2z"/><path d="M4 18h16M4 20.2h16M4 18v2.2M20 18v2.2"/>`,
    menu: `<path d="M4 6h16M4 8.2h16M4 12h16M4 14.2h16M4 18h10M4 20.2h10"/>`,
  },
};

/* 10 · Compass ------------------------------------------------------ */
const c10 = {
  id: "10", slug: "compass", cn: "圆规", en: "Compass",
  tagline: "只允许圆、弧和点，直线在这里不存在。",
  desc: "整套图标只用圆规能画出的元素：整圆、圆弧、点和椭圆。于是所有转角都变成弧线，所有端点都变成圆点，视觉上像一组天体轨道或机械表盘。",
  grammar: "CIRCLES / ARCS / DOTS ONLY / NO LINE",
  rules: [
    "禁止任何直线段与尖角，只使用圆、弧、椭圆、点",
    "所有开放弧至少留 45° 缺口，避免误读为整圆",
    "端点统一用实心圆点收束，半径 1.6–2.2px",
  ],
  specs: [["网格", "24 × 24"], ["描边", "1.70 px"], ["直线", "0"], ["弧口", "≥45°"], ["端点", "Dot"], ["最小尺寸", "16 px"]],
  keywords: ["圆规", "轨道", "弧线", "星盘"],
  signature: "screen", signatureCn: "整屏截图 · Display", signatureSub: "concentric orbits + cradle arc", signatureEn: "Orbit Screen",
  svg: { sw: 1.7, caps: "round", join: "round" },
  icons: {
    logo: `<path d="M12 3.5A8.5 8.5 0 1 1 3.5 12"/><path d="M12 8.2A3.8 3.8 0 1 0 8.2 12"/><circle cx="12" cy="12" r="1.6" fill="currentColor" stroke="none"/>`,
    region: `<path d="M3.5 6.5A3 3 0 0 1 6.5 3.5M17.5 3.5A3 3 0 0 1 20.5 6.5M20.5 17.5A3 3 0 0 1 17.5 20.5M6.5 20.5A3 3 0 0 1 3.5 17.5"/><circle cx="12" cy="12" r="2.2"/><circle cx="12" cy="7.6" r=".9" fill="currentColor" stroke="none"/><circle cx="12" cy="16.4" r=".9" fill="currentColor" stroke="none"/><circle cx="7.6" cy="12" r=".9" fill="currentColor" stroke="none"/><circle cx="16.4" cy="12" r=".9" fill="currentColor" stroke="none"/>`,
    window: `<path d="M3.5 9.5A4.5 4.5 0 0 1 8 5M16 5A4.5 4.5 0 0 1 20.5 9.5M20.5 14.5A4.5 4.5 0 0 1 16 19M8 19A4.5 4.5 0 0 1 3.5 14.5"/><path d="M5.3 8.5A7 7 0 0 1 18.7 8.5"/><circle cx="8" cy="7" r=".9" fill="currentColor" stroke="none"/><circle cx="10.2" cy="7" r=".9" fill="currentColor" stroke="none"/>`,
    screen: `<path d="M2.5 9A4 4 0 0 1 6.5 5M17.5 5A4 4 0 0 1 21.5 9M21.5 12.5A4 4 0 0 1 17.5 16.5M6.5 16.5A4 4 0 0 1 2.5 12.5"/><path d="M10 17A2.5 2.5 0 0 0 14 17"/><ellipse cx="12" cy="19.8" rx="3.6" ry="1.1"/>`,
    rect: `<path d="M3.5 9A5 5 0 0 1 8.5 4M15.5 4A5 5 0 0 1 20.5 9M20.5 14A5 5 0 0 1 15.5 19M8.5 19A5 5 0 0 1 3.5 14"/><circle cx="12" cy="11.5" r="1.8" fill="currentColor" stroke="none"/>`,
    circle: `<circle cx="12" cy="12" r="8.5"/><circle cx="12" cy="12" r="5.5"/><circle cx="12" cy="12" r="1.8" fill="currentColor" stroke="none"/>`,
    arrow: `<path d="M4.5 19.5A24 24 0 0 1 19.5 4.5"/><path d="M13 3.5A7 7 0 0 1 19.5 4.5"/><path d="M19.5 4.5A7 7 0 0 1 20.5 11"/><circle cx="4.5" cy="19.5" r="1.8" fill="currentColor" stroke="none"/>`,
    type: `<path d="M12 4A12 12 0 0 0 4.5 20"/><path d="M12 4A12 12 0 0 1 19.5 20"/><path d="M8 15A4.5 4.5 0 0 1 16 15"/><circle cx="4.5" cy="20" r="1.4" fill="currentColor" stroke="none"/><circle cx="19.5" cy="20" r="1.4" fill="currentColor" stroke="none"/>`,
    mosaic: `<circle cx="7.2" cy="7.2" r="3.4"/><circle cx="16.8" cy="7.2" r="2.4" fill="currentColor" stroke="none"/><circle cx="7.2" cy="16.8" r="2.2" fill="currentColor" stroke="none"/><circle cx="16.8" cy="16.8" r="3.6"/><circle cx="12" cy="12" r="1.5"/>`,
    grid: `<circle cx="5.6" cy="5.6" r="1.4"/><circle cx="12" cy="5.6" r="1.4"/><circle cx="18.4" cy="5.6" r="1.4"/><circle cx="5.6" cy="12" r="1.4"/><circle cx="12" cy="12" r="1.9"/><circle cx="18.4" cy="12" r="1.4"/><circle cx="5.6" cy="18.4" r="1.4"/><circle cx="12" cy="18.4" r="1.4"/><circle cx="18.4" cy="18.4" r="1.4"/>`,
    undo: `<path d="M12 3.5A8.5 8.5 0 1 0 20.5 12"/><path d="M12 6.5A5.5 5.5 0 1 0 17.5 12"/><circle cx="20.5" cy="12" r="2" fill="currentColor" stroke="none"/><circle cx="17.5" cy="12" r="1.2" fill="currentColor" stroke="none"/>`,
    redo: `<path d="M12 3.5A8.5 8.5 0 1 1 3.5 12"/><path d="M12 6.5A5.5 5.5 0 1 1 6.5 12"/><circle cx="3.5" cy="12" r="2" fill="currentColor" stroke="none"/><circle cx="6.5" cy="12" r="1.2" fill="currentColor" stroke="none"/>`,
    pin: `<path d="M12 21A9.5 9.5 0 0 1 5.5 9.5A6.5 6.5 0 0 1 18.5 9.5A9.5 9.5 0 0 1 12 21Z"/><circle cx="12" cy="9.5" r="1.6" fill="currentColor" stroke="none"/>`,
    copy: `<ellipse cx="9" cy="10.2" rx="5.2" ry="6.6"/><ellipse cx="15" cy="13.8" rx="5.2" ry="6.6"/><circle cx="15" cy="13.8" r="1.4" fill="currentColor" stroke="none"/>`,
    save: `<path d="M12 3.5A14 14 0 0 1 12 12"/><path d="M8.5 8A5 5 0 0 0 12 12"/><path d="M15.5 8A5 5 0 0 1 12 12"/><path d="M4.5 15A7.5 7.5 0 0 0 19.5 15"/>`,
    menu: `<ellipse cx="12" cy="6.5" rx="8" ry="1.4"/><ellipse cx="12" cy="12" rx="8" ry="1.4"/><ellipse cx="12" cy="17.5" rx="5" ry="1.4"/>`,
  },
};

export default [c08, c09, c10];
