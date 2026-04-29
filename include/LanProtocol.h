#ifndef LANPROTOCOL_H
#define LANPROTOCOL_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include "NetTypes.h"

QString lanPacketTypeToWireName(LanPacketType type);
bool lanPacketTypeFromWireName(const QString& name, LanPacketType* outType);

QJsonObject lanPlayerToJson(const LanPlayerInfo& player);
LanPlayerInfo lanPlayerFromJson(const QJsonObject& object);

QJsonObject lanCombatInputToJson(const LanCombatInputFrame& input);
LanCombatInputFrame lanCombatInputFromJson(const QJsonObject& object);

QJsonObject lanCombatantStatsToJson(const LanCombatantStats& stats);
LanCombatantStats lanCombatantStatsFromJson(const QJsonObject& object);

QJsonObject lanCombatStateToJson(const LanCombatState& state);
LanCombatState lanCombatStateFromJson(const QJsonObject& object);

QByteArray encodeLanMessage(LanPacketType type, const QJsonObject& payload);
bool decodeLanMessage(const QByteArray& line, LanPacketType* type, QJsonObject* payload, QString* errorMessage);

#endif // LANPROTOCOL_H
