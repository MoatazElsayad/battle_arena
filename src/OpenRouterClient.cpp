#include "OpenRouterClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QDebug>
#include <QStringList>
#include <utility>

OpenRouterClient::OpenRouterClient(QObject *parent)
    : QObject(parent),
      networkManager_(new QNetworkAccessManager(this)),
      apiKey_(readConfigValue(QStringLiteral("OPENROUTER_API_KEY"))),
      model_(readConfigValue(QStringLiteral("OPENROUTER_MODEL"))),
      fallbackModels_(),
      appTitle_(readConfigValue(QStringLiteral("OPENROUTER_APP_TITLE"))),
      siteUrl_(readConfigValue(QStringLiteral("OPENROUTER_SITE_URL"))),
      timeoutMs_(readConfigValue(QStringLiteral("OPENROUTER_TIMEOUT_MS")).toInt()) {
    if (model_.trimmed().isEmpty()) {
        model_ = QStringLiteral("openai/gpt-4o-mini");
    }
    const QStringList fallbackKeys = {
        QStringLiteral("OPENROUTER_FALLBACK_MODEL"),
        QStringLiteral("OPENROUTER_SECOND_FALLBACK_MODEL"),
        QStringLiteral("OPENROUTER_THIRD_FALLBACK_MODEL"),
        QStringLiteral("OPENROUTER_FOURTH_FALLBACK_MODEL")
    };
    for (const QString& key : fallbackKeys) {
        const QString fallbackModel = readConfigValue(key).trimmed();
        if (!fallbackModel.isEmpty()
            && fallbackModel.compare(model_, Qt::CaseInsensitive) != 0
            && !fallbackModels_.contains(fallbackModel, Qt::CaseInsensitive)) {
            fallbackModels_.append(fallbackModel);
        }
    }
    if (appTitle_.trimmed().isEmpty()) {
        appTitle_ = QStringLiteral("Gladiators Combat Advisor");
    }
    if (timeoutMs_ <= 0) {
        timeoutMs_ = 9000;
    }
}

bool OpenRouterClient::isConfigured() const {
    return !apiKey_.trimmed().isEmpty() && !model_.trimmed().isEmpty();
}

QString OpenRouterClient::configurationError() const {
    if (apiKey_.trimmed().isEmpty()) {
        return QStringLiteral("OPENROUTER_API_KEY is missing");
    }
    if (model_.trimmed().isEmpty()) {
        return QStringLiteral("OPENROUTER_MODEL is missing");
    }
    return QString();
}

void OpenRouterClient::requestRecommendation(const QString& systemPrompt,
                                             const QString& userPrompt,
                                             CompletionCallback callback) {
    if (!isConfigured()) {
        if (callback) {
            callback(false, QString(), configurationError());
        }
        return;
    }

    QStringList models;
    models.append(model_);
    models.append(fallbackModels_);
    requestRecommendationWithModels(systemPrompt,
                                    userPrompt,
                                    models,
                                    0,
                                    true,
                                    QString(),
                                    std::move(callback));
}

void OpenRouterClient::requestRecommendationWithModels(const QString& systemPrompt,
                                                       const QString& userPrompt,
                                                       const QStringList& models,
                                                       int modelIndex,
                                                       bool useJsonMode,
                                                       const QString& lastError,
                                                       CompletionCallback callback) {
    if (modelIndex >= models.size()) {
        if (useJsonMode) {
            qWarning() << "OpenRouterClient: JSON-mode attempts failed; retrying without response_format."
                       << lastError.left(220);
            requestRecommendationWithModels(systemPrompt,
                                            userPrompt,
                                            models,
                                            0,
                                            false,
                                            lastError,
                                            std::move(callback));
            return;
        }
        if (callback) {
            callback(false, QString(), lastError.isEmpty()
                                         ? QStringLiteral("All OpenRouter models failed")
                                         : lastError);
        }
        return;
    }

    const QString currentModel = models.at(modelIndex).trimmed();
    if (currentModel.isEmpty()) {
        requestRecommendationWithModels(systemPrompt,
                                        userPrompt,
                                        models,
                                        modelIndex + 1,
                                        useJsonMode,
                                        lastError,
                                        std::move(callback));
        return;
    }

    qInfo() << "OpenRouterClient: requesting combat advice from" << currentModel
            << (useJsonMode ? "with JSON mode" : "without JSON mode");

    QNetworkRequest request(QUrl(QStringLiteral("https://openrouter.ai/api/v1/chat/completions")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(apiKey_).toUtf8());
    request.setRawHeader("X-Title", appTitle_.toUtf8());
    if (!siteUrl_.trimmed().isEmpty()) {
        request.setRawHeader("HTTP-Referer", siteUrl_.toUtf8());
    }

    QJsonArray messages;
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                {QStringLiteral("content"), systemPrompt}});
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                {QStringLiteral("content"), userPrompt}});

    QJsonObject body;
    body.insert(QStringLiteral("model"), currentModel);
    body.insert(QStringLiteral("messages"), messages);
    body.insert(QStringLiteral("temperature"), 0.25);
    body.insert(QStringLiteral("max_tokens"), 120);
    body.insert(QStringLiteral("stream"), false);
    if (useJsonMode) {
        body.insert(QStringLiteral("response_format"),
                    QJsonObject{{QStringLiteral("type"), QStringLiteral("json_object")}});
    }

    QNetworkReply *reply = networkManager_->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    auto *timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    QObject::connect(timeout, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    timeout->start(timeoutMs_);

    QObject::connect(reply, &QNetworkReply::finished, reply, [this,
                                                              reply,
                                                              systemPrompt,
                                                              userPrompt,
                                                              models,
                                                              modelIndex,
                                                              useJsonMode,
                                                              currentModel,
                                                              callback]() mutable {
        const QByteArray raw = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            const QString error = QStringLiteral("%1 failed: %2")
                                      .arg(currentModel, reply->errorString());
            qWarning() << "OpenRouterClient:" << error;
            reply->deleteLater();
            requestRecommendationWithModels(systemPrompt,
                                            userPrompt,
                                            models,
                                            modelIndex + 1,
                                            useJsonMode,
                                            error,
                                            std::move(callback));
            return;
        }
        if (status < 200 || status >= 300) {
            const QString error = QStringLiteral("%1 HTTP %2: %3")
                                      .arg(currentModel)
                                      .arg(status)
                                      .arg(QString::fromUtf8(raw.left(260)));
            qWarning() << "OpenRouterClient:" << error.left(340);
            reply->deleteLater();
            requestRecommendationWithModels(systemPrompt,
                                            userPrompt,
                                            models,
                                            modelIndex + 1,
                                            useJsonMode,
                                            error,
                                            std::move(callback));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        const QJsonObject root = doc.object();
        const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
        const QString content = choices.isEmpty()
            ? QString()
            : choices.first().toObject()
                  .value(QStringLiteral("message")).toObject()
                  .value(QStringLiteral("content")).toString();

        if (content.trimmed().isEmpty()) {
            const QString error = QStringLiteral("%1 response had no message content").arg(currentModel);
            qWarning() << "OpenRouterClient:" << error;
            requestRecommendationWithModels(systemPrompt,
                                            userPrompt,
                                            models,
                                            modelIndex + 1,
                                            useJsonMode,
                                            error,
                                            std::move(callback));
        } else if (callback) {
            qInfo() << "OpenRouterClient: combat advice succeeded with" << currentModel;
            callback(true, content, QString());
        }
        reply->deleteLater();
    });
}

QString OpenRouterClient::readConfigValue(const QString& key) {
    const QString envValue = QString::fromLocal8Bit(qgetenv(key.toUtf8().constData())).trimmed();
    if (!envValue.isEmpty()) {
        return envValue;
    }

    const QStringList candidates = {
        QDir::current().filePath(QStringLiteral(".env")),
        QDir::current().filePath(QStringLiteral("../.env")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral(".env")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../.env")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../.env"))
    };

    for (const QString& candidate : candidates) {
        QFile file(candidate);
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        while (!file.atEnd()) {
            QString line = QString::fromUtf8(file.readLine()).trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
                continue;
            }
            const int equals = line.indexOf(QLatin1Char('='));
            if (equals <= 0) {
                continue;
            }
            const QString name = line.left(equals).trimmed();
            if (name != key) {
                continue;
            }
            QString value = line.mid(equals + 1).trimmed();
            if ((value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"'))) ||
                (value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\'')))) {
                value = value.mid(1, value.size() - 2);
            }
            return value.trimmed();
        }
    }

    return QString();
}
