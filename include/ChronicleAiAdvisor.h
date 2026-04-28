#ifndef CHRONICLEAIADVISOR_H
#define CHRONICLEAIADVISOR_H

#include <QObject>
#include <QString>

#include "ChronicleAiTypes.h"

class ChronicleOpenRouterClient;

class ChronicleAiAdvisor : public QObject {
    Q_OBJECT

public:
    explicit ChronicleAiAdvisor(QObject *parent = nullptr);

    void requestSummary(const ChronicleBattleReport& report);
    void prefetchSummary(const ChronicleBattleReport& report);
    ChronicleSummary fallbackSummary(const ChronicleBattleReport& report,
                                     const QString& reason = QString()) const;

signals:
    void summaryReady(const ChronicleSummary& summary);

private:
    QString buildSystemPrompt() const;
    QString buildUserPrompt(const ChronicleBattleReport& report) const;
    QString cacheKeyForReport(const ChronicleBattleReport& report) const;
    ChronicleSummary parseSummary(const QString& content,
                                  const ChronicleBattleReport& report,
                                  const QString& rawError = QString()) const;
    ChronicleSummary cachedSummary(const ChronicleBattleReport& report) const;
    void requestSummaryInternal(const ChronicleBattleReport& report, bool emitWhenReady);

    ChronicleOpenRouterClient *client_;
};

#endif // CHRONICLEAIADVISOR_H
