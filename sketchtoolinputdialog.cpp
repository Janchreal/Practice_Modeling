#include "sketchtoolinputdialog.h"
#include "ui_sketchtoolinputdialog.h"

#include <QCloseEvent>
#include <QDoubleValidator>
#include <QVBoxLayout>

SketchToolInputDialog::SketchToolInputDialog(ObjectKind initialObject, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SketchToolInputDialog)
    , objectKind_(initialObject)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);

    setWindowTitle(tr("轮廓"));
    ui->label_title->setText(tr("轮廓"));

    if (ui->toolButton_settings) {
        ui->toolButton_settings->setAutoRaise(true);
        ui->toolButton_settings->setText(QStringLiteral("\u2699"));
    }

    updating_ = true;
    if (objectKind_ == ObjArc) {
        ui->toolButton_objArc->setChecked(true);
        ui->toolButton_objLine->setChecked(false);
    } else {
        ui->toolButton_objLine->setChecked(true);
        ui->toolButton_objArc->setChecked(false);
    }
    updating_ = false;

    for (auto* le : {ui->lineEdit_xc, ui->lineEdit_yc, ui->lineEdit_length, ui->lineEdit_angle,
                     ui->lineEdit_radius, ui->lineEdit_sweep}) {
        if (le) {
            auto* dv = new QDoubleValidator(le);
            dv->setNotation(QDoubleValidator::StandardNotation);
            le->setValidator(dv);
        }
    }

    connect(ui->toolButton_close, &QToolButton::clicked, this, &SketchToolInputDialog::onCloseClicked);
    connect(ui->toolButton_modeCoord, &QToolButton::toggled, this, &SketchToolInputDialog::onModeCoordToggled);
    connect(ui->toolButton_modeParam, &QToolButton::toggled, this, &SketchToolInputDialog::onModeParamToggled);
    connect(ui->toolButton_objLine, &QToolButton::toggled, this, &SketchToolInputDialog::onObjLineToggled);
    connect(ui->toolButton_objArc, &QToolButton::toggled, this, &SketchToolInputDialog::onObjArcToggled);
    if (ui->toolButton_sketch) {
        connect(ui->toolButton_sketch, &QToolButton::clicked, this, &SketchToolInputDialog::onSketchClicked);
        ui->toolButton_sketch->setStyleSheet(QStringLiteral("QToolButton { border: 1px solid #ccc; border-radius: 4px; }"));
    }

    const auto hookEdit = [this](QLineEdit* le) {
        if (!le) return;
        connect(le, &QLineEdit::editingFinished, this, &SketchToolInputDialog::onAnyEditingFinished);
        connect(le, &QLineEdit::returnPressed, this, &SketchToolInputDialog::onAnyEditingFinished);
    };
    hookEdit(ui->lineEdit_xc);
    hookEdit(ui->lineEdit_yc);
    hookEdit(ui->lineEdit_length);
    hookEdit(ui->lineEdit_angle);
    hookEdit(ui->lineEdit_radius);
    hookEdit(ui->lineEdit_sweep);

    applyToolPages();
    syncModeButtons();
    syncObjectButtons();
}

SketchToolInputDialog::~SketchToolInputDialog()
{
    delete ui;
}

SketchToolInputDialog::ObjectKind SketchToolInputDialog::objectKind() const
{
    return objectKind_;
}

void SketchToolInputDialog::setObjectKind(ObjectKind k)
{
    if (objectKind_ == k) return;
    objectKind_ = k;
    updating_ = true;
    if (k == ObjArc) {
        ui->toolButton_objArc->setChecked(true);
        ui->toolButton_objLine->setChecked(false);
    } else {
        ui->toolButton_objLine->setChecked(true);
        ui->toolButton_objArc->setChecked(false);
    }
    updating_ = false;
    applyToolPages();
    syncObjectButtons();
}

SketchToolInputDialog::InputKind SketchToolInputDialog::inputKind() const
{
    if (ui->toolButton_modeParam && ui->toolButton_modeParam->isChecked()) {
        return Parameter;
    }
    return Coordinate;
}

void SketchToolInputDialog::setInputKind(InputKind k)
{
    updating_ = true;
    if (k == Parameter) {
        ui->toolButton_modeParam->setChecked(true);
        ui->toolButton_modeCoord->setChecked(false);
    } else {
        ui->toolButton_modeCoord->setChecked(true);
        ui->toolButton_modeParam->setChecked(false);
    }
    updating_ = false;
    applyToolPages();
    syncModeButtons();
}

void SketchToolInputDialog::setCoordinateValues(double xc, double yc)
{
    if (updating_) return;
    updating_ = true;
    ui->lineEdit_xc->setText(QString::number(xc, 'f', 3));
    ui->lineEdit_yc->setText(QString::number(yc, 'f', 3));
    updating_ = false;
}

void SketchToolInputDialog::setLineParameters(double length, double angleDeg)
{
    if (updating_) return;
    updating_ = true;
    ui->lineEdit_length->setText(QString::number(length, 'f', 3));
    ui->lineEdit_angle->setText(QString::number(angleDeg, 'f', 2));
    updating_ = false;
}

void SketchToolInputDialog::setArcParameters(double radius, double sweepDeg)
{
    if (updating_) return;
    updating_ = true;
    ui->lineEdit_radius->setText(QString::number(radius, 'f', 3));
    ui->lineEdit_sweep->setText(QString::number(sweepDeg, 'f', 2));
    updating_ = false;
}

double SketchToolInputDialog::xcValue() const
{
    bool ok = false;
    const double v = ui->lineEdit_xc->text().trimmed().toDouble(&ok);
    return ok ? v : 0.0;
}

double SketchToolInputDialog::ycValue() const
{
    bool ok = false;
    const double v = ui->lineEdit_yc->text().trimmed().toDouble(&ok);
    return ok ? v : 0.0;
}

double SketchToolInputDialog::lengthValue() const
{
    bool ok = false;
    const double v = ui->lineEdit_length->text().trimmed().toDouble(&ok);
    return ok ? v : 0.0;
}

double SketchToolInputDialog::angleValue() const
{
    bool ok = false;
    const double v = ui->lineEdit_angle->text().trimmed().toDouble(&ok);
    return ok ? v : 0.0;
}

double SketchToolInputDialog::radiusValue() const
{
    bool ok = false;
    const double v = ui->lineEdit_radius->text().trimmed().toDouble(&ok);
    return ok ? v : 0.0;
}

double SketchToolInputDialog::sweepAngleValue() const
{
    bool ok = false;
    const double v = ui->lineEdit_sweep->text().trimmed().toDouble(&ok);
    return ok ? v : 0.0;
}

void SketchToolInputDialog::closeEvent(QCloseEvent* event)
{
    emit closedByUser();
    QDialog::closeEvent(event);
}

void SketchToolInputDialog::onCloseClicked()
{
    emit closedByUser();
    reject();
}

void SketchToolInputDialog::onModeCoordToggled(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        ui->toolButton_modeParam->setChecked(false);
        updating_ = false;
        applyToolPages();
        syncModeButtons();
        emit inputKindChanged(Coordinate);
    } else if (!ui->toolButton_modeParam->isChecked()) {
        updating_ = true;
        ui->toolButton_modeCoord->setChecked(true);
        updating_ = false;
    }
}

void SketchToolInputDialog::onModeParamToggled(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        ui->toolButton_modeCoord->setChecked(false);
        updating_ = false;
        applyToolPages();
        syncModeButtons();
        emit inputKindChanged(Parameter);
    } else if (!ui->toolButton_modeCoord->isChecked()) {
        updating_ = true;
        ui->toolButton_modeParam->setChecked(true);
        updating_ = false;
    }
}

void SketchToolInputDialog::onObjLineToggled(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        ui->toolButton_objArc->setChecked(false);
        updating_ = false;
        objectKind_ = ObjLine;
        applyToolPages();
        syncObjectButtons();
        emit objectKindChanged(ObjLine);
    } else if (!ui->toolButton_objArc->isChecked()) {
        updating_ = true;
        ui->toolButton_objLine->setChecked(true);
        updating_ = false;
    }
}

void SketchToolInputDialog::onObjArcToggled(bool checked)
{
    if (updating_) return;
    if (checked) {
        updating_ = true;
        ui->toolButton_objLine->setChecked(false);
        updating_ = false;
        objectKind_ = ObjArc;
        applyToolPages();
        syncObjectButtons();
        emit objectKindChanged(ObjArc);
    } else if (!ui->toolButton_objLine->isChecked()) {
        updating_ = true;
        ui->toolButton_objArc->setChecked(true);
        updating_ = false;
    }
}

void SketchToolInputDialog::onAnyEditingFinished()
{
    if (updating_) return;
    emit valuesCommitted();
}

void SketchToolInputDialog::onSketchClicked()
{
    emit sketchButtonClicked();
}

void SketchToolInputDialog::applyToolPages()
{
    if (!ui->stackedWidget) return;

    const InputKind ik = inputKind();
    if (ik == Coordinate) {
        ui->stackedWidget->setCurrentWidget(ui->page_coord);
    } else {
        if (objectKind_ == ObjLine) {
            ui->stackedWidget->setCurrentWidget(ui->page_line_param);
        } else {
            ui->stackedWidget->setCurrentWidget(ui->page_arc_param);
        }
    }
}

void SketchToolInputDialog::syncModeButtons()
{
    const bool coord = ui->toolButton_modeCoord && ui->toolButton_modeCoord->isChecked();
    const QString base = QStringLiteral(
        "QToolButton { border: 1px solid #ccc; border-radius: 4px; background: %1; font-weight: bold; }");
    if (ui->toolButton_modeCoord) {
        ui->toolButton_modeCoord->setStyleSheet(base.arg(coord ? QStringLiteral("#b8e6e0") : QStringLiteral("#ffffff")));
    }
    if (ui->toolButton_modeParam) {
        ui->toolButton_modeParam->setStyleSheet(base.arg(!coord ? QStringLiteral("#b8e6e0") : QStringLiteral("#ffffff")));
    }
}

void SketchToolInputDialog::syncObjectButtons()
{
    const bool line = ui->toolButton_objLine && ui->toolButton_objLine->isChecked();
    const QString base = QStringLiteral(
        "QToolButton { border: 1px solid #ccc; border-radius: 4px; background: %1; font-weight: bold; }");
    if (ui->toolButton_objLine) {
        ui->toolButton_objLine->setStyleSheet(base.arg(line ? QStringLiteral("#b8e6e0") : QStringLiteral("#ffffff")));
    }
    if (ui->toolButton_objArc) {
        ui->toolButton_objArc->setStyleSheet(base.arg(!line ? QStringLiteral("#b8e6e0") : QStringLiteral("#ffffff")));
    }
}
