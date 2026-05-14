#ifndef COMBAT_AI_ADVISOR_H
#define COMBAT_AI_ADVISOR_H

#include "CombatAiTypes.h"

#include <QObject>
#include <QString>

class OpenRouterClient;

class CombatAiAdvisor : public QObject {
    Q_OBJECT

public:
    explicit CombatAiAdvisor(QObject *parent = nullptr);

    bool isConfigured() const;
    void requestRecommendation(const CombatSnapshot& snapshot);
    AiRecommendation fallbackRecommendation(const CombatSnapshot& snapshot,
                                            const QString& reason = QString()) const;

signals:
    void recommendationReady(const AiRecommendation& recommendation);

private:
    QString buildSystemPrompt() const;
    QString buildUserPrompt(const CombatSnapshot& snapshot) const;
    AiRecommendation parseRecommendation(const QString& response,
                                         const CombatSnapshot& snapshot) const;

    OpenRouterClient *client_;
};

#endif // COMBAT_AI_ADVISOR_H
