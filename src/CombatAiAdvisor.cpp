// Implement CombatAiAdvisor here.
// Keep version 1 small:
// - one short JSON prompt
// - one short JSON response
// - one safe AiRecommendation
// This class should guide enemy style, not fully control combat.

#include <iostream>
#include <OpenRouterClient.h>
#include <CombatAiTypes.h>
#include <Enums.h>
using namespace std;

QString build_prompt(const CombatSnapshot& snapshot)
{ 

      
QString prompt;
prompt = prompt + "I  want you to assist the enemy by giving short tactical recommendations sometimes during combat by giving advice such as play aggressive,keep distance, use projectile, defend and wait";
prompt = prompt + "The player type is: " + snapshot.player_type + "\n";
prompt = prompt +  "The Enemy type is: " + snapshot.enemy_type + "\n";
prompt = prompt + "The player hp is: " + QString::number(snapshot.player_hp) + "\n";
prompt = prompt + "The enemy hp is " + QString::number(snapshot.enemy_hp) + "\n";
prompt = prompt + "The distance between the player and the enemy is: " + QString::number(snapshot.distance_between_player_enemy) + "\n";
prompt = prompt + "The difficulty level is: " + snapshot.current_level + "\n";
prompt = prompt + "Enemy projectile available: " + (snapshot.is_enemy_projectile_available ? "yes" : "no") + "\n";
prompt = prompt + "The recent player action is: " + snapshot.recent_player_action + "\n";
prompt = prompt + "The recent enemy action is " + snapshot.recent_enemy_action + "\n"; 
prompt = prompt + "The format of the reponse I want it to be something like this: ";
prompt = prompt + "{ \"strategy\": \"keep_distance\",";
prompt = prompt +  "\"PreferredAttack\": \"projectile\",";
prompt = prompt +  "\"durationMs\": 3000} ";  

return prompt;

}



void CombatAiAdvisor::requestAdvice(const CombatSnapshot& snapshot)
{
  OpenRouterClient client;
  QString prompt = build_prompt(snapshot);
  client.SendRequest(prompt, [=](QString content)
{
     AiRecommendation rec = ParseResponse(content);
     applyRecommendation(rec);
};
}


 void CombatAiAdvisor::applyRecommendation(const AiRecommendation& recommendation)
    {
      if(recommendation.strategy == Strategy::aggressive)
      {
         
      }
      else if(recommendation.strategy == Strategy::keep_distance)
      {

      }
      else if(recommendation.strategy == Strategy::defensive)
      {

      }
      else if(recommendation.strategy == Strategy::bait_attack)
      {

      }

      if(recommendation.PreferredAttack == PreferredAttack::melee)
      {

      }
      else if(recommendation.PreferredAttack ==  PreferredAttack::projectile)
      {

      }
     
      

    }

  AiRecommendaiton CombatAiAdvisor::ParseResponse(QString content)
  {
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
   QJsonObject json = doc.object();
   
   QString strategy_value =  json["strategy"].toString().toLower();
   
   QString attack_value =  json["PreferredAttack"].toString().toLower();
 
   double duration_value = json["durationMs"].toDouble();


   QMap<QString, Strategy> strategyMap;
   strategyMap["aggressive"] = Strategy::aggressive;
    strategyMap["keep_distance"] = Strategy::keep_distance;
     strategyMap["defensive"] = Strategy::defensive;
      strategyMap["bait_attack"] = Strategy::bait_attack;

   QMap<QString, PreferredAttack> PreferredAttackMap;
    PreferredAttackMap["melee"] = PreferredAttack::melee;
    PreferredAttackMap["projectile"] = PreferredAttack::projectile;
    PreferredAttackMap["defensive"] = PreferredAttack::defensive;
    PreferredAttackMap["bait_attack"] = PreferredAttack::bait_attack;


   AiRecommendation recommendation;
   recommendation.strategy = strategyMap.value(strategy_value, Strategy::defensive);
   recommendation.Attack = preferredAttackMap.value(Attack_value, PreferredAttack::melee);
   recommendation.durationMs = duration_value;

   
    return recommendation;
  }


----------




  QString characterTypeToString(CharacterType type)
  {
    
  }

  void CombatAiAdvisor::requestRecommendation(const CombatSnapshot& snapshot)
  {
     
  }



QString CombatAiAdvisor::buildSystemPrompt() const
{
   QString prompt;
   prompt = prompt + "You are a tactical combat AI Advisor for a video game \n";
   prompt = prompt + "Return ONLY valid JSON \n";
   prompt = prompt + "Do not explain. \n";
   prompt = prompt + "Do not use markdown. \n";
   prompt = prompt + "Do not use code fences. \n";
   prompt = prompt + "Allowed strategies: Aggressive, KeepDistance, Defensive, BaitAttack. \n";
prompt = prompt + "Allowed attacks: Melee, Projectile, Any \n";
prompt = prompt + "The durationMs should be between 800 and 4000 inclusive \n";
prompt = prompt + "I want you to return JSON only using this format: \n";
prompt = prompt + "{ \n"
prompt = prompt + "\"strategy\": \"KeepDistance\", \n";
prompt = prompt +  "\"preferredAttack\": \"Projectile\", \n";
prompt = prompt +  "\"durationMs\": 3000 \n"; 
prompt = prompt +  "} \n";

return prompt;
}
 QString CombatAiAdvisor::buildUserPrompt(const CombatSnapshot& snapshot) const
 {
    QString prompt;

prompt = prompt +  "Player Type: " + characterTypeToString(snapshot.playerType) + "\n";
prompt = prompt +  "Enemy Type: " + characterTypeToString(snapshot.enemyType) + "\n";
prompt = prompt + "Player HP: " + QString::number(snapshot.playerHp) + "\n";
prompt = prompt + "Enemy HP: " + QString::number(snapshot.enemyHp) + "\n";
prompt = prompt + "Distance: " + QString::number(snapshot.distance) + "\n";
prompt = prompt + "Difficulty level: " + QString::number(snapshot.currentLevel) + "\n";
prompt = prompt + "Enemy Projectile Available: " + (snapshot.projectileAvailable ? "yes" : "no") + "\n";
prompt = prompt + "Recent Player Action: " + snapshot.recentPlayerAction + "\n";
prompt = prompt + "Recent Enemy Action: " + snapshot.recentEnemyAction + "\n"; 

return prompt;
 }

  AiRecommendation CombatAiAdvisor::parseRecommendation(
    const QByteArray& response,
    const CombatSnapshot& snapshot
) const
{ 
  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(response, &error);

  if(error.error != QJsonParseError::NoError)
  {
    return fallbackRecommendation(snapshot, error.errorString());
  }

  if(!doc.isObject())
  {
    return fallbackRecommendation(snapshot, "Response is not a JSON object");
  }
   QJsonObject json = doc.object();

   QJsonValue strategyVal = json["strategy"];

   if(!strategyVal.isString())
   {
    return fallbackRecommendation(snapshot, "Missing or invalid strategy");
   }
   
   QString strategy_value = strategyVal.toString();

   if(!strategyMap.contains(strategy_value))
   {
    return fallbackRecommendation(snapshot, "Invalid strategy value");
   }

   QJsonValue attackVal = json["preferredAttack"];
   if(!attackVal.isString())
   {
    return fallbackRecommendation(snapshot, "Missing or invalid attack");
   }
   
   QString attack_value =  attackVal.toString();

if(!PreferredAttackMap.contains(attack_value))
   {
    return fallbackRecommendation(snapshot, "Invalid attack value");
   }
QJsonValue durationVal = json["durationMs"];
{
  if(!durationVal.isDouble())
  {
    return fallbackRecommendation(snapshot, "Missing or invalid duration");
  }
}

 
   double duration_value = durationVal.toDouble();


   QMap<QString, Strategy> strategyMap;
   strategyMap["Aggressive"] = Strategy::Aggressive;
    strategyMap["KeepDistance"] = Strategy::KeepDistance;
     strategyMap["Defensive"] = Strategy::Defensive;
      strategyMap["BaitAttack"] = Strategy::BaitAttack;

   QMap<QString, PreferredAttack> PreferredAttackMap;
    PreferredAttackMap["Melee"] = PreferredAttack::Melee;
    PreferredAttackMap["Projectile"] = PreferredAttack::Projectile;
    PreferredAttackMap["Any"] = PreferredAttack::Any;



   AiRecommendation recommendation;
   recommendation.strategy = strategyMap.value(strategy_value, Strategy::Defensive);
   recommendation.PreferredAttack = PreferredAttackMap.value(attack_value, PreferredAttack::Melee);
   recommendation.durationMs = duration_value;
   
   int duration = recommendation.durationMs;
  if(duration < 800)
  {
    recommendation.durationMs = 800;
  }
  else if (duration > 4000)
  {
    recommendation.durationMs = 4000;
  }



   
    return recommendation;
}

AiRecommendation CombatAiAdvisor::fallbackRecommendation(
        const CombatSnapshot& snapshot,
        const QString& reason = QString()
) const
{
  AiRecommendation recommendation;
  recommendation.fromFallback = true;
  if(snapshot.enemyHp < 20)
  {
    recommendation.strategy = Strategy::Defensive;
    recommendation.preferredAttack = PreferredAttack::Any;
  }

  else if(snapshot.projectileAvailable == true && snapshot.distance >= 15 )
  {
     recommendation.strategy = Strategy::KeepDistance;
    recommendation.preferredAttack = PreferredAttack::Projectile;
  }
  else if(snapshot.distance <= 10)
  {
    recommendation.strategy = Strategy::BaitAttack;
    recommendation.preferredAttack = PreferredAttack::Melee;
  }
  else
  {
    recommendation.strategy = Strategy::Aggressive;
    recommendation.preferredAttack = PreferredAttack::Melee;
  }
 
  recommendation.reason = reason;

  
  return recommendation;
}







