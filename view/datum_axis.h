#ifndef DATUM_AXIS_H
#define DATUM_AXIS_H

#include "axisdirection.h"

#include <QDialog>

class QComboBox;
class QPushButton;

/** 基准轴对话框：选择 XC/YC/ZC 轴，并可切换反向 */
class DatumAxisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DatumAxisDialog(QWidget* parent = nullptr);

    AxisDirection axisDirection() const;
    QString axisText() const;
    bool isReversed() const { return reversed_; }

signals:
    void parametersChanged();

private:
    void setReversed(bool on);
    void refreshReverseButtonStyle();

    QComboBox* axisCombo_ = nullptr;
    QPushButton* reverseBtn_ = nullptr;
    bool reversed_ = false;
};

#endif // DATUM_AXIS_H
