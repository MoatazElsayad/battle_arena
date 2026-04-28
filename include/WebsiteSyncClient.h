#ifndef WEBSITESYNCCLIENT_H
#define WEBSITESYNCCLIENT_H

#include <QObject>
#include <QHash>
#include <QString>

class QNetworkAccessManager;

struct WebsiteBattleUpload {
    QString username;
    QString characterType;
    QString characterName;
    QString mode;
    int levelIndex = 0;
    QString levelName;
    QString enemyType;
    QString enemyName;
    QString opponentUsername;
    bool victory = false;
    int score = 0;
    int damageDealt = 0;
    int damageTaken = 0;
    int playerHpEnd = 0;
    int playerMaxHp = 0;
    double battleDurationSeconds = 0.0;
    bool campaignComplete = false;
};

class WebsiteSyncClient : public QObject {
    Q_OBJECT

public:
    explicit WebsiteSyncClient(QObject* parent = nullptr);

    bool isConfigured() const;
    QString endpointBaseUrl() const;
    void uploadBattleResult(const WebsiteBattleUpload& payload);

signals:
    void uploadSucceeded(const QString& username, const QString& message);
    void uploadFailed(const QString& username, const QString& message);

private:
    QString readConfigValue(const QString& key) const;
    QHash<QString, QString> readDotEnv() const;
    QString sanitizedBaseUrl() const;

    QNetworkAccessManager* networkManager_;
    QString baseUrl_;
    QString apiKey_;
    QString gameVersion_;
    QString platform_;
    int timeoutMs_;
};

#endif // WEBSITESYNCCLIENT_H
