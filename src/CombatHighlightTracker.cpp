#include "CombatHighlightTracker.h"

#include <QtGlobal>

namespace {
bool isLowHpMoment(const CombatHighlightCandidate& candidate) {
    return candidate.playerMaxHp > 0
        && candidate.playerHpBefore * 100 <= candidate.playerMaxHp * 35;
}

bool leavesEnemyCritical(const CombatHighlightCandidate& candidate) {
    return candidate.enemyHpBefore > 0
        && candidate.enemyHpAfter > 0
        && candidate.enemyHpAfter * 100 <= candidate.enemyHpBefore * 35;
}
}

QString highlightAttackTypeApiKey(HighlightAttackType attackType) {
    switch (attackType) {
        case HighlightAttackType::Attack2:
            return QStringLiteral("attack_2");
        case HighlightAttackType::Attack3:
            return QStringLiteral("attack_3");
        case HighlightAttackType::Attack1:
        default:
            return QStringLiteral("attack_1");
    }
}

QString highlightAttackTypeDisplayName(HighlightAttackType attackType, bool wasProjectile) {
    QString label;
    switch (attackType) {
        case HighlightAttackType::Attack2:
            label = QStringLiteral("Attack 2");
            break;
        case HighlightAttackType::Attack3:
            label = QStringLiteral("Attack 3");
            break;
        case HighlightAttackType::Attack1:
        default:
            label = QStringLiteral("Attack 1");
            break;
    }

    if (wasProjectile) {
        label += QStringLiteral(" Projectile");
    }
    return label;
}

void CombatHighlightTracker::reset() {
    bestSnapshot_.reset();
    capturePending_ = false;
}

bool CombatHighlightTracker::considerCandidate(const CombatHighlightCandidate& candidate) {
    const int candidateScore = scoreCandidate(candidate);
    if (candidateScore <= 0) {
        return false;
    }

    if (bestSnapshot_.has_value() && candidateScore <= bestSnapshot_->highlightScore) {
        return false;
    }

    CombatHighlightSnapshot snapshot;
    snapshot.attackType = candidate.attackType;
    snapshot.damage = qMax(0, candidate.damage);
    snapshot.wasProjectile = candidate.wasProjectile;
    snapshot.wasFinisher = candidate.wasFinisher;
    snapshot.highlightScore = candidateScore;
    snapshot.playerHpBefore = qMax(0, candidate.playerHpBefore);
    snapshot.playerHpAfter = qMax(0, candidate.playerHpAfter);
    snapshot.playerMaxHp = qMax(0, candidate.playerMaxHp);
    snapshot.enemyHpBefore = qMax(0, candidate.enemyHpBefore);
    snapshot.enemyHpAfter = qMax(0, candidate.enemyHpAfter);

    bestSnapshot_ = snapshot;
    capturePending_ = true;
    return true;
}

bool CombatHighlightTracker::needsCapture() const {
    return capturePending_ && bestSnapshot_.has_value();
}

void CombatHighlightTracker::completeCapture(const QByteArray& imageBytes,
                                             const QString& imageMimeType,
                                             const QDateTime& capturedAtUtc) {
    if (!capturePending_ || !bestSnapshot_.has_value()) {
        return;
    }

    bestSnapshot_->imageBytes = imageBytes;
    bestSnapshot_->imageMimeType = imageMimeType.trimmed();
    bestSnapshot_->capturedAtUtc = capturedAtUtc;
    capturePending_ = false;
}

void CombatHighlightTracker::completeClip(const QByteArray& clipSheetBytes,
                                          const QString& clipSheetMimeType,
                                          const QString& clipKind,
                                          int clipFrameCount,
                                          int clipFps,
                                          int clipFrameWidth,
                                          int clipFrameHeight,
                                          double clipDurationSeconds) {
    if (!bestSnapshot_.has_value() || clipSheetBytes.isEmpty()) {
        return;
    }

    bestSnapshot_->clipSheetBytes = clipSheetBytes;
    bestSnapshot_->clipSheetMimeType = clipSheetMimeType.trimmed();
    bestSnapshot_->clipKind = clipKind.trimmed();
    bestSnapshot_->clipFrameCount = qMax(0, clipFrameCount);
    bestSnapshot_->clipFps = qMax(0, clipFps);
    bestSnapshot_->clipFrameWidth = qMax(0, clipFrameWidth);
    bestSnapshot_->clipFrameHeight = qMax(0, clipFrameHeight);
    bestSnapshot_->clipDurationSeconds = qMax(0.0, clipDurationSeconds);
}

std::optional<CombatHighlightSnapshot> CombatHighlightTracker::snapshot() const {
    if (!bestSnapshot_.has_value() || bestSnapshot_->imageBytes.isEmpty()) {
        return std::nullopt;
    }

    return bestSnapshot_;
}

int CombatHighlightTracker::scoreCandidate(const CombatHighlightCandidate& candidate) {
    int score = qMax(0, candidate.damage) * 100;

    switch (candidate.attackType) {
        case HighlightAttackType::Attack2:
            score += 90;
            break;
        case HighlightAttackType::Attack3:
            score += 140;
            break;
        case HighlightAttackType::Attack1:
        default:
            break;
    }

    if (candidate.wasProjectile) {
        score += 80;
    }
    if (leavesEnemyCritical(candidate)) {
        score += 110;
    }
    if (candidate.wasFinisher) {
        score += candidate.wasProjectile ? 260 : 220;
    }
    if (isLowHpMoment(candidate)) {
        score += 70;
    }

    return score;
}
