
#include <OpenRouterClient.h>
#include <Enums.h>

Class CombatAiAdvisor : Public QObject
{
  Q_OBJECT


  Public:
    explicit CombatAiAdvisor(QObject *parent = nullptr);
    void requestRecommendation(const CombatSnapshot& snapshot);

    AiRecommendation fallbackRecommendation(
        AiRecommendation fallbackRecommendation(
        const CombatSnapshot& snapshot,
        const QString& reason = QString()
) const;
    )

    
  Signals:
 void recommendationReady(const AiRecommendation& recommendation);
  Private:
  QString buildSystemPrompt() const;
  QString buildUserPrompt(const CombatSnapshot& snapshot) const;
  AiRecommendation parseRecommendation(
    const QByteArray& response,
    const CombatSnapshot& snapshot
) const;

private:
ChronicleOpenRouterClient *client_;
};

