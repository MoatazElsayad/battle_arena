// Shared combat AI data only.
// Add:
// - CombatSnapshot: current battle state sent to the advisor
// - AiRecommendation: short tactical result returned to combat
// Keep both structs small and simple.


#include "Enums.h"

enum class Strategy {
    aggressive,
    keep_distance,
    defensive,
    bait_attack
};

enum class PreferredAttack {
    melee,
    projectile,
    any
};

struct CombatSnapshot
{
PlayerType player_type;
EnemyType enemy_type;
int player_hp;
int enemy_hp;
int distance_between_player_enemy;
DifficultyLevel current_level;
bool is_enemy_projectile_available;
AnimationState recent_player_action;
AnimationState recent_enemy_action;
};

struct AiRecommendation
{
Strategy strategy;
PreferredAttack Attack;
double durationMs;
};
