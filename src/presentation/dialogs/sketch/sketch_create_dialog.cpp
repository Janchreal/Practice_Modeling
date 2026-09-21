#include "sketch_create_dialog.h"
#include "ui_sketch_create_dialog.h"

#include <QCheckBox>
#include <QToolButton>

#include <gp_Ax3.hxx>

SketchCreateDialog::SketchCreateDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SketchCreateDialog)
{
    ui->setupUi(this);

    if (ui->toolButton_pickPlane) {
        connect(ui->toolButton_pickPlane, &QToolButton::clicked, this, [this]() {
            emit requestPickPlane();
        });
    }
    if (ui->checkBox_showPrincipalPlanes) {
        ui->checkBox_showPrincipalPlanes->setChecked(true);
        connect(ui->checkBox_showPrincipalPlanes, &QCheckBox::toggled,
                this, &SketchCreateDialog::showPrincipalPlanesChanged);
    }
    if (ui->toolButton_reverseNormal) {
        connect(ui->toolButton_reverseNormal, &QToolButton::clicked, this, [this]() {
            if (!hasPlane_) {
                return;
            }
            const gp_Ax3 ax = plane_.Position();
            const gp_Dir reversedNormal = ax.Direction().Reversed();
            plane_ = gp_Pln(gp_Ax3(ax.Location(), reversedNormal, ax.XDirection()));
            refreshPlaneText();
            emit planeNormalReversed(plane_);
        });
    }

    refreshPlaneText();
}

SketchCreateDialog::~SketchCreateDialog()
{
    delete ui;
}

void SketchCreateDialog::setPickedPlane(const gp_Pln& pln)
{
    plane_ = pln;
    hasPlane_ = true;
    refreshPlaneText();
}

bool SketchCreateDialog::hasPickedPlane() const
{
    return hasPlane_;
}

gp_Pln SketchCreateDialog::pickedPlane() const
{
    return plane_;
}

bool SketchCreateDialog::showPrincipalPlanes() const
{
    return ui && ui->checkBox_showPrincipalPlanes
        ? ui->checkBox_showPrincipalPlanes->isChecked()
        : true;
}

void SketchCreateDialog::refreshPlaneText()
{
    if (!ui || !ui->label_planeInfo) return;
    if (!hasPlane_) {
        ui->label_planeInfo->setText(tr("参考平面：未指定（请点击右侧拾取）"));
        return;
    }
    const gp_Pnt o = plane_.Location();
    const gp_Dir n = plane_.Axis().Direction();
    ui->label_planeInfo->setText(
        tr("参考平面：原点(%1, %2, %3) 法向(%4, %5, %6)")
            .arg(o.X(), 0, 'f', 3).arg(o.Y(), 0, 'f', 3).arg(o.Z(), 0, 'f', 3)
            .arg(n.X(), 0, 'f', 3).arg(n.Y(), 0, 'f', 3).arg(n.Z(), 0, 'f', 3)
    );
}

