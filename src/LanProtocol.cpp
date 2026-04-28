#include "LanProtocol.h"

#include <QJsonDocument>

namespace {

QString packetNameForType(LanPacketType type) {
    switch (type) {
        case LanPacketType::HELLO:
            return QStringLiteral("hello");
        case LanPacketType::CHARACTER_SELECT:
            return QStringLiteral("character_select");
        case LanPacketType::READY_STATE:
            return QStringLiteral("ready_state");
        case LanPacketType::START_MATCH:
            return QStringLiteral("start_match");
        case LanPacketType::COMBAT_INPUT:
            return QStringLiteral("combat_input");
        case LanPacketType::COMBAT_STATE:
            return QStringLiteral("combat_state");
        case LanPacketType::PING:
            return QStringLiteral("ping");
        case LanPacketType::PONG:
            return QStringLiteral("pong");
        case LanPacketType::DISCONNECT:
        default:
            return QStringLiteral("disconnect");
    }
}

} // namespace

QString lanPacketTypeToWireName(LanPacketType type) {
    return packetNameForType(type);
}

bool lanPacketTypeFromWireName(const QString& name, LanPacketType* outType) {
    if (!outType) {
        return false;
    }

    const QString normalized = name.trimmed().toLower();
    if (normalized == QStringLiteral("hello")) {
        *outType = LanPacketType::HELLO;
        return true;
    }
    if (normalized == QStringLiteral("character_select")) {
        *outType = LanPacketType::CHARACTER_SELECT;
        return true;
    }
    if (normalized == QStringLiteral("ready_state")) {
        *outType = LanPacketType::READY_STATE;
        return true;
    }
    if (normalized == QStringLiteral("start_match")) {
        *outType = LanPacketType::START_MATCH;
        return true;
    }
    if (normalized == QStringLiteral("combat_input")) {
        *outType = LanPacketType::COMBAT_INPUT;
        return true;
    }
    if (normalized == QStringLiteral("combat_state")) {
        *outType = LanPacketType::COMBAT_STATE;
        return true;
    }
    if (normalized == QStringLiteral("ping")) {
        *outType = LanPacketType::PING;
        return true;
    }
    if (normalized == QStringLiteral("pong")) {
        *outType = LanPacketType::PONG;
        return true;
    }
    if (normalized == QStringLiteral("disconnect")) {
        *outType = LanPacketType::DISCONNECT;
        return true;
    }

    return false;
}

QJsonObject lanPlayerToJson(const LanPlayerInfo& player) {
    QJsonObject object;
    object.insert(QStringLiteral("username"), player.username);
    object.insert(QStringLiteral("fighterName"), player.fighterName);
    object.insert(QStringLiteral("fighterType"), player.fighterType);
    object.insert(QStringLiteral("ready"), player.ready);
    return object;
}

LanPlayerInfo lanPlayerFromJson(const QJsonObject& object) {
    LanPlayerInfo player;
    player.username = object.value(QStringLiteral("username")).toString();
    player.fighterName = object.value(QStringLiteral("fighterName")).toString();
    player.fighterType = object.value(QStringLiteral("fighterType")).toInt(-1);
    player.ready = object.value(QStringLiteral("ready")).toBool(false);
    return player;
}

QJsonObject lanCombatInputToJson(const LanCombatInputFrame& input) {
    QJsonObject object;
    object.insert(QStringLiteral("sequence"), static_cast<int>(input.sequence));
    object.insert(QStringLiteral("inputBits"), static_cast<int>(input.inputBits));
    return object;
}

LanCombatInputFrame lanCombatInputFromJson(const QJsonObject& object) {
    LanCombatInputFrame input;
    input.sequence = static_cast<quint32>(object.value(QStringLiteral("sequence")).toInt(0));
    input.inputBits = static_cast<quint8>(object.value(QStringLiteral("inputBits")).toInt(0));
    return input;
}

QJsonObject lanCombatantStatsToJson(const LanCombatantStats& stats) {
    QJsonObject object;
    object.insert(QStringLiteral("score"), stats.score);
    object.insert(QStringLiteral("damageDealt"), stats.damageDealt);
    object.insert(QStringLiteral("damageTaken"), stats.damageTaken);
    object.insert(QStringLiteral("healsUsed"), stats.healsUsed);
    object.insert(QStringLiteral("attacks"), stats.attacks);
    object.insert(QStringLiteral("hits"), stats.hits);
    object.insert(QStringLiteral("misses"), stats.misses);
    object.insert(QStringLiteral("projectilesFired"), stats.projectilesFired);
    object.insert(QStringLiteral("projectilesHit"), stats.projectilesHit);
    return object;
}

LanCombatantStats lanCombatantStatsFromJson(const QJsonObject& object) {
    LanCombatantStats stats;
    stats.score = object.value(QStringLiteral("score")).toInt(0);
    stats.damageDealt = object.value(QStringLiteral("damageDealt")).toInt(0);
    stats.damageTaken = object.value(QStringLiteral("damageTaken")).toInt(0);
    stats.healsUsed = object.value(QStringLiteral("healsUsed")).toInt(0);
    stats.attacks = object.value(QStringLiteral("attacks")).toInt(0);
    stats.hits = object.value(QStringLiteral("hits")).toInt(0);
    stats.misses = object.value(QStringLiteral("misses")).toInt(0);
    stats.projectilesFired = object.value(QStringLiteral("projectilesFired")).toInt(0);
    stats.projectilesHit = object.value(QStringLiteral("projectilesHit")).toInt(0);
    return stats;
}

QJsonObject lanCombatStateToJson(const LanCombatState& state) {
    QJsonObject object;
    object.insert(QStringLiteral("tick"), static_cast<int>(state.tick));
    object.insert(QStringLiteral("battleActive"), state.battleActive);
    object.insert(QStringLiteral("matchFinished"), state.matchFinished);
    object.insert(QStringLiteral("winner"), static_cast<int>(state.winner));
    object.insert(QStringLiteral("battleDurationSeconds"), state.battleDurationSeconds);
    object.insert(QStringLiteral("introLockTime"), state.introLockTime);
    object.insert(QStringLiteral("hostX"), state.hostX);
    object.insert(QStringLiteral("guestX"), state.guestX);
    object.insert(QStringLiteral("hostHp"), state.hostHp);
    object.insert(QStringLiteral("guestHp"), state.guestHp);
    object.insert(QStringLiteral("hostFacingLeft"), state.hostFacingLeft);
    object.insert(QStringLiteral("guestFacingLeft"), state.guestFacingLeft);
    object.insert(QStringLiteral("hostAnimation"), static_cast<int>(state.hostAnimation));
    object.insert(QStringLiteral("guestAnimation"), static_cast<int>(state.guestAnimation));
    object.insert(QStringLiteral("hostAttackCooldown"), state.hostAttackCooldown);
    object.insert(QStringLiteral("guestAttackCooldown"), state.guestAttackCooldown);
    object.insert(QStringLiteral("hostHealCooldown"), state.hostHealCooldown);
    object.insert(QStringLiteral("guestHealCooldown"), state.guestHealCooldown);
    object.insert(QStringLiteral("hostProjectileActive"), state.hostProjectileActive);
    object.insert(QStringLiteral("hostProjectileFacingRight"), state.hostProjectileFacingRight);
    object.insert(QStringLiteral("hostProjectileX"), state.hostProjectileX);
    object.insert(QStringLiteral("hostProjectileY"), state.hostProjectileY);
    object.insert(QStringLiteral("hostProjectileFrame"), state.hostProjectileFrame);
    object.insert(QStringLiteral("guestProjectileActive"), state.guestProjectileActive);
    object.insert(QStringLiteral("guestProjectileExploding"), state.guestProjectileExploding);
    object.insert(QStringLiteral("guestProjectileFacingRight"), state.guestProjectileFacingRight);
    object.insert(QStringLiteral("guestProjectileType"), static_cast<int>(state.guestProjectileType));
    object.insert(QStringLiteral("guestProjectileX"), state.guestProjectileX);
    object.insert(QStringLiteral("guestProjectileY"), state.guestProjectileY);
    object.insert(QStringLiteral("guestProjectileFrame"), state.guestProjectileFrame);
    object.insert(QStringLiteral("statusMessage"), state.statusMessage);
    object.insert(QStringLiteral("statusSeconds"), state.statusSeconds);
    object.insert(QStringLiteral("hostStats"), lanCombatantStatsToJson(state.hostStats));
    object.insert(QStringLiteral("guestStats"), lanCombatantStatsToJson(state.guestStats));
    return object;
}

LanCombatState lanCombatStateFromJson(const QJsonObject& object) {
    LanCombatState state;
    state.tick = static_cast<quint32>(object.value(QStringLiteral("tick")).toInt(0));
    state.battleActive = object.value(QStringLiteral("battleActive")).toBool(false);
    state.matchFinished = object.value(QStringLiteral("matchFinished")).toBool(false);
    state.winner = static_cast<LanCombatWinner>(object.value(QStringLiteral("winner")).toInt(0));
    state.battleDurationSeconds = object.value(QStringLiteral("battleDurationSeconds")).toDouble(0.0);
    state.introLockTime = object.value(QStringLiteral("introLockTime")).toDouble(0.0);
    state.hostX = object.value(QStringLiteral("hostX")).toDouble(0.0);
    state.guestX = object.value(QStringLiteral("guestX")).toDouble(0.0);
    state.hostHp = object.value(QStringLiteral("hostHp")).toInt(0);
    state.guestHp = object.value(QStringLiteral("guestHp")).toInt(0);
    state.hostFacingLeft = object.value(QStringLiteral("hostFacingLeft")).toBool(false);
    state.guestFacingLeft = object.value(QStringLiteral("guestFacingLeft")).toBool(false);
    state.hostAnimation = static_cast<AnimationState>(object.value(QStringLiteral("hostAnimation")).toInt(0));
    state.guestAnimation = static_cast<AnimationState>(object.value(QStringLiteral("guestAnimation")).toInt(0));
    state.hostAttackCooldown = object.value(QStringLiteral("hostAttackCooldown")).toDouble(0.0);
    state.guestAttackCooldown = object.value(QStringLiteral("guestAttackCooldown")).toDouble(0.0);
    state.hostHealCooldown = object.value(QStringLiteral("hostHealCooldown")).toDouble(0.0);
    state.guestHealCooldown = object.value(QStringLiteral("guestHealCooldown")).toDouble(0.0);
    state.hostProjectileActive = object.value(QStringLiteral("hostProjectileActive")).toBool(false);
    state.hostProjectileFacingRight = object.value(QStringLiteral("hostProjectileFacingRight")).toBool(true);
    state.hostProjectileX = object.value(QStringLiteral("hostProjectileX")).toDouble(0.0);
    state.hostProjectileY = object.value(QStringLiteral("hostProjectileY")).toDouble(0.0);
    state.hostProjectileFrame = object.value(QStringLiteral("hostProjectileFrame")).toInt(0);
    state.guestProjectileActive = object.value(QStringLiteral("guestProjectileActive")).toBool(false);
    state.guestProjectileExploding = object.value(QStringLiteral("guestProjectileExploding")).toBool(false);
    state.guestProjectileFacingRight = object.value(QStringLiteral("guestProjectileFacingRight")).toBool(false);
    state.guestProjectileType = static_cast<EnemyType>(object.value(QStringLiteral("guestProjectileType")).toInt(0));
    state.guestProjectileX = object.value(QStringLiteral("guestProjectileX")).toDouble(0.0);
    state.guestProjectileY = object.value(QStringLiteral("guestProjectileY")).toDouble(0.0);
    state.guestProjectileFrame = object.value(QStringLiteral("guestProjectileFrame")).toInt(0);
    state.statusMessage = object.value(QStringLiteral("statusMessage")).toString();
    state.statusSeconds = object.value(QStringLiteral("statusSeconds")).toDouble(0.0);
    state.hostStats = lanCombatantStatsFromJson(object.value(QStringLiteral("hostStats")).toObject());
    state.guestStats = lanCombatantStatsFromJson(object.value(QStringLiteral("guestStats")).toObject());
    return state;
}

QByteArray encodeLanMessage(LanPacketType type, const QJsonObject& payload) {
    QJsonObject envelope;
    envelope.insert(QStringLiteral("type"), lanPacketTypeToWireName(type));
    envelope.insert(QStringLiteral("payload"), payload);

    QByteArray data = QJsonDocument(envelope).toJson(QJsonDocument::Compact);
    data.append('\n');
    return data;
}

bool decodeLanMessage(const QByteArray& line, LanPacketType* type, QJsonObject* payload, QString* errorMessage) {
    const QJsonDocument document = QJsonDocument::fromJson(line);
    if (document.isNull() || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid LAN packet JSON.");
        }
        return false;
    }

    const QJsonObject envelope = document.object();
    const QString typeName = envelope.value(QStringLiteral("type")).toString();
    if (!lanPacketTypeFromWireName(typeName, type)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unknown LAN packet type.");
        }
        return false;
    }

    if (payload) {
        *payload = envelope.value(QStringLiteral("payload")).toObject();
    }

    return true;
}
