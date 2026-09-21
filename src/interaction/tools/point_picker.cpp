#include "point_picker.h"

#include <limits>

void PointPicker::startPick(const PointSnapMode& mode)
{
    snapMode_ = mode;
    candidates_.clear();
    currentResult_ = PointPickResult{};
    currentCandidateIndex_ = -1;
    state_ = PointPickerState::Searching;
}

void PointPicker::stopPick()
{
    reset();
    state_ = PointPickerState::Inactive;
}

void PointPicker::reset()
{
    candidates_.clear();
    currentResult_ = PointPickResult{};
    currentCandidateIndex_ = -1;
    state_ = snapMode_.hasAnyType() ? PointPickerState::Searching
                                    : PointPickerState::Inactive;
}

void PointPicker::setSnapMode(const PointSnapMode& mode)
{
    snapMode_ = mode;
    if (state_ == PointPickerState::Inactive && mode.hasAnyType()) {
        state_ = PointPickerState::Searching;
    }
}

PointSnapMode PointPicker::snapMode() const
{
    return snapMode_;
}

void PointPicker::setCandidates(const PointCandidateList& candidates)
{
    candidates_ = candidates;
    currentResult_ = PointPickResult{};
    currentCandidateIndex_ = -1;
    if (state_ != PointPickerState::Inactive) {
        state_ = candidates_.isEmpty() ? PointPickerState::Searching
                                       : PointPickerState::CandidateFound;
    }
}

bool PointPicker::updateSnappedCandidate(
    const std::function<double(const gp_Pnt&)>& screenDistanceSquared,
    double snapPixelTolerance)
{
    currentResult_ = PointPickResult{};
    currentCandidateIndex_ = -1;

    if (candidates_.isEmpty() || !screenDistanceSquared) {
        if (state_ != PointPickerState::Inactive) {
            state_ = PointPickerState::Searching;
        }
        return false;
    }

    double bestDistanceSquared = std::numeric_limits<double>::max();
    for (int i = 0; i < candidates_.size(); ++i) {
        const double d2 = screenDistanceSquared(candidates_[i].point);
        candidates_[i].screenDistanceSquared = d2;
        if (d2 < bestDistanceSquared) {
            bestDistanceSquared = d2;
            currentCandidateIndex_ = i;
        }
    }

    if (currentCandidateIndex_ < 0) {
        state_ = PointPickerState::Searching;
        return false;
    }

    currentResult_ = resultFromCandidate(candidates_[currentCandidateIndex_], false);
    const double toleranceSquared = snapPixelTolerance * snapPixelTolerance;
    if (bestDistanceSquared <= toleranceSquared) {
        state_ = PointPickerState::Snapped;
        return true;
    }

    state_ = PointPickerState::CandidateFound;
    currentResult_.valid = false;
    return false;
}

bool PointPicker::confirmPoint()
{
    if (currentCandidateIndex_ < 0 || currentCandidateIndex_ >= candidates_.size()) {
        return false;
    }
    currentResult_ = resultFromCandidate(candidates_[currentCandidateIndex_], true);
    state_ = PointPickerState::Confirmed;
    return true;
}

void PointPicker::cancelPick()
{
    currentResult_ = PointPickResult{};
    currentCandidateIndex_ = -1;
    state_ = snapMode_.hasAnyType() ? PointPickerState::Searching
                                    : PointPickerState::Inactive;
}

bool PointPicker::hasCandidatePoint() const
{
    return !candidates_.isEmpty();
}

bool PointPicker::hasSnappedPoint() const
{
    return state_ == PointPickerState::Snapped || state_ == PointPickerState::Confirmed;
}

bool PointPicker::isConfirmed() const
{
    return state_ == PointPickerState::Confirmed && currentResult_.valid;
}

int PointPicker::currentCandidateIndex() const
{
    return currentCandidateIndex_;
}

PointPickerState PointPicker::state() const
{
    return state_;
}

PointPickResult PointPicker::currentResult() const
{
    return currentResult_;
}

PointCandidateList PointPicker::candidates() const
{
    return candidates_;
}

PointPickResult PointPicker::resultFromCandidate(const PointCandidate& candidate,
                                                 bool confirmed)
{
    PointPickResult result;
    result.valid = true;
    result.point = candidate.point;
    result.snapType = candidate.snapType;
    result.sourceShape = candidate.sourceShape;
    result.sourceEdge = candidate.sourceEdge;
    result.parameter = candidate.parameter;
    result.hasParameter = candidate.hasParameter;
    result.isConfirmed = confirmed;
    return result;
}
