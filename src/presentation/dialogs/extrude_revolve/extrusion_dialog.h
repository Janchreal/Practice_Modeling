#ifndef EXTRUSIONDIALOG_H
#define EXTRUSIONDIALOG_H

#include <QDialog>

class QComboBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QToolButton;
class QWidget;

namespace Ui {
class ExtrusionDialog;
}

class ExtrusionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExtrusionDialog(QWidget *parent = nullptr);
    ~ExtrusionDialog();

    double getStartDistance() const;
    double getEndDistance() const;
    void setStartDistance(double v);
    void setEndDistance(double v);
    QString getSectionType() const;
    QString getBodyType() const;
    bool isSheetBodyType() const;
    QString getSelectionMode() const;

    void setSelectedGeometryCount(int count);

    bool isAxisVectorSelected() const { return axisVectorSelected_; }
    bool isAxisReversed() const { return axisReversed_; }

    /** 0=自动判断；其它与矢量对话框 modeIndex 一致 */
    int vectorModeIndex() const { return vectorModeIndex_; }
    bool isAutoVectorMode() const { return vectorModeIndex_ == 0; }
    void setVectorModeIndex(int modeIndex);

    /** -1=无, 0=合并(Fuse), 1=求交(Common), 2=减去(Cut) */
    int booleanMode() const;
    int booleanTargetIndex() const { return booleanTargetIndex_; }
    void setBooleanTargetIndex(int index, const QString& name = QString());

    /** 结果预览锁定：灰化参数控件，预览按钮变为「取消预览结果」 */
    bool isResultPreviewActive() const { return resultPreviewActive_; }
    void setResultPreviewActive(bool active);

signals:
    void previewRequested();
    void cancelPreviewRequested();
    void startSelection();
    void clearSelection();
    void selectionModeChanged(const QString& mode);
    void vectorSelectionRequested();
    void requestVectorMode(int modeIndex);
    void parametersChanged();
    void startBooleanTargetSelection();
    void booleanModeChanged(int mode);

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void on_geometrySelectorButton_clicked();
    void on_clearSelectionButton_clicked();
    void on_sectionTypeCombo_currentTextChanged(const QString &text);
    void on_startCombo_currentTextChanged(const QString &text);
    void on_endCombo_currentTextChanged(const QString &text);
    void on_previewButton_clicked();
    void on_selectButton_clicked();

private:
    void initializeUIState();
    void updateSelectionDisplay();
    void setupBooleanSection();
    void refreshBooleanUi();
    void setVectorModeLabel(int modeIndex);
    void applyResultPreviewUiLock(bool locked);

    Ui::ExtrusionDialog *ui;
    int selectedCount = 0;
    bool axisVectorSelected_ = false;
    bool axisReversed_ = false;
    int vectorModeIndex_ = 0;
    bool resultPreviewActive_ = false;

    QGroupBox* booleanGroup_ = nullptr;
    QWidget* booleanContent_ = nullptr;
    QComboBox* booleanCombo_ = nullptr;
    QPushButton* booleanSelectBodyBtn_ = nullptr;
    QLabel* booleanTargetLabel_ = nullptr;
    int booleanTargetIndex_ = -1;
};

#endif // EXTRUSIONDIALOG_H
