/* 04–07: notch, isometric, one-line, collage. */

/* 04 · Notch -------------------------------------------------------- */
const c04 = {
  id: "04", slug: "notch", cn: "断口", en: "Notch",
  tagline: "每个图标都被切了一刀，缺口本身就是方向。",
  desc: "所有轮廓在同一个位置断开，留下 45° 的切口和一小块脱离的碎片。断口不是装饰，而是功能提示：它告诉用户“这里可以切开、可以选取”。",
  grammar: "OPEN CONTOUR / 45° CUT / DETACHED SHARD",
  rules: [
    "每枚图标至少留一个 45° 缺口，绝不闭合",
    "缺口宽度固定 3px，不做圆滑过渡",
    "断开的碎片保留，作为造型的第二笔",
  ],
  specs: [["网格", "24 × 24"], ["描边", "1.90 px"], ["切口", "45°"], ["缺口", "3.0 px"], ["端点", "Butt"], ["最小尺寸", "15 px"]],
  keywords: ["断口", "碎片", "切开", "开放轮廓"],
  signature: "logo", signatureCn: "主标识 · Aperture", signatureSub: "open ring + detached shard", signatureEn: "Cut Ring",
  svg: { sw: 1.9, caps: "butt", join: "miter" },
  icons: {
    logo: `<path d="M19.9 7.4A9.5 9.5 0 1 1 7.4 19.9"/><path d="M14.8 9.2 9.2 14.8"/><path d="M17.2 3.4A9.5 9.5 0 0 1 20.6 7.8"/>`,
    region: `<path d="M3.5 8V3.5H8M15.5 3.5h4.5V8M8 20.5H3.5V16M20.5 15.5 15.5 20.5"/><path d="M19.4 8.6h1.1v1.1z" fill="currentColor" stroke="none"/><circle cx="12" cy="12" r="2.3"/><path d="M12 7.5v1.4M12 15.1v1.4M7.5 12h1.4M15.1 12h1.4"/>`,
    window: `<path d="M3 19.5V6.5h11.5"/><path d="M18.5 9.5 21 12v7.5H3"/><path d="M3 9h8.5"/><circle cx="6.2" cy="7.4" r=".65" fill="currentColor" stroke="none"/><circle cx="8.5" cy="7.4" r=".65" fill="currentColor" stroke="none"/><path d="M18.3 6.6 19.5 5.4"/>`,
    screen: `<path d="M2.5 4h19v9.5L18 17H2.5z"/><path d="M20.6 15.6 17.4 18.8"/><path d="M12 17v3.5M8 20.5h8"/>`,
    rect: `<path d="M3.5 18.5V6.5h11"/><path d="M18.5 9.5 21 12v6.5H3.5"/><path d="M17.4 6.4 18.6 5.2"/>`,
    circle: `<path d="M15.4 4.5A8.5 8.5 0 1 1 4.5 15.4"/><path d="M17.2 3.4A9.5 9.5 0 0 1 20.6 7.8"/><circle cx="12" cy="12" r="2.2"/>`,
    arrow: `<path d="M5 19 10.2 13.8"/><path d="M13.8 10.2 19 5"/><path d="M9.5 5H19v9.5"/><path d="M11.4 12.6 12.6 11.4"/>`,
    type: `<path d="M4.5 5.5h8"/><path d="M15.5 5.5h4"/><path d="M12 5.5v14M9 19.5h6"/><path d="M14.6 4.2 13.4 6.8"/>`,
    mosaic: `<path d="M3.5 3.5h6v6h-6zM14.5 3.5h6v6h-6zM3.5 14.5h6v6h-6z"/><path d="M14.5 14.5h6v6"/><path d="M14.5 18 18 14.5l2.5 2.5L17 20.5z"/><path d="M12.4 16.2 14 14.6"/>`,
    grid: `<path d="M3.5 3.5h17v17h-17zM3.5 12h7"/><path d="M13 12h7.5M12 3.5v8"/><path d="M12 14v6.5"/><path d="M10.5 13.5 13.5 10.5"/>`,
    undo: `<path d="M4 9h9"/><path d="M16 9h.5a5 5 0 0 1 5 5v2"/><path d="M9 14 4 9l5-5"/><path d="M12.6 8.9 14.4 10.7"/>`,
    redo: `<path d="M20 9h-9"/><path d="M8 9h-.5a5 5 0 0 0-5 5v2"/><path d="M15 14l5-5-5-5"/><path d="M11.4 8.9 9.6 10.7"/>`,
    pin: `<path d="M9 7h2"/><path d="M13.5 7H15v3.6l2 2V16H7v-3.4l2-2V7"/><path d="M12 17v4.5"/><path d="M10.4 6 13 4.2"/>`,
    copy: `<path d="M3.5 8V3.5H13V8"/><path d="M3.5 11v4.5H8"/><path d="M10 6h4.5L21 12.5V20.5H10V6z"/><path d="M17.2 5.2 19.8 7.8"/>`,
    save: `<path d="M12 3.5v6"/><path d="M12 12.5v2"/><path d="m7.5 10.5 4.5 4.5 4.5-4.5"/><path d="M4 19.5h7M13 19.5h7"/><path d="M11.4 17.4 13.4 15.4"/>`,
    menu: `<path d="M4 7h8.5M15 7h5M4 12h16M4 17h11M17 17h3"/><path d="M13.8 5.8 12.6 8.2M15.6 15.8 14.4 18.2"/>`,
  },
};

/* 05 · Isometric ---------------------------------------------------- */
const ip = (x, y, z = 0) => [12 + (x - y) * 1.22, 11 + (x + y) * 0.58 - z * 1.22];
const pts = (arr) => arr.map(([x, y, z]) => { const [a, b] = ip(x, y, z); return `${a.toFixed(2)},${b.toFixed(2)}`; }).join(" ");
const poly = (arr, attrs = "") => `<polygon points="${pts(arr)}" ${attrs}/>`;
const pline = (arr, attrs = "") => `<polyline points="${pts(arr)}" ${attrs}/>`;
const slab = (x, y, w, h, z = 0, d = 1.6, attrs = "") => {
  const top = poly([[x, y, z], [x + w, y, z], [x + w, y + h, z], [x, y + h, z]], attrs);
  const left = poly([[x, y + h, z], [x + w, y + h, z], [x + w, y + h, z - d], [x, y + h, z - d]], attrs);
  const right = poly([[x + w, y + h, z], [x + w, y, z], [x + w, y, z - d], [x + w, y + h, z - d]], attrs);
  return left + right + top;
};
const isoEllipse = (cx, cy, rx, ry, z = 0) => {
  const [a, b] = ip(cx, cy, z);
  return `<ellipse cx="${a.toFixed(2)}" cy="${b.toFixed(2)}" rx="${rx}" ry="${ry}"/>`;
};
const panel = (x0, x1, z0, z1, y0 = 0, d = 0.9) => {
  const front = poly([[x0, y0, z0], [x1, y0, z0], [x1, y0, z1], [x0, y0, z1]]);
  const side = poly([[x1, y0, z0], [x1, y0 + d, z0], [x1, y0 + d, z1], [x1, y0, z1]]);
  const top = poly([[x0, y0, z1], [x1, y0, z1], [x1, y0 + d, z1], [x0, y0 + d, z1]]);
  return side + top + front;
};
const dot3 = (x, y, z = 0, r = 1.3) => { const [a, b] = ip(x, y, z); return `<circle cx="${a.toFixed(2)}" cy="${b.toFixed(2)}" r="${r}" fill="currentColor" stroke="none"/>`; };

const c05 = {
  id: "05", slug: "isometric", cn: "等轴测", en: "Isometric",
  tagline: "把工具放倒，让它们在 30° 网格上站起来。",
  desc: "所有图标都使用同一套 30° 等轴测投影：平面变成菱形，厚度统一 2px。它不靠线条粗细制造层级，而靠“面”和“深度”区分前后，因此剪影是一组倾斜的积木。",
  grammar: "30° ISO PROJECTION / FACE + DEPTH / NO FLAT VIEW",
  rules: [
    "统一 30° 等轴测投影，不混用平面视图",
    "每个形体保留 2px 厚度，方向一致",
    "不画隐藏边，靠面与面的明暗区分",
  ],
  specs: [["投影", "30° ISO"], ["线宽", "1.50 px"], ["厚度", "2.0 px"], ["面数", "2–3"], ["平面", "禁用"], ["最小尺寸", "18 px"]],
  keywords: ["等轴测", "厚度", "菱形", "立体"],
  signature: "screen", signatureCn: "整屏截图 · Display", signatureSub: "standing panel + base slab", signatureEn: "Iso Panel",
  svg: { sw: 1.5, caps: "round", join: "round" },
  icons: {
    logo: `${slab(0.5, 0.5, 6, 6, 3, 3)}${isoEllipse(3.5, 3.5, 5.4, 2.6, 3)}${dot3(3.5, 3.5, 3, 1.2)}`,
    region: `${slab(0.5, 0.5, 8, 8, 0, 0.8)}${poly([[1.9, 1.9, 0], [7.1, 1.9, 0], [7.1, 7.1, 0], [1.9, 7.1, 0]])}${dot3(4.5, 4.5, 0, 1.4)}`,
    window: `${panel(-4.5, 4.5, 0.8, 6)}${slab(-3.4, 0, 6.8, 1.4, 0.8)}${pline([[-4.5, 0, 4.5], [4.5, 0, 4.5]])}${poly([[-3.7, 0, 1.6], [3.7, 0, 1.6], [3.7, 0, 3.9], [-3.7, 0, 3.9]])}${dot3(-3, 0, 5.2, .6)}${dot3(-1.6, 0, 5.2, .6)}`,
    screen: `${poly([[-5, 0, 0.8], [5, 0, 0.8], [5, 0, 5.6], [-5, 0, 5.6]])}${poly([[-5, 0, 5.6], [5, 0, 5.6], [5, 0.9, 5.6], [-5, 0.9, 5.6]])}${poly([[5, 0, 0.8], [5, 0.9, 0.8], [5, 0.9, 5.6], [5, 0, 5.6]])}${poly([[-3.9, 0, 1.6], [3.9, 0, 1.6], [3.9, 0, 4.7], [-3.9, 0, 4.7]])}${slab(-3.2, 0.2, 6.4, 1.8, 0.8)}`,
    rect: `${slab(-5, -3, 10, 6)}${poly([[-3.6, -1.6, 0], [3.6, -1.6, 0], [3.6, 1.6, 0], [-3.6, 1.6, 0]])}`,
    circle: `${isoEllipse(0, 0, 8.4, 4, 0)}${dot3(0, 0, 0, 1.3)}`,
    arrow: `${slab(-6, -1.2, 8, 2.4)}${poly([[2, 3, 0], [6.5, 0, 0], [6.5, 0, -0.8], [2, 3, -0.8]])}${poly([[6.5, 0, 0], [2, -3, 0], [2, -3, -0.8], [6.5, 0, -0.8]])}${poly([[2, -3, 0], [6.5, 0, 0], [2, 3, 0]])}`,
    type: `${slab(-4.5, -3, 9, 2.2)}${slab(-1.1, -0.8, 2.2, 7.8)}`,
    mosaic: `${slab(-6.2, -6.2, 4, 4)}${slab(-1.4, -6.2, 4, 4, 1.2)}${slab(-6.2, -1.4, 4, 4, 2.2)}${slab(-1.4, -1.4, 4, 4, 0.6)}`,
    grid: `${slab(-4.5, -4.5, 9, 9, 0, 0.8)}${slab(-1.6, -4.5, 0.9, 9, 0.8, 0.8)}${slab(0.7, -4.5, 0.9, 9, 0.8, 0.8)}${slab(-4.5, -1.6, 9, 0.9, 0.8, 0.8)}${slab(-4.5, 0.7, 9, 0.9, 0.8, 0.8)}`,
    undo: `${slab(-4.5, -4.5, 9, 9, 0, 0.8)}${pline([[-3.5, -3.5, 0.8], [-0.5, -3.5, 0.8], [2, -1, 0.8], [2, 2, 0.8], [-0.5, 4, 0.8], [-3.5, 4, 0.8]], 'fill="none" stroke-width="1.8"')}${poly([[-5.5, 4, 0.8], [-3.5, 2.8, 0.8], [-3.5, 5.2, 0.8]])}`,
    redo: `${slab(-4.5, -4.5, 9, 9, 0, 0.8)}${pline([[3.5, -3.5, 0.8], [0.5, -3.5, 0.8], [-2, -1, 0.8], [-2, 2, 0.8], [0.5, 4, 0.8], [3.5, 4, 0.8]], 'fill="none" stroke-width="1.8"')}${poly([[5.5, 4, 0.8], [3.5, 2.8, 0.8], [3.5, 5.2, 0.8]])}`,
    pin: `${slab(-2.5, -2.5, 5, 5)}<path d="M12 7.8V11.2"/>${(() => { const [hx, hy] = ip(0, 0, 5.2); return `<circle cx="${hx}" cy="${hy}" r="3.2"/>${dot3(0, 0, 5.2, 1)}`; })()}`,
    copy: `${slab(-4.5, -1.5, 7, 5)}${slab(-3, 0.5, 7, 5, 1.8)}`,
    save: `${slab(-4.5, 1.5, 9, 4)}${slab(-0.8, -4, 1.6, 4, 3.2, 0.8)}${poly([[-2.5, 0, 3.2], [0, 2.6, 3.2], [0, 2.6, 2.4], [-2.5, 0, 2.4]])}${poly([[0, 2.6, 3.2], [2.5, 0, 3.2], [2.5, 0, 2.4], [0, 2.6, 2.4]])}${poly([[-2.5, 0, 3.2], [2.5, 0, 3.2], [0, 2.6, 3.2]])}`,
    menu: `${slab(-4.5, -4, 9, 1.7)}${slab(-4.5, -1.4, 9, 1.7, 1.8)}${slab(-4.5, 1.2, 9, 1.7, 3.6)}`,
  },
};

/* 06 · One line ----------------------------------------------------- */
const c06 = {
  id: "06", slug: "one-line", cn: "一笔", en: "One Line",
  tagline: "不提笔，一笔画完；收笔处留一个点。",
  desc: "每枚图标只有一条连续路径，允许自交、回笔和绕圈，但绝不出现第二个起点。它像用马克笔在玻璃上一笔画成，收笔的统一圆点就是签名。",
  grammar: "ONE CONTINUOUS PATH / SELF-CROSS / END DOT",
  rules: [
    "每枚图标只有一条连续路径，不拆分部件",
    "允许回笔与自交，用重叠线制造转折",
    "收笔统一落一个 3.4px 圆点作为签名",
  ],
  specs: [["网格", "24 × 24"], ["描边", "2.00 px"], ["路径", "1"], ["回笔", "允许"], ["端点", "Round"], ["最小尺寸", "16 px"]],
  keywords: ["一笔", "连续", "回笔", "圆点"],
  signature: "logo", signatureCn: "主标识 · Aperture", signatureSub: "single-stroke spiral + end dot", signatureEn: "One Stroke",
  svg: { sw: 2, caps: "round", join: "round" },
  icons: {
    logo: `<path d="M12 3a9 9 0 1 0 9 9 6 6 0 0 0-6-6 3.6 3.6 0 0 0-3.6 3.6"/><circle cx="11.4" cy="9.6" r="1.7" fill="currentColor" stroke="none"/>`,
    region: `<path d="M12 12 7 7v10h10V7H9 12"/><circle cx="12" cy="12" r="1.7" fill="currentColor" stroke="none"/>`,
    window: `<path d="M12 12 6 6h12v12H6V9h9"/><circle cx="15" cy="9" r="1.7" fill="currentColor" stroke="none"/>`,
    screen: `<path d="M12 12 4 12V5h16v13H4v2h16"/><circle cx="20" cy="20" r="1.7" fill="currentColor" stroke="none"/>`,
    rect: `<path d="M12 12 6 12 6 6 18 6 18 18 6 18 6 10 12 10 12 12"/><circle cx="12" cy="12" r="1.7" fill="currentColor" stroke="none"/>`,
    circle: `<path d="M12 3.5a8.5 8.5 0 1 0 8.5 8.5 5.5 5.5 0 0 0-5.5-5.5 3 3 0 0 0-3 3"/><circle cx="12" cy="9.5" r="1.7" fill="currentColor" stroke="none"/>`,
    arrow: `<path d="M4 20 18 6h-6l6 0v6"/><circle cx="18" cy="12" r="1.7" fill="currentColor" stroke="none"/>`,
    type: `<path d="M4 6h16H12v14H8h8H12"/><circle cx="12" cy="20" r="1.7" fill="currentColor" stroke="none"/>`,
    mosaic: `<path d="M4 4h7v7H4v8h7v-8h9V4h-9"/><circle cx="11" cy="4" r="1.7" fill="currentColor" stroke="none"/>`,
    grid: `<path d="M4 4h16v16H4v-8h8V4h8"/><circle cx="20" cy="4" r="1.7" fill="currentColor" stroke="none"/>`,
    undo: `<path d="M9 4 4 9l5 5-5-5h10a5 5 0 0 1 0 10h-3"/><circle cx="11" cy="19" r="1.7" fill="currentColor" stroke="none"/>`,
    redo: `<path d="M15 4 20 9l-5 5 5-5H10a5 5 0 0 0 0 10h3"/><circle cx="10" cy="19" r="1.7" fill="currentColor" stroke="none"/>`,
    pin: `<path d="M12 21v-8l3-3V6H9v4l3 3v8"/><circle cx="12" cy="21" r="1.7" fill="currentColor" stroke="none"/>`,
    copy: `<path d="M9 9h11v11H9V9H3.5V3.5H15V9"/><circle cx="15" cy="9" r="1.7" fill="currentColor" stroke="none"/>`,
    save: `<path d="M12 3v10l-4-4 4 4 4-4-4 4v3H4v3h16"/><circle cx="20" cy="19" r="1.7" fill="currentColor" stroke="none"/>`,
    menu: `<path d="M4 7h16H4v5h16H4v5h10"/><circle cx="14" cy="17" r="1.7" fill="currentColor" stroke="none"/>`,
  },
};

/* 07 · Collage ------------------------------------------------------ */
const c07 = {
  id: "07", slug: "collage", cn: "拼贴", en: "Collage",
  tagline: "圆、方、三角各自独立，靠位置发生关系。",
  desc: "每枚图标由三到五个互不相连的几何原语拼成：圆、半圆、矩形、三角。它们不描边、不闭合，靠留白和错位让眼睛自己把形状补全。",
  grammar: "DISCONNECTED PRIMITIVES / GAP / 3-STEP INK",
  rules: [
    "只用圆、半圆、矩形、三角四种原语",
    "部件之间至少留 1.5px 空隙，不粘连",
    "用 35 / 65 / 100% 三层墨色建立前后关系",
  ],
  specs: [["网格", "24 × 24"], ["原语", "4 种"], ["间隙", "≥1.5 px"], ["层次", "3 层"], ["描边", "0"], ["最小尺寸", "15 px"]],
  keywords: ["拼贴", "包豪斯", "错位", "几何"],
  signature: "region", signatureCn: "区域截图 · Region", signatureSub: "four quarter-discs + dot", signatureEn: "Quarter Discs",
  svg: { fill: "currentColor", stroke: "none", sw: 0 },
  icons: {
    logo: `<circle cx="12" cy="12" r="8" opacity=".34"/><path d="M12 4a8 8 0 0 1 8 8h-8z"/><circle cx="9.4" cy="14.6" r="2.7" opacity=".68"/>`,
    region: `<path d="M3.5 3.5h5a5 5 0 0 1-5 5z"/><path d="M20.5 3.5v5a5 5 0 0 0-5-5z" opacity=".7"/><path d="M20.5 20.5h-5a5 5 0 0 1 5-5z"/><path d="M3.5 20.5v-5a5 5 0 0 0 5 5z" opacity=".7"/><circle cx="12" cy="12" r="2.2"/>`,
    window: `<rect x="3.5" y="3.5" width="17" height="3.4" rx="1.2"/><rect x="3.5" y="9" width="8" height="8" rx="1.2" opacity=".62"/><rect x="13.5" y="9" width="7" height="8" rx="1.2" opacity=".34"/><circle cx="7" cy="15" r="1.2" opacity=".9"/>`,
    screen: `<path d="M4 15a8 8 0 0 1 16 0z"/><rect x="8" y="16.8" width="8" height="2.2" rx="1.1"/><rect x="6" y="20.2" width="12" height="2" rx="1" opacity=".5"/>`,
    rect: `<path d="M3.5 3.5h10v3h-7v7h-3z"/><path d="M20.5 20.5h-10v-3h7v-7h3z" opacity=".56"/>`,
    circle: `<path d="M12 3a9 9 0 0 1 6.4 15.4L12 12z"/><path d="M12 21A9 9 0 0 1 5.6 5.6L12 12z" opacity=".56"/>`,
    arrow: `<path d="M20.5 3.5v9.5l-9.5-9.5z"/><path d="M14.6 10.4 5 20l-1.5-1.5 9.6-9.6z" opacity=".62"/>`,
    type: `<rect x="4" y="4" width="16" height="3.2" rx="1"/><rect x="10.4" y="7.2" width="3.2" height="13" rx="1"/><rect x="15.6" y="15.4" width="4.4" height="4.4" rx="1" opacity=".42"/>`,
    mosaic: `<path d="M3.5 3.5h5a5 5 0 0 1-5 5z"/><rect x="13.5" y="3.5" width="7" height="7" rx="1.4" opacity=".65"/><rect x="3.5" y="13.5" width="7" height="7" rx="1.4" opacity=".65"/><path d="M20.5 20.5h-5a5 5 0 0 1 5-5z" opacity=".36"/>`,
    grid: `<rect x="3.5" y="3.5" width="6.5" height="6.5" rx="1.4"/><rect x="14" y="3.5" width="6.5" height="6.5" rx="1.4" opacity=".62"/><rect x="3.5" y="14" width="6.5" height="6.5" rx="1.4" opacity=".62"/><rect x="14" y="14" width="6.5" height="6.5" rx="1.4"/><circle cx="12" cy="12" r="2.1" opacity=".85"/>`,
    undo: `<path d="M12 4a8 8 0 0 1 0 16v-4a4 4 0 0 0 0-8z"/><path d="M2.5 12 7 7.5v9z" opacity=".62"/>`,
    redo: `<path d="M12 4a8 8 0 0 0 0 16v-4a4 4 0 0 1 0-8z"/><path d="M21.5 12 17 7.5v9z" opacity=".62"/>`,
    pin: `<circle cx="12" cy="7.4" r="4.6"/><path d="M8.4 10.2 12 21l3.6-10.8z" opacity=".55"/>`,
    copy: `<path fill-rule="evenodd" d="M3.5 3.5h13v13h-13zM8 7.5a3 3 0 1 0 0 6 3 3 0 0 0 0-6Z"/><rect x="11" y="11" width="9" height="9" rx="2" opacity=".5"/>`,
    save: `<path d="M12 3.5 18 11h-4.5v5h-3v-5H6z"/><rect x="4" y="19" width="16" height="2.6" rx="1" opacity=".55"/>`,
    menu: `<rect x="4" y="5.5" width="16" height="2.6" rx="1"/><rect x="4" y="10.7" width="12" height="2.6" rx="1" opacity=".66"/><rect x="4" y="15.9" width="7" height="2.6" rx="1" opacity=".4"/><circle cx="19.5" cy="12" r="1.6"/><circle cx="14.5" cy="17.2" r="1.6" opacity=".6"/>`,
  },
};

export default [c04, c05, c06, c07];
