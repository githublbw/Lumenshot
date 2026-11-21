import pw from "/tmp/shot/node_modules/playwright-core/index.js";
const { chromium } = pw;
import { readdirSync, mkdirSync } from "node:fs";
import { dirname, join, basename } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const htmlDir = join(here, "html");
const outDir = join(here, "png");
mkdirSync(outDir, { recursive: true });

const files = readdirSync(htmlDir).filter(f => f.endsWith(".html")).sort();
const only = process.argv[2];

const browser = await chromium.launch({ args: ["--force-color-profile=srgb", "--font-render-hinting=none"] });
const page = await browser.newPage({ viewport: { width: 1600, height: 1180 }, deviceScaleFactor: 1.5 });

for (const f of files) {
  if (only && !f.startsWith(only)) continue;
  const url = pathToFileURL(join(htmlDir, f)).href;
  await page.goto(url, { waitUntil: "load" });
  await page.evaluate(() => document.fonts.ready);
  await page.waitForTimeout(180);
  const name = basename(f, ".html") + ".png";
  await page.locator(".board, .ov").screenshot({ path: join(outDir, name) });
  console.log("shot", name);
}
await browser.close();
