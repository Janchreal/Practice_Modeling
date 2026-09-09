#ifndef SKETCHMODEDIALOGS_H
#define SKETCHMODEDIALOGS_H

#include <QDialog>

class QCloseEvent;
class QLabel;
class QToolButton;

/** 长方体草图：矩形方法（两点对角 / 三点 / 从中心） */
class SketchRectangleModeDialog : public QDialog
{
    Q_OBJECT
public:
    enum Method { TwoDiagonal = 0, ThreePoint = 1, FromCenter = 2 };

    explicit SketchRectangleModeDialog(QWidget* parent = nullptr);
    Method method() const;
    void setMethod(Method m);

signals:
    void methodChanged(SketchRectangleModeDialog::Method m);
    void closedByUser();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCloseClicked();
    void onM0(bool checked);
    void onM1(bool checked);
    void onM2(bool checked);

private:
    void syncButtons();
    QToolButton* btnTwo_ = nullptr;
    QToolButton* btnThree_ = nullptr;
    QToolButton* btnCenter_ = nullptr;
    Method method_ = TwoDiagonal;
    bool updating_ = false;
};

/** 圆：圆心+半径（直径由两点确定）/ 三点定圆 */
class SketchCircleModeDialog : public QDialog
{
    Q_OBJECT
public:
    enum Method { CenterRadius = 0, ThreePoint = 1 };

    explicit SketchCircleModeDialog(QWidget* parent = nullptr);
    Method method() const;
    void setMethod(Method m);

signals:
    void methodChanged(SketchCircleModeDialog::Method m);
    void closedByUser();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCloseClicked();
    void onCenter(bool checked);
    void onThree(bool checked);

private:
    void syncButtons();
    QToolButton* btnCenter_ = nullptr;
    QToolButton* btnThree_ = nullptr;
    Method method_ = CenterRadius;
    bool updating_ = false;
};

#endif // SKETCHMODEDIALOGS_H
