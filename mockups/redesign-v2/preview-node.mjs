/* Render a 1:1 usage preview of the exported node-graph assets. */
import pw from "/tmp/shot/node_modules/playwright-core/index.js";
const { chromium } = pw;
import { writeFileSync, mkdirSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const assets = join(here, "png", "node-assets");
const outDir = join(here, "png");
mkdirSync(outDir, { recursive: true });

const mode = [["scan", "区域", true], ["focus", "窗口", false], ["monitor", "整屏", false]];
const tools = [["rectangle-horizontal", "矩形"], ["circle", "椭圆"], ["move-up-right", "箭头"],
  ["type", "文字"], ["blend", "模糊"], ["grid-3x3", "马赛克"]];
const history = [["undo-2", "撤销"], ["redo-2", "重做"]];
const extra = [["pin", "固定"], ["pin-off", "取消固定"], ["copy", "复制"], ["download", "保存"], ["menu", "菜单"], ["x", "关闭"]];

const mask = (file, size, color) =>
  `<img class="ic" src="${assets}/${file}.png" style="width:${size}px;height:${size}px;opacity:${color === "#A6A6AE" ? ".72" : "1"}">`;

const row = (items, size, color, cls = "") => `<div class="row ${cls}">` +
  items.map(([f, label]) => `<span class="item">${mask(f, size, color)}<em>${label}</em></span>`).join("") +
  `</div>`;

const html = `<!doctype html><meta charset="utf-8"><style>
  *{box-sizing:border-box}
  body{margin:0;width:1180px;background:#0b0b0d;color:#e9e9ec;
       font:11px/1.2 Inter,"Noto Sans CJK SC",sans-serif;padding:28px 32px}
  h2{font-size:12px;font-weight:600;letter-spacing:.08em;color:#8e8e97;margin:0 0 10px}
  h1{font-size:18px;font-weight:650;margin:0 0 4px}
  p{margin:0 0 24px;color:#8e8e97}
  .section{margin-bottom:26px}
  .row{display:flex;align-items:flex-end;gap:26px;background:#0e0e11;
       border:1px solid rgba(255,255,255,.10);border-radius:14px;padding:14px 20px;width:max-content}
  .row.wide{gap:34px}
  .item{display:flex;flex-direction:column;align-items:center;gap:7px;min-width:34px}
  .item em{font-style:normal;font-size:9.5px;color:#7d7d86;white-space:nowrap}
  .ic{display:block;-webkit-mask-repeat:no-repeat;-webkit-mask-position:center;-webkit-mask-size:contain;
      mask-repeat:no-repeat;mask-position:center;mask-size:contain}
  .bar{display:flex;align-items:center;gap:6px;padding:0 10px;height:48px;background:#0e0e11;
       border:1px solid rgba(255,255,255,.10);border-radius:14px;width:max-content}
  .mode{display:flex;align-items:center;gap:7px;padding:0 11px;height:30px;border-radius:8px;font-size:11.5px;color:#a6a6ae}
  .mode.primary{color:#f4f4f5;background:rgba(255,255,255,.07)}
  .mode.primary i{background:#A3F0CE}
  .sep{width:1px;height:18px;background:rgba(255,255,255,.12);margin:0 5px}
  .tool{display:grid;place-items:center;width:30px;height:30px;border-radius:8px}
  .tool:hover{background:rgba(255,255,255,.08)}
  .big .ic{opacity:.95}
  .big{display:flex;gap:30px;align-items:center}
  .big .item em{font-size:10.5px;color:#9a9aa2}
</style>
<h1>03 节点 · 实际尺寸预览</h1>
<p>深色面板、单色染色，按 Qt 实际使用尺寸 1:1 渲染（截图 2x DPR）。</p>

<div class="section"><h2>模式按钮 · 15px</h2>
  <div class="bar">
    ${mode.map(([f, l, p]) => `<span class="mode ${p ? "primary" : ""}">${mask(f, 15, p ? "#A3F0CE" : "#A6A6AE")}${l}</span>`).join("")}
    <span class="sep"></span>
    ${["rectangle-horizontal", "circle", "move-up-right", "type", "blend", "grid-3x3"].map(f => `<span class="tool">${mask(f, 17, "#D2D2D8")}</span>`).join("")}
    <span class="sep"></span>
    ${["undo-2", "redo-2"].map(f => `<span class="tool">${mask(f, 17, "#D2D2D8")}</span>`).join("")}
  </div>
</div>

<div class="section"><h2>工具栏 · 17px / 固定栏 · 16px</h2>
  ${row(tools, 17, "#D2D2D8")}
  <div style="height:12px"></div>
  ${row([...history, ...extra], 16, "#C9C9CF")}
</div>

<div class="section"><h2>标识与大尺寸</h2>
  <div class="row wide big">
    <span class="item">${mask("aperture", 14, "#B5F0D8")}<em>logo tile 14</em></span>
    <span class="item">${mask("aperture", 32, "#B5F0D8")}<em>logo 32</em></span>
    ${["scan", "focus", "monitor", "blend", "grid-3x3", "pin-off"].map(f => `<span class="item">${mask(f, 34, "#D2D2D8")}<em>${f}</em></span>`).join("")}
  </div>
</div>`;

const htmlPath = join(here, "html", "node-usage.html");
writeFileSync(htmlPath, html);

const browser = await chromium.launch({ args: ["--force-color-profile=srgb", "--font-render-hinting=none"] });
const page = await browser.newPage({ viewport: { width: 1180, height: 620 }, deviceScaleFactor: 2 });
await page.goto("file://" + htmlPath, { waitUntil: "load" });
await page.evaluate(() => document.fonts.ready);
await page.waitForTimeout(150);
await page.screenshot({ path: join(outDir, "03-node-usage.png"), fullPage: true });
console.log("shot 03-node-usage.png");
await browser.close();
