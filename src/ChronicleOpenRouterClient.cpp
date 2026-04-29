#include "ChronicleOpenRouterClient.h"

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
#include <QRandomGenerator>
#include <QTimer>
#include <QUrl>
#include <QStringList>

ChronicleOpenRouterClient::ChronicleOpenRouterClient(QObject *parent)
    : QObject(parent),
      networkManager_(new QNetworkAccessManager(this)),
      apiKey_(readConfigValue("OPENROUTER_API_KEY")),
      model_(readConfigValue("OPENROUTER_MODEL")),
      fallbackModels_(),
      lastSuccessfulModel_(),
      appTitle_(readConfigValue("OPENROUTER_APP_TITLE")),
      siteUrl_(readConfigValue("OPENROUTER_SITE_URL")),
      requestTimeoutMs_(qBound(3000, readConfigValue("OPENROUTER_TIMEOUT_MS").toInt(), 30000)) {
    const QString configuredFallback = readConfigValue("OPENROUTER_FALLBACK_MODEL").trimmed();
    const QString configuredSecondFallback = readConfigValue("OPENROUTER_SECOND_FALLBACK_MODEL").trimmed();
    const QString configuredThirdFallback = readConfigValue("OPENROUTER_THIRD_FALLBACK_MODEL").trimmed();
    const QString configuredFourthFallback = readConfigValue("OPENROUTER_FOURTH_FALLBACK_MODEL").trimmed();
    fallbackModels_ = {
        configuredFallback.isEmpty() ? QStringLiteral("nvidia/nemotron-3-super-120b-a12b:free")
                                     : configuredFallback,
        configuredSecondFallback.isEmpty() ? QStringLiteral("z-ai/glm-4.5-air:free")
                                           : configuredSecondFallback,
        configuredThirdFallback.isEmpty() ? QStringLiteral("tencent/hy3-preview:free")
                                          : configuredThirdFallback,
        configuredFourthFallback.isEmpty() ? QStringLiteral("openai/gpt-oss-120b:free")
                                           : configuredFourthFallback
    };
    if (requestTimeoutMs_ == 3000 && readConfigValue("OPENROUTER_TIMEOUT_MS").trimmed().isEmpty()) {
        requestTimeoutMs_ = 15000;
    }
    if (appTitle_.trimmed().isEmpty()) {
        appTitle_ = "Gladiators Chronicle";
    }
}

bool ChronicleOpenRouterClient::isConfigured() const {
    return !apiKey_.trimmed().isEmpty() && !model_.trimmed().isEmpty();
}

QString ChronicleOpenRouterClient::configuredModel() const {
    return model_;
}

QString ChronicleOpenRouterClient::configurationError() const {
    if (apiKey_.trimmed().isEmpty()) {
        return "OPENROUTER_API_KEY is missing";
    }
    if (model_.trimmed().isEmpty()) {
        return "OPENROUTER_MODEL is missing";
    }
    return QString();
}

void ChronicleOpenRouterClient::requestJsonCompletion(const QString& systemPrompt,
                                                      const QString& userPrompt,
                                                      CompletionCallback callback) {
    if (!isConfigured()) {
        if (callback) {
            callback(false, QString(), configurationError());
        }
        return;
    }

    QStringList modelChain;
    modelChain.append(model_);
    for (const QString& fallbackModel : fallbackModels_) {
        const QString trimmed = fallbackModel.trimmed();
        if (!trimmed.isEmpty() && !modelChain.contains(trimmed)) {
            modelChain.append(trimmed);
        }
    }

    if (!lastSuccessfulModel_.trimmed().isEmpty()) {
        const int preferredIndex = modelChain.indexOf(lastSuccessfulModel_);
        if (preferredIndex > 0) {
            modelChain.swapItemsAt(0, preferredIndex);
        }
    } else {
        // Spread requests across the free-tier models so one rate-limited model
        // does not become the first choice for every level transition.
        for (int i = modelChain.size() - 1; i > 0; --i) {
            const int j = QRandomGenerator::global()->bounded(i + 1);
            modelChain.swapItemsAt(i, j);
        }
    }

    dispatchRequest(systemPrompt, userPrompt, modelChain, 0, true, {}, callback);
}

void ChronicleOpenRouterClient::dispatchRequest(const QString& systemPrompt,
                                                const QString& userPrompt,
                                                const QStringList& modelChain,
                                                int modelIndex,
                                                bool useResponseFormat,
                                                const QStringList& errorHistory,
                                                CompletionCallback callback) {
    if (!isConfigured()) {
        if (callback) {
            callback(false, QString(), configurationError());
        }
        return;
    }
    if (modelIndex < 0 || modelIndex >= modelChain.size()) {
        if (callback) {
            callback(false, QString(), errorHistory.join(QStringLiteral(" | ")));
        }
        return;
    }

    const QString modelName = modelChain.at(modelIndex).trimmed();
    if (modelName.isEmpty()) {
        continueWithNextModel(systemPrompt,
                              userPrompt,
                              modelChain,
                              modelIndex,
                              errorHistory,
                              QStringLiteral("Encountered an empty model name in the fallback chain"),
                              callback);
        return;
    }

    QNetworkRequest request(QUrl("https://openrouter.ai/api/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey_).toUtf8());
    request.setRawHeader("X-Title", appTitle_.toUtf8());
    if (!siteUrl_.trimmed().isEmpty()) {
        request.setRawHeader("HTTP-Referer", siteUrl_.toUtf8());
    }

    QJsonArray messages;
    messages.append(QJsonObject{{"role", "system"}, {"content", systemPrompt}});
    messages.append(QJsonObject{{"role", "user"}, {"content", userPrompt}});

    QJsonObject body;
    body.insert("model", modelName);
    if (modelIndex + 1 < modelChain.size()) {
        QJsonArray fallbackArray;
        for (int i = modelIndex + 1; i < modelChain.size(); ++i) {
            fallbackArray.append(modelChain.at(i));
        }
        if (!fallbackArray.isEmpty()) {
            body.insert("models", fallbackArray);
        }
    }
    body.insert("messages", messages);
    body.insert("temperature", 0.45);
    body.insert("max_tokens", 420);
    body.insert("stream", false);
    body.insert("provider", QJsonObject{
        {"allow_fallbacks", true},
        {"require_parameters", false},
        {"sort", QJsonObject{
            {"by", "throughput"},
            {"partition", "none"}
        }},
        {"preferred_max_latency", QJsonObject{
            {"p90", 6},
            {"p99", 12}
        }},
        {"preferred_min_throughput", QJsonObject{
            {"p50", 15},
            {"p90", 8}
        }}
    });
    if (useResponseFormat) {
        body.insert("response_format", QJsonObject{{"type", "json_object"}});
    }

    QNetworkReply *reply = networkManager_->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    auto* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        if (!reply->isFinished()) {
            reply->setProperty("chronicle_timeout", true);
            reply->abort();
        }
    });
    timeoutTimer->start(requestTimeoutMs_);

    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer, callback, systemPrompt, userPrompt, modelChain, modelIndex, modelName, useResponseFormat, errorHistory]() {
        timeoutTimer->stop();
        const QByteArray raw = reply->readAll();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool networkOk = reply->error() == QNetworkReply::NoError;
        const bool timedOut = reply->property("chronicle_timeout").toBool();

        if (timedOut) {
            continueWithNextModel(systemPrompt,
                                  userPrompt,
                                  modelChain,
                                  modelIndex,
                                  errorHistory,
                                  QString("[%1] Timed out after %2 ms").arg(modelName).arg(requestTimeoutMs_),
                                  callback);
            reply->deleteLater();
            return;
        }

        if (!networkOk || httpStatus < 200 || httpStatus >= 300) {
            const QString error = !reply->errorString().isEmpty()
                ? reply->errorString()
                : QString("OpenRouter HTTP %1").arg(httpStatus);
            continueWithNextModel(systemPrompt,
                                  userPrompt,
                                  modelChain,
                                  modelIndex,
                                  errorHistory,
                                  QString("[%1] %2: %3").arg(modelName, error, QString::fromUtf8(raw.left(300))),
                                  callback);
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            continueWithNextModel(systemPrompt,
                                  userPrompt,
                                  modelChain,
                                  modelIndex,
                                  errorHistory,
                                  QString("[%1] Invalid OpenRouter response JSON").arg(modelName),
                                  callback);
            reply->deleteLater();
            return;
        }

        const QJsonArray choices = doc.object().value("choices").toArray();
        if (choices.isEmpty()) {
            continueWithNextModel(systemPrompt,
                                  userPrompt,
                                  modelChain,
                                  modelIndex,
                                  errorHistory,
                                  QString("[%1] OpenRouter returned no choices").arg(modelName),
                                  callback);
            reply->deleteLater();
            return;
        }

        const QJsonObject message = choices.first().toObject().value("message").toObject();
        const QString content = extractMessageContent(message);
        if (content.isEmpty()) {
            if (useResponseFormat) {
                reply->deleteLater();
                dispatchRequest(systemPrompt, userPrompt, modelChain, modelIndex, false, errorHistory, callback);
                return;
            }

            continueWithNextModel(systemPrompt,
                                  userPrompt,
                                  modelChain,
                                  modelIndex,
                                  errorHistory,
                                  QString("[%1] OpenRouter response content is empty").arg(modelName),
                                  callback);
            reply->deleteLater();
            return;
        }

        const QString actualModel = doc.object().value("model").toString().trimmed();
        lastSuccessfulModel_ = actualModel.isEmpty() ? modelName : actualModel;
        if (callback) {
            callback(true, content, QString());
        }
        reply->deleteLater();
    });
}

void ChronicleOpenRouterClient::continueWithNextModel(const QString& systemPrompt,
                                                      const QString& userPrompt,
                                                      const QStringList& modelChain,
                                                      int modelIndex,
                                                      const QStringList& errorHistory,
                                                      const QString& latestError,
                                                      CompletionCallback callback) {
    QStringList updatedErrors = errorHistory;
    if (!latestError.trimmed().isEmpty()) {
        updatedErrors.append(latestError.trimmed());
    }

    const int nextModelIndex = modelIndex + 1;
    if (nextModelIndex < modelChain.size()) {
        dispatchRequest(systemPrompt, userPrompt, modelChain, nextModelIndex, true, updatedErrors, callback);
        return;
    }

    if (callback) {
        callback(false, QString(), updatedErrors.join(QStringLiteral(" | ")));
    }
}

QString ChronicleOpenRouterClient::extractMessageContent(const QJsonObject& message) const {
    const QJsonValue contentValue = message.value("content");
    if (contentValue.isString()) {
        return contentValue.toString().trimmed();
    }

    if (contentValue.isObject()) {
        const QString text = contentValue.toObject().value("text").toString().trimmed();
        if (!text.isEmpty()) {
            return text;
        }
    }

    if (contentValue.isArray()) {
        QStringList chunks;
        const QJsonArray parts = contentValue.toArray();
        for (const QJsonValue& partValue : parts) {
            if (partValue.isString()) {
                const QString text = partValue.toString().trimmed();
                if (!text.isEmpty()) {
                    chunks.append(text);
                }
                continue;
            }

            if (!partValue.isObject()) {
                continue;
            }

            const QJsonObject part = partValue.toObject();
            const QString text = part.value("text").toString().trimmed();
            if (!text.isEmpty()) {
                chunks.append(text);
                continue;
            }

            const QString nestedText = part.value("content").toString().trimmed();
            if (!nestedText.isEmpty()) {
                chunks.append(nestedText);
            }
        }

        return chunks.join(QString());
    }

    return QString();
}

QString ChronicleOpenRouterClient::readConfigValue(const QString& key) const {
    const QString envValue = QString::fromLocal8Bit(qgetenv(key.toUtf8().constData())).trimmed();
    if (!envValue.isEmpty()) {
        return envValue;
    }

    return readDotEnv().value(key).trimmed();
}

QHash<QString, QString> ChronicleOpenRouterClient::readDotEnv() const {
    QHash<QString, QString> values;
    const QStringList candidates = {
        QDir::current().filePath(".env"),
        QDir::current().filePath("../.env"),
        QDir(QCoreApplication::applicationDirPath()).filePath(".env"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../.env"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../../.env")
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
