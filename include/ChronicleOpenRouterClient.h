#ifndef CHRONICLEOPENROUTERCLIENT_H
#define CHRONICLEOPENROUTERCLIENT_H

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

class QNetworkAccessManager;

class ChronicleOpenRouterClient : public QObject {
    Q_OBJECT

public:
    using CompletionCallback = std::function<void(bool success, const QString& content, const QString& error)>;

    explicit ChronicleOpenRouterClient(QObject *parent = nullptr);

    bool isConfigured() const;
    QString configuredModel() const;
    QString configurationError() const;

    void requestJsonCompletion(const QString& systemPrompt,
                               const QString& userPrompt,
                               CompletionCallback callback);

private:
    void dispatchRequest(const QString& systemPrompt,
                         const QString& userPrompt,
                         const QStringList& modelChain,
                         int modelIndex,
                         bool useResponseFormat,
                         const QStringList& errorHistory,
                         CompletionCallback callback);
    void continueWithNextModel(const QString& systemPrompt,
                               const QString& userPrompt,
                               const QStringList& modelChain,
                               int modelIndex,
                               const QStringList& errorHistory,
                               const QString& latestError,
                               CompletionCallback callback);
    QString extractMessageContent(const QJsonObject& message) const;
    QString readConfigValue(const QString& key) const;
    QHash<QString, QString> readDotEnv() const;

    QNetworkAccessManager *networkManager_;
    QString apiKey_;
    QString model_;
    QStringList fallbackModels_;
    QString lastSuccessfulModel_;
    QString appTitle_;
    QString siteUrl_;
    int requestTimeoutMs_;
};

#endif // CHRONICLEOPENROUTERCLIENT_H
