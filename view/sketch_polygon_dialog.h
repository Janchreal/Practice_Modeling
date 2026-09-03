#ifndef SKETCHPOLYGONDIALOG_H
#define SKETCHPOLYGONDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QToolButton;
class QWidget;

/** 草图多边形：中心点、边数、大小（内切/外接/边长）与锁定参数 */
class SketchPolygonDialog : public QDialog
{
    Q_OBJECT
public:
    enum SizeMode { InscribedRadius = 0, CircumscribedRadius = 1, SideLength = 2 };
    enum PickField { PickNone = -1, PickCenter = 0, PickSize = 1 };

    explicit SketchPolygonDialog(QWidget* parent = nullptr);

    int sideCount() const;
    SizeMode sizeMode() const;
    /** 内切/外接半径或边长（mm） */
    double sizeValue() const;
    double rotationDeg() const;
    bool sizeLocked() const;
    bool rotationLocked() const;
    /** -1 任意点；0 最近点…5 象限点（与长方体 originSnap 一致） */
    int centerSnapKind() const;
    int sizeSnapKind() const;

    void setCenterText(const QString& s);
    void setSizeText(const QString& s);
    void setSizeValue(double v);
    void setRotationDeg(double v);
    void highlightPickField(PickField field);
    void syncSizeRowLabels();

signals:
    void pickCenterRequested();
    void pickSizeRequested();
    void sizeModeChanged(SketchPolygonDialog::SizeMode mode);
    void paramsChanged();
    void closedByUser();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCloseClicked();
    void onSizeModeIndexChanged(int index);

private:
    static void fillOriginSnapMenu(QToolButton* btn, int* kindStorage, QWidget* host);
    void restylePickRows();

    QFrame* rowCenter_ = nullptr;
    QFrame* rowSize_ = nullptr;
    QLineEdit* centerEdit_ = nullptr;
    QLineEdit* sizeEdit_ = nullptr;
    QToolButton* centerSnapBtn_ = nullptr;
    QToolButton* sizeSnapBtn_ = nullptr;
    QSpinBox* sideSpin_ = nullptr;
    QComboBox* sizeModeCombo_ = nullptr;
    QCheckBox* sizeLockChk_ = nullptr;
    QDoubleSpinBox* sizeSpin_ = nullptr;
    QLabel* sizeParamLabel_ = nullptr;
    QCheckBox* rotLockChk_ = nullptr;
    QDoubleSpinBox* rotSpin_ = nullptr;
    int centerSnapKind_ = -1;
    int sizeSnapKind_ = -1;
    PickField highlightField_ = PickCenter;
};

/** 多边形尺寸阶段：跟随鼠标的半径/长度 + 旋转输入框 */
class SketchPolygonValueDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SketchPolygonValueDialog(QWidget* parent = nullptr);

    void setSizeMode(SketchPolygonDialog::SizeMode mode);
    void setSizeValue(double v);
    void setRotationDeg(double v);
    double sizeValue() const;
    double rotationDeg() const;

    /** 0 = 尺寸栏，1 = 旋转栏 */
    void focusField(int index);
    void focusFirstFieldFromArrowKey();
    void focusNextFieldOnEnter();
    void switchFieldByVerticalArrow(int key);

signals:
    void valuesCommitted();
    void closedByUser();

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onSizeEditingFinished();
    void onRotEditingFinished();

private:
    void hookLineEdit(QLineEdit* le, int fieldIndex);

    QLineEdit* sizeEdit_ = nullptr;
    QLineEdit* rotEdit_ = nullptr;
    QLabel* sizeLabel_ = nullptr;
    int focusedField_ = 0;
    SketchPolygonDialog::SizeMode sizeMode_ = SketchPolygonDialog::InscribedRadius;
};

#endif // SKETCHPOLYGONDIALOG_H
