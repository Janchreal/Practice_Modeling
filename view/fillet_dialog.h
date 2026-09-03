#ifndef FILLETDIALOG_H
#define FILLETDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>

namespace Ui {
class filletdialog;
}

class filletdialog : public QDialog
{
    Q_OBJECT

public:
    explicit filletdialog(QWidget *parent = nullptr);
    ~filletdialog();

    QString continuityText() const;  // "G1（相切）" / "G2（曲率）"
    QString shapeText() const;       // "圆" / "二次曲线"
    double radiusValue() const;
    void setRadiusValue(double v);
    double rhoValue() const;
    void setSelectedEdgeCount(int count);

signals:
    void edgeModeHintRequested();
    void parametersChanged();

private:
    void updateUiByContinuity();

    Ui::filletdialog *ui;
    QLabel* rhoLabel_ = nullptr;
    QDoubleSpinBox* rhoSpinBox_ = nullptr;
};

#endif // FILLETDIALOG_H
