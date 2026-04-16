#ifndef BATTLEWIDGET_H
#define BATTLEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QPixmap>
#include "Enums.h"

class GameManager;
class Player;
class Enemy;
class SoundManager;
class AnimatedCharacter;
class AnimationManager;

class BattleWidget : public QWidget {
    Q_OBJECT

public:
    explicit BattleWidget(QWidget *parent = nullptr);
    ~BattleWidget();

    void setGameManager(GameManager *gm);
    void setSoundManager(SoundManager *sm);
    void startBattle();
    void stopBattle();

signals:
    void battleFinished();
    void pauseRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void advanceFrame();

private:
    void loadPrototypeAnimations();
    void updatePlayerMovement(double dt);
    void updateEnemyAI(double dt);
    void tryPlayerAttack(double dt);
    void tryEnemyAttack(double dt);
    void drawFighter(QPainter &painter, double x, double y, int hp, int maxHp, 
                     const QString &name, bool isPlayer, bool attacking);
    void drawHealthBar(QPainter &painter, double x, double y, int hp, int maxHp);
    void drawStatus(QPainter &painter);
    void drawHUD(QPainter &painter);
    void drawFighterWithAnimation(QPainter &painter, double x, double y, int hp, int maxHp,
                                   const QString &name, AnimatedCharacter* animChar, bool isPlayer);
    void tryPlayerHeal();

    GameManager *gameManager_;
    SoundManager *soundManager_;
    AnimatedCharacter *playerAnimChar_;
    AnimatedCharacter *enemyAnimChar_;
    AnimationManager *playerAnimManager_;
    AnimationManager *enemyAnimManager_;
    QPixmap playerFallbackSprite_;
    QPixmap enemyFallbackSprite_;
    
    QTimer frameTimer_;
    QElapsedTimer elapsedTimer_;

    // Input state
    bool movingLeft_;
    bool movingRight_;
    bool attackPressed_;
    bool healPressed_;
    bool pausePressed_;

    // Game state
    bool battleActive_;
    double playerCooldown_;
    double enemyCooldown_;
    double healCooldown_;
    double battleEndDelay_;

    // HUD state
    double playerHpDisplay_;
    double enemyHpDisplay_;
    int score_;

    // Positions
    double playerX_;
    double enemyX_;
    double playerHeight_;
    double enemyHeight_;

    // Combat tracking
    QString statusMessage_;
    double statusDisplayTime_;
    AnimationState queuedPlayerAttackState_;

    static constexpr double PLAYER_START_X = 150.0;
    static constexpr double ENEMY_START_X = 850.0;
    static constexpr double GROUND_Y = 520.0;
    static constexpr double MOVE_SPEED = 200.0;
    static constexpr double ATTACK_RANGE = 120.0;
    static constexpr double PLAYER_ATTACK_COOLDOWN = 0.8;
    static constexpr double ENEMY_ATTACK_COOLDOWN = 1.2;
};

#endif // BATTLEWIDGET_H
