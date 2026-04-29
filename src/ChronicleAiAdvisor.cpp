#include "ChronicleAiAdvisor.h"

#include "ChronicleOpenRouterClient.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QPointer>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

namespace {
QHash<QString, ChronicleSummary> gChronicleSummaryCache;
QHash<QString, QList<QPointer<ChronicleAiAdvisor>>> gPendingListeners;
bool gChronicleCacheLoaded = false;

QString safeText(QString value, const QString& fallback) {
    value = value.trimmed();
    return value.isEmpty() ? fallback : value;
}

QString clipped(QString value, int maxLength) {
    value = value.simplified();
    if (value.size() <= maxLength) {
        return value;
    }
    return value.left(maxLength - 1).trimmed() + ".";
}

QString singleSentence(QString value) {
    value = value.simplified();
    if (value.isEmpty()) {
        return value;
    }

    const QRegularExpression sentenceBreak(QStringLiteral(R"([.!?](?:\s|$))"));
    const QRegularExpressionMatch match = sentenceBreak.match(value);
    if (match.hasMatch()) {
        value = value.left(match.capturedEnd()).trimmed();
    }

    while (!value.isEmpty() && QStringLiteral(",;:-").contains(value.back())) {
        value.chop(1);
        value = value.trimmed();
    }

    if (!value.endsWith('.') && !value.endsWith('!') && !value.endsWith('?')) {
        value += '.';
    }

    return value;
}

QString removeFillerPhrases(QString value) {
    static const QStringList fillerPhrases = {
        QStringLiteral("while looking for openings to strike"),
        QStringLiteral("while waiting for openings"),
        QStringLiteral("when looking for openings"),
        QStringLiteral("if possible"),
        QStringLiteral("when possible"),
        QStringLiteral("be careful of"),
        QStringLiteral("you should"),
        QStringLiteral("it will be important to")
    };

    for (const QString& phrase : fillerPhrases) {
        value.replace(QRegularExpression(QRegularExpression::escape(phrase),
                                         QRegularExpression::CaseInsensitiveOption),
                      QString());
    }

    value.replace(QRegularExpression(QStringLiteral("\\s+,\\s+")), QStringLiteral(", "));
    value.replace(QRegularExpression(QStringLiteral("\\s{2,}")), QStringLiteral(" "));
    return value.trimmed();
}

QString stripRepeatedEnemyLead(QString value, const QString& enemyName) {
    const QString trimmedEnemyName = enemyName.trimmed();
    if (trimmedEnemyName.isEmpty()) {
        return value.trimmed();
    }

    const QString escapedEnemy = QRegularExpression::escape(trimmedEnemyName);
    const QRegularExpression repeatedEnemyLead(
        QStringLiteral("^(?:against|versus|for)\\s+(?:the\\s+)?%1\\s*,\\s*").arg(escapedEnemy),
        QRegularExpression::CaseInsensitiveOption);

    value.remove(repeatedEnemyLead);
    return value.trimmed();
}

QString polishChronicleLine(QString value, int maxLength) {
    value = removeFillerPhrases(value);
    value = singleSentence(value);
    return clipped(value, maxLength);
}

QString conciseFallbackReason(QString reason) {
    reason = reason.simplified();
    if (reason.isEmpty()) {
        return reason;
    }

    const QString lowered = reason.toLower();
    if (reason.contains(QStringLiteral(" | "))) {
        if (lowered.contains("timed out")) {
            return QStringLiteral("All AI models took too long. Using local battle counsel.");
        }
        if (lowered.contains("rate limit") || lowered.contains("429")) {
            return QStringLiteral("All AI models were unavailable or rate-limited. Using local battle counsel.");
        }
        return QStringLiteral("All AI models failed. Using local battle counsel.");
    }
    if (lowered.contains("openrouter_api_key")) {
        return QStringLiteral("OpenRouter API key is missing.");
    }
    if (lowered.contains("openrouter_model")) {
        return QStringLiteral("OpenRouter model is missing.");
    }
    if (lowered.contains("rate limit") || lowered.contains("rate-limited") || lowered.contains("429")) {
        return QStringLiteral("Provider rate limit hit. Try again shortly or switch models.");
    }
    if (lowered.contains("no provider") || lowered.contains("provider") && lowered.contains("upstream")) {
        return QStringLiteral("The selected AI provider is unavailable right now.");
    }
    if (lowered.contains("response content is empty")) {
        return QStringLiteral("The AI service returned an empty reply.");
    }
    if (lowered.contains("json parse")) {
        return QStringLiteral("The AI reply could not be parsed cleanly.");
    }
    if (lowered.contains("timed out") || lowered.contains("timeout")) {
        return QStringLiteral("The AI request timed out.");
    }
    if (lowered.contains("host not found") || lowered.contains("could not resolve host")) {
        return QStringLiteral("Network lookup failed while contacting the AI service.");
    }
    if (lowered.contains("ssl") || lowered.contains("tls")) {
        return QStringLiteral("Secure connection to the AI service failed.");
    }

    return clipped(singleSentence(reason), 80);
}

QString chronicleCacheFilePath() {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (baseDir.trimmed().isEmpty()) {
        baseDir = QDir::current().filePath(QStringLiteral(".chronicle-cache"));
    }

    QDir dir(baseDir);
    dir.mkpath(QStringLiteral("."));
    return dir.filePath(QStringLiteral("chronicle_ai_cache.json"));
}

QJsonObject summaryToJson(const ChronicleSummary& summary) {
    QJsonArray tags;
    for (const QString& tag : summary.focusTags) {
        tags.append(tag);
    }

    return QJsonObject{
        {"lastLevelSummary", summary.lastLevelSummary},
        {"nextLevelPreview", summary.nextLevelPreview},
        {"recommendation", summary.recommendation},
        {"focusTags", tags},
        {"fromFallback", summary.fromFallback},
        {"fromCache", false},
        {"error", summary.error}
    };
}

ChronicleSummary summaryFromJson(const QJsonObject& obj) {
    ChronicleSummary summary;
    summary.lastLevelSummary = obj.value("lastLevelSummary").toString();
    summary.nextLevelPreview = obj.value("nextLevelPreview").toString();
    summary.recommendation = obj.value("recommendation").toString();
    summary.fromFallback = obj.value("fromFallback").toBool(true);
    summary.fromCache = obj.value("fromCache").toBool(false);
    summary.error = obj.value("error").toString();

    const QJsonArray tags = obj.value("focusTags").toArray();
    for (const QJsonValue& tag : tags) {
        const QString cleaned = tag.toString().trimmed();
        if (!cleaned.isEmpty()) {
            summary.focusTags.append(cleaned);
        }
    }

    return summary;
}

void loadChronicleCacheIfNeeded() {
    if (gChronicleCacheLoaded) {
        return;
    }
    gChronicleCacheLoaded = true;

    QFile file(chronicleCacheFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        ChronicleSummary summary = summaryFromJson(it.value().toObject());
        if (summary.lastLevelSummary.isEmpty() ||
            summary.nextLevelPreview.isEmpty() ||
            summary.recommendation.isEmpty()) {
            continue;
        }
        summary.fromCache = true;
        gChronicleSummaryCache.insert(it.key(), summary);
    }
}

void saveChronicleCache() {
    QJsonObject root;
    for (auto it = gChronicleSummaryCache.begin(); it != gChronicleSummaryCache.end(); ++it) {
        ChronicleSummary summary = it.value();
        if (summary.fromFallback) {
            continue;
        }
        summary.fromCache = false;
        root.insert(it.key(), summaryToJson(summary));
    }

    QSaveFile file(chronicleCacheFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
}

QJsonObject reportJson(const ChronicleBattleReport& report) {
    return {
        {"playerName", safeText(report.playerName, "Gladiator")},
        {"playerType", safeText(report.playerType, "Unknown")},
        {"completedLevel", report.completedLevel},
        {"totalLevels", report.totalLevels},
        {"defeatedEnemyName", safeText(report.defeatedEnemyName, "Enemy")},
        {"defeatedEnemyType", safeText(report.defeatedEnemyType, "Unknown")},
        {"nextEnemyName", safeText(report.nextEnemyName, report.campaignComplete ? "King rescued" : "Unknown")},
        {"nextEnemyType", safeText(report.nextEnemyType, report.campaignComplete ? "Victory" : "Unknown")},
        {"currentScore", report.currentScore},
        {"playerHp", report.playerHp},
        {"playerMaxHp", report.playerMaxHp},
        {"enemyMaxHp", report.enemyMaxHp},
        {"damageDealt", report.damageDealt},
        {"damageTaken", report.damageTaken},
        {"healsUsed", report.healsUsed},
        {"playerAttacks", report.playerAttacks},
        {"playerHits", report.playerHits},
        {"playerMisses", report.playerMisses},
        {"projectilesFired", report.projectilesFired},
        {"projectilesHit", report.projectilesHit},
        {"enemyHits", report.enemyHits},
        {"battleDurationSeconds", report.battleDurationSeconds},
        {"victory", report.victory},
        {"campaignComplete", report.campaignComplete}
    };
}
}

ChronicleAiAdvisor::ChronicleAiAdvisor(QObject *parent)
    : QObject(parent),
      client_(new ChronicleOpenRouterClient(this)) {
    loadChronicleCacheIfNeeded();
}

void ChronicleAiAdvisor::requestSummary(const ChronicleBattleReport& report) {
    const ChronicleSummary cached = cachedSummary(report);
    if (!cached.lastLevelSummary.isEmpty()) {
        QTimer::singleShot(0, this, [this, cached]() {
            emit summaryReady(cached);
        });
        return;
    }

    requestSummaryInternal(report, true);
}

void ChronicleAiAdvisor::prefetchSummary(const ChronicleBattleReport& report) {
    if (!cachedSummary(report).lastLevelSummary.isEmpty()) {
        return;
    }

    requestSummaryInternal(report, false);
}

ChronicleSummary ChronicleAiAdvisor::fallbackSummary(const ChronicleBattleReport& report,
                                                     const QString& reason) const {
    ChronicleSummary summary;
    summary.fromFallback = true;
    summary.fromCache = false;
    summary.error = conciseFallbackReason(reason);

    const int hpPercent = report.playerMaxHp > 0
        ? qBound(0, (report.playerHp * 100) / report.playerMaxHp, 100)
        : 0;
    const int hitRate = report.playerAttacks > 0
        ? qBound(0, (report.playerHits * 100) / report.playerAttacks, 100)
        : 0;

    summary.lastLevelSummary = QString("Realm %1 cleared: %2 damage dealt, %3 taken, %4% health left.")
        .arg(qMax(1, report.completedLevel))
        .arg(qMax(0, report.damageDealt))
        .arg(qMax(0, report.damageTaken))
        .arg(hpPercent);

    if (report.campaignComplete) {
        summary.nextLevelPreview = "The king is safe; the final return to the courtyard awaits.";
        summary.recommendation = "Stay close to the king and finish the rescue with discipline.";
    } else {
        summary.nextLevelPreview = QString("Next threat: %1. Expect a harder rhythm and tighter openings.")
            .arg(safeText(report.nextEnemyName, "Unknown enemy"));
        summary.recommendation = hitRate < 45
            ? "Slow your attacks, wait for openings, and avoid wasting cooldowns."
            : "Keep pressure steady, but save healing for the dangerous second half.";
    }

    summary.focusTags = {"SURVIVE", "POSITION", "PUNISH"};
    return summary;
}

QString ChronicleAiAdvisor::buildSystemPrompt() const {
    return QStringLiteral(
        "You are the Royal Battle Chronicler and tactical coach for a 2D pixel-art gladiator game called Gladiators.\n"
        "The player has just finished one Save the Kings level and is viewing a between-level Chronicle screen.\n"
        "Write a short, useful, dramatic briefing that helps the player understand what happened and prepare for the next level.\n\n"
        "Hard rules:\n"
        "- Return JSON only. No markdown. No text outside JSON.\n"
        "- Use exactly these keys: lastLevelSummary, nextLevelPreview, recommendation, focusTags.\n"
        "- lastLevelSummary: exactly 1 sentence, max 85 characters. Mention performance using stats.\n"
        "- nextLevelPreview: exactly 1 sentence, max 80 characters. Mention the next enemy if provided; if campaignComplete is true, mention rescue/return.\n"
        "- recommendation: exactly 1 sentence, max 95 characters. Give one concrete combat tip for the next fight.\n"
        "- focusTags: exactly 3 uppercase tags, each 3 to 12 letters.\n"
        "- Tone: royal, encouraging, tactical, concise.\n"
        "- Keep each line punchy and direct. Prefer result -> threat -> action.\n"
        "- Recommendation must start with an imperative verb such as BLOCK, DODGE, PUNISH, SPACE, PRESSURE, HEAL, or STRIKE.\n"
        "- Do not repeat the enemy name in recommendation if it already appears in nextLevelPreview.\n"
        "- Avoid filler phrases such as 'while looking for openings', 'be careful', 'if possible', or 'cunning movement'.\n"
        "- Do not invent hidden mechanics, abilities, controls, story events, or enemies.\n"
        "- Do not mention API, AI, prompt, JSON, or implementation details.\n\n"
        "Decision guidance:\n"
        "- Low HP or high damageTaken means recommend spacing, blocking tempo, or saving heals.\n"
        "- Many misses or low hit rate means recommend patience and punishing recovery windows.\n"
        "- Strong damage and high HP means recommend controlled aggression.\n"
        "- Many projectile hits means recommend maintaining range; projectile misses mean recommend cleaner timing.\n\n"
        "Response schema:\n"
        "{\n"
        "  \"lastLevelSummary\": \"string\",\n"
        "  \"nextLevelPreview\": \"string\",\n"
        "  \"recommendation\": \"string\",\n"
        "  \"focusTags\": [\"TAG\", \"TAG\", \"TAG\"]\n"
        "}");
}

QString ChronicleAiAdvisor::buildUserPrompt(const ChronicleBattleReport& report) const {
    return QString(
        "Create the Chronicle briefing from this battle report JSON. Be specific and brief.\n"
        "%1")
        .arg(QString::fromUtf8(QJsonDocument(reportJson(report)).toJson(QJsonDocument::Compact)));
}

QString ChronicleAiAdvisor::cacheKeyForReport(const ChronicleBattleReport& report) const {
    const QByteArray raw = QJsonDocument(reportJson(report)).toJson(QJsonDocument::Compact);
    return QString::fromLatin1(QCryptographicHash::hash(raw, QCryptographicHash::Sha1).toHex());
}

ChronicleSummary ChronicleAiAdvisor::cachedSummary(const ChronicleBattleReport& report) const {
    loadChronicleCacheIfNeeded();

    ChronicleSummary summary = gChronicleSummaryCache.value(cacheKeyForReport(report));
    if (!summary.lastLevelSummary.isEmpty()) {
        summary.fromCache = true;
    }
    return summary;
}

void ChronicleAiAdvisor::requestSummaryInternal(const ChronicleBattleReport& report, bool emitWhenReady) {
    const QString key = cacheKeyForReport(report);
    const bool alreadyPending = gPendingListeners.contains(key);
    if (!alreadyPending) {
        gPendingListeners.insert(key, {});
    }

    if (emitWhenReady) {
        gPendingListeners[key].append(QPointer<ChronicleAiAdvisor>(this));
    } else if (alreadyPending) {
        return;
    }

    if (alreadyPending) {
        return;
    }

    if (!client_->isConfigured()) {
        const ChronicleSummary fallback = fallbackSummary(report, client_->configurationError());
        const QList<QPointer<ChronicleAiAdvisor>> listeners = gPendingListeners.take(key);
        if (emitWhenReady) {
            QTimer::singleShot(0, this, [listeners, fallback]() {
                for (const QPointer<ChronicleAiAdvisor>& listener : listeners) {
                    if (listener) {
                        emit listener->summaryReady(fallback);
                    }
                }
            });
        }
        return;
    }

    client_->requestJsonCompletion(
        buildSystemPrompt(),
        buildUserPrompt(report),
        [this, report, key](bool success, const QString& content, const QString& error) {
            ChronicleSummary resolved;
            if (success) {
                resolved = parseSummary(content, report);
                resolved.fromCache = false;
                if (!resolved.fromFallback) {
                    gChronicleSummaryCache.insert(key, resolved);
                    saveChronicleCache();
                }
            } else {
                resolved = fallbackSummary(report, error);
            }

            const QList<QPointer<ChronicleAiAdvisor>> listeners = gPendingListeners.take(key);
            for (const QPointer<ChronicleAiAdvisor>& listener : listeners) {
                if (listener) {
                    emit listener->summaryReady(resolved);
                }
            }
        });
}

ChronicleSummary ChronicleAiAdvisor::parseSummary(const QString& content,
                                                  const ChronicleBattleReport& report,
                                                  const QString& rawError) const {
    QString jsonText = content.trimmed();
    const int firstBrace = jsonText.indexOf('{');
    const int lastBrace = jsonText.lastIndexOf('}');
    if (firstBrace >= 0 && lastBrace > firstBrace) {
        jsonText = jsonText.mid(firstBrace, lastBrace - firstBrace + 1);
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return fallbackSummary(report, rawError.isEmpty() ? "AI summary JSON parse failed" : rawError);
    }

    const QJsonObject obj = doc.object();
    ChronicleSummary summary;
    summary.fromFallback = false;
    summary.fromCache = false;
    summary.lastLevelSummary = polishChronicleLine(obj.value("lastLevelSummary").toString(), 85);
    summary.nextLevelPreview = polishChronicleLine(obj.value("nextLevelPreview").toString(), 80);
    summary.recommendation = stripRepeatedEnemyLead(obj.value("recommendation").toString(),
                                                    safeText(report.nextEnemyName, QString()));
    summary.recommendation = polishChronicleLine(summary.recommendation, 95);

    const QJsonArray tags = obj.value("focusTags").toArray();
    for (const QJsonValue& tag : tags) {
        const QString cleaned = tag.toString().trimmed().toUpper().left(12);
        if (!cleaned.isEmpty()) {
            summary.focusTags.append(cleaned);
        }
        if (summary.focusTags.size() == 3) {
            break;
        }
    }

    if (summary.lastLevelSummary.isEmpty() ||
        summary.nextLevelPreview.isEmpty() ||
        summary.recommendation.isEmpty()) {
        return fallbackSummary(report, "AI summary response missed required fields");
    }

    while (summary.focusTags.size() < 3) {
        summary.focusTags.append(summary.focusTags.isEmpty() ? "FOCUS" : "READY");
    }

    return summary;
}
