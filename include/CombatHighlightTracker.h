#ifndef COMBATHIGHLIGHTTRACKER_H
#define COMBATHIGHLIGHTTRACKER_H

#include <optional>

#include "CombatHighlightTypes.h"

class CombatHighlightTracker {
public:
    void reset();
    bool considerCandidate(const CombatHighlightCandidate& candidate);
    bool needsCapture() const;
    void completeCapture(const QByteArray& imageBytes,
                         const QString& imageMimeType,
                         const QDateTime& capturedAtUtc);
    std::optional<CombatHighlightSnapshot> snapshot() const;

private:
    static int scoreCandidate(const CombatHighlightCandidate& candidate);

    std::optional<CombatHighlightSnapshot> bestSnapshot_;
    bool capturePending_ = false;
};

#endif // COMBATHIGHLIGHTTRACKER_H
