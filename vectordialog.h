#ifndef VECTORDIALOG_H
#define VECTORDIALOG_H

#include <QDialog>

class QPushButton;
class QToolButton;
class QComboBox;
class QDoubleSpinBox;
class QLabel;

namespace Ui {
class vectordialog;
}

class vectordialog : public QDialog
{
    Q_OBJECT

public:
    explicit vectordialog(QWidget *parent = nullptr);
    ~vectordialog();

    void setModeIndex(int modeIndex);
    int modeIndex() const;

    // 仅“曲线上矢量”（modeIndex==4）使用：由 Widget 回写曲线总弧长与UI启用态
    void setCurveTotalLength(double totalLength);
    void setCurvePicked(bool picked);
    void setVectorDirDisplay(double x, double y, double z);

signals:
    void vectorModeChanged(int modeIndex);
    void vectorReverseToggled(bool reversed);
    void twoPointStartRequested();
    void twoPointEndRequested();
    // 两点模式：由 toolbutton 指定捕捉类型后触发拾取
    // snapKind 约定（对应 Widget 里的 snap_.xxx 逻辑）：
    // 1=端点, 2=中点, 5=象限点, 6=圆弧中点, 3=交点
    // 点击“指定出发点”时：同时把当前终点 toolbutton 选择的 snapKind 一并传给 Widget
    void twoPointStartRequestedWithSnap(int startSnapKind, int endSnapKind);
    void twoPointEndRequestedWithSnap(int snapKind);
    void curveSelectionRequested();

    // 曲线上矢量：位置类型与数值变化
    // positionMode: 0=弧长百分比, 1=弧长
    void curveVectorPositionModeChanged(int positionMode);
    void curveVectorPositionValueChanged(double value);

private:
    void rebuildVectorDefineArea(int modeIndex);

    bool reverse_ = false;
    QPushButton* twoPointStartBtn_ = nullptr;
    QPushButton* twoPointEndBtn_ = nullptr;
    QToolButton* twoPointStartSnapToolButton_ = nullptr;
    QToolButton* twoPointEndSnapToolButton_ = nullptr;

    // 两点模式：起点/终点捕捉类型（由 toolbutton 指定）
    int twoPointStartSnapKind_ = 1; // 默认：端点
    int twoPointEndSnapKind_ = 1;   // 默认：端点

    // 两点模式：是否由 toolbutton 明确选择了捕捉类型
    // 若为 false，则表示“任意点”模式（主界面启用多类型捕捉）
    bool twoPointStartSnapChosen_ = false;
    bool twoPointEndSnapChosen_ = false;

    // 曲线上矢量：位置控件
    QComboBox* curvePosModeCombo_ = nullptr;
    QDoubleSpinBox* curvePosValueSpin_ = nullptr;
    QLabel* curveTotalLenLabel_ = nullptr;
    QLabel* curveVectorDirLabel_ = nullptr;
    bool curvePicked_ = false;
    double curveTotalLength_ = 0.0;

private:
    Ui::vectordialog *ui;
};

#endif // VECTORDIALOG_H
