import { FileBlob, PresentationFile } from "@oai/artifact-tool";

const presentation = await PresentationFile.importPptx(await FileBlob.load("C:\\Users\\陈景煌\\Desktop\\资料\\汇报PPT\\2026.8.25.pptx"));
const snapshot = await presentation.inspect({ kind: "table", maxChars: 12000 });
const first = snapshot.ndjson.split(/\r?\n/).filter(Boolean).map((line) => JSON.parse(line))[0];
const table = presentation.resolve(first.id);

console.log({
  anchor: first.id,
  ownKeys: Object.keys(table),
  protoKeys: Object.getOwnPropertyNames(Object.getPrototypeOf(table)),
  frame: table.frame,
  position: table.position,
  left: table.left,
  top: table.top,
  width: table.width,
  height: table.height,
  hasDelete: typeof table.delete,
  dataKeys: Object.keys(table.data ?? {}),
  placement: table.data?.placement,
});
