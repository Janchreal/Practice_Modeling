#ifndef SKETCHTOOLINPUTDIALOG_H
#define SKETCHTOOLINPUTDIALOG_H

#include <QCloseEvent>
#include <QDialog>

namespace Ui {
class SketchToolInputDialog;
}

/**
 * 草图「轮廓」浮动输入框：对象类型（直线/圆弧）、坐标模式（XC/YC）与参数模式
 * （直线：长度+角度；圆弧：半径+扫掠角，度）
 */
class SketchToolInputDialog : public QDialog
{
    Q_OBJECT

public:
    enum ObjectKind { ObjLine, ObjArc };
    enum InputKind { Coordinate, Parameter };

    explicit SketchToolInputDialog(ObjectKind initialObject, QWidget* parent = nullptr);
    ~SketchToolInputDialog() override;

    ObjectKind objectKind() const;
    void setObjectKind(ObjectKind k);

    InputKind inputKind() const;
    void setInputKind(InputKind k);

    void setCoordinateValues(double xc, double yc);
    void setLineParameters(double length, double angleDeg);
    void setArcParameters(double radius, double sweepDeg);

    double xcValue() const;
    double ycValue() const;
    double lengthValue() const;
    double angleValue() const;
    double radiusValue() const;
    double sweepAngleValue() const;

signals:
    void closedByUser();
    void inputKindChanged(SketchToolInputDialog::InputKind k);
    void objectKindChanged(SketchToolInputDialog::ObjectKind k);
    void valuesCommitted();
    void sketchButtonClicked();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCloseClicked();
    void onModeCoordToggled(bool checked);
    void onModeParamToggled(bool checked);
    void onObjLineToggled(bool checked);
    void onObjArcToggled(bool checked);
    void onAnyEditingFinished();
    void onSketchClicked();

private:
    Ui::SketchToolInputDialog* ui;
    ObjectKind objectKind_;
    bool updating_ = false;

    void applyToolPages();
    void syncModeButtons();
    void syncObjectButtons();
};

#endif // SKETCHTOOLINPUTDIALOG_H
