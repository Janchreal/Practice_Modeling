#ifndef CONEPARAMSDIALOG_H
#define CONEPARAMSDIALOG_H

#include <QDialog>

class QPushButton;

namespace Ui {
class ConeParamsDialog;
}

class ConeParamsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConeParamsDialog(QWidget *parent = nullptr);
    ~ConeParamsDialog();

    double getRadius1() const;  // 底部半径
    double getRadius2() const;  // 顶部半径
    double getHeight() const;   // 高度
    
    // 设置参数值（用于编辑现有模型）
    void setRadius1(double radius1);
    void setRadius2(double radius2);
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
    // 使用 QDialogButtonBox 的标准按钮信号
    void on_okandcancel_rejected();

    void on_okandcancel_accepted();
    void on_designated_point_clicked();  // 指定点按钮点击
    void on_applyButton_clicked();  // 应用按钮点击
    void onShowResultButtonClicked();

private:
    void applyResultPreviewUiLock(bool locked);
    void ensureShowResultButton();

    Ui::ConeParamsDialog *ui;
    QPushButton* showResultButton_ = nullptr;
    bool originPointSet;  // 是否已设置原点
    double originX, originY, originZ;  // 原点坐标
    bool axisReversed = false;
    int originSnapKind_ = -1; // -1 表示未选择：走原逻辑；0最近点/1端点/2中点/3交点/4圆心/5象限点
    bool resultPreviewActive_ = false;
};

#endif // CONEPARAMSDIALOG_H
