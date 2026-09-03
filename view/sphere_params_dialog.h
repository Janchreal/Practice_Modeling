#ifndef SPHEREPARAMSDIALOG_H
#define SPHEREPARAMSDIALOG_H

#include <QDialog>

class QPushButton;

namespace Ui {
class SphereParamsDialog;
}

class SphereParamsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SphereParamsDialog(QWidget *parent = nullptr);
    ~SphereParamsDialog();

    // 获取用户输入的参数
    double getRadius() const;
    int getThetaResolution() const;
    int getPhiResolution() const;
    
    // 设置参数值（用于编辑现有模型）
    void setRadius(double radius);
    void setThetaResolution(int resolution);
    void setPhiResolution(int resolution);
    
    // 获取和设置原点坐标
    void setOriginPoint(double x, double y, double z);
    void getOriginPoint(double& x, double& y, double& z) const;
    bool hasOriginPoint() const { return originPointSet; }

    bool isResultPreviewActive() const { return resultPreviewActive_; }
    void setResultPreviewActive(bool active);

signals:
    void requestPointSelection();  // 请求点选择信号
    void requestPointSelectionWithSnap(int snapKind);
    void applyRequested();  // 应用请求信号
    void previewRequested();
    void cancelPreviewRequested();

private slots:
    void on_okButton_clicked();

    void on_cancleButton_clicked();
    void on_designated_point_clicked();  // 指定点按钮点击
    void on_applyButton_clicked();  // 应用按钮点击
    void onShowResultButtonClicked();

private:
    void applyResultPreviewUiLock(bool locked);
    void ensureShowResultButton();

    Ui::SphereParamsDialog *ui;
    QPushButton* showResultButton_ = nullptr;
    bool originPointSet;  // 是否已设置原点
    double originX, originY, originZ;  // 原点坐标
    int originSnapKind_ = -1; // -1 表示未选择：走原逻辑；0最近点/1端点/2中点/3交点/4圆心/5象限点
    bool resultPreviewActive_ = false;
};

#endif // SPHEREPARAMSDIALOG_H
