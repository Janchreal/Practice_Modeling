#ifndef INTERACTION_TOOLS_POINT_PICKER_H
#define INTERACTION_TOOLS_POINT_PICKER_H

#include "point_snap_types.h"

#include <functional>

enum class PointPickerState {
    Inactive,
    Searching,
    CandidateFound,
    Snapped,
    Confirmed
};

class PointPicker {
public:
    void startPick(const PointSnapMode& mode);
    void stopPick();
    void reset();

    void setSnapMode(const PointSnapMode& mode);
    PointSnapMode snapMode() const;

    void setCandidates(const PointCandidateList& candidates);
    bool updateSnappedCandidate(const std::function<double(const gp_Pnt&)>& screenDistanceSquared,
                                double snapPixelTolerance);
    bool confirmPoint();
    void cancelPick();

    bool hasCandidatePoint() const;
    bool hasSnappedPoint() const;
    bool isConfirmed() const;
    int currentCandidateIndex() const;
    PointPickerState state() const;
    PointPickResult currentResult() const;
    PointCandidateList candidates() const;

private:
    static PointPickResult resultFromCandidate(const PointCandidate& candidate,
                                               bool confirmed);

    PointPickerState state_ = PointPickerState::Inactive;
    PointSnapMode snapMode_;
    PointCandidateList candidates_;
    PointPickResult currentResult_;
    int currentCandidateIndex_ = -1;
};

#endif // INTERACTION_TOOLS_POINT_PICKER_H
