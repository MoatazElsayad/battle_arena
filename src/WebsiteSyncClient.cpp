#include "WebsiteSyncClient.h"

#include <QCoreApplication>
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

WebsiteSyncClient::WebsiteSyncClient(QObject* parent)
    : QObject(parent),
      networkManager_(new QNetworkAccessManager(this)),
      baseUrl_(readConfigValue(QStringLiteral("GLADIATORS_WEB_API_URL"))),
      apiKey_(readConfigValue(QStringLiteral("GLADIATORS_WEB_API_KEY"))),
      gameVersion_(readConfigValue(QStringLiteral("GLADIATORS_GAME_VERSION"))),
      platform_(readConfigValue(QStringLiteral("GLADIATORS_GAME_PLATFORM"))),
      timeoutMs_(qBound(2000, readConfigValue(QStringLiteral("GLADIATORS_WEB_TIMEOUT_MS")).toInt(), 20000)) {
    if (baseUrl_.trimmed().isEmpty()) {
        baseUrl_ = QStringLiteral("http://localhost:3001");
    }
    if (timeoutMs_ == 2000 && readConfigValue(QStringLiteral("GLADIATORS_WEB_TIMEOUT_MS")).trimmed().isEmpty()) {
        timeoutMs_ = 7000;
    }
    if (gameVersion_.trimmed().isEmpty()) {
        gameVersion_ = QStringLiteral("0.1.0");
    }
    if (platform_.trimmed().isEmpty()) {
        platform_ = QStringLiteral("windows");
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
