#include "CombatAiAdvisor.h"

#include "OpenRouterClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>

namespace {
QString playerTypeToString(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN: return QStringLiteral("Arcen");
        case PlayerType::DEMON_SLAYER: return QStringLiteral("Demon Slayer");
        case PlayerType::FANTASY_WARRIOR: return QStringLiteral("Fantasy Warrior");
        case PlayerType::HUNTRESS: return QStringLiteral("Huntress");
        case PlayerType::KNIGHT: return QStringLiteral("Knight");
        case PlayerType::MARTIAL: return QStringLiteral("Martial");
        case PlayerType::MARTIAL_HERO: return QStringLiteral("Martial Hero");
        case PlayerType::MEDIEVAL_WARRIOR: return QStringLiteral("Medieval Warrior");
        case PlayerType::WIZARD: return QStringLiteral("Wizard");
    }
    return QStringLiteral("Unknown");
}

QString enemyTypeToString(EnemyType type) {
    switch (type) {
        case EnemyType::FIRE_WORM: return QStringLiteral("Fire Worm");
        case EnemyType::FIRE_WIZARD: return QStringLiteral("Fire Wizard");
        case EnemyType::FLYING_DEMON: return QStringLiteral("Flying Demon");
        case EnemyType::NIGHTWEAVER: return QStringLiteral("Nightweaver");
        case EnemyType::EVIL_WIZARD: return QStringLiteral("Evil Wizard");
        case EnemyType::BLACK_WEREWOLF: return QStringLiteral("Black Werewolf");
        case EnemyType::RED_WEREWOLF: return QStringLiteral("Red Werewolf");
        case EnemyType::WHITE_WEREWOLF: return QStringLiteral("White Werewolf");
        case EnemyType::ZOMBIE_1: return QStringLiteral("Zombie 1");
        case EnemyType::ZOMBIE_2: return QStringLiteral("Zombie 2");
        case EnemyType::ZOMBIE_3: return QStringLiteral("Zombie 3");
        case EnemyType::ZOMBIE_4: return QStringLiteral("Zombie 4");
        case EnemyType::ADVANCED_ZOMBIE_1: return QStringLiteral("Advanced Zombie 1");
        case EnemyType::ADVANCED_ZOMBIE_2: return QStringLiteral("Advanced Zombie 2");
        case EnemyType::ADVANCED_ZOMBIE_3: return QStringLiteral("Advanced Zombie 3");
    }
    return QStringLiteral("Unknown");
}

QString difficultyToString(DifficultyLevel difficulty) {
    switch (difficulty) {
        case DifficultyLevel::EASY: return QStringLiteral("Easy");
        case DifficultyLevel::HARD: return QStringLiteral("Hard");
        case DifficultyLevel::NORMAL:
        default: return QStringLiteral("Normal");
    }
}

QString stripCodeFence(QString text) {
    text = text.trimmed();
    if (!text.startsWith(QStringLiteral("```"))) {
        return text;
    }
    const int firstNewline = text.indexOf(QLatin1Char('\n'));
    const int lastFence = text.lastIndexOf(QStringLiteral("```"));
    if (firstNewline >= 0 && lastFence > firstNewline) {
        return text.mid(firstNewline + 1, lastFence - firstNewline - 1).trimmed();
    }
    return text;
}
}

CombatAiAdvisor::CombatAiAdvisor(QObject *parent)
    : QObject(parent),
      client_(new OpenRouterClient(this)) {
}

bool CombatAiAdvisor::isConfigured() const {
    return client_ && client_->isConfigured();
}

void CombatAiAdvisor::requestRecommendation(const CombatSnapshot& snapshot) {
    if (!client_ || !client_->isConfigured()) {
        emit recommendationReady(fallbackRecommendation(snapshot, client_ ? client_->configurationError()
                                                                          : QStringLiteral("Client unavailable")));
        return;
    }

    client_->requestRecommendation(
        buildSystemPrompt(),
        buildUserPrompt(snapshot),
        [this, snapshot](bool ok, const QString& content, const QString& error) {
            if (!ok) {
                emit recommendationReady(fallbackRecommendation(snapshot, error));
                return;
            }
            emit recommendationReady(parseRecommendation(content, snapshot));
        });
}

QString CombatAiAdvisor::buildSystemPrompt() const {
    return QStringLiteral(
        "You are a tactical combat AI advisor for a 2D fighting game enemy.\n"
        "Return ONLY valid JSON. No markdown, no code fences, no explanations.\n"
        "Allowed strategy values: Aggressive, KeepDistance, Defensive, BaitAttack.\n"
        "Allowed preferredAttack values: Melee, Projectile, Any.\n"
        "durationMs must be an integer between 800 and 4000.\n"
        "Use Defensive when the enemy is below 50% HP and healing is available.\n"
        "Use BaitAttack when the player recently missed or is overcommitting.\n"
        "Use KeepDistance when projectile is available and distance is useful.\n"
        "Use this exact JSON shape: "
        "{\"strategy\":\"KeepDistance\",\"preferredAttack\":\"Projectile\",\"durationMs\":3000}");
}

QString CombatAiAdvisor::buildUserPrompt(const CombatSnapshot& snapshot) const {
    QString prompt;
    prompt += QStringLiteral("Player Type: %1\n").arg(playerTypeToString(snapshot.playerType));
    prompt += QStringLiteral("Enemy Type: %1\n").arg(enemyTypeToString(snapshot.enemyType));
    prompt += QStringLiteral("Player HP: %1/%2\n").arg(snapshot.playerHp).arg(snapshot.playerMaxHp);
    prompt += QStringLiteral("Enemy HP: %1/%2\n").arg(snapshot.enemyHp).arg(snapshot.enemyMaxHp);
    prompt += QStringLiteral("Distance: %1 px\n").arg(snapshot.distance, 0, 'f', 1);
    prompt += QStringLiteral("Level: %1\n").arg(snapshot.currentLevel);
    prompt += QStringLiteral("Difficulty: %1\n").arg(difficultyToString(snapshot.difficulty));
    prompt += QStringLiteral("Enemy Projectile Available: %1\n").arg(snapshot.projectileAvailable ? QStringLiteral("yes") : QStringLiteral("no"));
    prompt += QStringLiteral("Enemy Heal Available: %1\n").arg(snapshot.healAvailable ? QStringLiteral("yes") : QStringLiteral("no"));
    prompt += QStringLiteral("Recent Player Action: %1\n").arg(snapshot.recentPlayerAction.isEmpty() ? QStringLiteral("none") : snapshot.recentPlayerAction);
    prompt += QStringLiteral("Recent Enemy Action: %1\n").arg(snapshot.recentEnemyAction.isEmpty() ? QStringLiteral("none") : snapshot.recentEnemyAction);
    prompt += QStringLiteral("If Enemy HP is below 50% and Enemy Heal Available is yes, strongly consider Defensive so the enemy can retreat and heal.\n");
    prompt += QStringLiteral("Recommend a short high-level enemy tactic for the next few seconds.");
    return prompt;
}

AiRecommendation CombatAiAdvisor::parseRecommendation(const QString& response,
                                                      const CombatSnapshot& snapshot) const {
    const QByteArray jsonBytes = stripCodeFence(response).toUtf8();
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return fallbackRecommendation(snapshot, error.errorString());
    }

    const QJsonObject json = doc.object();
    const QString strategyValue = json.value(QStringLiteral("strategy")).toString().trimmed();
    const QString attackValue = json.value(QStringLiteral("preferredAttack")).toString().trimmed();
    const int duration = json.value(QStringLiteral("durationMs")).toInt(0);

    const QMap<QString, Strategy> strategyMap = {
        {QStringLiteral("Aggressive"), Strategy::Aggressive},
        {QStringLiteral("KeepDistance"), Strategy::KeepDistance},
        {QStringLiteral("Defensive"), Strategy::Defensive},
        {QStringLiteral("BaitAttack"), Strategy::BaitAttack}
    };
    const QMap<QString, PreferredAttack> attackMap = {
        {QStringLiteral("Melee"), PreferredAttack::Melee},
        {QStringLiteral("Projectile"), PreferredAttack::Projectile},
        {QStringLiteral("Any"), PreferredAttack::Any}
    };

    if (!strategyMap.contains(strategyValue)) {
        return fallbackRecommendation(snapshot, QStringLiteral("Invalid strategy value: %1").arg(strategyValue));
    }
    if (!attackMap.contains(attackValue)) {
        return fallbackRecommendation(snapshot, QStringLiteral("Invalid preferredAttack value: %1").arg(attackValue));
    }
    if (duration <= 0) {
        return fallbackRecommendation(snapshot, QStringLiteral("Missing or invalid durationMs"));
    }

    AiRecommendation recommendation;
    recommendation.strategy = strategyMap.value(strategyValue);
    recommendation.preferredAttack = attackMap.value(attackValue);
    recommendation.durationMs = qBound(800, duration, 4000);
    recommendation.fromFallback = false;
    return recommendation;
}

AiRecommendation CombatAiAdvisor::fallbackRecommendation(const CombatSnapshot& snapshot,
                                                         const QString& reason) const {
    AiRecommendation recommendation;
    recommendation.fromFallback = true;
    recommendation.reason = reason;
    recommendation.durationMs = 1200;

    const double enemyHealth = snapshot.enemyMaxHp > 0
        ? static_cast<double>(snapshot.enemyHp) / snapshot.enemyMaxHp
        : 1.0;

    if (enemyHealth < 0.25) {
        recommendation.strategy = Strategy::Defensive;
        recommendation.preferredAttack = PreferredAttack::Any;
    } else if (snapshot.projectileAvailable && snapshot.distance > 150.0) {
        recommendation.strategy = Strategy::KeepDistance;
        recommendation.preferredAttack = PreferredAttack::Projectile;
    } else if (snapshot.distance <= 115.0) {
        recommendation.strategy = Strategy::BaitAttack;
        recommendation.preferredAttack = PreferredAttack::Melee;
    } else {
        recommendation.strategy = Strategy::Aggressive;
        recommendation.preferredAttack = PreferredAttack::Melee;
    }

    return recommendation;
}
