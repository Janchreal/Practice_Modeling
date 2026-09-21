#ifndef SKETCHCREATEDIALOG_H
#define SKETCHCREATEDIALOG_H

#include <QDialog>
#include <gp_Pln.hxx>

namespace Ui {
class SketchCreateDialog;
}

class SketchCreateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SketchCreateDialog(QWidget* parent = nullptr);
    ~SketchCreateDialog();

    void setPickedPlane(const gp_Pln& pln);
    bool hasPickedPlane() const;
    gp_Pln pickedPlane() const;
    bool showPrincipalPlanes() const;

signals:
    void requestPickPlane();
    void showPrincipalPlanesChanged(bool visible);
    void planeNormalReversed(const gp_Pln& plane);

private:
    Ui::SketchCreateDialog* ui;
    bool hasPlane_ = false;
    gp_Pln plane_;
    void refreshPlaneText();
};

#endif // SKETCHCREATEDIALOG_H

