// OpenRouter API client only.
// This file should handle:
// - reading OPENROUTER_API_KEY
// - sending async requests
// - returning raw AI response or error
// Suggested functions:
// - constructor
// - isConfigured()
// - requestRecommendation(...)
// - handleNetworkReply(...)

class OpenRouterClient
{
  private:
QNetworkAccessManager* Manager;

public:

OpenRouterClient::OpenRouterClient();

void OpenRouterClient::SendRequest(QString prompt,function<void(QString)> callback);

  
}


// Keep it networking-only.
// No gameplay logic in this file.
// Main steps:
// - build request
// - send async POST
// - parse response
// - return success or fallback error
