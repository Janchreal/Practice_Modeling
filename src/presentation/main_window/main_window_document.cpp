// 文档：新建/打开/保存、最近文件、自动恢复
#include "main_window.h"
#include "ui_main_window.h"
#include "infrastructure/serialization/feature_serializer.h"
#include "infrastructure/serialization/model_document_serializer.h"
#include "geometry/primitives/primitive_build_request.h"

#include <algorithm>
#include <sstream>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>

#include <QAction>
#include <QCloseEvent>
#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QStatusBar>
#include <QVBoxLayout>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

namespace {

static QColor colorFromJson(const QJsonObject& o, const QColor& fallback = QColor(180, 180, 180))
{
    if (!o.contains("r") || !o.contains("g") || !o.contains("b")) {
        return fallback;
    }
    return QColor(o.value("r").toInt(fallback.red()),
                  o.value("g").toInt(fallback.green()),
                  o.value("b").toInt(fallback.blue()),
                  o.value("a").toInt(fallback.alpha()));
}

} // namespace

void Widget::updateDocumentWindowTitle()
{
    const QString docName = currentDocumentPath_.isEmpty()
        ? QStringLiteral("未命名")
        : QFileInfo(currentDocumentPath_).fileName();
    const QString title = QStringLiteral("Practice Modeling - %1%2")
        .arg(docName, documentModified_ ? QStringLiteral(" *") : QString());
    setWindowTitle(title);
}

void Widget::markDocumentModified(bool modified)
{
    if (isLoadingDocument_) return;
    if (documentModified_ == modified) return;
    documentModified_ = modified;
    updateDocumentWindowTitle();
}

bool Widget::maybeSaveDocument()
{
    if (!documentModified_) return true;

    QMessageBox::StandardButton choice = QMessageBox::warning(
        this,
        tr("未保存更改"),
        tr("当前模型已修改，是否先保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (choice == QMessageBox::Cancel) return false;
    if (choice == QMessageBox::Discard) return true;
    return saveDocument(false);
}

bool Widget::saveDocument(bool forceSaveAs)
{
    QString filePath = currentDocumentPath_;
    if (forceSaveAs || filePath.isEmpty()) {
        filePath = QFileDialog::getSaveFileName(
            this,
            tr("保存模型"),
            currentDocumentPath_.isEmpty() ? QStringLiteral("untitled.pmx") : currentDocumentPath_,
            tr("Practice Modeling 文件 (*.pmx);;JSON 文件 (*.json)"));
        if (filePath.isEmpty()) return false;
    }

    if (saveDocumentToPath(filePath)) {
        currentDocumentPath_ = filePath;
        markDocumentModified(false);
        addRecentFile(filePath);
        removeAutoRecoverySnapshot();
        if (statusBar()) statusBar()->showMessage(tr("已保存: %1").arg(QFileInfo(filePath).fileName()), 3000);
        return true;
    }
    return false;
}

bool Widget::saveDocumentToPath(const QString& filePath)
{
    QJsonObject root;
    // v3：增加特征配方持久化，支持完整建模历史流
    root["schemaVersion"] = 3;
    root["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["models"] = modelDocumentToJson(
        modelDocument_,
        [this](const ModelingHistory& record) { return geometryStateFor(record).occShape; },
        [this](const ModelingHistory& record) {
            const ModelRenderState& state = renderStateFor(record);
            return state.actor ? state.actor->GetVisibility() != 0 : true;
        });

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("保存失败"), tr("无法写入文件:\n%1").arg(filePath));
        return false;
    }

    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        QMessageBox::critical(this, tr("保存失败"), tr("写入提交失败:\n%1").arg(filePath));
        return false;
    }
    return true;
}

void Widget::clearAllModelsInternal()
{
    for (int i = 0; i < historyList.size(); ++i) {
        if (renderStateFor(historyList[i]).actor) removeSceneActor(renderStateFor(historyList[i]).actor);
        if (renderStateFor(historyList[i]).outlineActor) removeSceneActor(renderStateFor(historyList[i]).outlineActor);
        if (renderStateFor(historyList[i]).highlightActor) removeSceneActor(renderStateFor(historyList[i]).highlightActor);
        if (renderStateFor(historyList[i]).profilePickActor) removeSceneActor(renderStateFor(historyList[i]).profilePickActor);
    }

    clearIntersectionRenderStates();
    clearReferenceCsysState();
    modelDocument_.clear();
    geometryStore_.clear();
    renderStore_.clear();
    currentSelectedIndex = -1;
    selectedTargetIndex = -1;
    selectedToolIndices.clear();
    currentSelectionMode = None;
    extrusionSelectedIndices.clear();

    commandManager_.clear();
    emit undoStateChanged(false, false);

    updateHistoryList();
    updateFeatureTree();
    if (vtkWidget && vtkWidget->renderWindow()) vtkWidget->renderWindow()->Render();
}

bool Widget::loadDocumentFromPath(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        recentFiles_.removeAll(QFileInfo(filePath).absoluteFilePath());
        saveRecentFilesToSettings();
        refreshRecentFilesUi();
        rebuildOpenRecentMenu();
        QMessageBox::critical(this, tr("打开失败"), tr("无法读取文件:\n%1").arg(filePath));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::critical(this, tr("打开失败"), tr("文件格式无效:\n%1").arg(parseError.errorString()));
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonArray models = root.value("models").toArray();

    isLoadingDocument_ = true;
    clearAllModelsInternal();

    int skippedCount = 0;
    for (const QJsonValue& v : models) {
        if (!v.isObject()) continue;
        const QJsonObject m = v.toObject();
        const ModelType type = static_cast<ModelType>(m.value("type").toInt(-1));
        const QString name = m.value("name").toString();
        const QColor color = colorFromJson(m.value("color").toObject(), QColor(180, 180, 180));
        const double p1 = m.value("param1").toDouble();
        const double p2 = m.value("param2").toDouble();
        const double p3 = m.value("param3").toDouble();
        const bool visible = m.value("visible").toBool(true);
        const auto placement = modelPlacementFromJson(m);

        // v2：优先按 BREP 恢复 OCC 形状（支持拉伸/旋转/布尔/倒角/挖空等）
        const QString brepText = m.value("brep").toString();
        if (!brepText.isEmpty()) {
            TopoDS_Shape restored;
            BRep_Builder builder;
            std::istringstream iss(brepText.toStdString());
            // OCCT 不同版本的 BRepTools::Read 返回类型可能是 void 或 bool，
            // 这里统一用 restored 是否为空来判断结果。
            BRepTools::Read(restored, iss, builder);
            if (!restored.IsNull()) {
                const int insertAt = historyList.size();
                restoreModel(insertAt, name, type, color, p1, p2, p3, restored,
                             placement);
                if (insertAt >= 0 && insertAt < historyList.size() && renderStateFor(historyList[insertAt]).actor) {
                    renderStateFor(historyList[insertAt]).actor->SetVisibility(visible ? 1 : 0);
                }
                applyHistoryMetadataFromJson(insertAt, m);
                continue;
            }
            ++skippedCount;
            continue;
        }

        // v1 兼容：无 BREP 时，仍按可参数重建的基础体恢复
        if (type == CUBOID || type == CYLINDER || type == CONE || type == SPHERE) {
            int idx = createGeometryDirectly(
                name, color,
                PrimitiveGeometry::PrimitiveBuildRequest(type, p1, p2, p3, placement));
            if (idx >= 0 && idx < historyList.size() && renderStateFor(historyList[idx]).actor) {
                renderStateFor(historyList[idx]).actor->SetVisibility(visible ? 1 : 0);
            }
            applyHistoryMetadataFromJson(idx, m);
        } else if (type == WORK_CSYS) {
            // WORK_CSYS：无 OCC 形状，使用 param1/2/3 作为 x/y/z 恢复
            const int insertAt = historyList.size();
            restoreModel(insertAt, name, type, color, p1, p2, p3, TopoDS_Shape());
            applyHistoryMetadataFromJson(insertAt, m);
        } else if (type == REFERENCE_CSYS) {
            const int insertAt = historyList.size();
            restoreReferenceCsys(insertAt, name, color);
            if (insertAt >= 0 && insertAt < historyList.size() && renderStateFor(historyList[insertAt]).actor) {
                renderStateFor(historyList[insertAt]).actor->SetVisibility(visible ? 1 : 0);
                setReferenceCsysVisible(visible);
            }
            applyHistoryMetadataFromJson(insertAt, m);
        } else {
            ++skippedCount;
        }
    }

    isLoadingDocument_ = false;
    ensureDefaultReferenceCsysIfMissing();
    currentDocumentPath_ = filePath;
    markDocumentModified(false);
    addRecentFile(filePath);
    if (statusBar()) statusBar()->showMessage(tr("已打开: %1").arg(QFileInfo(filePath).fileName()), 3000);

    if (skippedCount > 0) {
        QMessageBox::information(this, tr("部分内容未恢复"),
                                 tr("有 %1 条历史记录缺少几何数据，未能恢复。").arg(skippedCount));
    }
    updateFeatureTree();
    return true;
}

void Widget::handleNewFile()
{
    if (!maybeSaveDocument()) return;
    clearAllModelsInternal();
    initDefaultReferenceCsys();
    currentDocumentPath_.clear();
    markDocumentModified(false); // 新建后仅含默认基准坐标系，不算已修改
    removeAutoRecoverySnapshot();
}

void Widget::handleOpenFile()
{
    if (!maybeSaveDocument()) return;

    QString path = QFileDialog::getOpenFileName(
        this, tr("打开模型"), QString(),
        tr("Practice Modeling 文件 (*.pmx *.json);;所有文件 (*.*)"));
    if (path.isEmpty() && !recentFiles_.isEmpty()) {
        QStringList items;
        for (const QString& p : recentFiles_) {
            items << p;
        }
        bool ok = false;
        const QString pick = QInputDialog::getItem(
            this, tr("最近文件"), tr("从最近文件中选择:"), items, 0, false, &ok);
        if (ok && !pick.isEmpty()) {
            path = pick;
        }
    }
    if (path.isEmpty()) return;

    loadDocumentFromPath(path);
}

void Widget::handleSaveFile()
{
    // 始终弹出路径选择，便于另存或确认保存位置
    saveDocument(true);
}

void Widget::handleSaveFileAs()
{
    saveDocument(true);
}

void Widget::closeEvent(QCloseEvent* event)
{
    if (maybeSaveDocument()) event->accept();
    else event->ignore();
}

void Widget::setupRecentFilesUi()
{
    if (!ui || !ui->tab || !ui->gridLayout_22) return;
    if (recentFilesGroup_) return;

    recentFilesGroup_ = new QGroupBox(tr("最近文件"), ui->tab);
    auto* vbox = new QVBoxLayout(recentFilesGroup_);
    vbox->setContentsMargins(8, 8, 8, 8);
    vbox->setSpacing(4);

    for (int i = 0; i < 5; ++i) {
        QPushButton* btn = new QPushButton(recentFilesGroup_);
        btn->setFlat(true);
        btn->setVisible(false);
        btn->setStyleSheet(QStringLiteral("text-align:left; padding:4px;"));
        connect(btn, &QPushButton::clicked, this, [this, i]() { openRecentFileAt(i); });
        recentFileButtons_.append(btn);
        vbox->addWidget(btn);
    }

    ui->gridLayout_22->addWidget(recentFilesGroup_, 1, 0, 1, 1);
}

void Widget::refreshRecentFilesUi()
{
    if (!recentFilesGroup_) return;

    for (int i = 0; i < recentFileButtons_.size(); ++i) {
        QPushButton* btn = recentFileButtons_[i];
        if (!btn) continue;

        if (i < recentFiles_.size()) {
            const QString absPath = recentFiles_[i];
            const QFileInfo info(absPath);
            btn->setText(QStringLiteral("%1. %2").arg(i + 1).arg(info.fileName()));
            btn->setToolTip(absPath);
            btn->setVisible(true);
            btn->setEnabled(true);
        } else {
            btn->setVisible(false);
            btn->setEnabled(false);
        }
    }

    recentFilesGroup_->setVisible(!recentFiles_.isEmpty());
}

void Widget::openRecentFileAt(int index)
{
    if (index < 0 || index >= recentFiles_.size()) return;
    if (!maybeSaveDocument()) return;

    const QString path = recentFiles_[index];
    if (!QFileInfo::exists(path)) {
        recentFiles_.removeAt(index);
        saveRecentFilesToSettings();
        refreshRecentFilesUi();
        QMessageBox::warning(this, tr("文件不存在"), tr("该最近文件已不存在，已从列表移除。"));
        return;
    }

    loadDocumentFromPath(path);
}

void Widget::rebuildOpenRecentMenu()
{
    if (!openRecentMenu_) return;
    openRecentMenu_->clear();

    if (recentFiles_.isEmpty()) {
        QAction* empty = openRecentMenu_->addAction(tr("暂无最近文件"));
        empty->setEnabled(false);
        return;
    }

    const int count = std::min<int>(8, static_cast<int>(recentFiles_.size()));
    for (int i = 0; i < count; ++i) {
        const QString absPath = recentFiles_[i];
        const QFileInfo info(absPath);
        QAction* act = openRecentMenu_->addAction(
            QStringLiteral("%1. %2").arg(i + 1).arg(info.fileName()));
        act->setToolTip(absPath);
        connect(act, &QAction::triggered, this, [this, i]() { openRecentFileAt(i); });
    }

    openRecentMenu_->addSeparator();
    QAction* clearAct = openRecentMenu_->addAction(tr("清空最近文件"));
    connect(clearAct, &QAction::triggered, this, &Widget::clearRecentFiles);
}

void Widget::clearRecentFiles()
{
    recentFiles_.clear();
    saveRecentFilesToSettings();
    refreshRecentFilesUi();
    rebuildOpenRecentMenu();
}

void Widget::addRecentFile(const QString& filePath)
{
    if (filePath.isEmpty()) return;
    const QString norm = QFileInfo(filePath).absoluteFilePath();
    recentFiles_.removeAll(norm);
    recentFiles_.prepend(norm);
    while (recentFiles_.size() > 10) {
        recentFiles_.removeLast();
    }
    saveRecentFilesToSettings();
    refreshRecentFilesUi();
    rebuildOpenRecentMenu();
}

void Widget::saveRecentFilesToSettings() const
{
    QSettings settings(QStringLiteral("PracticeModeling"), QStringLiteral("PracticeModelingApp"));
    QStringList paths;
    for (const QString& p : recentFiles_) {
        paths << p;
    }
    settings.setValue(QStringLiteral("files/recent"), paths);
}

void Widget::loadRecentFilesFromSettings()
{
    QSettings settings(QStringLiteral("PracticeModeling"), QStringLiteral("PracticeModelingApp"));
    const QStringList paths = settings.value(QStringLiteral("files/recent")).toStringList();
    recentFiles_.clear();
    for (const QString& p : paths) {
        if (!p.isEmpty() && QFileInfo::exists(p)) {
            recentFiles_.append(QFileInfo(p).absoluteFilePath());
        }
    }
    while (recentFiles_.size() > 10) {
        recentFiles_.removeLast();
    }
    refreshRecentFilesUi();
    rebuildOpenRecentMenu();
}

QString Widget::autoRecoveryFilePath() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::tempPath() + QStringLiteral("/PracticeModeling");
    }
    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return dir.filePath(QStringLiteral("autosave_recovery.pmx"));
}

void Widget::saveAutoRecoverySnapshot()
{
    if (isLoadingDocument_ || !documentModified_) return;

    QJsonObject root;
    root["schemaVersion"] = 3;
    root["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["isAutoRecovery"] = true;
    root["sourceDocument"] = currentDocumentPath_;
    root["models"] = modelDocumentToJson(
        modelDocument_,
        [this](const ModelingHistory& record) { return geometryStateFor(record).occShape; },
        [this](const ModelingHistory& record) {
            const ModelRenderState& state = renderStateFor(record);
            return state.actor ? state.actor->GetVisibility() != 0 : true;
        });

    QSaveFile file(autoRecoveryFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.commit();
}

void Widget::removeAutoRecoverySnapshot()
{
    QFile::remove(autoRecoveryFilePath());
}

void Widget::tryRecoverFromAutoSnapshotOnStartup()
{
    const QString recPath = autoRecoveryFilePath();
    if (!QFileInfo::exists(recPath)) return;

    QMessageBox::StandardButton choice = QMessageBox::question(
        this,
        tr("发现自动恢复数据"),
        tr("检测到上次未正常退出的恢复数据，是否恢复？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (choice != QMessageBox::Yes) {
        removeAutoRecoverySnapshot();
        return;
    }

    if (loadDocumentFromPath(recPath)) {
        recentFiles_.removeAll(QFileInfo(recPath).absoluteFilePath());
        saveRecentFilesToSettings();
        refreshRecentFilesUi();
        rebuildOpenRecentMenu();
        currentDocumentPath_.clear();
        markDocumentModified(true); // 恢复后的会话视为未正式保存
        if (statusBar()) statusBar()->showMessage(tr("已恢复上次会话，请尽快另存为正式文件"), 5000);
    }
}

void Widget::applyHistoryMetadataFromJson(int index, const QJsonObject& obj)
{
    if (index < 0 || index >= historyList.size()) {
        return;
    }

    ModelingHistory& record = historyList[index];
    applyModelingHistoryFromJson(obj, record);
    if (record.recipe.hasRecipe) {
        assignFeatureRecipe(index, record.recipe);
    }
}
