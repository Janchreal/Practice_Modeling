import fs from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { FileBlob, PresentationFile } from "@oai/artifact-tool";

const workspaceDir = "D:\\ProgramFiles\\Practice_Modeling_47";
const SKILL_DIR = "C:\\Users\\陈景煌\\.codex\\plugins\\cache\\openai-primary-runtime\\presentations\\26.909.12148\\skills\\presentations";
const RUNTIME_PYTHON = "C:\\Users\\陈景煌\\.cache\\codex-runtimes\\codex-primary-runtime\\dependencies\\python\\python.exe";
const RUNTIME_NODE_MODULES = "C:\\Users\\陈景煌\\.cache\\codex-runtimes\\codex-primary-runtime\\dependencies\\node\\node_modules";
const RUNTIME_BIN_DIR = "C:\\Users\\陈景煌\\.cache\\codex-runtimes\\codex-primary-runtime\\dependencies\\bin\\override";
const sourceTemplatePath = "C:\\Users\\陈景煌\\Desktop\\资料\\汇报PPT\\2026.8.25.pptx";
const sourceTemplateSha256 = "bba7dcf6085538dab7a4536512d4fa3e9d47246e5d5ee0a050396df412b1aece";
const outputDir = path.join(workspaceDir, "outputs");
const stagingDir = path.join(workspaceDir, ".codex-finalizer");
const renderDir = path.join(workspaceDir, ".codex-ppt-build", "final-renders");

await fs.mkdir(outputDir, { recursive: true });
await fs.mkdir(stagingDir, { recursive: true });
await fs.mkdir(renderDir, { recursive: true });
process.env.RUNTIME_NODE_MODULES = RUNTIME_NODE_MODULES;
process.env.RUNTIME_BIN_DIR = RUNTIME_BIN_DIR;

async function uniqueOutputPath(baseName) {
  for (let i = 1; i < 100; i += 1) {
    const suffix = i === 1 ? "" : `_v${i}`;
    const candidate = path.join(outputDir, `${baseName}${suffix}.pptx`);
    try {
      await fs.access(candidate);
    } catch {
      return candidate;
    }
  }
  throw new Error("Unable to find a free output filename");
}

const FINAL_PPTX = await uniqueOutputPath("weekly_report_2026-09-07_09-13");
const presentation = await PresentationFile.importPptx(await FileBlob.load(sourceTemplatePath));

const BLUE = "#2F477F";
const LIGHT_BLUE = "#5A77C8";
const TABLE_BLUE = "#5B8FDC";
const TABLE_LIGHT = "#D9E4F6";
const TABLE_ALT = "#EEF3FB";
const TEXT = "#111111";
const MUTED = "#3A3A3A";
const CODE_BG = "#F6F8FC";
const CODE_LINE = "#C9D3EA";
const FONT = "微软雅黑";
const COVER_FONT = "华文新魏";

function parseRecords(ndjson) {
  return ndjson
    .split(/\r?\n/)
    .filter(Boolean)
    .map((line) => JSON.parse(line));
}

async function inspectRecords() {
  const snapshot = await presentation.inspect({
    kind: "slide,textbox,shape,image,table,chart,notes",
    include: "id,slide,name,title,text,textPreview,textChars,textLines,bbox,bboxUnit,rows,cols,chartType",
    maxChars: 120000,
  });
  return parseRecords(snapshot.ndjson);
}

function slide(num) {
  return presentation.slides.getItem(num - 1);
}

function bbox(record) {
  return Array.isArray(record.bbox) ? record.bbox : [0, 0, 0, 0];
}

function setTextStyle(shape, options = {}) {
  shape.text.style = {
    typeface: options.typeface ?? FONT,
    fontSize: options.fontSize ?? 24,
    color: options.color ?? TEXT,
    bold: options.bold ?? false,
    autoFit: options.autoFit ?? "shrinkText",
  };
}

function addText(sl, text, position, options = {}) {
  const shape = sl.shapes.add({
    geometry: "textbox",
    name: options.name,
    position,
    fill: "none",
    line: { fill: "none", width: 0 },
  });
  shape.text = text;
  setTextStyle(shape, options);
  return shape;
}

function addSection(sl, number, title, top) {
  addText(sl, `${number} ${title}`, { left: 136, top, width: 1040, height: 36 }, {
    name: `section-${number}`,
    fontSize: 29,
    color: LIGHT_BLUE,
    bold: false,
  });
}

function addBody(sl, text, top, height, options = {}) {
  return addText(sl, text, {
    left: options.left ?? 150,
    top,
    width: options.width ?? 1010,
    height,
  }, {
    name: options.name ?? "body",
    fontSize: options.fontSize ?? 23,
    color: options.color ?? TEXT,
    bold: options.bold ?? false,
  });
}

function addCode(sl, text, position, options = {}) {
  const box = sl.shapes.add({
    geometry: "rect",
    name: options.name ?? "code-box",
    position,
    fill: CODE_BG,
    line: { style: "solid", fill: CODE_LINE, width: 1 },
  });
  box.text = text;
  box.text.style = {
    typeface: "Arial",
    fontSize: options.fontSize ?? 16,
    color: "#26344D",
    autoFit: "shrinkText",
  };
  return box;
}

function addMiniLabel(sl, text, position, options = {}) {
  const shape = sl.shapes.add({
    geometry: "rect",
    name: options.name,
    position,
    fill: options.fill ?? "#ECF2FB",
    line: { style: "solid", fill: "#B7C6E6", width: 1 },
  });
  shape.text = text;
  shape.text.style = {
    typeface: FONT,
    fontSize: options.fontSize ?? 18,
    bold: true,
    color: BLUE,
    autoFit: "shrinkText",
  };
  return shape;
}

function addConnector(sl, left, top, width) {
  sl.shapes.add({
    geometry: "line",
    name: "process-line",
    position: { left, top, width, height: 0 },
    fill: "none",
    line: { style: "solid", fill: BLUE, width: 2, headEnd: { type: "triangle" } },
  });
}

function styleTable(table, rows, cols) {
  table.borders.assign({ style: "solid", fill: "#FFFFFF", width: 1 });
  for (let r = 0; r < rows; r += 1) {
    for (let c = 0; c < cols; c += 1) {
      const cell = table.getCell(r, c);
      cell.fill = r === 0 ? TABLE_BLUE : (r % 2 ? TABLE_LIGHT : TABLE_ALT);
      cell.text.style = {
        typeface: FONT,
        fontSize: r === 0 ? 20 : 18,
        bold: r === 0,
        color: r === 0 ? "#FFFFFF" : "#111111",
        autoFit: "shrinkText",
      };
    }
  }
}

function addTable(sl, values, position, columnWidths) {
  const table = sl.tables.add({
    rows: values.length,
    columns: values[0].length,
    left: position.left,
    top: position.top,
    width: position.width,
    height: position.height,
    values,
    columnWidths,
  });
  table.styleOptions = { headerRow: true, bandedRows: true };
  styleTable(table, values.length, values[0].length);
  return table;
}

function keepImage(record) {
  const [left, top, width, height] = bbox(record);
  if (width >= 1200 && height >= 650) return true;
  if (left < 40 && top > 360 && width > 1100 && height > 80) return true;
  if (top > 545 && width > 520 && height > 80) return true;
  return false;
}

function keepContentTitle(record) {
  const [left, top, width, height] = bbox(record);
  return left > 70 && left < 120 && top > 35 && top < 70 && width > 350 && height > 35;
}

async function cleanContentSlides() {
  const records = await inspectRecords();
  for (const record of records) {
    if (record.slide < 3 || record.slide > 10) continue;
    const [left, top, width, height] = bbox(record);
    let keep = false;
    if (record.kind === "textbox") keep = keepContentTitle(record);
    if (record.kind === "shape") keep = top > 660 && width > 900 && height < 60;
    if (record.kind === "image") keep = keepImage(record);
    if (record.kind === "notes" || record.kind === "slide") keep = true;
    if (!keep && ["textbox", "shape", "image", "table", "chart"].includes(record.kind)) {
      try {
        const target = presentation.resolve(record.id);
        if (typeof target.delete === "function") {
          target.delete();
        } else if (target.data) {
          target.data.hidden = true;
        }
      } catch (error) {
        console.warn(`Could not delete ${record.kind} ${record.id}: ${error.message}`);
      }
    }
  }
}

async function setExistingSlideText() {
  const records = await inspectRecords();

  for (const record of records) {
    if (record.slide === 1 && record.kind === "textbox" && record.textPreview === "周汇报") {
      const target = presentation.resolve(record.id);
      target.text = "周汇报";
      target.text.style = { typeface: COVER_FONT, fontSize: 50, color: BLUE, bold: false };
    }
    if (record.slide === 1 && record.kind === "textbox" && record.textPreview?.includes("汇报人")) {
      const target = presentation.resolve(record.id);
      target.text = "✪汇报人：陈景煌    2026.9.7-9.13";
      target.text.style = { typeface: FONT, fontSize: 19, color: BLUE, bold: false };
    }
    if (record.slide === 11 && record.kind === "textbox" && record.textPreview === "周汇报") {
      const target = presentation.resolve(record.id);
      target.text = "周汇报";
      target.text.style = { typeface: COVER_FONT, fontSize: 50, color: BLUE, bold: false };
    }
    if (record.slide === 11 && record.kind === "textbox" && record.textPreview?.includes("汇报人")) {
      const target = presentation.resolve(record.id);
      target.text = "✪汇报人：陈景煌";
      target.text.style = { typeface: FONT, fontSize: 20, color: BLUE, bold: false };
    }
  }

  const directory = records
    .filter((record) => record.slide === 2 && record.kind === "textbox")
    .filter((record) => /^MH_Entry/.test(record.name ?? ""))
    .sort((a, b) => bbox(a)[1] - bbox(b)[1]);
  const entries = ["手柄功能", "矢量工具整理", "渲染问题", "总结与下周计划"];
  for (let i = 0; i < Math.min(directory.length, entries.length); i += 1) {
    const target = presentation.resolve(directory[i].id);
    target.text = entries[i];
    target.text.style = { typeface: FONT, fontSize: 24, color: BLUE, bold: false };
  }
}

function setContentTitle(num, title) {
  const sl = slide(num);
  const titleShape = sl.shapes.items.find((shape) => {
    const pos = shape.position ?? {};
    return pos.left > 70 && pos.left < 120 && pos.top > 35 && pos.top < 80;
  });
  if (titleShape) {
    titleShape.text = title;
    titleShape.text.style = { typeface: FONT, fontSize: 33, color: BLUE, bold: false };
  } else {
    addText(sl, title, { left: 95, top: 46, width: 760, height: 55 }, {
      name: "slide-title",
      fontSize: 33,
      color: BLUE,
    });
  }
}

function notes(num, text) {
  slide(num).speakerNotes.textFrame.setText(text);
}

await cleanContentSlides();
await setExistingSlideText();

setContentTitle(3, "01 本周工作总览");
addSection(slide(3), "1.1", "主要工作：完成手柄交互和显示渲染问题的收敛", 112);
addBody(slide(3), [
  "① 两点确定矢量：补齐起点、终点点柄，支持选中、拖拽和捕捉吸附",
  "② 矢量方向控制：矢量箭头支持双击反向，对话框状态和预览同步更新",
  "③ 工具归档：矢量捕捉与矢量手柄集中到 interaction/tools，减少主窗口耦合",
  "④ 渲染链路：从业务层手动构建 Mesh，收敛到 IVtk 统一生成显示数据",
].join("\n"), 166, 170);
addTable(slide(3), [
  ["提交", "本周主题", "结果"],
  ["80c2250", "点选反馈与两点矢量交互", "Release 构建通过并推送"],
  ["de28007", "显示三角化交给 IVtk", "移除业务层显式 Mesh 调用"],
], { left: 150, top: 382, width: 970, height: 168 }, [190, 390, 390]);
notes(3, "依据：C:\\Users\\陈景煌\\Desktop\\9.7-9.13.txt；git show --stat 80c2250；git show --stat de28007。");

setContentTitle(4, "02 两点确定矢量：点操作手柄");
addSection(slide(4), "2.1", "主要工作：两个点柄承担起点和终点的选择与拖拽", 112);
addBody(slide(4), [
  "① 起点、终点使用球形 actor 表示，鼠标按下先判断是否命中点柄",
  "② 拖拽时把屏幕位移投影到视图平面，再结合点捕捉器求新的三维坐标",
  "③ 拖拽过程中实时刷新矢量预览、对话框参数和参考覆盖层尺寸",
  "④ 点选完成后立即同步 overlay 相机并 Render，解决高亮延迟一拍的问题",
].join("\n"), 166, 170);
addCode(slide(4), [
  "handleVectorTwoPointHandleMouseDown(x, y)",
  "handleVectorTwoPointHandleMouseMove(x, y)",
  "handleVectorTwoPointHandleMouseUp(x, y)",
  "",
  "showSelectedPoint(point)",
  "refreshPointSelectionOverlay()",
].join("\n"), { left: 188, top: 400, width: 840, height: 150 }, { fontSize: 18 });
notes(4, "依据：src\\interaction\\tools\\vector_handles.cpp；src\\interaction\\tools\\vector_snap.cpp；src\\viewport\\main_view\\main_viewport.cpp。");

setContentTitle(5, "03 两点确定矢量：选中与反向");
addSection(slide(5), "3.1", "主要工作：点柄可选中，矢量箭头支持双击反向", 112);
addBody(slide(5), [
  "① 选中：通过 picker 判断 start/end 点柄，进入拖拽或确认状态",
  "② 候选点：边附近只显示端点、中点候选，未进入激活半径时不落点",
  "③ 反向：双击矢量箭头后交换起点和终点，当前矢量方向同步翻转",
  "④ 同步：反向后刷新 dialog、预览 actor 和已选点反馈，避免状态分裂",
].join("\n"), 166, 190);
addMiniLabel(slide(5), "点柄命中", { left: 190, top: 436, width: 175, height: 48 });
addConnector(slide(5), 365, 460, 120);
addMiniLabel(slide(5), "拖拽更新", { left: 490, top: 436, width: 175, height: 48 });
addConnector(slide(5), 665, 460, 120);
addMiniLabel(slide(5), "参数同步", { left: 790, top: 436, width: 175, height: 48 });
addCode(slide(5), "handleVectorTwoPointArrowDoubleClick(x, y)\nreverseVectorTwoPointDirection()", {
  left: 250,
  top: 530,
  width: 700,
  height: 72,
}, { fontSize: 18 });
notes(5, "依据：src\\interaction\\tools\\vector_handles.cpp；src\\presentation\\main_window\\main_window_events.cpp；src\\viewport\\main_view\\mouse_interactor.cpp。");

setContentTitle(6, "04 其他矢量确定方式");
addSection(slide(6), "4.1", "主要工作：把不同矢量来源纳入同一种反向语义", 112);
addBody(slide(6), [
  "① 曲线上矢量：点柄沿曲线移动，矢量取该点的切线方向",
  "② 面和平面法向量：箭头代表当前法向，双击后方向翻转",
  "③ 轴矢量：从基准轴或模型轴获得方向，复用箭头反向入口",
  "④ 反向只改变方向符号，不改变点、边、面等几何来源",
].join("\n"), 166, 184);
addTable(slide(6), [
  ["矢量来源", "操作柄", "反向方式"],
  ["两点", "起点球、终点球、方向箭头", "双击方向箭头"],
  ["曲线", "曲线点柄、方向箭头", "沿切线取向后翻转"],
  ["面/平面/轴", "方向箭头", "复用同一反向状态"],
], { left: 150, top: 392, width: 980, height: 210 }, [220, 420, 340]);
notes(6, "依据：C:\\Users\\陈景煌\\Desktop\\9.7-9.13.txt；src\\presentation\\dialogs\\tools\\vector_dialog.cpp；src\\presentation\\dialogs\\coordinates\\datum_axis.cpp。");

setContentTitle(7, "05 矢量工具整理");
addSection(slide(7), "5.1", "主要工作：矢量工具从主窗口逻辑中单独抽出管理", 112);
addBody(slide(7), [
  "① 主窗口只负责接收交互结果和触发刷新，工具细节下沉到 src/interaction/tools",
  "② 捕捉、高亮、拖拽和双击反向分别有清晰入口，后续维护更容易定位",
].join("\n"), 166, 95);
addTable(slide(7), [
  ["文件", "当前责任", "汇报重点"],
  ["vector_snap.cpp", "点捕捉、候选点预览、点选反馈", "解决选中后高亮延迟"],
  ["vector_handles.cpp", "两点矢量点柄、箭头手柄、拖拽和双击", "完成可操作手柄"],
  ["handle_spec.*", "手柄规则、形状、状态描述", "保留规则声明层"],
  ["main_view/*", "鼠标事件转发和 overlay 刷新", "统一交互入口"],
], { left: 120, top: 300, width: 1040, height: 270 }, [250, 485, 305]);
notes(7, "依据：rg --files；Practice_Modeling.vcxproj 中 vector_snap.cpp 和 vector_handles.cpp 的编译项；80c2250 提交统计。");

setContentTitle(8, "06 渲染问题：自动显示管线");
addSection(slide(8), "6.1", "总工作：从手动构建转为 IVtk 自动构建", 112);
addBody(slide(8), [
  "① 旧链路：显示、预览、高亮和镜像各处都可能手动构建 Mesh",
  "② 问题：球与圆柱、圆锥交界处容易出现锯齿状突出角，调参也容易反复",
  "③ 新链路：所有显示入口先创建 ShapeDataSource，再由显示管线生成 VTK 数据",
  "④ 结果：业务层不再直接调用 BRepMesh_IncrementalMesh，质量策略集中到渲染层",
].join("\n"), 166, 192);
addCode(slide(8), [
  "TopoDS_Shape",
  "IVtkOCC_Shape",
  "ModelShapePipeline::createShapeDataSource()",
  "ModelShapePipeline::copyShapeDataSourceOutput()",
  "Mapper / Actor / Render",
].join("\n"), { left: 226, top: 430, width: 760, height: 138 }, { fontSize: 18 });
notes(8, "依据：src\\rendering\\pipeline\\model_shape_pipeline.cpp；src\\rendering\\model\\shape_presentation_factory.cpp；de28007 提交统计。");

setContentTitle(9, "07 BRepMesh_IncrementalMesh");
addSection(slide(9), "7.1", "是什么：OCCT 用于把 B-Rep 形状离散成三角网格的工具", 112);
addTable(slide(9), [
  ["问题", "回答"],
  ["控制什么", "线性偏差、角度偏差、相对偏差模式以及是否并行构建"],
  ["为什么之前会关注它", "球、圆柱、圆锥等曲面靠三角化显示，偏差会影响交界处观感"],
  ["为什么现在业务层不需要它", "项目显示入口统一走 IVtk/OCCT 显示数据源，业务逻辑不再散落 Mesh 调用"],
  ["仍要注意什么", "TKMesh 仍是 OCCT/IVtk 的模块依赖，不能因为源码无显式调用就删除链接"],
], { left: 116, top: 176, width: 1050, height: 310 }, [250, 800]);
addCode(slide(9), [
  "BRepMesh_IncrementalMesh(shape, linDeflection,",
  "                         isRelative, angDeflection,",
  "                         isInParallel)",
].join("\n"), { left: 245, top: 528, width: 690, height: 82 }, { fontSize: 17 });
notes(9, "依据：D:\\OCCT7.7.0\\Install\\inc\\BRepMesh_IncrementalMesh.hxx；src 中对 BRepMesh_IncrementalMesh 的全局搜索；de28007 最终说明。");

setContentTitle(10, "08 问题处理与下周计划");
addSection(slide(10), "8.1", "遇到的问题：显示质量、刷新时机和二进制依赖交织在一起", 112);
addBody(slide(10), [
  "① 点选反馈延迟：选中点后 actor 已创建，但 overlay 相机和屏幕缩放没有立即同步",
  "② 交界处锯齿：显示边界暴露曲面离散误差，球与圆柱、圆锥组合更明显",
  "③ 构建依赖：曾出现 Release 程序加载 Debug VTK 的情况，需要保持 TKIVtk 与 VTK 版本一致",
].join("\n"), 166, 145, { fontSize: 22 });
addSection(slide(10), "8.2", "下周计划：围绕测试文档继续收敛交互稳定性", 350);
addBody(slide(10), [
  "① 回归测试两点矢量、曲线矢量、面法向和轴矢量的反向逻辑",
  "② 检查预览、确认、撤销恢复、保存读取后的矢量状态是否一致",
  "③ 继续减少主窗口中的交互细节，让工具层和渲染层边界更清楚",
].join("\n"), 405, 138, { fontSize: 22 });
notes(10, "依据：C:\\Users\\陈景煌\\Desktop\\9.7-9.13.txt；过往任务记录中关于点选高亮、球/圆柱交界锯齿、TKIVtk 与 VTK 依赖排查的说明。");

const expectedSlideSizeEmu = "12192000,6858000";
const fontPolicy = {
  basis: "reference",
  families: ["微软雅黑", "华文新魏", "Arial"],
  referencePath: sourceTemplatePath,
  referenceSha256: sourceTemplateSha256,
};
const requirements = {
  explicitTotalSlideCount: 11,
  requiredNativeTableOwnerSlides: [],
  requiredNativeChartOwnerSlides: [],
};

const candidatePath = path.join(stagingDir, "candidate_weekly_report.pptx");
await (await PresentationFile.exportPptx(presentation)).save(candidatePath);

const { finalizePresentation } = await import(pathToFileURL(
  path.join(SKILL_DIR, "container_tools", "artifact_tool_utils.mjs"),
).href);

const result = await finalizePresentation({
  ...requirements,
  workspaceDir,
  candidatePath,
  finalPath: FINAL_PPTX,
  pythonExecutable: RUNTIME_PYTHON,
  integrityValidatorPath: path.join(SKILL_DIR, "container_tools", "inspect_presentation_package_integrity.py"),
  layoutValidatorPath: path.join(SKILL_DIR, "container_tools", "inspect_presentation_layout_geometry.py"),
  layoutArgs: [
    "--expected-slide-size-emu", expectedSlideSizeEmu,
    "--validate-bullet-geometry",
    "--validate-heading-fit",
  ],
  fontPolicy,
  verifyArtifactToolImport: true,
  receiptPath: path.join(stagingDir, `${path.basename(FINAL_PPTX)}.validation.json`),
});

const finalDeck = await PresentationFile.importPptx(await FileBlob.load(FINAL_PPTX));
for (let i = 0; i < finalDeck.slides.items.length; i += 1) {
  const preview = await finalDeck.slides.getItem(i).export({ format: "png", scale: 1 });
  await fs.writeFile(
    path.join(renderDir, `weekly-report-slide-${String(i + 1).padStart(2, "0")}.png`),
    new Uint8Array(await preview.arrayBuffer()),
  );
}
const montage = await finalDeck.export({ format: "webp", montage: true, scale: 0.6 });
await fs.writeFile(
  path.join(renderDir, "weekly-report-montage.webp"),
  new Uint8Array(await montage.arrayBuffer()),
);

const finalSnapshot = await finalDeck.inspect({
  kind: "slide,textbox,table,notes",
  include: "slide,title,textPreview,rows,cols",
  maxChars: 60000,
});
await fs.writeFile(path.join(renderDir, "weekly-report-final.inspect.ndjson"), finalSnapshot.ndjson, "utf8");

console.log(JSON.stringify({
  finalPath: FINAL_PPTX,
  slideCount: finalDeck.slides.items.length,
  validation: result,
}, null, 2));
