#include "BattleWidget.h"
#include "GameManager.h"
#include "Player.h"
#include "Enemy.h"
#include "AnimatedCharacter.h"
#include "AnimationManager.h"
#include "InputHandler.h"
#include <QPainter>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <cmath>
#include <cstdlib>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>

namespace {
QString resolveAssetPath(const QString& relativePath) {
    const QStringList candidates = {
        QDir::current().filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath("../" + relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath("../../" + relativePath)
    };

    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return QDir::cleanPath(path);
        }
    }
    return QString();
}
}

BattleWidget::BattleWidget(QWidget *parent)
    : QWidget(parent),
      gameManager_(nullptr),
      soundManager_(nullptr),
      playerAnimChar_(new AnimatedCharacter(this)),
      enemyAnimChar_(new AnimatedCharacter(this)),
    playerAnimManager_(new AnimationManager()),
    enemyAnimManager_(new AnimationManager()),
      movingLeft_(false),
      movingRight_(false),
      attackPressed_(false),
      healPressed_(false),
      pausePressed_(false),
      battleActive_(false),
      playerCooldown_(0.0),
      enemyCooldown_(0.0),
      healCooldown_(0.0),
      battleEndDelay_(0.0),
      playerX_(PLAYER_START_X),
      enemyX_(ENEMY_START_X),
      playerHeight_(0.0),
      enemyHeight_(0.0),
      statusMessage_("Press A/D to move, J to attack, H to heal, ESC to pause"),
      statusDisplayTime_(0.0),
      queuedPlayerAttackState_(AnimationState::ATTACK1),
      playerHpDisplay_(1.0),
      enemyHpDisplay_(1.0),
      score_(0) {
    
    setStyleSheet("QWidget { background-color: #2A1810; }");
    setFocusPolicy(Qt::StrongFocus);
    
    // Initialize animation managers
    playerAnimChar_->setAnimationManager(playerAnimManager_);
    enemyAnimChar_->setAnimationManager(enemyAnimManager_);
    loadPrototypeAnimations();
    
    frameTimer_.setInterval(16); // ~60 FPS
    connect(&frameTimer_, &QTimer::timeout, this, &BattleWidget::advanceFrame);
}

BattleWidget::~BattleWidget() {
    delete playerAnimManager_;
    delete enemyAnimManager_;
}

void BattleWidget::setGameManager(GameManager *gm) {
    gameManager_ = gm;
}

void BattleWidget::setSoundManager(SoundManager *sm) {
    soundManager_ = sm;
}

void BattleWidget::startBattle() {
    if (!gameManager_) return;

    loadPrototypeAnimations();
    
    // Reset animation states
    playerAnimChar_->reset();
    enemyAnimChar_->reset();
    playerAnimChar_->setAnimationState(AnimationState::IDLE);
    enemyAnimChar_->setAnimationState(AnimationState::IDLE);
    
    battleActive_ = true;
    battleEndDelay_ = 0.0;
    playerCooldown_ = 0.0;
    enemyCooldown_ = 1.0; // Enemy starts with slight delay
    playerX_ = PLAYER_START_X;
    enemyX_ = ENEMY_START_X;
    statusMessage_ = "Battle started!";
    statusDisplayTime_ = 2.0;
    
    elapsedTimer_.start();
    frameTimer_.start();
}

void BattleWidget::stopBattle() {
    battleActive_ = false;
    frameTimer_.stop();
}

void BattleWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#2A1810"));
    
    if (!gameManager_ || !gameManager_->getPlayer() || !gameManager_->getCurrentEnemy()) {
        painter.setPen(QColor("#D4AF37"));
        painter.drawText(rect(), Qt::AlignCenter, "Initializing battle...");
        return;
    }
    
    // Draw arena background
    painter.fillRect(50, 80, width() - 100, height() - 160, QColor("#3D2817"));
    painter.setPen(QPen(QColor("#D4AF37"), 3));
    painter.drawRect(50, 80, width() - 100, height() - 160);
    
    // Draw ground line
    painter.setPen(QPen(QColor("#8B7355"), 2));
    painter.drawLine(50, GROUND_Y, width() - 50, GROUND_Y);
    
    const Player *player = gameManager_->getPlayer();
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    
// Draw fighters with animation support - Draw enemy first so player is on top
    drawFighterWithAnimation(painter, enemyX_, GROUND_Y, enemy->getHealth(), enemy->getMaxHealth(),
                             QString::fromStdString(enemy->getName()), enemyAnimChar_, false);
    drawFighterWithAnimation(painter, playerX_, GROUND_Y, player->getHealth(), player->getMaxHealth(),
                             QString::fromStdString(player->getName()), playerAnimChar_, true);
    
    // Draw HUD and status
    drawHUD(painter);
    drawStatus(painter);
}

void BattleWidget::drawFighter(QPainter &painter, double x, double y, int hp, int maxHp,
                               const QString &name, bool isPlayer, bool attacking) {
    // Body
    QColor bodyColor = isPlayer ? QColor("#4169E1") : QColor("#DC143C");
    if (attacking) {
        bodyColor = isPlayer ? QColor("#6495ED") : QColor("#FF69B4");
    }
    
    painter.fillRect(x - 30, y - 60, 60, 80, bodyColor);
    
    // Head
    painter.fillRect(x - 20, y - 90, 40, 35, bodyColor.lighter());
    
    // Border
    painter.setPen(QPen(QColor("#D4AF37"), 2));
    painter.drawRect(x - 30, y - 60, 60, 80);
    painter.drawRect(x - 20, y - 90, 40, 35);
    
    // Name label
    painter.setPen(QColor("#D4AF37"));
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(x - 40, y + 80, 80, 20, Qt::AlignCenter, name);
}

void BattleWidget::drawHealthBar(QPainter &painter, double x, double y, int hp, int maxHp) {
    int barWidth = 80;
    int barHeight = 15;
    
    // Background
    painter.fillRect(x, y, barWidth, barHeight, QColor("#5C4033"));
    painter.setPen(QPen(QColor("#D4AF37"), 1));
    painter.drawRect(x, y, barWidth, barHeight);
    
    // Health fill
    if (maxHp > 0) {
        int fillWidth = (hp * barWidth) / maxHp;
        QColor healthColor = (hp > maxHp / 2) ? QColor("#00FF00") : QColor("#FF8800");
        if (hp <= maxHp / 4) healthColor = QColor("#FF0000");
        painter.fillRect(x + 1, y + 1, fillWidth - 2, barHeight - 2, healthColor);
    }
    
    // HP text
    painter.setPen(QColor("#FFD700"));
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    painter.drawText(x, y, barWidth, barHeight, Qt::AlignCenter,
                     QString("%1/%2").arg(hp).arg(maxHp));
}

void BattleWidget::drawStatus(QPainter &painter) {
    painter.setPen(QColor("#FFD700"));
    QFont font = painter.font();
    font.setPointSize(11);
    font.setBold(true);
    painter.setFont(font);
    
    painter.drawText(50, 50, width() - 100, 25, Qt::AlignCenter, statusMessage_);
    
    // Instructions
    font.setPointSize(9);
    font.setBold(false);
    painter.setFont(font);
    painter.setPen(QColor("#D4AF37"));
    painter.drawText(50, height() - 80, width() - 100, 20, Qt::AlignCenter,
                     "A/D: Move  |  J/K/L: Attack 1/2/3  |  W: Jump  |  H: Heal  |  ESC: Pause");
}

void BattleWidget::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;

    CharacterType playerType = CharacterType::KNIGHT;
    if (gameManager_) {
        playerType = gameManager_->getSelectedCharacterType();
    }

    auto rejectAction = [this](const QString &text) {
        statusMessage_ = text;
        statusDisplayTime_ = 1.2;
    };
    
    switch (event->key()) {
        case Qt::Key_A:
        case Qt::Key_Left:
            movingLeft_ = true;
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            movingRight_ = true;
            break;
        case Qt::Key_J:
        case Qt::Key_Space:
            if (InputHandler::canPerformAction(playerType, PlayerAction::ATTACK1)) {
                attackPressed_ = true;
                queuedPlayerAttackState_ = AnimationState::ATTACK1;
            } else {
                rejectAction("This character cannot use Attack 1");
            }
            break;
        case Qt::Key_K:
            if (InputHandler::canPerformAction(playerType, PlayerAction::ATTACK2)) {
                attackPressed_ = true;
                queuedPlayerAttackState_ = AnimationState::ATTACK2;
                statusMessage_ = "Attack 2 queued";
                statusDisplayTime_ = 0.8;
            } else {
                rejectAction("This character has no Attack 2");
            }
            break;
        case Qt::Key_L:
            if (InputHandler::canPerformAction(playerType, PlayerAction::ATTACK3)) {
                attackPressed_ = true;
                queuedPlayerAttackState_ = AnimationState::ATTACK3;
                statusMessage_ = "Attack 3 queued";
                statusDisplayTime_ = 0.8;
            } else {
                rejectAction("This character has no Attack 3");
            }
            break;
        case Qt::Key_W:
        case Qt::Key_Up:
            if (InputHandler::canPerformAction(playerType, PlayerAction::JUMP)) {
                statusMessage_ = "Jump action triggered";
                statusDisplayTime_ = 0.8;
            } else {
                rejectAction("This character cannot jump");
            }
            break;
        case Qt::Key_H:
            healPressed_ = true;
            break;
        case Qt::Key_Escape:
            emit pauseRequested();
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

void BattleWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    
    switch (event->key()) {
        case Qt::Key_A:
        case Qt::Key_Left:
            movingLeft_ = false;
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            movingRight_ = false;
            break;
        case Qt::Key_J:
        case Qt::Key_Space:
        case Qt::Key_K:
        case Qt::Key_L:
            attackPressed_ = false;
            break;
        case Qt::Key_H:
            healPressed_ = false;
            break;
        default:
            QWidget::keyReleaseEvent(event);
    }
}

void BattleWidget::advanceFrame() {
    if (!gameManager_ || !battleActive_) return;
    
    const double dt = qMin(0.033, elapsedTimer_.restart() / 1000.0);
    
    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    
    if (!player || !enemy) return;
    
    // Let death animations play before leaving battle.
    if (!player->isAlive() || !enemy->isAlive()) {
        if (battleEndDelay_ <= 0.0) {
            battleEndDelay_ = 2.5; // Increased delay to 2.5 seconds to see the full death animation
            if (!player->isAlive() && playerAnimChar_) {
                playerAnimChar_->kill();
            }
            if (!enemy->isAlive() && enemyAnimChar_) {
                enemyAnimChar_->kill();
            }
        }

        battleEndDelay_ -= dt;
        if (battleEndDelay_ <= 0.0) {
            battleActive_ = false;
            emit battleFinished();
        }
        update();
        return;
    }
    
    // Update cooldowns
    playerCooldown_ = qMax(0.0, playerCooldown_ - dt);
    enemyCooldown_ = qMax(0.0, enemyCooldown_ - dt);
    healCooldown_ = qMax(0.0, healCooldown_ - dt);
    statusDisplayTime_ = qMax(0.0, statusDisplayTime_ - dt);
    
    // Lerp HP display
    const Player *p = gameManager_->getPlayer();
    const Enemy *e = gameManager_->getCurrentEnemy();
    if (p) {
        double target = p->getMaxHealth() > 0 ? (double)p->getHealth() / p->getMaxHealth() : 0.0;
        playerHpDisplay_ += (target - playerHpDisplay_) * qMin(1.0, 5.0 * dt);
    }
    if (e) {
        double target = e->getMaxHealth() > 0 ? (double)e->getHealth() / e->getMaxHealth() : 0.0;
        enemyHpDisplay_ += (target - enemyHpDisplay_) * qMin(1.0, 5.0 * dt);
    }

    // Player movement
    updatePlayerMovement(dt);
    
    // Enemy AI
    updateEnemyAI(dt);
    
    // Combat
    if (healPressed_) {
        tryPlayerHeal();
        healPressed_ = false;
    }
    if (attackPressed_) {
        tryPlayerAttack(dt);
        attackPressed_ = false; // One attack per press
    }
    
    if (enemyCooldown_ <= 0.0) {
        tryEnemyAttack(dt);
    }
    
    update();
}

void BattleWidget::updatePlayerMovement(double dt) {
    double moveAmount = MOVE_SPEED * dt;
    
    if (movingLeft_) {
        playerX_ = qMax(60.0, playerX_ - moveAmount);
    }
    if (movingRight_) {
        playerX_ = qMin(double(width()) - 60.0, playerX_ + moveAmount);
    }

    if (playerAnimChar_) {
        const AnimationState st = playerAnimChar_->getCurrentState();
        const bool locked = (st == AnimationState::ATTACK1 || st == AnimationState::ATTACK2 ||
                             st == AnimationState::ATTACK3 || st == AnimationState::STRONG_ATTACK ||
                             st == AnimationState::HURT || st == AnimationState::DEATH);
        if (!locked) {
            if (movingLeft_ || movingRight_) {
                playerAnimChar_->setAnimationState(AnimationState::RUN);
            } else {
                playerAnimChar_->setAnimationState(AnimationState::IDLE);
            }
        }
    }
}

void BattleWidget::updateEnemyAI(double dt) {
    double distToPlayer = playerX_ - enemyX_;
    double moveAmount = 150.0 * dt; // Enemy is slightly slower
    
    // Simple AI: approach if far, retreat if close
    if (std::abs(distToPlayer) > ATTACK_RANGE + 50) {
        // Approach player
        if (distToPlayer > 0) {
            enemyX_ = qMin(double(width()) - 60.0, enemyX_ + moveAmount);
        } else {
            enemyX_ = qMax(60.0, enemyX_ - moveAmount);
        }
    } else if (std::abs(distToPlayer) < ATTACK_RANGE - 30) {
        // Retreat a bit
        if (distToPlayer > 0) {
            enemyX_ = qMax(60.0, enemyX_ - moveAmount * 0.5);
        } else {
            enemyX_ = qMin(double(width()) - 60.0, enemyX_ + moveAmount * 0.5);
        }
    }

    if (enemyAnimChar_) {
        const AnimationState st = enemyAnimChar_->getCurrentState();
        const bool locked = (st == AnimationState::ATTACK1 || st == AnimationState::ATTACK2 ||
                             st == AnimationState::ATTACK3 || st == AnimationState::STRONG_ATTACK ||
                             st == AnimationState::HURT || st == AnimationState::DEATH);
        if (!locked) {
            if (std::abs(distToPlayer) > ATTACK_RANGE - 10) {
                enemyAnimChar_->setAnimationState(AnimationState::RUN);
            } else {
                enemyAnimChar_->setAnimationState(AnimationState::IDLE);
            }
        }
    }
}

void BattleWidget::tryPlayerAttack(double dt) {
    if (playerCooldown_ > 0.0) return;
    
    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    
    if (!player || !enemy) return;
    
    // Trigger player attack animation regardless of distance
    if (playerAnimChar_) {
        playerAnimChar_->setAnimationState(queuedPlayerAttackState_);
    }

    double distToEnemy = std::abs(enemyX_ - playerX_);
    
    if (distToEnemy <= ATTACK_RANGE) {
        int damage = player->calculateDamage();
        QString attackLabel = "Attack";
        if (queuedPlayerAttackState_ == AnimationState::ATTACK2) {
            damage = static_cast<int>(damage * 1.2);
        } else if (queuedPlayerAttackState_ == AnimationState::ATTACK3) {
            damage = static_cast<int>(damage * 1.35);
        }

        enemy->takeDamage(damage);
        score_ += damage * 10;
        
        // Trigger enemy hurt animation
        if (enemyAnimChar_) {
            enemyAnimChar_->takeDamage();
        }
        
        statusMessage_ = QString("Hit! Dealt %1 damage!").arg(damage);
        statusDisplayTime_ = 1.0;
        
        if (!enemy->isAlive()) {
            statusMessage_ = "Victory! Enemy defeated!";
            statusDisplayTime_ = 3.0;
            if (enemyAnimChar_) {
                enemyAnimChar_->kill();
            }
        }
    } else {
        statusMessage_ = "Miss! Too far away!";
        statusDisplayTime_ = 1.0;
    }

    playerCooldown_ = PLAYER_ATTACK_COOLDOWN;
}

void BattleWidget::tryEnemyAttack(double dt) {
    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    
    if (!player || !enemy) return;
    
    double distToPlayer = std::abs(playerX_ - enemyX_);
    
    if (distToPlayer <= ATTACK_RANGE) {
        int damage = enemy->calculateDamage();
        AnimationState enemyAttackAnim = AnimationState::ATTACK1;
        const int roll = std::rand() % 100;
        if (roll < 45) {
            enemyAttackAnim = AnimationState::ATTACK1;
        } else if (roll < 78) {
            enemyAttackAnim = AnimationState::ATTACK2;
            damage = static_cast<int>(damage * 1.15);
        } else {
            enemyAttackAnim = AnimationState::ATTACK3;
            damage = static_cast<int>(damage * 1.3);
        }

        player->takeDamage(damage);
        
        // Trigger enemy attack animation
        if (enemyAnimChar_) {
            enemyAnimChar_->setAnimationState(enemyAttackAnim);
        }
        
        // Trigger player hurt animation
        if (playerAnimChar_) {
            playerAnimChar_->takeDamage();
        }
        
        enemyCooldown_ = ENEMY_ATTACK_COOLDOWN;
        statusMessage_ = QString("Enemy deals %1 damage!").arg(damage);
        statusDisplayTime_ = 1.5;
        
        if (!player->isAlive()) {
            statusMessage_ = "Defeat! You were defeated!";
            statusDisplayTime_ = 3.0;
            if (playerAnimChar_) {
                playerAnimChar_->kill();
            }
        }
    } else {
        // Enemy moves closer instead of attacking
        enemyCooldown_ = 0.3;
    }
}

void BattleWidget::loadPrototypeAnimations() {
    if (!playerAnimManager_ || !enemyAnimManager_) {
        return;
    }

    const QString knightBase = resolveAssetPath("../../Battle_Arena/assets/Knight 2D Pixel Art/Sprites/with_outline");
    const QString flyingDemonBase = resolveAssetPath("../../Battle_Arena/assets/Flying Demon 2D Pixel Art/Sprites/with_outline");

    playerFallbackSprite_ = QPixmap();
    enemyFallbackSprite_ = QPixmap();

    if (!knightBase.isEmpty()) {
        playerAnimManager_->loadAnimation(AnimationState::IDLE, 7, knightBase + "/IDLE.png", true, 150);
        playerAnimManager_->loadAnimation(AnimationState::RUN, 8, knightBase + "/RUN.png", true, 95);
        playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, knightBase + "/ATTACK 1.png", false, 60);
        playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 5, knightBase + "/ATTACK 2.png", false, 70);
        playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, knightBase + "/ATTACK 3.png", false, 60);
        playerAnimManager_->loadAnimation(AnimationState::DEATH, 12, knightBase + "/DEATH.png", false, 110);
        playerAnimManager_->loadAnimation(AnimationState::HURT, 4, knightBase + "/HURT.png", false, 90); // Fixed: Knight HURT is 4 frames

        QPixmap knightIdle(knightBase + "/IDLE.png");
        if (!knightIdle.isNull()) {
            const int frameWidth = knightIdle.width() / 7; // Fixed: Knight IDLE is 7 frames from Lobby
            playerFallbackSprite_ = knightIdle.copy(0, 0, frameWidth, knightIdle.height());
        }
    }

    if (!flyingDemonBase.isEmpty()) {
        enemyAnimManager_->loadAnimation(AnimationState::IDLE, 4, flyingDemonBase + "/IDLE.png", true, 180);
        enemyAnimManager_->loadAnimation(AnimationState::RUN, 4, flyingDemonBase + "/FLYING.png", true, 105);
        enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 8, flyingDemonBase + "/ATTACK.png", false, 60);
        enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 8, flyingDemonBase + "/ATTACK.png", false, 60);
        enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 8, flyingDemonBase + "/ATTACK.png", false, 60);
        enemyAnimManager_->loadAnimation(AnimationState::DEATH, 7, flyingDemonBase + "/DEATH.png", false, 120);
        enemyAnimManager_->loadAnimation(AnimationState::HURT, 4, flyingDemonBase + "/HURT.png", false, 95);

        QPixmap demonIdle(flyingDemonBase + "/IDLE.png");
        if (!demonIdle.isNull()) {
            const int frameWidth = demonIdle.width() / 4;
            enemyFallbackSprite_ = demonIdle.copy(0, 0, frameWidth, demonIdle.height());
        }
    }
}

void BattleWidget::drawHUD(QPainter &painter) {
    const int hudY = 30;
    const int barWidth = 300;
    const int barHeight = 25;
    
    // Player Health Bar (Top Left)
    QRect pBarRect(50, hudY, barWidth, barHeight);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#444444")); // Background
    painter.drawRect(pBarRect);
    
    QRect pFillRect(50, hudY, static_cast<int>(barWidth * playerHpDisplay_), barHeight);
    painter.setBrush(QColor("#FF0000")); // Health Color
    painter.drawRect(pFillRect);
    
    painter.setPen(QPen(QColor("#D4AF37"), 2)); // Border
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(pBarRect);
    
    // Player Name under bar
    painter.setFont(QFont("Showcard Gothic", 10));
    painter.drawText(50, hudY + barHeight + 15, "PLAYER");

    // Enemy Health Bar (Top Right)
    int eBarX = width() - 50 - barWidth;
    QRect eBarRect(eBarX, hudY, barWidth, barHeight);
    painter.setBrush(QColor("#444444"));
    painter.drawRect(eBarRect);
    
    QRect eFillRect(eBarX + (barWidth - (int)(barWidth * enemyHpDisplay_)), hudY, (int)(barWidth * enemyHpDisplay_), barHeight);
    painter.setBrush(QColor("#FF0000"));
    painter.drawRect(eFillRect);
    
    painter.setPen(QPen(QColor("#D4AF37"), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(eBarRect);
    
    // Enemy Name
    painter.drawText(QRect(eBarX, hudY + barHeight, barWidth, 20), Qt::AlignRight, "ENEMY");

    // Score (Center)
    painter.setFont(QFont("Showcard Gothic", 24));
    painter.setPen(QColor("#FFD700"));
    painter.drawText(0, hudY, width(), 40, Qt::AlignCenter, QString::number(score_));
}

void BattleWidget::tryPlayerHeal() {
    if (healCooldown_ > 0.0) return;
    
    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    if (!player) return;
    
    player->takeDamage(-25); // Heal 25 HP
    healCooldown_ = 5.0; // 5 second cooldown
    
    if (playerAnimChar_) {
        playerAnimChar_->setAnimationState(AnimationState::IDLE); // Play idle as heal pose
    }
    
    statusMessage_ = "Healing used! +25 HP";
    statusDisplayTime_ = 1.5;
}

void BattleWidget::drawFighterWithAnimation(QPainter &painter, double x, double y, int hp, int maxHp,
                                            const QString &name, AnimatedCharacter* animChar, bool isPlayer) {
    QPixmap frame = animChar ? animChar->getCurrentFrame() : QPixmap();
    if (frame.isNull()) {
        frame = isPlayer ? playerFallbackSprite_ : enemyFallbackSprite_;
    }

    if (!frame.isNull()) {
        const qreal scale = 5.0;
        const qreal targetWidth = frame.width() * scale;
        const qreal targetHeight = frame.height() * scale;

        const bool shouldFaceRight = isPlayer ? (enemyX_ >= playerX_) : (playerX_ <= enemyX_);

        painter.save();
        if (!shouldFaceRight) {
            painter.translate(x, 0);
            painter.scale(-1.0, 1.0);
            painter.translate(-x, 0);
        }

        // Apply a specific offset to push the sprite down (because characters are drawn with padding in their frames)
        const qreal targetYOffset = targetHeight * 0.15; // Push down by 15% of the sprites' height

        painter.drawPixmap(QRectF(x - targetWidth * 0.5, y - targetHeight + targetYOffset, targetWidth, targetHeight),
                           frame, QRectF(0, 0, frame.width(), frame.height()));
        painter.restore();
    } else {
        QColor bodyColor = isPlayer ? QColor("#4169E1") : QColor("#DC143C");
        painter.fillRect(x - 30, y - 60, 60, 80, bodyColor);
        painter.fillRect(x - 20, y - 90, 40, 35, bodyColor.lighter());
        painter.setPen(QPen(QColor("#D4AF37"), 2));
        painter.drawRect(x - 30, y - 60, 60, 80);
        painter.drawRect(x - 20, y - 90, 40, 35);
    }
    
    // Name label
    painter.setPen(QColor("#D4AF37"));
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(x - 40, y + 80, 80, 20, Qt::AlignCenter, name);
}

