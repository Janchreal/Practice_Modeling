#ifndef REVOLVEDIALOG_H
#define REVOLVEDIALOG_H

#include <QDialog>
#include <QString>

class QComboBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QWidget;

namespace Ui {
class revolvedialog;
}

class revolvedialog : public QDialog
{
    Q_OBJECT

public:
    explicit revolvedialog(QWidget *parent = nullptr);
    ~revolvedialog();

    double getStartAngle() const;
    double getEndAngle() const;
    void setStartAngle(double deg);
    void setEndAngle(double deg);
    QString getSectionType() const;
    QString getSelectionMode() const;

    void setSelectedGeometryCount(int count);

    bool isAxisVectorSelected() const { return axisVectorSelected_; }
    bool isAxisReversed() const { return axisReversed_; }
    bool isCenterPointSelected() const { return centerPointSelected_; }

    int vectorModeIndex() const { return vectorModeIndex_; }
    bool isAutoVectorMode() const { return vectorModeIndex_ == 0; }
    void setVectorModeIndex(int modeIndex);

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
    void pointSelectionRequested();
    void pointSelectionRequestedWithSnap(int snapKind);
    void parametersChanged();
    void startBooleanTargetSelection();
    void booleanModeChanged(int mode);

private slots:
    void on_geometrySelectorButton_clicked();
    void on_clearSelectionButton_clicked();
    void on_sectionTypeCombo_currentTextChanged(const QString &text);
    void on_startCombo_currentTextChanged(const QString &text);
    void on_endCombo_currentTextChanged(const QString &text);
    void on_previewButton_clicked();
    void on_selectButton_clicked();
    void on_pushButton_clicked();

private:
    void initializeUIState();
    void updateSelectionDisplay();
    void setupBooleanSection();
    void refreshBooleanUi();
    void setVectorModeLabel(int modeIndex);
    void applyResultPreviewUiLock(bool locked);

    Ui::revolvedialog *ui;
    int selectedCount = 0;
    bool axisVectorSelected_ = false;
    bool axisReversed_ = false;
    bool centerPointSelected_ = false;
    int originSnapKind_ = -1;
    int vectorModeIndex_ = 0;
    bool resultPreviewActive_ = false;

    QGroupBox* booleanGroup_ = nullptr;
    QWidget* booleanContent_ = nullptr;
    QComboBox* booleanCombo_ = nullptr;
    QPushButton* booleanSelectBodyBtn_ = nullptr;
    QLabel* booleanTargetLabel_ = nullptr;
    int booleanTargetIndex_ = -1;
};

#endif // REVOLVEDIALOG_H
