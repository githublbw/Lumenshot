/* 01–05: five different construction grammars. */

/* 01 · Corner marks ------------------------------------------------ */
const cornerFrame = `
<path d="M3.5 8.3V3.5H8.3"/>
<path d="M15.7 3.5h4.8v4.8"/>
<path d="M20.5 15.7v4.8h-4.8"/>
<path d="M8.3 20.5H3.5v-4.8"/>`;

const c01 = {
  id: "01", slug: "corner-marks", cn: "角标", en: "Corner Marks",
  tagline: "只用四角与刻度定义边界，像印刷套准标记。",
  desc: "每个图标最多保留四个角标和几根短刻度，绝不画完整外框。识别靠“角的朝向”而不是“线的连续”，因此缩到 15px 仍然是一组锐利的标记，而不是一团线。",
  grammar: "CORNER / TICK / SQUARE NODE / OPEN FORM",
  rules: [
    "每个图标最多四个角标，绝不画完整外框",
    "角标长度固定 4.8，方向一律朝内",
    "中心信息只用 2px 实心方点或圆点表示",
  ],
  specs: [["网格", "24 × 24"], ["描边", "1.70 px"], ["角标", "4.8 px"], ["闭合", "否"], ["端点", "Square"], ["最小尺寸", "14 px"]],
  keywords: ["角标", "刻度", "套准", "不闭合"],
  signature: "region", signatureCn: "区域截图 · Region", signatureSub: "corner frame + centre reticle", signatureEn: "Corner Frame",
  svg: { sw: 1.7, caps: "square", join: "miter" },
  icons: {
    logo: `<path d="M8.9 6.1 12 3l3.1 3.1"/><path d="M17.9 8.9 21 12l-3.1 3.1"/><path d="M15.1 17.9 12 21l-3.1-3.1"/><path d="M6.1 15.1 3 12l3.1-3.1"/><circle cx="12" cy="12" r="2.2" fill="currentColor" stroke="none"/>`,
    region: `${cornerFrame}<circle cx="12" cy="12" r="2.3"/><path d="M12 6.8v2M12 15.2v2M6.8 12h2M15.2 12h2"/>`,
    window: `${cornerFrame}<path d="M3.5 9h17"/><circle cx="7" cy="6.2" r=".75" fill="currentColor" stroke="none"/><circle cx="9.5" cy="6.2" r=".75" fill="currentColor" stroke="none"/><path d="M7 15.5v-3h3M17 15.5v-3h-3"/>`,
    screen: `<path d="M2.5 9V5.5h4.8M16.7 5.5h4.8V9M21.5 15.5v3h-4.8M7.3 18.5H2.5v-3"/><path d="M12 18.5V21M8 21h8"/>`,
    rect: `${cornerFrame}<path d="M12 3.5v2M12 18.5v2M3.5 12h2M18.5 12h2"/><circle cx="12" cy="12" r="1.6" fill="currentColor" stroke="none"/>`,
    circle: `<path d="M15.5 4.4A8.5 8.5 0 0 1 19.6 8.5"/><path d="M19.6 15.5A8.5 8.5 0 0 1 15.5 19.6"/><path d="M8.5 19.6A8.5 8.5 0 0 1 4.4 15.5"/><path d="M4.4 8.5A8.5 8.5 0 0 1 8.5 4.4"/><path d="M18.2 5.8 19.5 4.5M18.2 18.2 19.5 19.5M5.8 18.2 4.5 19.5M5.8 5.8 4.5 4.5"/><circle cx="12" cy="12" r="2" fill="currentColor" stroke="none"/>`,
    arrow: `<path d="M7 17 17 7"/><path d="M8.5 7H17v8.5"/><circle cx="5.4" cy="18.6" r="1.2" fill="currentColor" stroke="none"/>`,
    type: `<path d="M4.5 5.5h15M4.5 3.8v3.4M19.5 3.8v3.4M12 5.5v14M9 19.5h6"/>`,
    mosaic: `<circle cx="7" cy="7" r="1.9" fill="currentColor" stroke="none"/><circle cx="17" cy="7" r="1.9" fill="currentColor" stroke="none"/><circle cx="7" cy="17" r="1.9" fill="currentColor" stroke="none"/><circle cx="17" cy="17" r="1.9" fill="currentColor" stroke="none"/><circle cx="12" cy="12" r="1.9" fill="currentColor" stroke="none"/><circle cx="12" cy="7" r="1.45"/><circle cx="7" cy="12" r="1.45"/><circle cx="17" cy="12" r="1.45"/><circle cx="12" cy="17" r="1.45"/>`,
    grid: `${cornerFrame}<path d="M9.17 3.5v2.1M9.17 18.4v2.1M14.83 3.5v2.1M14.83 18.4v2.1M3.5 9.17h2.1M18.4 9.17h2.1M3.5 14.83h2.1M18.4 14.83h2.1"/>`,
    undo: `<path d="M4 9h10.5a5 5 0 0 1 5 5v1.6"/><path d="M9 14 4 9l5-5"/><path d="M15.5 19.5h4.5"/>`,
    redo: `<path d="M20 9H9.5a5 5 0 0 0-5 5v1.6"/><path d="M15 14l5-5-5-5"/><path d="M4 19.5h4.5"/>`,
    pin: `<path d="M12 4l4 4-4 4-4-4z"/><path d="M12 12v6.5"/><path d="M8.5 20.5h7"/>`,
    copy: `<path d="M3.5 8V3.5H8M12 3.5h3.5V8M15.5 12v3.5H12M8 15.5H3.5V12"/><path d="M9 13.5V9h4.5M17.5 9H21v4.5M21 17v3.5h-4M13 20.5H9V17"/>`,
    save: `<path d="M12 4v10M8 10l4 4 4-4"/><path d="M4 20h16"/><path d="M4 18v3.5M20 18v3.5"/>`,
    menu: `<path d="M4 7h16M2.5 5.6v2.8M21.5 5.6v2.8M4 12h16M2.5 10.6v2.8M21.5 10.6v2.8M4 17h10M2.5 15.6v2.8M15.5 15.6v2.8"/>`,
  },
};

/* 02 · Solid / negative space -------------------------------------- */
const c02 = {
  id: "02", slug: "solid", cn: "实心", en: "Solid",
  tagline: "用正负形代替线条，先有重量，再读细节。",
  desc: "整套图标没有一条开放线：所有信息都由实心形与挖空构成。因此它在小尺寸下最稳，远看是一块块清楚的剪影，近看才露出内部的负形细节。",
  grammar: "SOLID MASS / NEGATIVE SPACE / ROUNDED CUT",
  rules: [
    "外形全部实心，细节只靠挖空表达",
    "圆角统一 2px，外轮廓不留尖角",
    "最小负形间隙 1.5px，保证 15px 可读",
  ],
  specs: [["网格", "24 × 24"], ["填充", "Solid"], ["挖空", "≥1.5 px"], ["圆角", "2.0 px"], ["描边", "0"], ["最小尺寸", "14 px"]],
  keywords: ["正负形", "实心", "剪影", "挖空"],
  signature: "logo", signatureCn: "主标识 · Aperture", signatureSub: "squircle mass − lens cutout", signatureEn: "Negative Cut",
  svg: { fill: "currentColor", stroke: "none", sw: 0 },
  icons: {
    logo: `<path fill-rule="evenodd" d="M9 3h6a6 6 0 0 1 6 6v6a6 6 0 0 1-6 6H9a6 6 0 0 1-6-6V9a6 6 0 0 1 6-6Zm3 5.6a3.4 3.4 0 1 0 0 6.8 3.4 3.4 0 0 0 0-6.8Z"/><circle cx="12" cy="12" r="2.1"/>`,
    region: `<path d="M3.5 10V3.5H10v3.4H6.9V10zM14 3.5h6.5V10h-3.4V6.9H14zM20.5 14v6.5H14v-3.4h3.1V14zM10 20.5H3.5V14h3.4v3.1H10z"/><circle cx="12" cy="12" r="2"/>`,
    window: `<path fill-rule="evenodd" d="M5 4.5h14A2.5 2.5 0 0 1 21.5 7v10a2.5 2.5 0 0 1-2.5 2.5H5A2.5 2.5 0 0 1 2.5 17V7A2.5 2.5 0 0 1 5 4.5Zm.6 2.9v2.3h12.8V7.4H5.6Zm3 4.6v4.8h7.8V12H8.6Z"/>`,
    screen: `<path fill-rule="evenodd" d="M5 4h14a2.5 2.5 0 0 1 2.5 2.5v8A2.5 2.5 0 0 1 19 17H5a2.5 2.5 0 0 1-2.5-2.5v-8A2.5 2.5 0 0 1 5 4Zm1.5 3v6h11V7h-11Z"/><path d="M10.4 17h3.2v2.6h3.4V21H7v-1.4h3.4V17Z"/>`,
    rect: `<path fill-rule="evenodd" d="M5 6h14a2.5 2.5 0 0 1 2.5 2.5v7A2.5 2.5 0 0 1 19 18H5a2.5 2.5 0 0 1-2.5-2.5v-7A2.5 2.5 0 0 1 5 6Zm1 3v6h12V9H6Z"/>`,
    circle: `<path fill-rule="evenodd" d="M12 3.5a8.5 8.5 0 1 0 0 17 8.5 8.5 0 0 0 0-17Zm0 3a5.5 5.5 0 1 0 0 11 5.5 5.5 0 0 0 0-11Z"/>`,
    arrow: `<path d="M9.4 3.6h11v11l-3.3-3.3-7.6 7.6-4.4-4.4 7.6-7.6-3.3-3.3Z"/>`,
    type: `<path d="M4.5 4h15v4h-5.6v12h-3.8V8H4.5z"/>`,
    mosaic: `<path d="M3.5 3.5h5.2v5.2H3.5zM15.3 3.5h5.2v5.2h-5.2zM9.4 9.4h5.2v5.2H9.4zM3.5 15.3h5.2v5.2H3.5zM15.3 15.3h5.2v5.2h-5.2z"/>`,
    grid: `<path d="M3.5 3.5h7.2v7.2H3.5zM13.3 3.5h7.2v7.2h-7.2zM3.5 13.3h7.2v7.2H3.5zM13.3 13.3h7.2v7.2h-7.2z"/>`,
    undo: `<path d="M9 14 4 9l5-5" stroke="currentColor" stroke-width="2.8" stroke-linecap="round" stroke-linejoin="round"/><path d="M4.5 9h10a5 5 0 0 1 5 5v2" stroke="currentColor" stroke-width="2.8" stroke-linecap="round" fill="none"/>`,
    redo: `<path d="m15 14 5-5-5-5" stroke="currentColor" stroke-width="2.8" stroke-linecap="round" stroke-linejoin="round"/><path d="M19.5 9h-10a5 5 0 0 0-5 5v2" stroke="currentColor" stroke-width="2.8" stroke-linecap="round" fill="none"/>`,
    pin: `<path fill-rule="evenodd" d="M12 2.5a7 7 0 0 0-7 7c0 4.9 7 12 7 12s7-7.1 7-12a7 7 0 0 0-7-7Zm0 4.4a2.6 2.6 0 1 1 0 5.2 2.6 2.6 0 0 1 0-5.2Z"/>`,
    copy: `<path fill-rule="evenodd" d="M3.5 3.5h11v3.2H6.2v8.3H3.5V3.5Zm5 5h12v12h-12v-12Zm3 3v6h6v-6h-6Z"/>`,
    save: `<path d="M10.2 3h3.6v8.2h3.9L12 18.2 6.3 11.2h3.9V3Z"/><path d="M3.5 18.5h17V22h-17z"/>`,
    menu: `<path d="M4 5.5h16v2.6H4zM4 10.7h16v2.6H4zM4 15.9h10.5v2.6H4z"/>`,
  },
};

/* 03 · Node graph --------------------------------------------------- */
const n = (x, y, r = 1.35) => `<circle cx="${x}" cy="${y}" r="${r}" fill="currentColor" stroke="none"/>`;
const c03 = {
  id: "03", slug: "node-graph", cn: "节点", en: "Node Graph",
  tagline: "把图标画成电路：线是连接，点是决策。",
  desc: "所有转折点都必须落一颗节点，直线只允许 90° 与 45°；圆形作为唯一的闭合轨道，节点落在四向测量点上。图标的“骨架”被直接暴露出来，看起来像一张可追踪的连线图，适合强调精确与可测量。",
  grammar: "POLYLINE / DOT AT EVERY JOINT / 90° + 45° / ORBIT",
  rules: [
    "所有转折点都放一颗 2.7px 节点，不允许裸转角",
    "直线只允许 90° 与 45°，圆是唯一的闭合轨道",
    "线宽 1.1px，节点直径至少是线宽的两倍",
  ],
  specs: [["网格", "24 × 24"], ["线宽", "1.10 px"], ["节点", "2.7 px"], ["角度", "90° / 45° / 圆"], ["曲线", "仅圆"], ["最小尺寸", "16 px"]],
  keywords: ["节点", "连线", "直角", "电路感"],
  signature: "region", signatureCn: "区域截图 · Region", signatureSub: "dashed frame + reticle nodes", signatureEn: "Node Lattice",
  svg: { sw: 1.1, caps: "butt", join: "miter" },
  icons: {
    logo: `<path d="M20.5 12 16.25 4.64 7.75 4.64 3.5 12 7.75 19.36 16.25 19.36Z"/><path d="M12 12 20.5 12M12 12 16.25 4.64M12 12 7.75 4.64M12 12 3.5 12M12 12 7.75 19.36M12 12 16.25 19.36"/>${n(20.5, 12)}${n(16.25, 4.64)}${n(7.75, 4.64)}${n(3.5, 12)}${n(7.75, 19.36)}${n(16.25, 19.36)}${n(12, 12, 1.7)}`,
    region: `<path d="M3.5 3.5h17v17h-17z" stroke-dasharray="2 3"/><path d="M12 8.6v6.8M8.6 12h6.8"/>${n(3.5, 3.5)}${n(20.5, 3.5)}${n(3.5, 20.5)}${n(20.5, 20.5)}${n(12, 12, 1.7)}${n(12, 8.6)}${n(12, 15.4)}${n(8.6, 12)}${n(15.4, 12)}`,
    window: `<rect x="3" y="4.5" width="18" height="15"/><path d="M3 9h18"/><rect x="7" y="12" width="10" height="4.8"/>${n(3, 4.5)}${n(21, 4.5)}${n(3, 19.5)}${n(21, 19.5)}${n(3, 9)}${n(12, 9)}${n(21, 9)}${n(7, 12)}${n(17, 12)}${n(7, 16.8)}${n(17, 16.8)}`,
    screen: `<rect x="2.5" y="4" width="19" height="13"/><path d="M12 17v3.5M8 20.5h8"/>${n(2.5, 4)}${n(21.5, 4)}${n(2.5, 17)}${n(21.5, 17)}${n(12, 17)}${n(12, 20.5)}${n(8, 20.5)}${n(16, 20.5)}`,
    rect: `<rect x="3.5" y="6" width="17" height="12"/>${n(3.5, 6)}${n(20.5, 6)}${n(3.5, 18)}${n(20.5, 18)}${n(12, 6)}${n(12, 18)}${n(3.5, 12)}${n(20.5, 12)}`,
    circle: `<circle cx="12" cy="12" r="8.5"/>${n(12, 3.5)}${n(20.5, 12)}${n(12, 20.5)}${n(3.5, 12)}${n(12, 12, 1.7)}`,
    arrow: `<path d="M4 20 12 12 20 4"/><path d="M11 4h9v9"/>${n(4, 20)}${n(12, 12)}${n(20, 4)}${n(11, 4)}${n(20, 13)}`,
    type: `<path d="M5 5h14M12 5v14M9 19h6"/>${n(5, 5)}${n(19, 5)}${n(12, 5)}${n(12, 19)}${n(9, 19)}${n(15, 19)}${n(12, 12)}`,
    mosaic: `<path d="M6 6h6v6H6zM12 12h6v6h-6z"/>${n(6, 6)}${n(12, 6)}${n(6, 12)}${n(12, 12)}${n(18, 12)}${n(12, 18)}${n(18, 18)}`,
    grid: `<path d="M4 4h16v16H4zM4 12h16M12 4v16"/>${n(4, 4)}${n(12, 4)}${n(20, 4)}${n(4, 12)}${n(12, 12)}${n(20, 12)}${n(4, 20)}${n(12, 20)}${n(20, 20)}`,
    undo: `<path d="M4 9h10.5a5 5 0 0 1 5 5v2"/><path d="M9 14 4 9l5-5"/>${n(4, 9)}${n(9, 4)}${n(9, 14)}${n(14.5, 16)}${n(19.5, 16)}`,
    redo: `<path d="M20 9H9.5a5 5 0 0 0-5 5v2"/><path d="M15 14l5-5-5-5"/>${n(20, 9)}${n(15, 4)}${n(15, 14)}${n(9.5, 16)}${n(4.5, 16)}`,
    pin: `<path d="M12 20.5V17M9 10.6V7h6v3.6l2 2V16H7v-3.4z"/>${n(12, 20.5)}${n(9, 10.6)}${n(15, 10.6)}${n(7, 16)}${n(17, 16)}${n(12, 12)}`,
    copy: `<rect x="3.5" y="3.5" width="11" height="11"/><rect x="9" y="9" width="11" height="11"/>${n(3.5, 3.5)}${n(14.5, 3.5)}${n(3.5, 14.5)}${n(14.5, 14.5)}${n(9, 9)}${n(20, 9)}${n(9, 20)}${n(20, 20)}`,
    save: `<path d="M12 3.5v11.5M7.5 10.5 12 15l4.5-4.5M4 19.5h16"/>${n(12, 3.5)}${n(12, 15)}${n(7.5, 10.5)}${n(16.5, 10.5)}${n(4, 19.5)}${n(20, 19.5)}`,
    menu: `<path d="M4 7h16M4 12h16M4 17h10"/>${n(4, 7)}${n(20, 7)}${n(4, 12)}${n(20, 12)}${n(4, 17)}${n(14, 17)}`,
  },
};

export default [c01, c02, c03];
