#ifndef CYLINDERDIALOG_H
#define CYLINDERDIALOG_H

#include <QDialog>

class QPushButton;

namespace Ui {
class CylinderDialog;
}

class CylinderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CylinderDialog(QWidget *parent = nullptr);
    ~CylinderDialog();

    // 获取用户输入的参数
    double getRadius() const;
    double getHeight() const;
    
    // 设置参数值（用于编辑现有模型）
    void setRadius(double radius);
    void setHeight(double height);
    
    // 获取和设置原点坐标
    void setOriginPoint(double x, double y, double z);
    void getOriginPoint(double& x, double& y, double& z) const;
    bool hasOriginPoint() const { return originPointSet; }

    // 矢量方向是否反转（用于沿所选轴反向创建）
    bool isAxisReversed() const { return axisReversed; }

    bool isResultPreviewActive() const { return resultPreviewActive_; }
    void setResultPreviewActive(bool active);

signals:
    void requestPointSelection();  // 请求点选择信号
    void requestPointSelectionWithSnap(int snapKind);
    void applyRequested();  // 应用请求信号
    void requestVectorMode(int modeIndex); // toolButton 选择矢量模式
    void previewRequested();
    void cancelPreviewRequested();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_designated_point_clicked();  // 指定点按钮点击
    void on_applyButton_clicked();  // 应用按钮点击
    void onShowResultButtonClicked();

private:
    void applyResultPreviewUiLock(bool locked);
    void ensureShowResultButton();

    Ui::CylinderDialog *ui;
    QPushButton* showResultButton_ = nullptr;
    bool originPointSet;  // 是否已设置原点
    double originX, originY, originZ;  // 原点坐标
    bool axisReversed = false;
    int originSnapKind_ = -1; // -1 表示未选择：走原逻辑；0最近点/1端点/2中点/3交点/4圆心/5象限点
    bool resultPreviewActive_ = false;
};

#endif // CYLINDERDIALOG_H
