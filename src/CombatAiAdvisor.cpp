// Implement CombatAiAdvisor here.
// Keep version 1 small:
// - one short JSON prompt
// - one short JSON response
// - one safe AiRecommendation
// This class should guide enemy style, not fully control combat.

#include <iostream>
#include <OpenRouterClient.h>
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






