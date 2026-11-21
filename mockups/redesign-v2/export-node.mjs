/* Export concept 03 (Node Graph) as 96x96 transparent PNGs. */
import pw from "/tmp/shot/node_modules/playwright-core/index.js";
const { chromium } = pw;
import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import concepts from "./concepts-a.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const outDir = join(here, "png", "node-assets");
mkdirSync(outDir, { recursive: true });

const c03 = concepts.find(c => c.id === "03");
if (!c03) throw new Error("concept 03 not found");

const n = (x, y, r = 1.35) =>
  `<circle cx="${x}" cy="${y}" r="${r}" fill="currentColor" stroke="none"/>`;

/* App-only icons, authored in the same node-graph grammar. */
const extras = {
  // Blur: a bokeh node field, no connecting lines.
  blend: [
    [6, 6, 0.55], [12, 6, 0.8], [18, 6, 0.55],
    [6, 12, 0.8], [12, 12, 1.45], [18, 12, 0.8],
    [6, 18, 0.55], [12, 18, 0.8], [18, 18, 0.55],
  ].map(([x, y, r]) => n(x, y, r)).join(""),

  // Pin off: the pin is knocked out around a 45-degree terminal.
  "pin-off": `<defs>
      <mask id="pinOffCut" maskUnits="userSpaceOnUse" x="0" y="0" width="24" height="24">
        <rect width="24" height="24" fill="#fff"/>
        <path d="M3.6 3.6 20.4 20.4" stroke="#000" stroke-width="3.4"/>
      </mask>
    </defs>
    <g mask="url(#pinOffCut)">
      <path d="M12 20.5V17M9 10.6V7h6v3.6l2 2V16H7v-3.4z"/>
      ${n(12, 20.5)}${n(9, 10.6)}${n(15, 10.6)}${n(7, 16)}${n(17, 16)}${n(12, 12)}
    </g>
    <path d="M4.6 4.6 19.4 19.4"/>
    ${n(4.6, 4.6)}${n(19.4, 19.4)}`,

  // Close: two 45-degree terminals meeting at a centre node.
  x: `<path d="M6.5 6.5 17.5 17.5M17.5 6.5 6.5 17.5"/>
    ${n(6.5, 6.5)}${n(17.5, 6.5)}${n(6.5, 17.5)}${n(17.5, 17.5)}${n(12, 12, 1.15)}`,
};

const MINT = "#B5F0D8";
const INK = "#FFFFFF";

const mapping = [
  ["aperture", "logo", MINT],
  ["scan", "region", INK],
  ["focus", "window", INK],
  ["monitor", "screen", INK],
  ["rectangle-horizontal", "rect", INK],
  ["circle", "circle", INK],
  ["move-up-right", "arrow", INK],
  ["type", "type", INK],
  ["blend", "blend", INK],
  ["grid-3x3", "grid", INK],
  ["undo-2", "undo", INK],
  ["redo-2", "redo", INK],
  ["pin", "pin", INK],
  ["pin-off", "pin-off", INK],
  ["copy", "copy", INK],
  ["download", "save", INK],
  ["menu", "menu", INK],
  ["x", "x", INK],
];

const a = c03.svg || {};
const svgFor = (body, color) =>
  `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24"
     fill="${a.fill || "none"}" stroke="${a.stroke || "currentColor"}"
     stroke-width="${a.sw ?? 1.1}" stroke-linecap="${a.caps || "butt"}"
     stroke-linejoin="${a.join || "miter"}" style="color:${color}">${body}</svg>`;

const cells = mapping.map(([file, key, color]) => {
  const body = c03.icons[key] ?? extras[key];
  if (!body) throw new Error(`missing icon: ${key}`);
  return `<div class="cell" data-file="${file}">${svgFor(body, color)}</div>`;
}).join("\n");

const html = `<!doctype html><meta charset="utf-8"><style>
  html,body{margin:0;background:transparent}
  .cell{width:24px;height:24px;display:block}
  svg{display:block}
</style>${cells}`;

const htmlPath = join(here, "html", "node-assets.html");
writeFileSync(htmlPath, html);

const browser = await chromium.launch({ args: ["--force-color-profile=srgb", "--font-render-hinting=none"] });
const page = await browser.newPage({ viewport: { width: 400, height: 400 }, deviceScaleFactor: 4 });
await page.goto("file://" + htmlPath, { waitUntil: "load" });
await page.waitForTimeout(120);

for (const el of await page.locator(".cell").all()) {
  const file = await el.getAttribute("data-file");
  await el.screenshot({ path: join(outDir, file + ".png"), omitBackground: true });
  console.log("export", file + ".png");
}
await browser.close();
