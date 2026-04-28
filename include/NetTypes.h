#ifndef NETTYPES_H
#define NETTYPES_H

#include <QMetaType>
#include <QString>
#include <QtGlobal>

#include "Enums.h"

enum class LanRole {
    NONE,
    HOST,
    GUEST
};

enum class LanSessionState {
    IDLE,
    HOSTING,
    CONNECTING,
    LINKED,
    READY_CHECK,
    MATCH_PRIMED,
    ERROR
};

enum class LanPacketType {
    HELLO,
    CHARACTER_SELECT,
    READY_STATE,
    START_MATCH,
    COMBAT_INPUT,
    COMBAT_STATE,
    PING,
    PONG,
    DISCONNECT
};

enum class LanCombatWinner {
    NONE = 0,
    HOST = 1,
    GUEST = 2
};

enum LanCombatInputBit : quint8 {
    LanInputMoveLeft = 1 << 0,
    LanInputMoveRight = 1 << 1,
    LanInputAttack1 = 1 << 2,
    LanInputAttack2 = 1 << 3,
    LanInputAttack3 = 1 << 4,
    LanInputHeal = 1 << 5,
    LanInputPause = 1 << 6
};

struct LanPlayerInfo {
    QString username;
    QString fighterName;
    int fighterType = -1;
    bool ready = false;
};

struct LanCombatInputFrame {
    quint32 sequence = 0;
    quint8 inputBits = 0;
};

struct LanCombatantStats {
    int score = 0;
    int damageDealt = 0;
    int damageTaken = 0;
    int healsUsed = 0;
    int attacks = 0;
    int hits = 0;
    int misses = 0;
    int projectilesFired = 0;
    int projectilesHit = 0;
};

struct LanCombatState {
    quint32 tick = 0;
    bool battleActive = false;
    bool matchFinished = false;
    LanCombatWinner winner = LanCombatWinner::NONE;
    double battleDurationSeconds = 0.0;
    double introLockTime = 0.0;

    double hostX = 0.0;
    double guestX = 0.0;
    int hostHp = 0;
    int guestHp = 0;
    bool hostFacingLeft = false;
    bool guestFacingLeft = false;
    AnimationState hostAnimation = AnimationState::IDLE;
    AnimationState guestAnimation = AnimationState::IDLE;
    double hostAttackCooldown = 0.0;
    double guestAttackCooldown = 0.0;
    double hostHealCooldown = 0.0;
    double guestHealCooldown = 0.0;

    bool hostProjectileActive = false;
    bool hostProjectileFacingRight = true;
    double hostProjectileX = 0.0;
    double hostProjectileY = 0.0;
    int hostProjectileFrame = 0;

    bool guestProjectileActive = false;
    bool guestProjectileExploding = false;
    bool guestProjectileFacingRight = false;
    EnemyType guestProjectileType = EnemyType::FIRE_WORM;
    double guestProjectileX = 0.0;
    double guestProjectileY = 0.0;
    int guestProjectileFrame = 0;

    QString statusMessage;
    double statusSeconds = 0.0;

    LanCombatantStats hostStats;
    LanCombatantStats guestStats;
};

struct LanSessionSnapshot {
    LanRole localRole = LanRole::NONE;
    LanSessionState state = LanSessionState::IDLE;
    QString hostAddress;
    quint16 port = 44777;
    QString arenaName = QStringLiteral("Molten Gate");
    LanPlayerInfo localPlayer;
    LanPlayerInfo remotePlayer;
    bool remoteConnected = false;
    bool canStartMatch = false;
    int lastPingMs = -1;
    QString statusLine = QStringLiteral("Open Arena Link to prepare a nearby duel.");
};

inline QString lanRoleDisplayName(LanRole role) {
    switch (role) {
        case LanRole::HOST:
            return QStringLiteral("Host");
        case LanRole::GUEST:
            return QStringLiteral("Guest");
        case LanRole::NONE:
        default:
            return QStringLiteral("Offline");
    }
}

inline QString lanSessionStateDisplayName(LanSessionState state) {
    switch (state) {
        case LanSessionState::HOSTING:
            return QStringLiteral("Hosting");
        case LanSessionState::CONNECTING:
            return QStringLiteral("Connecting");
        case LanSessionState::LINKED:
            return QStringLiteral("Linked");
        case LanSessionState::READY_CHECK:
            return QStringLiteral("Ready Check");
        case LanSessionState::MATCH_PRIMED:
            return QStringLiteral("Match Primed");
        case LanSessionState::ERROR:
            return QStringLiteral("Alert");
        case LanSessionState::IDLE:
        default:
            return QStringLiteral("Idle");
    }
}

constexpr quint16 kDefaultLanPort = 44777;

Q_DECLARE_METATYPE(LanPlayerInfo)
Q_DECLARE_METATYPE(LanCombatInputFrame)
Q_DECLARE_METATYPE(LanCombatantStats)
Q_DECLARE_METATYPE(LanCombatState)
Q_DECLARE_METATYPE(LanSessionSnapshot)

#endif // NETTYPES_H
