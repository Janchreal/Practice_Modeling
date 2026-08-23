#ifndef CUBOIDPARAMSDIALOG_H
#define CUBOIDPARAMSDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;

// 前向声明UI类
namespace Ui {
class CuboidParamsDialog;
}

class CuboidParamsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CuboidParamsDialog(QWidget *parent = nullptr);
    ~CuboidParamsDialog();

    double getLength() const;
    double getWidth() const;
    double getHeight() const;

    // 表达式原文（尺寸组 XC/YC/ZC）
    QString lengthExpression() const;
    QString widthExpression() const;
    QString heightExpression() const;

    // 设置参数值（用于编辑现有模型）
    void setLength(double length);
    void setWidth(double width);
    void setHeight(double height);

    // 原点相关方法
    bool hasOriginPoint() const;
    void getOriginPoint(double& x, double& y, double& z) const;
    void setOriginPoint(double x, double y, double z);

    // 矢量方向反转（与 vectordialog 的 reverse 叠加用于高度轴反向）
    bool isAxisReversed() const { return axisReversed_; }

    /** 显示结果锁定：灰化参数控件，按钮变为「撤销结果」 */
    bool isResultPreviewActive() const { return resultPreviewActive_; }
    void setResultPreviewActive(bool active);

    /** 创建方式（默认：原点和边长） */
    QString creationModeText() const;

signals:
    void requestPointSelection();
    void requestPointSelectionWithSnap(int snapKind);
    void applyRequested();
    void requestVectorMode(int modeIndex); // toolButton 选择矢量模式
    void previewRequested();
    void cancelPreviewRequested();
    void dimensionsChanged(); // 尺寸表达式变更

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void on_designated_point_clicked();  // 指定点按钮点击

    void on_applyButton_clicked();
    void onShowResultButtonClicked();

private:
    void applyResultPreviewUiLock(bool locked);
    void ensureShowResultButton();
    void setupDimensionExpressionFields();
    bool commitExpressionField(QLineEdit* edit, double* cachedValue) const;

    Ui::CuboidParamsDialog *ui;  // UI指针
    QPushButton* showResultButton_ = nullptr;
    QLineEdit* lengthExprEdit_ = nullptr;
    QLineEdit* widthExprEdit_ = nullptr;
    QLineEdit* heightExprEdit_ = nullptr;
    double lengthValue_ = 2.0;
    double widthValue_ = 2.0;
    double heightValue_ = 2.0;
    double originX, originY, originZ;
    bool hasOrigin;
    int originSnapKind_ = -1; // -1 表示未选择：走原逻辑；0最近点/1端点/2中点/3交点/4圆心/5象限点

    bool axisReversed_ = false;
    bool resultPreviewActive_ = false;
};

#endif // CUBOIDPARAMSDIALOG_H
