#include "patternfeaturedialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QToolButton>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPatternDegPerRev = 360.0;

const char* kRotaryPanelStyle =
    "QGroupBox { background-color: #ebebeb; border: 1px solid #d0d0d0; "
    "border-radius: 2px; margin-top: 8px; padding: 6px; }"
    "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }";
} // namespace

PatternFeatureDialog::PatternFeatureDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("阵列特征"));
    setModal(false);
    buildUi();
    updateLayoutDependentUi();
    updatePolygonSpacingParamsVisible();
    updateDirection2Enabled();
    updateRadialParamsVisible();
    updateOkApplyEnabled();
}

void PatternFeatureDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);

    auto* featureGroup = new QGroupBox(tr("要形成阵列的特征"), this);
    auto* featureLay = new QHBoxLayout(featureGroup);
    selectBodiesBtn_ = new QPushButton(tr("选择特征"), featureGroup);
    selectCountLabel_ = new QLabel(tr("(0)"), featureGroup);
    selectCountLabel_->setMinimumWidth(36);
    featureLay->addWidget(selectBodiesBtn_);
    featureLay->addWidget(selectCountLabel_);
    featureLay->addStretch();
    root->addWidget(featureGroup);

    auto* defGroup = new QGroupBox(tr("阵列定义"), this);
    auto* defLay = new QVBoxLayout(defGroup);

    auto* layoutRow = new QHBoxLayout();
    layoutRow->addWidget(new QLabel(tr("布局"), defGroup));
    layoutCombo_ = new QComboBox(defGroup);
    layoutCombo_->addItem(tr("线性"));
    layoutCombo_->addItem(tr("圆形"));
    layoutCombo_->addItem(tr("多边形"));
    layoutCombo_->addItem(tr("螺旋"));
    layoutCombo_->addItem(tr("沿"));
    layoutCombo_->addItem(tr("常规"));
    layoutCombo_->addItem(tr("参考"));
    if (auto* model = qobject_cast<QStandardItemModel*>(layoutCombo_->model())) {
        for (int i = 3; i < layoutCombo_->count(); ++i) {
            if (QStandardItem* item = model->item(i)) {
                item->setEnabled(false);
            }
        }
    }
    layoutCombo_->setCurrentIndex(0);
    layoutRow->addWidget(layoutCombo_, 1);
    defLay->addLayout(layoutRow);

    layoutStack_ = new QStackedWidget(defGroup);

    // ---------- 线性布局页 ----------
    linearPage_ = new QWidget(defGroup);
    auto* linearLay = new QVBoxLayout(linearPage_);
    linearLay->setContentsMargins(0, 0, 0, 0);

    direction1Group_ = new QGroupBox(tr("方向 1"), linearPage_);
    auto* dir1Form = new QFormLayout(direction1Group_);
    auto* vecRow1 = new QHBoxLayout();
    vecRow1->addWidget(new QLabel(tr("指定矢量"), direction1Group_));
    reverseBtn1_ = new QPushButton(tr("反向"), direction1Group_);
    reverseBtn1_->setFixedWidth(48);
    openVectorBtn1_ = new QPushButton(tr("矢量"), direction1Group_);
    openVectorBtn1_->setFixedWidth(48);
    vectorToolBtn1_ = new QToolButton(direction1Group_);
    vectorToolBtn1_->setText(tr("模式"));
    vectorToolBtn1_->setToolTip(tr("快速选择矢量方式"));
    vecRow1->addWidget(reverseBtn1_);
    vecRow1->addWidget(openVectorBtn1_);
    vecRow1->addWidget(vectorToolBtn1_);
    vecRow1->addStretch();
    dir1Form->addRow(vecRow1);

    count1LinearSpin_ = new QSpinBox(direction1Group_);
    count1LinearSpin_->setRange(1, 1000);
    count1LinearSpin_->setValue(2);
    pitch1LinearSpin_ = new QDoubleSpinBox(direction1Group_);
    pitch1LinearSpin_->setRange(0.1, 1e6);
    pitch1LinearSpin_->setDecimals(4);
    pitch1LinearSpin_->setValue(10.0);
    pitch1LinearSpin_->setSuffix(tr(" mm"));
    dir1Form->addRow(tr("数量"), count1LinearSpin_);
    dir1Form->addRow(tr("节距"), pitch1LinearSpin_);
    linearLay->addWidget(direction1Group_);

    useDirection2Chk_ = new QCheckBox(tr("使用方向 2"), linearPage_);
    linearLay->addWidget(useDirection2Chk_);

    direction2Group_ = new QGroupBox(tr("方向 2"), linearPage_);
    auto* dir2Form = new QFormLayout(direction2Group_);
    auto* vecRow2 = new QHBoxLayout();
    vecRow2->addWidget(new QLabel(tr("指定矢量"), direction2Group_));
    reverseBtn2_ = new QPushButton(tr("反向"), direction2Group_);
    reverseBtn2_->setFixedWidth(48);
    openVectorBtn2_ = new QPushButton(tr("矢量"), direction2Group_);
    openVectorBtn2_->setFixedWidth(48);
    vectorToolBtn2_ = new QToolButton(direction2Group_);
    vectorToolBtn2_->setText(tr("模式"));
    vecRow2->addWidget(reverseBtn2_);
    vecRow2->addWidget(openVectorBtn2_);
    vecRow2->addWidget(vectorToolBtn2_);
    vecRow2->addStretch();
    dir2Form->addRow(vecRow2);

    count2Spin_ = new QSpinBox(direction2Group_);
    count2Spin_->setRange(1, 1000);
    count2Spin_->setValue(2);
    pitch2Spin_ = new QDoubleSpinBox(direction2Group_);
    pitch2Spin_->setRange(0.1, 1e6);
    pitch2Spin_->setDecimals(4);
    pitch2Spin_->setValue(10.0);
    pitch2Spin_->setSuffix(tr(" mm"));
    pitch2Label_ = new QLabel(tr("节距"), direction2Group_);
    dir2Form->addRow(tr("数量"), count2Spin_);
    dir2Form->addRow(pitch2Label_, pitch2Spin_);
    linearLay->addWidget(direction2Group_);
    linearLay->addStretch();

    layoutStack_->addWidget(linearPage_);

    // ---------- 圆形/多边形布局页 ----------
    rotaryPage_ = new QWidget(defGroup);
    auto* rotaryLay = new QVBoxLayout(rotaryPage_);
    rotaryLay->setContentsMargins(0, 0, 0, 0);

    rotationAxisGroup_ = new QGroupBox(tr("旋转轴"), rotaryPage_);
    auto* axisForm = new QFormLayout(rotationAxisGroup_);
    auto* axisVecRow = new QHBoxLayout();
    axisVecRow->addWidget(new QLabel(tr("指定矢量"), rotationAxisGroup_));
    reverseAxisBtn_ = new QPushButton(tr("反向"), rotationAxisGroup_);
    reverseAxisBtn_->setFixedWidth(48);
    openAxisVectorBtn_ = new QPushButton(tr("矢量"), rotationAxisGroup_);
    openAxisVectorBtn_->setFixedWidth(48);
    axisVectorToolBtn_ = new QToolButton(rotationAxisGroup_);
    axisVectorToolBtn_->setText(tr("模式"));
    axisVectorToolBtn_->setToolTip(tr("快速选择旋转轴矢量"));
    axisVecRow->addWidget(reverseAxisBtn_);
    axisVecRow->addWidget(openAxisVectorBtn_);
    axisVecRow->addWidget(axisVectorToolBtn_);
    axisVecRow->addStretch();
    axisForm->addRow(axisVecRow);

    auto* pointRow = new QHBoxLayout();
    pointRow->addWidget(new QLabel(tr("指定点"), rotationAxisGroup_));
    specifyPointBtn_ = new QPushButton(tr("指定点"), rotationAxisGroup_);
    specifyPointBtn_->setFlat(true);
    specifyPointBtn_->setStyleSheet(QStringLiteral("QPushButton { text-align: left; color: #1a5fb4; }"));
    pointSnapToolBtn_ = new QToolButton(rotationAxisGroup_);
    pointSnapToolBtn_->setText(tr("类型"));
    pointSnapToolBtn_->setToolTip(tr("点捕捉类型"));
    pointRow->addWidget(specifyPointBtn_, 1);
    pointRow->addWidget(pointSnapToolBtn_);
    axisForm->addRow(pointRow);
    rotaryLay->addWidget(rotationAxisGroup_);

    angularParamGroup_ = new QGroupBox(tr("斜角方向"), rotaryPage_);
    auto* angForm = new QFormLayout(angularParamGroup_);
    polygonSpanSpin_ = new QDoubleSpinBox(angularParamGroup_);
    polygonSpanSpin_->setRange(1.0, 360.0);
    polygonSpanSpin_->setDecimals(4);
    polygonSpanSpin_->setValue(360.0);
    polygonSpanSpin_->setSuffix(tr(" °"));
    count1Label_ = new QLabel(tr("数量"), angularParamGroup_);
    count1Spin_ = new QSpinBox(angularParamGroup_);
    count1Spin_->setRange(1, 1000);
    count1Spin_->setValue(8);
    polygonSpacingLabel_ = new QLabel(tr("间距"), angularParamGroup_);
    polygonSpacingCombo_ = new QComboBox(angularParamGroup_);
    polygonSpacingCombo_->addItem(tr("每边数目"), static_cast<int>(PolygonSpacingMode::CountPerSide));
    polygonSpacingCombo_->addItem(tr("沿边节距"), static_cast<int>(PolygonSpacingMode::PitchAlongEdge));

    polygonAlongCountRow_ = new QWidget(angularParamGroup_);
    auto* alongCountLay = new QHBoxLayout(polygonAlongCountRow_);
    alongCountLay->setContentsMargins(0, 0, 0, 0);
    polygonAlongCountLabel_ = new QLabel(tr("数量"), polygonAlongCountRow_);
    polygonAlongCountSpin_ = new QSpinBox(polygonAlongCountRow_);
    polygonAlongCountSpin_->setRange(1, 1000);
    polygonAlongCountSpin_->setValue(4);
    alongCountLay->addWidget(polygonAlongCountLabel_);
    alongCountLay->addWidget(polygonAlongCountSpin_);

    polygonEdgePitchRow_ = new QWidget(angularParamGroup_);
    auto* edgePitchLay = new QHBoxLayout(polygonEdgePitchRow_);
    edgePitchLay->setContentsMargins(0, 0, 0, 0);
    polygonEdgePitchLabel_ = new QLabel(tr("螺距"), polygonEdgePitchRow_);
    polygonEdgePitchSpin_ = new QDoubleSpinBox(polygonEdgePitchRow_);
    polygonEdgePitchSpin_->setRange(0.1, 1e6);
    polygonEdgePitchSpin_->setDecimals(4);
    polygonEdgePitchSpin_->setValue(25.0);
    polygonEdgePitchSpin_->setSuffix(tr(" mm"));
    edgePitchLay->addWidget(polygonEdgePitchLabel_);
    edgePitchLay->addWidget(polygonEdgePitchSpin_);

    pitch1Label_ = new QLabel(tr("节距角"), angularParamGroup_);
    pitch1Spin_ = new QDoubleSpinBox(angularParamGroup_);
    pitch1Spin_->setRange(0.1, 360.0);
    pitch1Spin_->setDecimals(4);
    pitch1Spin_->setValue(30.0);
    pitch1Spin_->setSuffix(tr(" °"));
    polygonSpanLabel_ = new QLabel(tr("跨距"), angularParamGroup_);
    angForm->addRow(count1Label_, count1Spin_);
    angForm->addRow(polygonSpacingLabel_, polygonSpacingCombo_);
    angForm->addRow(polygonAlongCountRow_);
    angForm->addRow(polygonEdgePitchRow_);
    angForm->addRow(pitch1Label_, pitch1Spin_);
    angForm->addRow(polygonSpanLabel_, polygonSpanSpin_);
    rotaryLay->addWidget(angularParamGroup_);

    radialGroup_ = new QGroupBox(tr("辐射"), rotaryPage_);
    auto* radialLay = new QVBoxLayout(radialGroup_);
    concentricChk_ = new QCheckBox(tr("创建同心成员"), radialGroup_);
    radialLay->addWidget(concentricChk_);

    radialParamsWidget_ = new QWidget(radialGroup_);
    auto* radialForm = new QFormLayout(radialParamsWidget_);
    // 与线性「方向 2」分开的独立控件，避免 addRow 抢走方向 2 的数量/节距
    count2RadialSpin_ = new QSpinBox(radialParamsWidget_);
    count2RadialSpin_->setRange(1, 1000);
    count2RadialSpin_->setValue(2);
    pitch2RadialSpin_ = new QDoubleSpinBox(radialParamsWidget_);
    pitch2RadialSpin_->setRange(0.1, 1e6);
    pitch2RadialSpin_->setDecimals(4);
    pitch2RadialSpin_->setValue(10.0);
    pitch2RadialSpin_->setSuffix(tr(" mm"));
    radialForm->addRow(tr("数量"), count2RadialSpin_);
    radialForm->addRow(tr("节距"), pitch2RadialSpin_);
    radialLay->addWidget(radialParamsWidget_);
    rotaryLay->addWidget(radialGroup_);
    rotaryLay->addStretch();

    layoutStack_->addWidget(rotaryPage_);
    defLay->addWidget(layoutStack_);
    root->addWidget(defGroup);

    auto* btnBox = new QDialogButtonBox(this);
    auto* previewBtn = btnBox->addButton(tr("显示结果"), QDialogButtonBox::ActionRole);
    okBtn_ = btnBox->addButton(QDialogButtonBox::Ok);
    applyBtn_ = btnBox->addButton(QDialogButtonBox::Apply);
    btnBox->addButton(QDialogButtonBox::Cancel);
    root->addWidget(btnBox);

    connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PatternFeatureDialog::onLayoutChanged);
    connect(selectBodiesBtn_, &QPushButton::clicked, this, &PatternFeatureDialog::requestBodySelection);
    auto syncCount1 = [this](int v) {
        QSignalBlocker b1(count1LinearSpin_);
        QSignalBlocker b2(count1Spin_);
        if (count1LinearSpin_) count1LinearSpin_->setValue(v);
        if (count1Spin_) count1Spin_->setValue(v);
        if (layoutType() == PatternLayoutType::Polygonal) {
            syncPolygonalAngularPitch();
        }
        emit parametersChanged();
    };
    connect(polygonAlongCountSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        emit parametersChanged();
    });
    connect(polygonEdgePitchSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        emit parametersChanged();
    });
    connect(polygonSpacingCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        updatePolygonSpacingParamsVisible();
        emit parametersChanged();
    });
    connect(count1LinearSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, syncCount1);
    connect(count1Spin_, QOverload<int>::of(&QSpinBox::valueChanged), this, syncCount1);
    auto syncPitch1 = [this](double v) {
        QSignalBlocker b1(pitch1LinearSpin_);
        QSignalBlocker b2(pitch1Spin_);
        if (pitch1LinearSpin_) pitch1LinearSpin_->setValue(v);
        if (pitch1Spin_) pitch1Spin_->setValue(v);
        emit pitch1Changed(v);
        emit parametersChanged();
    };
    connect(pitch1LinearSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, syncPitch1);
    connect(pitch1Spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, syncPitch1);
    connect(useDirection2Chk_, &QCheckBox::toggled, this, [this](bool) {
        updateDirection2Enabled();
        emit parametersChanged();
    });
    auto syncCount2 = [this](int v) {
        QSignalBlocker b1(count2Spin_);
        QSignalBlocker b2(count2RadialSpin_);
        if (count2Spin_) count2Spin_->setValue(v);
        if (count2RadialSpin_) count2RadialSpin_->setValue(v);
        emit parametersChanged();
    };
    connect(count2Spin_, QOverload<int>::of(&QSpinBox::valueChanged), this, syncCount2);
    connect(count2RadialSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, syncCount2);
    auto syncPitch2 = [this](double v) {
        QSignalBlocker b1(pitch2Spin_);
        QSignalBlocker b2(pitch2RadialSpin_);
        if (pitch2Spin_) pitch2Spin_->setValue(v);
        if (pitch2RadialSpin_) pitch2RadialSpin_->setValue(v);
        emit pitch2Changed(v);
        emit parametersChanged();
    };
    connect(pitch2Spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, syncPitch2);
    connect(pitch2RadialSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, syncPitch2);
    connect(polygonSpanSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
        if (layoutType() == PatternLayoutType::Polygonal) {
            syncPolygonalAngularPitch();
        }
        emit parametersChanged();
    });
    connect(concentricChk_, &QCheckBox::toggled, this, [this](bool) {
        updateRadialParamsVisible();
        emit parametersChanged();
    });
    connect(previewBtn, &QPushButton::clicked, this, &PatternFeatureDialog::previewRequested);
    connect(okBtn_, &QPushButton::clicked, this, &QDialog::accept);
    connect(applyBtn_, &QPushButton::clicked, this, &PatternFeatureDialog::applyRequested);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(reverseBtn1_, &QPushButton::clicked, this, [this]() { toggleReverse(1); });
    connect(reverseBtn2_, &QPushButton::clicked, this, [this]() { toggleReverse(2); });
    connect(reverseAxisBtn_, &QPushButton::clicked, this, [this]() { toggleReverse(1); });
    connect(openVectorBtn1_, &QPushButton::clicked, this, [this]() { emit requestOpenVectorDialog(1); });
    connect(openVectorBtn2_, &QPushButton::clicked, this, [this]() { emit requestOpenVectorDialog(2); });
    connect(openAxisVectorBtn_, &QPushButton::clicked, this, [this]() { emit requestOpenVectorDialog(1); });
    connect(specifyPointBtn_, &QPushButton::clicked, this, &PatternFeatureDialog::onSpecifyPointClicked);

    setupVectorToolButton(vectorToolBtn1_, 1);
    setupVectorToolButton(vectorToolBtn2_, 2);
    setupVectorToolButton(axisVectorToolBtn_, 1);
    setupPointSnapToolButton();
}

void PatternFeatureDialog::applyRotaryGroupStyle(QGroupBox* box, bool enabled)
{
    if (!box) return;
    box->setStyleSheet(enabled ? QString::fromUtf8(kRotaryPanelStyle) : QString());
}

void PatternFeatureDialog::setupVectorToolButton(QToolButton* btn, int directionIndex)
{
    if (!btn) return;
    QMenu* menu = new QMenu(this);
    auto addAct = [&](const QString& text, int modeIndex) {
        QAction* act = menu->addAction(text);
        connect(act, &QAction::triggered, this, [this, directionIndex, modeIndex]() {
            emit requestVectorMode(directionIndex, modeIndex);
        });
    };
    addAct(tr("自动判断"), 0);
    addAct(tr("两点"), 1);
    addAct(tr("曲线/轴矢量"), 3);
    addAct(tr("曲线上矢量"), 4);
    addAct(tr("面/平面法向量"), 5);
    addAct(tr("面上点的矢量"), 6);
    addAct(tr("XC轴"), 7);
    addAct(tr("YC轴"), 8);
    addAct(tr("ZC轴"), 9);
    addAct(tr("-XC轴"), 10);
    addAct(tr("-YC轴"), 11);
    addAct(tr("-ZC轴"), 12);
    addAct(tr("视图方向"), 13);
    btn->setMenu(menu);
    btn->setPopupMode(QToolButton::InstantPopup);
}

void PatternFeatureDialog::setupPointSnapToolButton()
{
    if (!pointSnapToolBtn_) return;
    QMenu* menu = new QMenu(this);
    auto addAct = [&](const QString& text, int kind) {
        QAction* act = menu->addAction(text);
        connect(act, &QAction::triggered, this, [this, kind, text]() {
            pointSnapKind_ = kind;
            pointSnapToolBtn_->setText(text);
        });
    };
    addAct(tr("任意点"), kPatternPointSnapArbitrary);
    addAct(tr("最近点"), 0);
    addAct(tr("端点"), 1);
    addAct(tr("中点"), 2);
    addAct(tr("交点"), 3);
    addAct(tr("圆心"), 4);
    addAct(tr("象限点"), 5);
    pointSnapToolBtn_->setMenu(menu);
    pointSnapToolBtn_->setPopupMode(QToolButton::InstantPopup);
    applyDefaultArbitraryPointSnap();
}

void PatternFeatureDialog::applyDefaultArbitraryPointSnap()
{
    pointSnapKind_ = kPatternPointSnapArbitrary;
    if (pointSnapToolBtn_) {
        pointSnapToolBtn_->setText(tr("任意点"));
    }
}

void PatternFeatureDialog::onSpecifyPointClicked()
{
    if (pointSnapKind_ == kPatternPointSnapArbitrary) {
        emit requestPointSelection();
    } else if (pointSnapKind_ >= 0) {
        emit requestPointSelectionWithSnap(pointSnapKind_);
    } else {
        emit requestPointSelection();
    }
}

void PatternFeatureDialog::toggleReverse(int directionIndex)
{
    if (directionIndex == 1) {
        direction1Reversed_ = !direction1Reversed_;
        if (hasDirection1_) {
            direction1_.Reverse();
        }
    } else if (directionIndex == 2) {
        direction2Reversed_ = !direction2Reversed_;
        if (hasDirection2_) {
            direction2_.Reverse();
        }
    }
    emit parametersChanged();
}

PatternLayoutType PatternFeatureDialog::layoutType() const
{
    const int idx = layoutIndex();
    if (idx == 1) return PatternLayoutType::Circular;
    if (idx == 2) return PatternLayoutType::Polygonal;
    return PatternLayoutType::Linear;
}

int PatternFeatureDialog::layoutIndex() const
{
    return layoutCombo_ ? layoutCombo_->currentIndex() : 0;
}

void PatternFeatureDialog::onLayoutChanged(int index)
{
    Q_UNUSED(index);
    updateLayoutDependentUi();
    updatePolygonSpacingParamsVisible();
    updateRadialParamsVisible();
    emit parametersChanged();
}

void PatternFeatureDialog::syncPolygonalAngularPitch()
{
    if (!pitch1Spin_ || !count1Spin_) return;
    const int n = (std::max)(1, count1Spin_->value());
    const double span = polygonSpanSpin_ ? polygonSpanSpin_->value() : kPatternDegPerRev;
    pitch1Spin_->setValue(span / static_cast<double>(n));
}

void PatternFeatureDialog::updateLayoutDependentUi()
{
    const PatternLayoutType layout = layoutType();
    const bool isLinear = (layout == PatternLayoutType::Linear);
    const bool isRotary = !isLinear;

    if (layoutStack_) {
        layoutStack_->setCurrentIndex(isLinear ? 0 : 1);
    }

    applyRotaryGroupStyle(rotationAxisGroup_, isRotary);
    applyRotaryGroupStyle(angularParamGroup_, isRotary);
    applyRotaryGroupStyle(radialGroup_, isRotary);

    if (angularParamGroup_) {
        if (layout == PatternLayoutType::Circular) {
            angularParamGroup_->setTitle(tr("斜角方向"));
        } else if (layout == PatternLayoutType::Polygonal) {
            angularParamGroup_->setTitle(tr("多边形定义"));
        }
    }

    if (count1Label_) {
        count1Label_->setText(layout == PatternLayoutType::Polygonal ? tr("边数") : tr("数量"));
    }
    const bool isCircular = (layout == PatternLayoutType::Circular);
    const bool isPolygonal = (layout == PatternLayoutType::Polygonal);

    if (polygonSpacingLabel_) {
        polygonSpacingLabel_->setVisible(isPolygonal);
    }
    if (polygonSpacingCombo_) {
        polygonSpacingCombo_->setVisible(isPolygonal);
    }
    if (polygonSpanLabel_) {
        polygonSpanLabel_->setVisible(isPolygonal);
    }
    if (polygonSpanSpin_) {
        polygonSpanSpin_->setVisible(isPolygonal);
        polygonSpanSpin_->setEnabled(isPolygonal);
    }
    updatePolygonSpacingParamsVisible();
    if (pitch1Label_) {
        pitch1Label_->setVisible(isCircular);
        if (isCircular) {
            pitch1Label_->setText(tr("节距角"));
        }
    }
    if (pitch1Spin_) {
        pitch1Spin_->setVisible(isCircular);
        if (isRotary) {
            pitch1Spin_->setRange(0.1, 360.0);
            pitch1Spin_->setSuffix(tr(" °"));
            if (isPolygonal) {
                syncPolygonalAngularPitch();
                pitch1Spin_->setEnabled(false);
                pitch1Spin_->setToolTip(tr("多边形布局下由跨距与边数自动计算"));
            } else if (isCircular) {
                pitch1Spin_->setEnabled(true);
                pitch1Spin_->setToolTip(QString());
                if (pitch1Spin_->value() > 360.0) {
                    pitch1Spin_->setValue(30.0);
                }
            }
        } else {
            pitch1Spin_->setRange(0.1, 1e6);
            pitch1Spin_->setSuffix(tr(" mm"));
            pitch1Spin_->setEnabled(true);
            pitch1Spin_->setToolTip(QString());
        }
    }
    if (pitch2Spin_) {
        pitch2Spin_->setRange(0.1, 1e6);
        pitch2Spin_->setSuffix(tr(" mm"));
    }
    if (pitch2Label_) {
        pitch2Label_->setText(tr("节距"));
    }

    if (isRotary) {
        applyDefaultArbitraryPointSnap();
    }

    updateDirection2Enabled();
}

void PatternFeatureDialog::updateRadialParamsVisible()
{
    const bool showRadialParams = concentricChk_ && concentricChk_->isChecked();
    if (radialParamsWidget_) {
        radialParamsWidget_->setVisible(showRadialParams);
    }
}

bool PatternFeatureDialog::createConcentricMembers() const
{
    return concentricChk_ && concentricChk_->isChecked();
}

double PatternFeatureDialog::polygonSpanDegrees() const
{
    return polygonSpanSpin_ ? polygonSpanSpin_->value() : kPatternDegPerRev;
}

PolygonSpacingMode PatternFeatureDialog::polygonSpacingMode() const
{
    if (!polygonSpacingCombo_) {
        return PolygonSpacingMode::CountPerSide;
    }
    const int data = polygonSpacingCombo_->currentData().toInt();
    return (data == static_cast<int>(PolygonSpacingMode::PitchAlongEdge)) ? PolygonSpacingMode::PitchAlongEdge
                                                                            : PolygonSpacingMode::CountPerSide;
}

int PatternFeatureDialog::polygonAlongEdgeCount() const
{
    return polygonAlongCountSpin_ ? polygonAlongCountSpin_->value() : 2;
}

double PatternFeatureDialog::polygonAlongEdgePitch() const
{
    return polygonEdgePitchSpin_ ? polygonEdgePitchSpin_->value() : 10.0;
}

void PatternFeatureDialog::updatePolygonSpacingParamsVisible()
{
    const bool isPolygonal = (layoutType() == PatternLayoutType::Polygonal);
    if (!isPolygonal) {
        if (polygonAlongCountRow_) {
            polygonAlongCountRow_->setVisible(false);
        }
        if (polygonEdgePitchRow_) {
            polygonEdgePitchRow_->setVisible(false);
        }
        return;
    }
    const bool countPerSide = (polygonSpacingMode() == PolygonSpacingMode::CountPerSide);
    if (polygonAlongCountRow_) {
        polygonAlongCountRow_->setVisible(countPerSide);
    }
    if (polygonEdgePitchRow_) {
        polygonEdgePitchRow_->setVisible(!countPerSide);
    }
}

int PatternFeatureDialog::count1() const
{
    if (layoutType() == PatternLayoutType::Linear) {
        return count1LinearSpin_ ? count1LinearSpin_->value() : 2;
    }
    return count1Spin_ ? count1Spin_->value() : 2;
}

double PatternFeatureDialog::pitch1() const
{
    if (layoutType() == PatternLayoutType::Linear) {
        return pitch1LinearSpin_ ? pitch1LinearSpin_->value() : 10.0;
    }
    return pitch1Spin_ ? pitch1Spin_->value() : 10.0;
}

bool PatternFeatureDialog::useDirection2() const
{
    if (layoutType() != PatternLayoutType::Linear) {
        return false;
    }
    return useDirection2Chk_ && useDirection2Chk_->isChecked();
}

int PatternFeatureDialog::count2() const
{
    if (layoutType() == PatternLayoutType::Linear) {
        return count2Spin_ ? count2Spin_->value() : 2;
    }
    return count2RadialSpin_ ? count2RadialSpin_->value() : 2;
}

double PatternFeatureDialog::pitch2() const
{
    if (layoutType() == PatternLayoutType::Linear) {
        return pitch2Spin_ ? pitch2Spin_->value() : 10.0;
    }
    return pitch2RadialSpin_ ? pitch2RadialSpin_->value() : 10.0;
}

void PatternFeatureDialog::setDirection1(const gp_Dir& dir, bool has)
{
    hasDirection1_ = has;
    direction1_ = dir;
    direction1Reversed_ = false;
    updateOkApplyEnabled();
}

void PatternFeatureDialog::setDirection2(const gp_Dir& dir, bool has)
{
    hasDirection2_ = has;
    direction2_ = dir;
    updateOkApplyEnabled();
}

void PatternFeatureDialog::setRotationCenter(const gp_Pnt& p, bool has)
{
    hasRotationCenter_ = has;
    rotationCenter_ = p;
    if (specifyPointBtn_) {
        if (has) {
            specifyPointBtn_->setText(
                tr("指定点: (%1, %2, %3)").arg(p.X(), 0, 'f', 2).arg(p.Y(), 0, 'f', 2).arg(p.Z(), 0, 'f', 2));
        } else {
            specifyPointBtn_->setText(tr("指定点"));
        }
    }
    updateOkApplyEnabled();
    emit parametersChanged();
}

void PatternFeatureDialog::setSelectedBodyCount(int count)
{
    if (selectCountLabel_) {
        selectCountLabel_->setText(QStringLiteral("(%1)").arg(count));
    }
    updateOkApplyEnabled();
}

void PatternFeatureDialog::setPitch1(double pitch)
{
    if (pitch1LinearSpin_) pitch1LinearSpin_->setValue(pitch);
    if (pitch1Spin_) pitch1Spin_->setValue(pitch);
}

void PatternFeatureDialog::setPitch2(double pitch)
{
    if (pitch2Spin_) pitch2Spin_->setValue(pitch);
    if (pitch2RadialSpin_) pitch2RadialSpin_->setValue(pitch);
}

void PatternFeatureDialog::setPolygonSpan(double spanDegrees)
{
    if (!polygonSpanSpin_) {
        return;
    }
    const double clamped = std::max(1.0, std::min(360.0, spanDegrees));
    QSignalBlocker blockSpan(polygonSpanSpin_);
    polygonSpanSpin_->setValue(clamped);
    syncPolygonalAngularPitch();
    emit parametersChanged();
}

void PatternFeatureDialog::updateDirection2Enabled()
{
    const bool linear = layoutType() == PatternLayoutType::Linear;
    if (useDirection2Chk_) {
        useDirection2Chk_->setVisible(linear);
    }
    if (direction2Group_) {
        direction2Group_->setVisible(linear);
        direction2Group_->setEnabled(linear && useDirection2());
    }
}

void PatternFeatureDialog::updateOkApplyEnabled()
{
    const bool hasBodies = selectCountLabel_ && selectCountLabel_->text() != QStringLiteral("(0)");
    bool ok = hasBodies && hasDirection1_;
    if (layoutType() == PatternLayoutType::Circular || layoutType() == PatternLayoutType::Polygonal) {
        ok = ok && hasRotationCenter_;
    }
    if (okBtn_) okBtn_->setEnabled(ok);
    if (applyBtn_) applyBtn_->setEnabled(ok);
}
