// Implement OpenRouterClient here.
#include <iostream>
#include "OpenRouterClient.h"
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
using namespace std;


OpenRouterClient::OpenRouterClient()
{
   Manager = new QNetworkAccessManager();
}

void OpenRouterClient::SendRequest(QString prompt, function<void(QString)> callback)
{
    // Get API key from environment
    QByteArray apiKeyBa = qgetenv("OPENROUTER_API_KEY");
    QString apiKey = QString::fromUtf8(apiKeyBa);

    
    if (apiKey.isEmpty()) {
        callback(QString("Error: OPENROUTER_API_KEY not set"));
        return;
    }

    // Get model from environment, default to gemini-2.0-flash
    QByteArray modelBa = qgetenv("OPENROUTER_MODEL");
    QString model = QString::fromUtf8(modelBa);
    if (model.isEmpty()) {
        model = QString("google/gemini-2.0-flash-001");
    }

    QUrl url("https://openrouter.ai/api/v1/chat/completions");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["model"] = model;
    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;
    messages.append(message);
    json["messages"] = messages;

    QJsonDocument json_in_doc(json);
    QByteArray array = json_in_doc.toJson(QJsonDocument::Compact);

    QNetworkReply* ptr = Manager->post(request, array);

    connect(ptr, &QNetworkReply::finished, [=]() {
        if (ptr->error() != QNetworkReply::NoError) {
            callback(QString("Network error: ") + ptr->errorString());
            ptr->deleteLater();
            return;
        }

        int httpStatus = ptr->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus != 200) {
            callback(QString("HTTP error: ") + QString::number(httpStatus));
            ptr->deleteLater();
            return;
        }

        QByteArray response = ptr->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isNull()) {
            callback(QString("Invalid JSON response"));
            ptr->deleteLater();
            return;
        }
        QJsonObject jsonObj = doc.object();

        QJsonArray choices = jsonObj["choices"].toArray();
        if (choices.isEmpty()) {
            callback(QString("No choices in response"));
            ptr->deleteLater();
            return;
        }
        QJsonObject first = choices[0].toObject();
        QJsonObject messageObj = first["message"].toObject();
        QString content = messageObj["content"].toString();
        if (content.isEmpty()) {
            callback(QString("Empty content in response"));
            ptr->deleteLater();
            return;
        }

        callback(content);
        ptr->deleteLater();
    });
}
