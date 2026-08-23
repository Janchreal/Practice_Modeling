#ifndef PATTERNFEATUREDIALOG_H
#define PATTERNFEATUREDIALOG_H

#include <QDialog>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QToolButton;
class QWidget;

enum class PatternLayoutType {
    Linear = 0,
    Circular = 1,
    Polygonal = 2
};

/** 多边形间距：每边数目 / 沿边节距 */
enum class PolygonSpacingMode {
    CountPerSide = 0,
    PitchAlongEdge = 1
};

/** 点捕捉：-1 任意点（面射线求交），0~5 为 snap 捕捉类型 */
constexpr int kPatternPointSnapArbitrary = -1;

class PatternFeatureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PatternFeatureDialog(QWidget* parent = nullptr);

    PatternLayoutType layoutType() const;
    int layoutIndex() const;
    int count1() const;
    double pitch1() const;
    bool useDirection2() const;
    int count2() const;
    double pitch2() const;
    bool createConcentricMembers() const;
    double polygonSpanDegrees() const;
    PolygonSpacingMode polygonSpacingMode() const;
    int polygonAlongEdgeCount() const;
    double polygonAlongEdgePitch() const;

    bool hasDirection1() const { return hasDirection1_; }
    bool hasDirection2() const { return hasDirection2_; }
    bool hasRotationCenter() const { return hasRotationCenter_; }
    gp_Dir direction1() const { return direction1_; }
    gp_Dir direction2() const { return direction2_; }
    bool direction1Reversed() const { return direction1Reversed_; }
    bool direction2Reversed() const { return direction2Reversed_; }
    gp_Pnt rotationCenter() const { return rotationCenter_; }

    void setDirection1(const gp_Dir& dir, bool has);
    void setDirection2(const gp_Dir& dir, bool has);
    void setRotationCenter(const gp_Pnt& p, bool has);
    void setSelectedBodyCount(int count);
    void setPitch1(double pitch);
    void setPitch2(double pitch);
    void setPolygonSpan(double spanDegrees);

signals:
    void requestBodySelection();
    void requestClearBodies();
    void previewRequested();
    void applyRequested();
    void requestVectorMode(int directionIndex, int modeIndex);
    void requestOpenVectorDialog(int directionIndex);
    void requestPointSelection();
    void requestPointSelectionWithSnap(int snapKind);
    void parametersChanged();
    void pitch1Changed(double pitch);
    void pitch2Changed(double pitch);

private:
    void buildUi();
    void onLayoutChanged(int index);
    void syncPolygonalAngularPitch();
    void updatePolygonSpacingParamsVisible();
    void updateLayoutDependentUi();
    void updateDirection2Enabled();
    void updateRadialParamsVisible();
    void updateOkApplyEnabled();
    void setupVectorToolButton(QToolButton* btn, int directionIndex);
    void setupPointSnapToolButton();
    void applyDefaultArbitraryPointSnap();
    void toggleReverse(int directionIndex);
    void onSpecifyPointClicked();
    void applyRotaryGroupStyle(QGroupBox* box, bool enabled);

    QLabel* selectCountLabel_ = nullptr;
    QPushButton* selectBodiesBtn_ = nullptr;
    QComboBox* layoutCombo_ = nullptr;
    QStackedWidget* layoutStack_ = nullptr;
    QWidget* linearPage_ = nullptr;
    QWidget* rotaryPage_ = nullptr;

    QGroupBox* direction1Group_ = nullptr;
    QGroupBox* direction2Group_ = nullptr;
    QCheckBox* useDirection2Chk_ = nullptr;

    QGroupBox* rotationAxisGroup_ = nullptr;
    QGroupBox* angularParamGroup_ = nullptr;
    QGroupBox* radialGroup_ = nullptr;
    QPushButton* specifyPointBtn_ = nullptr;
    QToolButton* pointSnapToolBtn_ = nullptr;
    QCheckBox* concentricChk_ = nullptr;
    QWidget* radialParamsWidget_ = nullptr;

    QSpinBox* count1LinearSpin_ = nullptr;
    QSpinBox* count1Spin_ = nullptr;
    QSpinBox* count2Spin_ = nullptr;
    QSpinBox* count2RadialSpin_ = nullptr;
    QDoubleSpinBox* pitch1LinearSpin_ = nullptr;
    QDoubleSpinBox* pitch1Spin_ = nullptr;
    QDoubleSpinBox* pitch2Spin_ = nullptr;
    QDoubleSpinBox* pitch2RadialSpin_ = nullptr;
    QDoubleSpinBox* polygonSpanSpin_ = nullptr;
    QPushButton* okBtn_ = nullptr;
    QPushButton* applyBtn_ = nullptr;
    QToolButton* vectorToolBtn1_ = nullptr;
    QToolButton* vectorToolBtn2_ = nullptr;
    QToolButton* axisVectorToolBtn_ = nullptr;
    QPushButton* reverseBtn1_ = nullptr;
    QPushButton* reverseBtn2_ = nullptr;
    QPushButton* reverseAxisBtn_ = nullptr;
    QPushButton* openVectorBtn1_ = nullptr;
    QPushButton* openVectorBtn2_ = nullptr;
    QPushButton* openAxisVectorBtn_ = nullptr;
    QLabel* pitch1Label_ = nullptr;
    QLabel* pitch2Label_ = nullptr;
    QLabel* count1Label_ = nullptr;
    QLabel* polygonSpanLabel_ = nullptr;
    QLabel* polygonSpacingLabel_ = nullptr;
    QComboBox* polygonSpacingCombo_ = nullptr;
    QLabel* polygonAlongCountLabel_ = nullptr;
    QSpinBox* polygonAlongCountSpin_ = nullptr;
    QLabel* polygonEdgePitchLabel_ = nullptr;
    QDoubleSpinBox* polygonEdgePitchSpin_ = nullptr;
    QWidget* polygonAlongCountRow_ = nullptr;
    QWidget* polygonEdgePitchRow_ = nullptr;

    bool hasDirection1_ = false;
    bool hasDirection2_ = false;
    bool hasRotationCenter_ = false;
    bool direction1Reversed_ = false;
    bool direction2Reversed_ = false;
    int pointSnapKind_ = -1;
    gp_Dir direction1_ = gp_Dir(1, 0, 0);
    gp_Dir direction2_ = gp_Dir(0, 1, 0);
    gp_Pnt rotationCenter_;
};

#endif // PATTERNFEATUREDIALOG_H
