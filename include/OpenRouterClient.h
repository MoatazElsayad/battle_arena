#ifndef OPENROUTERCLIENT_H
#define OPENROUTERCLIENT_H

#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

class QNetworkAccessManager;

class OpenRouterClient : public QObject {
    Q_OBJECT

public:
    using CompletionCallback = std::function<void(bool ok, const QString& content, const QString& error)>;

    explicit OpenRouterClient(QObject *parent = nullptr);

    bool isConfigured() const;
    QString configurationError() const;
    void requestRecommendation(const QString& systemPrompt,
                               const QString& userPrompt,
                               CompletionCallback callback);

private:
    static QString readConfigValue(const QString& key);
    void requestRecommendationWithModels(const QString& systemPrompt,
                                         const QString& userPrompt,
                                         const QStringList& models,
                                         int modelIndex,
                                         bool useJsonMode,
                                         const QString& lastError,
                                         CompletionCallback callback);

    QNetworkAccessManager *networkManager_;
    QString apiKey_;
    QString model_;
    QStringList fallbackModels_;
    QString appTitle_;
    QString siteUrl_;
    int timeoutMs_;
};

#endif // OPENROUTERCLIENT_H
