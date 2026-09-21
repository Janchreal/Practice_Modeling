import fs from "node:fs/promises";
import path from "node:path";
import { FileBlob, PresentationFile } from "@oai/artifact-tool";

const outDir = path.resolve(".codex-ppt-build", "reference-inspection");
await fs.mkdir(outDir, { recursive: true });

const decks = [
  {
    key: "2026-08-25",
    source: "C:\\Users\\陈景煌\\Desktop\\资料\\汇报PPT\\2026.8.25.pptx",
  },
  {
    key: "2026-08-10",
    source: "C:\\Users\\陈景煌\\Desktop\\资料\\汇报PPT\\2026.8.10.pptx",
  },
];

for (const deck of decks) {
  const presentation = await PresentationFile.importPptx(await FileBlob.load(deck.source));
  const snapshot = await presentation.inspect({
    kind: "deck,slide,textbox,shape,image,table,chart,layout,notes",
    include: "id,slide,name,title,textPreview,textChars,textLines,bbox,bboxUnit,rows,cols,chartType,alt,isPlaceholder,placeholders",
    maxChars: 50000,
  });
  await fs.writeFile(path.join(outDir, `${deck.key}.inspect.ndjson`), snapshot.ndjson, "utf8");

  const montage = await presentation.export({ format: "webp", montage: true, scale: 0.6 });
  await fs.writeFile(
    path.join(outDir, `${deck.key}.montage.webp`),
    new Uint8Array(await montage.arrayBuffer()),
  );

  const slideCount = presentation.slides.items.length;
  const interesting = new Set([
    0,
    1,
    2,
    Math.max(0, slideCount - 2),
    Math.max(0, slideCount - 1),
  ]);
  for (const index of interesting) {
    const slide = presentation.slides.getItem(index);
    const preview = await slide.export({ format: "png", scale: 1 });
    await fs.writeFile(
      path.join(outDir, `${deck.key}.slide-${index + 1}.png`),
      new Uint8Array(await preview.arrayBuffer()),
    );
  }

  console.log(`${deck.key}: ${slideCount} slides`);
}
