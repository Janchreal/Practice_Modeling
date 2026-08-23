#ifndef SKETCHCONICDIALOG_H
#define SKETCHCONICDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;

/** 草图二次曲线：三点 + Rho，拾取与预览由 Widget 驱动 */
class SketchConicDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SketchConicDialog(QWidget* parent = nullptr);

    double rho() const;
    bool previewEnabled() const;

    /** 与 armSnapFiltersForSketchToolFromMenuKind 一致：0 综合，1 端点…5 象限 */
    int startSnapKind() const;
    int endSnapKind() const;
    int controlSnapKind() const;

    void setOkApplyEnabled(bool on);
    void setStartText(const QString& s);
    void setEndText(const QString& s);
    void setControlText(const QString& s);
    /** -1 无高亮；0 起点；1 终点；2 控制点 */
    void highlightPickField(int field);

signals:
    void pickStartRequested();
    void pickEndRequested();
    void pickControlRequested();
    void rhoValueChanged(double v);
    void previewToggled(bool on);
    void applyRequested();

private slots:
    void onApply();
    void onOk();
    void emitRhoFromSpin(double v);

private:
    static void fillSnapCombo(QComboBox* cb);
    void restyleRows();

    QFrame* rowStart_ = nullptr;
    QFrame* rowEnd_ = nullptr;
    QFrame* rowCtrl_ = nullptr;
    QLineEdit* startEdit_ = nullptr;
    QLineEdit* endEdit_ = nullptr;
    QLineEdit* ctrlEdit_ = nullptr;
    QComboBox* startSnap_ = nullptr;
    QComboBox* endSnap_ = nullptr;
    QComboBox* ctrlSnap_ = nullptr;
    QDoubleSpinBox* rhoSpin_ = nullptr;
    QCheckBox* previewChk_ = nullptr;
    QPushButton* btnApply_ = nullptr;
    QPushButton* btnOk_ = nullptr;
    int highlightField_ = -1;
};

#endif // SKETCHCONICDIALOG_H
