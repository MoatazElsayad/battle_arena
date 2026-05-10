#ifndef WEBSITESYNCCLIENT_H
#define WEBSITESYNCCLIENT_H

#include <QObject>
#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QString>

class QNetworkAccessManager;

struct WebsiteBattleUpload {
    QString username;
    QString characterType;
    QString characterName;
    int totalScore = -1;
    int wins = -1;
    int losses = -1;
    int totalMatches = -1;
    QString rankLabel;
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

struct WebsiteHighlightUpload {
    QString username;
    QString mode;
    QString characterType;
    QString characterName;
    QString enemyType;
    QString enemyName;
    bool victory = false;
    double battleDurationSeconds = 0.0;
    int score = 0;
    QString attackType;
    int damage = 0;
    bool wasProjectile = false;
    bool wasFinisher = false;
    int highlightScore = 0;
    int playerHpBefore = 0;
    int playerHpAfter = 0;
    int playerMaxHp = 0;
    int enemyHpBefore = 0;
    int enemyHpAfter = 0;
    int levelIndex = 0;
    QString levelName;
    QDateTime capturedAtUtc;
    QByteArray imageBytes;
    QString imageMimeType;
};

class WebsiteSyncClient : public QObject {
    Q_OBJECT

public:
    explicit WebsiteSyncClient(QObject* parent = nullptr);

    bool isConfigured() const;
    QString endpointBaseUrl() const;
    void uploadBattleResult(const WebsiteBattleUpload& payload);
    void uploadBattleHighlight(const WebsiteHighlightUpload& payload);
    void registerWebsiteAccount(const QString& email, const QString& username, const QString& password);

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
