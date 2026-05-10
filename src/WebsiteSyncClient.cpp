#include "WebsiteSyncClient.h"

#include <QCoreApplication>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QDateTime>
#include <QDebug>
#include <QJsonParseError>
#include <QHttpMultiPart>

namespace {
constexpr auto kDefaultWebsiteApiUrl = "https://gladiators-website.vercel.app";
constexpr auto kDefaultWebsiteApiKey = "gladiators_secret_2026";
constexpr auto kDefaultGameVersion = "0.1.0";
constexpr auto kDefaultGamePlatform = "windows";
constexpr int kDefaultWebsiteTimeoutMs = 7000;
}

WebsiteSyncClient::WebsiteSyncClient(QObject* parent)
    : QObject(parent),
      networkManager_(new QNetworkAccessManager(this)),
      baseUrl_(readConfigValue(QStringLiteral("GLADIATORS_WEB_API_URL"))),
      apiKey_(readConfigValue(QStringLiteral("GLADIATORS_WEB_API_KEY"))),
      gameVersion_(readConfigValue(QStringLiteral("GLADIATORS_GAME_VERSION"))),
      platform_(readConfigValue(QStringLiteral("GLADIATORS_GAME_PLATFORM"))),
      timeoutMs_(qBound(2000, readConfigValue(QStringLiteral("GLADIATORS_WEB_TIMEOUT_MS")).toInt(), 20000)) {
    if (baseUrl_.trimmed().isEmpty()) {
        baseUrl_ = QString::fromLatin1(kDefaultWebsiteApiUrl);
    }
    if (apiKey_.trimmed().isEmpty()) {
        apiKey_ = QString::fromLatin1(kDefaultWebsiteApiKey);
    }
    if (timeoutMs_ == 2000 && readConfigValue(QStringLiteral("GLADIATORS_WEB_TIMEOUT_MS")).trimmed().isEmpty()) {
        timeoutMs_ = kDefaultWebsiteTimeoutMs;
    }
    if (gameVersion_.trimmed().isEmpty()) {
        gameVersion_ = QString::fromLatin1(kDefaultGameVersion);
    }
    if (platform_.trimmed().isEmpty()) {
        platform_ = QString::fromLatin1(kDefaultGamePlatform);
    }
}

bool WebsiteSyncClient::isConfigured() const {
    return !sanitizedBaseUrl().isEmpty();
}

QString WebsiteSyncClient::endpointBaseUrl() const {
    return sanitizedBaseUrl();
}

void WebsiteSyncClient::uploadBattleResult(const WebsiteBattleUpload& payload) {
    if (!isConfigured() || payload.username.trimmed().isEmpty()) {
        return;
    }

    QUrl endpoint(sanitizedBaseUrl() + QStringLiteral("/api/game-results"));
    if (!endpoint.isValid()) {
        qWarning() << "WebsiteSyncClient: invalid endpoint URL" << endpoint;
        return;
    }

    QJsonObject playerObject{
        {QStringLiteral("username"), payload.username.trimmed()},
        {QStringLiteral("characterType"), payload.characterType.trimmed()},
        {QStringLiteral("characterName"), payload.characterName.trimmed()}
    };
    if (payload.totalScore >= 0) {
        playerObject.insert(QStringLiteral("totalScore"), payload.totalScore);
    }
    if (payload.wins >= 0) {
        playerObject.insert(QStringLiteral("wins"), payload.wins);
    }
    if (payload.losses >= 0) {
        playerObject.insert(QStringLiteral("losses"), payload.losses);
    }
    if (payload.totalMatches >= 0) {
        playerObject.insert(QStringLiteral("matchesPlayed"), payload.totalMatches);
    }
    if (!payload.rankLabel.trimmed().isEmpty()) {
        playerObject.insert(QStringLiteral("rankLabel"), payload.rankLabel.trimmed());
    }

    QJsonObject battleObject{
        {QStringLiteral("mode"), payload.mode.trimmed()},
        {QStringLiteral("levelIndex"), payload.levelIndex},
        {QStringLiteral("levelName"), payload.levelName.trimmed()},
        {QStringLiteral("enemyType"), payload.enemyType.trimmed()},
        {QStringLiteral("enemyName"), payload.enemyName.trimmed()},
        {QStringLiteral("opponentUsername"), payload.opponentUsername.trimmed()},
        {QStringLiteral("victory"), payload.victory},
        {QStringLiteral("score"), payload.score},
        {QStringLiteral("damageDealt"), payload.damageDealt},
        {QStringLiteral("damageTaken"), payload.damageTaken},
        {QStringLiteral("playerHpEnd"), payload.playerHpEnd},
        {QStringLiteral("playerMaxHp"), payload.playerMaxHp},
        {QStringLiteral("battleDurationSeconds"), payload.battleDurationSeconds},
        {QStringLiteral("campaignComplete"), payload.campaignComplete},
        {QStringLiteral("playedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}
    };

    QJsonObject clientObject{
        {QStringLiteral("gameVersion"), gameVersion_},
        {QStringLiteral("platform"), platform_}
    };

    QJsonObject root{
        {QStringLiteral("player"), playerObject},
        {QStringLiteral("battle"), battleObject},
        {QStringLiteral("client"), clientObject}
    };

    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!apiKey_.trimmed().isEmpty()) {
        request.setRawHeader("X-Gladiators-Api-Key", apiKey_.trimmed().toUtf8());
    }

    QNetworkReply* reply = networkManager_->post(request, QJsonDocument(root).toJson(QJsonDocument::Compact));
    auto* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        if (!reply->isFinished()) {
            reply->setProperty("website_sync_timeout", true);
            reply->abort();
        }
    });
    timeoutTimer->start(timeoutMs_);

    const QString username = payload.username.trimmed();

    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer, username]() {
        timeoutTimer->stop();
        const bool timedOut = reply->property("website_sync_timeout").toBool();
        const QByteArray raw = reply->readAll();

        if (timedOut) {
            qWarning() << "WebsiteSyncClient: upload timed out";
            emit uploadFailed(username, QStringLiteral("Website sync timed out."));
            reply->deleteLater();
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "WebsiteSyncClient: upload failed:" << reply->errorString() << raw.left(240);
            emit uploadFailed(username, QStringLiteral("Website sync failed: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QString successMessage = QStringLiteral("Battle result synced to website.");
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            const QString rankLabel = doc.object()
                                          .value(QStringLiteral("player"))
                                          .toObject()
                                          .value(QStringLiteral("rankLabel"))
                                          .toString()
                                          .trimmed();
            if (!rankLabel.isEmpty()) {
                successMessage = QStringLiteral("Battle synced. Rank: %1").arg(rankLabel);
            }
        }

        qInfo() << "WebsiteSyncClient: upload succeeded for" << username;
        emit uploadSucceeded(username, successMessage);

        reply->deleteLater();
    });
}

void WebsiteSyncClient::uploadBattleHighlight(const WebsiteHighlightUpload& payload) {
    if (!isConfigured() || payload.username.trimmed().isEmpty() || payload.imageBytes.isEmpty()) {
        return;
    }

    QUrl endpoint(sanitizedBaseUrl() + QStringLiteral("/api/highlights"));
    if (!endpoint.isValid()) {
        qWarning() << "WebsiteSyncClient: invalid highlight endpoint URL" << endpoint;
        return;
    }

    auto addTextPart = [](QHttpMultiPart* multiPart, const QByteArray& name, const QString& value) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"%1\"").arg(QString::fromUtf8(name)));
        part.setBody(value.toUtf8());
        multiPart->append(part);
    };

    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    addTextPart(multiPart, "username", payload.username.trimmed());
    addTextPart(multiPart, "mode", payload.mode.trimmed());
    addTextPart(multiPart, "characterType", payload.characterType.trimmed());
    addTextPart(multiPart, "characterName", payload.characterName.trimmed());
    addTextPart(multiPart, "enemyType", payload.enemyType.trimmed());
    addTextPart(multiPart, "enemyName", payload.enemyName.trimmed());
    addTextPart(multiPart, "victory", payload.victory ? QStringLiteral("true") : QStringLiteral("false"));
    addTextPart(multiPart, "battleDurationSeconds", QString::number(payload.battleDurationSeconds, 'f', 3));
    addTextPart(multiPart, "score", QString::number(payload.score));
    addTextPart(multiPart, "attackType", payload.attackType.trimmed());
    addTextPart(multiPart, "damage", QString::number(payload.damage));
    addTextPart(multiPart, "wasProjectile", payload.wasProjectile ? QStringLiteral("true") : QStringLiteral("false"));
    addTextPart(multiPart, "wasFinisher", payload.wasFinisher ? QStringLiteral("true") : QStringLiteral("false"));
    addTextPart(multiPart, "highlightScore", QString::number(payload.highlightScore));
    addTextPart(multiPart, "playerHpBefore", QString::number(payload.playerHpBefore));
    addTextPart(multiPart, "playerHpAfter", QString::number(payload.playerHpAfter));
    addTextPart(multiPart, "playerMaxHp", QString::number(payload.playerMaxHp));
    addTextPart(multiPart, "enemyHpBefore", QString::number(payload.enemyHpBefore));
    addTextPart(multiPart, "enemyHpAfter", QString::number(payload.enemyHpAfter));
    addTextPart(multiPart, "levelIndex", QString::number(payload.levelIndex));
    addTextPart(multiPart, "levelName", payload.levelName.trimmed());
    addTextPart(multiPart,
                "capturedAt",
                (payload.capturedAtUtc.isValid() ? payload.capturedAtUtc : QDateTime::currentDateTimeUtc()).toString(Qt::ISODate));
    addTextPart(multiPart, "gameVersion", gameVersion_);
    addTextPart(multiPart, "platform", platform_);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader,
                        payload.imageMimeType.trimmed().isEmpty()
                            ? QStringLiteral("image/png")
                            : payload.imageMimeType.trimmed());
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QStringLiteral("form-data; name=\"image\"; filename=\"battle-highlight.png\""));
    auto* imageBuffer = new QBuffer(multiPart);
    imageBuffer->setData(payload.imageBytes);
    imageBuffer->open(QIODevice::ReadOnly);
    imagePart.setBodyDevice(imageBuffer);
    multiPart->append(imagePart);

    QNetworkRequest request(endpoint);
    if (!apiKey_.trimmed().isEmpty()) {
        request.setRawHeader("X-Gladiators-Api-Key", apiKey_.trimmed().toUtf8());
    }

    QNetworkReply* reply = networkManager_->post(request, multiPart);
    multiPart->setParent(reply);

    auto* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        if (!reply->isFinished()) {
            reply->setProperty("website_sync_timeout", true);
            reply->abort();
        }
    });
    timeoutTimer->start(timeoutMs_);

    const QString username = payload.username.trimmed();

    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer, username]() {
        timeoutTimer->stop();
        const bool timedOut = reply->property("website_sync_timeout").toBool();
        const QByteArray raw = reply->readAll();

        if (timedOut) {
            qWarning() << "WebsiteSyncClient: highlight upload timed out";
            emit uploadFailed(username, QStringLiteral("Highlight sync timed out."));
            reply->deleteLater();
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "WebsiteSyncClient: highlight upload failed:" << reply->errorString() << raw.left(240);
            emit uploadFailed(username, QStringLiteral("Highlight sync failed: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        qInfo() << "WebsiteSyncClient: highlight upload succeeded for" << username;
        reply->deleteLater();
    });
}

void WebsiteSyncClient::registerWebsiteAccount(const QString& email, const QString& username, const QString& password) {
    if (!isConfigured() || email.trimmed().isEmpty() || username.trimmed().isEmpty() || password.isEmpty()) {
        return;
    }

    QUrl endpoint(sanitizedBaseUrl() + QStringLiteral("/api/auth/register"));
    if (!endpoint.isValid()) {
        qWarning() << "WebsiteSyncClient: invalid account endpoint URL" << endpoint;
        return;
    }

    QJsonObject root{
        {QStringLiteral("email"), email.trimmed()},
        {QStringLiteral("username"), username.trimmed()},
        {QStringLiteral("password"), password}
    };

    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply* reply = networkManager_->post(request, QJsonDocument(root).toJson(QJsonDocument::Compact));
    auto* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        if (!reply->isFinished()) {
            reply->setProperty("website_sync_timeout", true);
            reply->abort();
        }
    });
    timeoutTimer->start(timeoutMs_);

    const QString cleanUsername = username.trimmed();
    connect(reply, &QNetworkReply::finished, this, [reply, timeoutTimer, cleanUsername]() {
        timeoutTimer->stop();
        const bool timedOut = reply->property("website_sync_timeout").toBool();
        const QByteArray raw = reply->readAll();

        if (timedOut) {
            qWarning() << "WebsiteSyncClient: website account sync timed out for" << cleanUsername;
            reply->deleteLater();
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "WebsiteSyncClient: website account sync failed:" << reply->errorString() << raw.left(240);
            reply->deleteLater();
            return;
        }

        qInfo() << "WebsiteSyncClient: website account synced for" << cleanUsername;
        reply->deleteLater();
    });
}

QString WebsiteSyncClient::readConfigValue(const QString& key) const {
    const QString envValue = QString::fromLocal8Bit(qgetenv(key.toUtf8().constData())).trimmed();
    if (!envValue.isEmpty()) {
        return envValue;
    }

    return readDotEnv().value(key).trimmed();
}

QHash<QString, QString> WebsiteSyncClient::readDotEnv() const {
    QHash<QString, QString> values;
    const QStringList candidates = {
        QDir::current().filePath(QStringLiteral(".env")),
        QDir::current().filePath(QStringLiteral("../.env")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral(".env")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../.env")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../.env"))
    };

    QString envPath;
    for (const QString& candidate : candidates) {
        if (QFile::exists(candidate)) {
            envPath = candidate;
            break;
        }
    }

    if (envPath.isEmpty()) {
        return values;
    }

    QFile file(envPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return values;
    }

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const int equals = line.indexOf('=');
        if (equals <= 0) {
            continue;
        }

        const QString key = line.left(equals).trimmed();
        QString value = line.mid(equals + 1).trimmed();
        if ((value.startsWith('"') && value.endsWith('"')) ||
            (value.startsWith('\'') && value.endsWith('\''))) {
            value = value.mid(1, value.size() - 2);
        }
        values.insert(key, value);
    }

    return values;
}

QString WebsiteSyncClient::sanitizedBaseUrl() const {
    QString trimmed = baseUrl_.trimmed();
    while (trimmed.endsWith('/')) {
        trimmed.chop(1);
    }
    return trimmed;
}
