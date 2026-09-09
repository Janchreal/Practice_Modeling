#ifndef SKETCHELLIPSEDIALOG_H
#define SKETCHELLIPSEDIALOG_H

#include <QDialog>

class QDoubleSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

/** 草图椭圆：中心、大/小半径指定点与数值、旋转角 */
class SketchEllipseDialog : public QDialog
{
    Q_OBJECT
public:
    enum PickField { PickNone = -1, PickCenter = 0, PickMajor = 1, PickMinor = 2 };

    explicit SketchEllipseDialog(QWidget* parent = nullptr);

    double majorRadius() const;
    double minorRadius() const;
    double rotationDeg() const;
    /** -1 任意点；0 最近点…5 象限点 */
    int centerSnapKind() const;
    int majorSnapKind() const;
    int minorSnapKind() const;

    void setCenterText(const QString& s);
    void setMajorPickText(const QString& s);
    void setMinorPickText(const QString& s);
    void setMajorRadius(double v);
    void setMinorRadius(double v);
    /** 同时设置大/小半径，不触发 paramsChanged（由调用方统一刷新几何） */
    void setRadii(double majorR, double minorR);
    void setRotationDeg(double v);
    void highlightPickField(PickField field);

signals:
    void pickCenterRequested();
    void pickMajorRequested();
    void pickMinorRequested();
    void paramsChanged();
    void applyRequested();
    void okRequested();
    void closedByUser();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCloseClicked();
    void onApply();
    void onOk();

private:
    static void fillOriginSnapMenu(QToolButton* btn, int* kindStorage, QWidget* host);
    void restylePickRows();
    QFrame* makePickRow(const QString& label, QLineEdit** editOut, QToolButton** snapBtnOut,
                        int* snapKindOut, QPushButton** pickBtnOut);

    QFrame* rowCenter_ = nullptr;
    QFrame* rowMajor_ = nullptr;
    QFrame* rowMinor_ = nullptr;
    QLineEdit* centerEdit_ = nullptr;
    QLineEdit* majorPickEdit_ = nullptr;
    QLineEdit* minorPickEdit_ = nullptr;
    QToolButton* centerSnapBtn_ = nullptr;
    QToolButton* majorSnapBtn_ = nullptr;
    QToolButton* minorSnapBtn_ = nullptr;
    QDoubleSpinBox* majorSpin_ = nullptr;
    QDoubleSpinBox* minorSpin_ = nullptr;
    QDoubleSpinBox* rotSpin_ = nullptr;
    int centerSnapKind_ = -1;
    int majorSnapKind_ = -1;
    int minorSnapKind_ = -1;
    PickField highlightField_ = PickCenter;
};

/** 椭圆旋转：跟随鼠标的角度输入框 */
class SketchEllipseAngleDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SketchEllipseAngleDialog(QWidget* parent = nullptr);

    void setRotationDeg(double v);
    double rotationDeg() const;
    void focusAngleField();
    bool angleFieldHasFocus() const;

signals:
    void angleCommitted();
    void closedByUser();

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QLineEdit* angleEdit_ = nullptr;
};

#endif // SKETCHELLIPSEDIALOG_H
