// Implement OpenRouterClient here.
#include <iostream>
#include "OpenRouterClient.h"
#include <QNetworkAccessManager>
#include <QJsonObject>
#include<QJsonDocument>
#include<QJsonArray>
#include<QNetworkReply>
#include<QNetworkRequest>
#include<QUrl>
using namespace std;


OpenRouterClient::OpenRouterClient()
{
   Manager = new QNetworkAccessManager();
}

void OpenRouterClient::SendRequest(QString prompt,function<void(QString)> callback)
{
  QUrl url("https://openrouter.ai/api/v1/chat/completions");
  QNetworkRequest request(url);
  request.setRawHeader("Authorization", "Bearer " + qgetenv("OPENROUTER_API_KEY"));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QJsonObject json;
  json["model"] = "";
  QJsonArray messages;
  QJsonObject message;
  message["role"] = "user";
  message["content"] = prompt;
  messages.append(message);
  json["messages"] = messages;

  QJsonDocument json_in_doc(json);
  QByteArray array;
  array = json_in_doc.toJson(QJsonDocument::Compact);

  QNetworkReply* ptr;
   ptr = Manager->post(request, array);

   connect(ptr, &QNetworkReply::finished,[=] () 
   {
    QByteArray response = ptr->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject json = doc.object();

   QJsonArray choices =  json["choices"].toArray();
   QJsonObject first = choices[0].toObject();
   QJsonObject message = first["message"].toObject();
   QString content = message["content"].toString();
   callback(content);

   ptr->deleteLater();

   });

   






}



// Keep it networking-only.
// No gameplay logic in this file.
// Main steps:
// - build request
// - send async POST
// - parse response
// - return success or fallback error
