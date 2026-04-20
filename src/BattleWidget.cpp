#include "BattleWidget.h"
#include "GameManager.h"
#include "Player.h"
#include "Enemy.h"
#include "AnimatedCharacter.h"
#include "AnimationManager.h"
#include "InputHandler.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <cmath>
#include <cstdlib>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QImage>

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

QPixmap loadArenaBackgroundForLevel(int level) {
    const QStringList candidates = {
        QString("assets/backgrounds/Level_%1.png").arg(level),
        QString("assets/backgrounds/Level_%1.jpg").arg(level),
        QString("assets/backgrounds/Level_1.png"),
        QString("assets/backgrounds/Level_1.jpg"),
        QString("assets/ui/arena_background_placeholder.png")
    };

    for (const QString& candidate : candidates) {
        const QString resolved = resolveAssetPath(candidate);
        if (!resolved.isEmpty()) {
            const QPixmap pixmap(resolved);
            if (!pixmap.isNull()) {
                return pixmap;
            }
        }
    }

    return QPixmap();
}

QColor sampleGroundColorFromBackground(const QPixmap& pixmap) {
    if (pixmap.isNull()) {
        return QColor("#5A3A23");
    }

    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    if (image.isNull()) {
        return QColor("#5A3A23");
    }

    const int startY = static_cast<int>(image.height() * 0.58);
    const int endY = static_cast<int>(image.height() * 0.92);
    const int stepX = qMax(1, image.width() / 96);
    const int stepY = qMax(1, qMax(1, endY - startY) / 48);

    qint64 totalR = 0;
    qint64 totalG = 0;
    qint64 totalB = 0;
    qint64 samples = 0;

    for (int y = startY; y < endY; y += stepY) {
        const QRgb* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); x += stepX) {
            const QRgb pixel = row[x];
            if (qAlpha(pixel) < 8) {
                continue;
            }

            totalR += qRed(pixel);
            totalG += qGreen(pixel);
            totalB += qBlue(pixel);
            ++samples;
        }
    }

    if (samples == 0) {
        return QColor("#5A3A23");
    }

    QColor base(static_cast<int>(totalR / samples),
                static_cast<int>(totalG / samples),
                static_cast<int>(totalB / samples));

    if (base.lightness() < 32) {
        base = base.lighter(150);
    } else if (base.lightness() > 180) {
        base = base.darker(120);
    }

    return base;
}

QRect opaqueBounds(const QPixmap& px) {
    if (px.isNull()) {
        return QRect();
    }

    const QImage img = px.toImage().convertToFormat(QImage::Format_ARGB32);
    const int w = img.width();
    const int h = img.height();

    int minX = w;
    int minY = h;
    int maxX = -1;
    int maxY = -1;

    for (int y = 0; y < h; ++y) {
        const QRgb* row = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            if (qAlpha(row[x]) > 0) {
                minX = qMin(minX, x);
                minY = qMin(minY, y);
                maxX = qMax(maxX, x);
                maxY = qMax(maxY, y);
            }
        }
    }

    if (maxX < minX || maxY < minY) {
        return QRect(0, 0, w, h);
    }

    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
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
    introLockTime_(0.0),
      playerX_(PLAYER_START_X),
      enemyX_(ENEMY_START_X),
      playerHeight_(0.0),
      enemyHeight_(0.0),
    arcenProjectileActive_(false),
    arcenProjectileFacingRight_(true),
    arcenProjectileDamage_(0),
    arcenProjectileX_(0.0),
    arcenProjectileY_(0.0),
    arcenProjectileSpeed_(900.0),
    arcenProjectileAnimTime_(0.0),
    arcenProjectileFrame_(0),
    enemyProjectileActive_(false),
    enemyProjectileExploding_(false),
    enemyProjectileFacingRight_(false),
    enemyProjectileType_(EnemyType::FIRE_WORM),
    enemyProjectileDamage_(0),
    enemyProjectileX_(0.0),
    enemyProjectileY_(0.0),
    enemyProjectileSpeed_(720.0),
    enemyProjectileAnimTime_(0.0),
    enemyProjectileFrame_(0),
    enemyProjectileExplosionTime_(0.0),
      statusMessage_("Press A/D to move, J to attack, H to heal, ESC to pause"),
      statusDisplayTime_(0.0),
      queuedPlayerAttackState_(AnimationState::ATTACK1),
      playerHpDisplay_(1.0),
      enemyHpDisplay_(1.0),
      arenaGroundBaseColor_(QColor("#5A3A23")),
        score_(0) {
    // Sound teammate:
    // Most combat SFX will be triggered from this class.

    arenaBackgroundPlaceholder_ = loadArenaBackgroundForLevel(1);
    arenaGroundBaseColor_ = sampleGroundColorFromBackground(arenaBackgroundPlaceholder_);
    setStyleSheet("QWidget { background-color: #22140D; }");
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

double BattleWidget::groundY() const {
    const double arenaBottom = qMax(140.0, static_cast<double>(height()) - 54.0);
    return arenaBottom - ARENA_FLOOR_OFFSET;
}

void BattleWidget::refreshArenaBackground() {
    const int level = gameManager_ ? qMax(1, gameManager_->getCurrentLevel()) : 1;
    arenaBackgroundPlaceholder_ = loadArenaBackgroundForLevel(level);
    arenaGroundBaseColor_ = sampleGroundColorFromBackground(arenaBackgroundPlaceholder_);
}

void BattleWidget::drawArenaBackground(QPainter &painter) {
    painter.save();

    const QRect fullRect = rect();

    if (!arenaBackgroundPlaceholder_.isNull()) {
        painter.setOpacity(1.0);
        painter.drawPixmap(fullRect, arenaBackgroundPlaceholder_);
    } else {
        QLinearGradient skyGradient(fullRect.topLeft(), QPointF(fullRect.left(), fullRect.bottom()));
        skyGradient.setColorAt(0.0, QColor("#120B08"));
        skyGradient.setColorAt(0.38, QColor("#2A1810"));
        skyGradient.setColorAt(0.72, QColor("#4C2F1B"));
        skyGradient.setColorAt(1.0, QColor("#24150E"));
        painter.fillRect(fullRect, skyGradient);

        painter.setPen(QPen(QColor(255, 220, 160, 18), 2, Qt::DashLine));
        const QRect placeholderRect(46, 96, width() - 92, height() - 188);
        painter.drawRoundedRect(placeholderRect, 28, 28);

        QFont placeholderFont("Segoe UI", 16, QFont::DemiBold);
        painter.setFont(placeholderFont);
        painter.setPen(QColor(255, 225, 170, 42));
        painter.drawText(placeholderRect, Qt::AlignCenter,
                         "Arena Background Placeholder\nDrop your background image here later");
    }

    QLinearGradient atmosphere(fullRect.topLeft(), QPointF(fullRect.left(), fullRect.bottom()));
    atmosphere.setColorAt(0.0, QColor(12, 8, 6, 110));
    atmosphere.setColorAt(0.35, QColor(22, 13, 8, 58));
    atmosphere.setColorAt(1.0, QColor(18, 12, 8, 140));
    painter.fillRect(fullRect, atmosphere);

    const QRectF upperGlow(0.0, 0.0, width(), height() * 0.35);
    QLinearGradient glowGradient(upperGlow.topLeft(), upperGlow.bottomLeft());
    glowGradient.setColorAt(0.0, QColor(255, 210, 120, 30));
    glowGradient.setColorAt(1.0, QColor(255, 210, 120, 0));
    painter.fillRect(upperGlow, glowGradient);

    const double floorY = groundY();
    const QRectF floorRect(0.0, floorY - 6.0, width(), height() - floorY + 6.0);
    QLinearGradient floorGradient(floorRect.topLeft(), floorRect.bottomLeft());
    floorGradient.setColorAt(0.0, arenaGroundBaseColor_.lighter(116));
    floorGradient.setColorAt(0.45, arenaGroundBaseColor_);
    floorGradient.setColorAt(1.0, arenaGroundBaseColor_.darker(150));
    painter.fillRect(floorRect, floorGradient);

    painter.setPen(QPen(arenaGroundBaseColor_.lighter(132), 3));
    painter.drawLine(QPointF(0.0, floorY), QPointF(width(), floorY));

    painter.restore();
}

void BattleWidget::setGameManager(GameManager *gm) {
    gameManager_ = gm;
}

void BattleWidget::setSoundManager(SoundManager *sm) {
    // Sound teammate:
    // Use the shared SoundManager here for all combat sounds.
    soundManager_ = sm;
}

void BattleWidget::startBattle() {
    // Sound teammate:
    // Play battle start / wave start sound here.
    if (!gameManager_) return;

    refreshArenaBackground();
    loadPrototypeAnimations();
    
    // Reset animation states
    playerAnimChar_->reset();
    enemyAnimChar_->reset();
    playerAnimChar_->setAnimationState(AnimationState::IDLE);
    enemyAnimChar_->setAnimationState(AnimationState::IDLE);
    
    battleActive_ = true;
    battleEndDelay_ = 0.0;
    introLockTime_ = 0.9; // Keep fighters at their spawn points briefly.
    playerCooldown_ = 0.0;
    enemyCooldown_ = 1.0; // Enemy starts with slight delay
    const double arenaLeft = ARENA_LEFT_X + 70.0;
    const double arenaRight = qMax(arenaLeft + 220.0, double(width()) - ARENA_RIGHT_MARGIN - 70.0);
    playerX_ = arenaLeft;
    enemyX_ = arenaRight;
    if (playerAnimChar_) {
        playerAnimChar_->setFacingLeft(enemyX_ < playerX_);
    }
    if (enemyAnimChar_) {
        enemyAnimChar_->setFacingLeft(playerX_ < enemyX_);
    }
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    if (enemy) {
        statusMessage_ = QString("Stage %1/%2 - %3 enters the arena!")
                             .arg(gameManager_->getCurrentLevel())
                             .arg(gameManager_->getTotalLevels())
                             .arg(QString::fromStdString(enemy->getName()));
    } else {
        statusMessage_ = "Battle started!";
    }
    statusDisplayTime_ = 2.0;
    enemyProjectileActive_ = false;
    enemyProjectileExploding_ = false;
    enemyProjectileAnimTime_ = 0.0;
    enemyProjectileFrame_ = 0;
    enemyProjectileExplosionTime_ = 0.0;
    
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
    drawArenaBackground(painter);
    const double floorY = groundY();
    
    if (!gameManager_ || !gameManager_->getPlayer() || !gameManager_->getCurrentEnemy()) {
        painter.setPen(QColor("#D4AF37"));
        painter.drawText(rect(), Qt::AlignCenter, "Initializing battle...");
        return;
    }
    
    const Player *player = gameManager_->getPlayer();
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    
// Draw fighters with animation support - Draw enemy first so player is on top
    drawFighterWithAnimation(painter, enemyX_, floorY, enemy->getHealth(), enemy->getMaxHealth(),
                             QString::fromStdString(enemy->getName()), enemyAnimChar_, false);
    drawFighterWithAnimation(painter, playerX_, floorY, player->getHealth(), player->getMaxHealth(),
                             QString::fromStdString(player->getName()), playerAnimChar_, true);

    drawArcenProjectile(painter);
    drawEnemyProjectile(painter);
    
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
    QFont font = painter.font();
    font.setPointSize(11);
    font.setBold(true);
    painter.setFont(font);

    const QString text = statusMessage_.trimmed();
    if (!text.isEmpty()) {
        QFontMetrics metrics(font);
        const int pillWidth = qMin(width() - 160, metrics.horizontalAdvance(text) + 48);
        const QRect statusRect((width() - pillWidth) / 2, 108, pillWidth, 34);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(18, 10, 6, 215));
        painter.drawRoundedRect(statusRect, 17, 17);

        painter.setPen(QPen(QColor("#D4AF37"), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(statusRect.adjusted(1, 1, -1, -1), 16, 16);

        painter.setPen(QColor("#FFD700"));
        painter.drawText(statusRect, Qt::AlignCenter, text);
    }

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

    PlayerType playerType = PlayerType::KNIGHT;
    if (gameManager_) {
        playerType = gameManager_->getSelectedPlayerType();
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
    introLockTime_ = qMax(0.0, introLockTime_ - dt);
    
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
    
    // Enemy AI (delay movement briefly so characters don't immediately collapse to center)
    if (introLockTime_ <= 0.0) {
        updateEnemyAI(dt);
    }

    if (playerAnimChar_) {
        playerAnimChar_->setFacingLeft(enemyX_ < playerX_);
    }
    if (enemyAnimChar_) {
        enemyAnimChar_->setFacingLeft(playerX_ < enemyX_);
    }
    
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

    updateArcenProjectile(dt);
    updateEnemyProjectile(dt);
    
    update();
}

void BattleWidget::updatePlayerMovement(double dt) {
    double moveAmount = MOVE_SPEED * dt;
    
    if (movingLeft_) {
        playerX_ = qMax(ARENA_LEFT_X + 10.0, playerX_ - moveAmount);
    }
    if (movingRight_) {
        playerX_ = qMin(double(width()) - ARENA_RIGHT_MARGIN - 10.0, playerX_ + moveAmount);
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

    const PlayerType playerType = gameManager_ ? gameManager_->getSelectedPlayerType() : PlayerType::KNIGHT;
    if (playerType == PlayerType::ARCEN) {
        int damage = player->calculateDamage();
        if (queuedPlayerAttackState_ == AnimationState::ATTACK2) {
            damage = static_cast<int>(damage * 1.2);
        } else if (queuedPlayerAttackState_ == AnimationState::ATTACK3) {
            damage = static_cast<int>(damage * 1.35);
        }

        if (!arcenProjectileActive_) {
            spawnArcenProjectile(damage);
            statusMessage_ = "Arrow fired!";
            statusDisplayTime_ = 0.7;
        } else {
            statusMessage_ = "Arrow already in flight";
            statusDisplayTime_ = 0.7;
        }

        playerCooldown_ = PLAYER_ATTACK_COOLDOWN;
        return;
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
    
    const EnemyType enemyType = enemy->getEnemyType();
    double distToPlayer = std::abs(playerX_ - enemyX_);
    const int roll = std::rand() % 100;

    const bool prefersProjectile = (enemyType == EnemyType::FIRE_WORM ||
                                    enemyType == EnemyType::NIGHTWEAVER ||
                                    enemyType == EnemyType::FLYING_DEMON);
    double projectileRange = ATTACK_RANGE + 180.0;
    if (enemyType == EnemyType::FIRE_WORM) {
        projectileRange = ATTACK_RANGE + 360.0;
    } else if (enemyType == EnemyType::NIGHTWEAVER) {
        projectileRange = ATTACK_RANGE + 260.0;
    } else if (enemyType == EnemyType::FLYING_DEMON) {
        projectileRange = ATTACK_RANGE + 240.0;
    }

    const bool shouldUseProjectile =
        prefersProjectile &&
        !enemyProjectileActive_ &&
        distToPlayer <= projectileRange &&
        distToPlayer > ATTACK_RANGE * 0.75 &&
        ((enemyType == EnemyType::FIRE_WORM) ||
         (enemyType == EnemyType::NIGHTWEAVER && roll >= 28) ||
         (enemyType == EnemyType::FLYING_DEMON && roll >= 42));

    if (shouldUseProjectile) {
        int damage = enemy->calculateDamage();
        if (enemyType == EnemyType::NIGHTWEAVER) {
            damage = static_cast<int>(damage * 1.15);
        } else if (enemyType == EnemyType::FLYING_DEMON) {
            damage = static_cast<int>(damage * 1.1);
        }

        if (enemyAnimChar_) {
            enemyAnimChar_->setAnimationState(AnimationState::ATTACK2);
        }

        spawnEnemyProjectile(enemyType, damage);
        enemyCooldown_ = ENEMY_ATTACK_COOLDOWN + 0.25;
        statusMessage_ = enemyType == EnemyType::FIRE_WORM ? "Fire Worm launches a fireball!"
                      : enemyType == EnemyType::FLYING_DEMON ? "Flying Demon hurls a hellfire orb!"
                                                             : "Nightweaver fires a shadow bolt!";
        statusDisplayTime_ = 1.3;
        return;
    }

    if (distToPlayer <= ATTACK_RANGE) {
        int damage = enemy->calculateDamage();
        AnimationState enemyAttackAnim = AnimationState::ATTACK1;
        if (enemyType == EnemyType::EVIL_WIZARD) {
            if (roll < 52) {
                enemyAttackAnim = AnimationState::ATTACK1;
            } else {
                enemyAttackAnim = AnimationState::ATTACK2;
                damage = static_cast<int>(damage * 1.18);
            }
        } else if (enemyType == EnemyType::FIRE_WIZARD) {
            enemyAttackAnim = (roll < 55) ? AnimationState::ATTACK1 : AnimationState::ATTACK2;
            if (enemyAttackAnim == AnimationState::ATTACK2) {
                damage = static_cast<int>(damage * 1.12);
            }
        } else if (roll < 45) {
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

void BattleWidget::loadCharacterAnimations(PlayerType type) {
    if (!playerAnimManager_) return;

    playerFallbackSprite_ = QPixmap();
    arcenArrowSprite_ = QPixmap();
    arcenArrowMoveSprite_ = QPixmap();
    QString basePath;

    // Determine character folder and animation parameters
    switch (type) {
        case PlayerType::KNIGHT:
            basePath = resolveAssetPath("assets/players/Knight/Sprites");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 7, basePath + "/IDLE.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/RUN.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/ATTACK 1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 5, basePath + "/ATTACK 2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, basePath + "/ATTACK 3.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 12, basePath + "/DEATH.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/HURT.png", false, 90);
            break;

        case PlayerType::DEMON_SLAYER:
            basePath = resolveAssetPath("assets/players/Demon_Slayer/Sprites");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 4, basePath + "/Idle.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 4, basePath + "/Attack1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 4, basePath + "/Attack2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 4, basePath + "/Attack2.png", false, 60); // Reuse Attack2
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 7, basePath + "/Death.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take hit.png", false, 90);
            break;

        case PlayerType::FANTASY_WARRIOR:
            basePath = resolveAssetPath("assets/players/Fantasy_Warrior/Sprites");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 7, basePath + "/Attack1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 7, basePath + "/Attack2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 8, basePath + "/Attack3.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 7, basePath + "/Death.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take hit.png", false, 90);
            break;

        case PlayerType::HUNTRESS:
            basePath = resolveAssetPath("assets/players/Huntress/Sprites");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 5, basePath + "/Attack1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 5, basePath + "/Attack2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 7, basePath + "/Attack3.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 8, basePath + "/Death.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take hit.png", false, 90);
            break;

        case PlayerType::ARCEN:
            basePath = resolveAssetPath("assets/players/Arcen/Sprites/Character");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/Attack.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 6, basePath + "/Attack.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, basePath + "/Attack.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 10, basePath + "/Death.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Get Hit.png", false, 90);
            arcenArrowSprite_ = QPixmap(resolveAssetPath("assets/players/Arcen/Sprites/Arrow/Static.png"));
            arcenArrowMoveSprite_ = QPixmap(resolveAssetPath("assets/players/Arcen/Sprites/Arrow/Move.png"));
            break;

        case PlayerType::MARTIAL:
            basePath = resolveAssetPath("assets/players/Martial/Sprite");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 7, basePath + "/Attack1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 6, basePath + "/Attack2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 9, basePath + "/Attack3.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 11, basePath + "/Death.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take Hit.png", false, 90);
            break;

        case PlayerType::MARTIAL_HERO:
            basePath = resolveAssetPath("assets/players/Martial_Hero/Sprites");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/Attack1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 6, basePath + "/Attack2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, basePath + "/Attack2.png", false, 60); // Reuse Attack2
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 6, basePath + "/Death.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/Take Hit.png", false, 90);
            break;

        case PlayerType::MEDIEVAL_WARRIOR:
            basePath = resolveAssetPath("assets/players/Medieval_Warrior/Sprites");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 6, basePath + "/Run.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 4, basePath + "/Attack1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 4, basePath + "/Attack2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 5, basePath + "/Attack3.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 9, basePath + "/Death.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Get Hit.png", false, 90);
            break;

        case PlayerType::WIZARD:
            basePath = resolveAssetPath("assets/players/Wizard/Sprites");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 6, basePath + "/Idle.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 8, basePath + "/Attack1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 8, basePath + "/Attack2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 8, basePath + "/Attack2.png", false, 60); // Reuse Attack2
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 7, basePath + "/Death.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/Hit.png", false, 90);
            break;

        default:
            // Default to Knight
            basePath = resolveAssetPath("assets/players/Knight/Sprites");
            playerAnimManager_->loadAnimation(AnimationState::IDLE, 7, basePath + "/IDLE.png", true, 150);
            playerAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/RUN.png", true, 95);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/ATTACK 1.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK2, 5, basePath + "/ATTACK 2.png", false, 70);
            playerAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, basePath + "/ATTACK 3.png", false, 60);
            playerAnimManager_->loadAnimation(AnimationState::DEATH, 12, basePath + "/DEATH.png", false, 110);
            playerAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/HURT.png", false, 90);
            break;
    }

    // Load fallback idle frame for smooth display
    QString idlePath;
    int idleFrames = 7; // Default
    switch (type) {
        case PlayerType::KNIGHT: idlePath = basePath + "/IDLE.png"; idleFrames = 7; break;
        case PlayerType::DEMON_SLAYER: idlePath = basePath + "/Idle.png"; idleFrames = 4; break;
        case PlayerType::FANTASY_WARRIOR: idlePath = basePath + "/Idle.png"; idleFrames = 10; break;
        case PlayerType::HUNTRESS: idlePath = basePath + "/Idle.png"; idleFrames = 8; break;
        case PlayerType::ARCEN: idlePath = basePath + "/Idle.png"; idleFrames = 10; break;
        case PlayerType::MARTIAL: idlePath = basePath + "/Idle.png"; idleFrames = 10; break;
        case PlayerType::MARTIAL_HERO: idlePath = basePath + "/Idle.png"; idleFrames = 8; break;
        case PlayerType::MEDIEVAL_WARRIOR: idlePath = basePath + "/Idle.png"; idleFrames = 10; break;
        case PlayerType::WIZARD: idlePath = basePath + "/Idle.png"; idleFrames = 6; break;
        default: break;
    }

    QPixmap idleSheet(idlePath);
    if (!idleSheet.isNull()) {
        const int frameWidth = idleSheet.width() / idleFrames;
        playerFallbackSprite_ = idleSheet.copy(0, 0, frameWidth, idleSheet.height());
    }
}

void BattleWidget::loadEnemyAnimations(EnemyType type) {
    if (!enemyAnimManager_) return;

    enemyFallbackSprite_ = QPixmap();
    enemyProjectileMoveSprite_ = QPixmap();
    enemyProjectileExplodeSprite_ = QPixmap();
    QString basePath;
    QString idlePath;
    int idleFrames = 1;

    switch (type) {
        case EnemyType::FIRE_WORM:
            basePath = resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Worm");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 9, basePath + "/Idle.png", true, 140);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 9, basePath + "/Walk.png", true, 105);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 16, basePath + "/Attack.png", false, 55);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 16, basePath + "/Attack.png", false, 55);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 16, basePath + "/Attack.png", false, 55);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 8, basePath + "/Death.png", false, 120);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Get Hit.png", false, 90);
            enemyProjectileMoveSprite_ = QPixmap(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Fire Ball/Move.png"));
            enemyProjectileExplodeSprite_ = QPixmap(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Fire Ball/Explosion.png"));
            idlePath = basePath + "/Idle.png";
            idleFrames = 9;
            break;

        case EnemyType::FIRE_WIZARD:
            basePath = resolveAssetPath("assets/enemies/Fire_Wizard/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Move.png", true, 105);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 8, basePath + "/Attack.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 8, basePath + "/Attack.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 8, basePath + "/Attack.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 5, basePath + "/Death.png", false, 120);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/Take Hit.png", false, 90);
            idlePath = basePath + "/Idle.png";
            idleFrames = 8;
            break;

        case EnemyType::FLYING_DEMON:
            basePath = resolveAssetPath("assets/enemies/Flying_Demon/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 4, basePath + "/IDLE.png", true, 180);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 4, basePath + "/FLYING.png", true, 105);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 8, basePath + "/ATTACK.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 8, basePath + "/ATTACK.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 8, basePath + "/ATTACK.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 7, basePath + "/DEATH.png", false, 120);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/HURT.png", false, 95);
            enemyProjectileMoveSprite_ = QPixmap(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Fire Ball/Move.png"));
            enemyProjectileExplodeSprite_ = QPixmap(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Fire Ball/Explosion.png"));
            idlePath = basePath + "/IDLE.png";
            idleFrames = 4;
            break;

        case EnemyType::NIGHTWEAVER:
            basePath = resolveAssetPath("assets/enemies/Nightweaver/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 100);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 13, basePath + "/Attack.png", false, 55);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 13, basePath + "/Attack.png", false, 55);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 13, basePath + "/Attack.png", false, 55);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 18, basePath + "/Death.png", false, 95);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Get hit.png", false, 90);
            enemyProjectileMoveSprite_ = QPixmap(resolveAssetPath("assets/enemies/Nightweaver/Sprites/Projectile/Moving.png"));
            enemyProjectileExplodeSprite_ = QPixmap(resolveAssetPath("assets/enemies/Nightweaver/Sprites/Projectile/Explode.png"));
            idlePath = basePath + "/Idle.png";
            idleFrames = 10;
            break;

        case EnemyType::EVIL_WIZARD:
            basePath = resolveAssetPath("assets/enemies/Evil_Wizard/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 100);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 8, basePath + "/Attack1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 8, basePath + "/Attack2.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 8, basePath + "/Attack2.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 7, basePath + "/Death.png", false, 120);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take hit.png", false, 90);
            idlePath = basePath + "/Idle.png";
            idleFrames = 8;
            break;
    }

    QPixmap idleSheet(idlePath);
    if (!idleSheet.isNull() && idleFrames > 0) {
        const int frameWidth = idleSheet.width() / idleFrames;
        enemyFallbackSprite_ = idleSheet.copy(0, 0, frameWidth, idleSheet.height());
    }
}

void BattleWidget::loadPrototypeAnimations() {
    if (!playerAnimManager_ || !enemyAnimManager_ || !gameManager_) {
        return;
    }

    const Player *player = gameManager_->getPlayer();
    if (!player) return;

    arcenProjectileActive_ = false;
    arcenProjectileDamage_ = 0;
    arcenProjectileAnimTime_ = 0.0;
    arcenProjectileFrame_ = 0;

    // Load player animations based on selected character
    loadCharacterAnimations(player->getPlayerType());

    const Enemy *enemy = gameManager_->getCurrentEnemy();
    if (enemy) {
        loadEnemyAnimations(enemy->getEnemyType());
    }
}

void BattleWidget::spawnArcenProjectile(int damage) {
    arcenProjectileActive_ = true;
    arcenProjectileDamage_ = qMax(1, damage);
    arcenProjectileFacingRight_ = enemyX_ >= playerX_;
    arcenProjectileAnimTime_ = 0.0;
    arcenProjectileFrame_ = 0;

    const qreal arenaHeight = qMax(1.0, static_cast<qreal>(height() - 160));
    const qreal desiredPlayerHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
    arcenProjectileY_ = groundY() - desiredPlayerHeight * ARCEN_ARROW_LAUNCH_HEIGHT_RATIO;
    arcenProjectileX_ = playerX_ + (arcenProjectileFacingRight_ ? 45.0 : -45.0);
}

void BattleWidget::updateArcenProjectile(double dt) {
    if (!arcenProjectileActive_) {
        return;
    }

    arcenProjectileAnimTime_ += dt;
    if (arcenProjectileAnimTime_ >= 0.08) {
        arcenProjectileAnimTime_ = 0.0;
        arcenProjectileFrame_ = (arcenProjectileFrame_ + 1) % 2;
    }

    const double dir = arcenProjectileFacingRight_ ? 1.0 : -1.0;
    arcenProjectileX_ += dir * arcenProjectileSpeed_ * dt;

    if (arcenProjectileX_ < ARENA_LEFT_X || arcenProjectileX_ > (width() - ARENA_RIGHT_MARGIN)) {
        arcenProjectileActive_ = false;
        return;
    }

    if (!gameManager_) {
        arcenProjectileActive_ = false;
        return;
    }

    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    if (!enemy || !enemy->isAlive()) {
        arcenProjectileActive_ = false;
        return;
    }

    const double hitDistance = std::abs(arcenProjectileX_ - enemyX_);
    if (hitDistance <= ARCEN_PROJECTILE_HIT_WIDTH) {
        enemy->takeDamage(arcenProjectileDamage_);
        score_ += arcenProjectileDamage_ * 10;

        if (enemyAnimChar_) {
            enemyAnimChar_->takeDamage();
        }

        statusMessage_ = QString("Arrow hit! Dealt %1 damage!").arg(arcenProjectileDamage_);
        statusDisplayTime_ = 1.0;

        if (!enemy->isAlive()) {
            statusMessage_ = "Victory! Enemy defeated!";
            statusDisplayTime_ = 3.0;
            if (enemyAnimChar_) {
                enemyAnimChar_->kill();
            }
        }

        arcenProjectileActive_ = false;
        arcenProjectileDamage_ = 0;
    }
}

void BattleWidget::spawnEnemyProjectile(EnemyType type, int damage) {
    enemyProjectileActive_ = true;
    enemyProjectileExploding_ = false;
    enemyProjectileType_ = type;
    enemyProjectileDamage_ = qMax(1, damage);
    enemyProjectileFacingRight_ = playerX_ >= enemyX_;
    enemyProjectileAnimTime_ = 0.0;
    enemyProjectileFrame_ = 0;
    enemyProjectileExplosionTime_ = 0.0;
    enemyProjectileSpeed_ = (type == EnemyType::FIRE_WORM) ? 520.0
                         : (type == EnemyType::FLYING_DEMON) ? 640.0
                                                             : 760.0;

    const qreal arenaHeight = qMax(1.0, static_cast<qreal>(height() - 160));
    const qreal desiredEnemyHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
    const qreal projectileHeightRatio = (type == EnemyType::FIRE_WORM) ? 0.28
                                    : (type == EnemyType::FLYING_DEMON) ? 0.72
                                                                        : 0.68;
    enemyProjectileY_ = groundY() - desiredEnemyHeight * projectileHeightRatio;
    enemyProjectileX_ = enemyX_ + (enemyProjectileFacingRight_ ? 46.0 : -46.0);
}

void BattleWidget::updateEnemyProjectile(double dt) {
    if (!enemyProjectileActive_) {
        return;
    }

    enemyProjectileAnimTime_ += dt;

    if (enemyProjectileExploding_) {
        enemyProjectileExplosionTime_ += dt;
        if (enemyProjectileExplosionTime_ >= 0.08) {
            enemyProjectileExplosionTime_ = 0.0;
            ++enemyProjectileFrame_;
        }

        const int maxExplosionFrames = 7;
        if (enemyProjectileFrame_ >= maxExplosionFrames) {
            enemyProjectileActive_ = false;
            enemyProjectileExploding_ = false;
        }
        return;
    }

    if (enemyProjectileAnimTime_ >= 0.08) {
        enemyProjectileAnimTime_ = 0.0;
        const int maxMoveFrames = (enemyProjectileType_ == EnemyType::FIRE_WORM ||
                                   enemyProjectileType_ == EnemyType::FLYING_DEMON) ? 6 : 4;
        enemyProjectileFrame_ = (enemyProjectileFrame_ + 1) % maxMoveFrames;
    }

    const double dir = enemyProjectileFacingRight_ ? 1.0 : -1.0;
    enemyProjectileX_ += dir * enemyProjectileSpeed_ * dt;

    Player *player = gameManager_ ? const_cast<Player*>(gameManager_->getPlayer()) : nullptr;
    if (!player || !player->isAlive()) {
        enemyProjectileActive_ = false;
        return;
    }

    if (enemyProjectileX_ < ARENA_LEFT_X || enemyProjectileX_ > (width() - ARENA_RIGHT_MARGIN)) {
        enemyProjectileActive_ = false;
        return;
    }

    const double hitDistance = std::abs(enemyProjectileX_ - playerX_);
    if (hitDistance <= ENEMY_PROJECTILE_HIT_WIDTH) {
        player->takeDamage(enemyProjectileDamage_);
        if (playerAnimChar_) {
            playerAnimChar_->takeDamage();
        }

        statusMessage_ = enemyProjectileType_ == EnemyType::FIRE_WORM
                             ? QString("Fireball hit! Took %1 damage!").arg(enemyProjectileDamage_)
                             : QString("Shadow bolt hit! Took %1 damage!").arg(enemyProjectileDamage_);
        statusDisplayTime_ = 1.2;

        enemyProjectileExploding_ = !enemyProjectileExplodeSprite_.isNull();
        enemyProjectileFrame_ = 0;
        enemyProjectileAnimTime_ = 0.0;
        enemyProjectileExplosionTime_ = 0.0;

        if (!enemyProjectileExploding_) {
            enemyProjectileActive_ = false;
        }

        if (!player->isAlive()) {
            statusMessage_ = "Defeat! You were defeated!";
            statusDisplayTime_ = 3.0;
            if (playerAnimChar_) {
                playerAnimChar_->kill();
            }
        }
    }
}

void BattleWidget::drawArcenProjectile(QPainter &painter) {
    if (!arcenProjectileActive_) {
        return;
    }

    QPixmap spriteSheet = arcenArrowMoveSprite_.isNull() ? arcenArrowSprite_ : arcenArrowMoveSprite_;
    if (spriteSheet.isNull()) {
        return;
    }

    QPixmap sprite;
    if (!arcenArrowMoveSprite_.isNull()) {
        const int frameWidth = qMax(1, spriteSheet.width() / 2);
        const int frameHeight = spriteSheet.height();
        const int x = qBound(0, arcenProjectileFrame_ * frameWidth, spriteSheet.width() - frameWidth);
        sprite = spriteSheet.copy(x, 0, frameWidth, frameHeight);
    } else {
        sprite = spriteSheet;
    }

    const qreal arenaHeight = qMax(1.0, static_cast<qreal>(height() - 160));
    const qreal desiredPlayerHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
    QPixmap arcenReferenceFrame;
    if (playerAnimManager_) {
        arcenReferenceFrame = playerAnimManager_->getFrame(AnimationState::IDLE, 0);
    }

    if (arcenReferenceFrame.isNull()) {
        arcenReferenceFrame = sprite;
    }

    const QRect referenceBounds = opaqueBounds(arcenReferenceFrame);
    const qreal referenceVisibleHeight = qMax(1, referenceBounds.height());
    const qreal scale = desiredPlayerHeight / referenceVisibleHeight;
    const qreal w = sprite.width() * scale;
    const qreal h = sprite.height() * scale;

    painter.save();
    if (!arcenProjectileFacingRight_) {
        painter.translate(arcenProjectileX_, 0.0);
        painter.scale(-1.0, 1.0);
        painter.translate(-arcenProjectileX_, 0.0);
    }

    painter.drawPixmap(QRectF(arcenProjectileX_ - w * 0.5, arcenProjectileY_ - h * 0.5, w, h),
                       sprite, QRectF(0, 0, sprite.width(), sprite.height()));
    painter.restore();
}

void BattleWidget::drawEnemyProjectile(QPainter &painter) {
    if (!enemyProjectileActive_) {
        return;
    }

    QPixmap spriteSheet = enemyProjectileExploding_ ? enemyProjectileExplodeSprite_ : enemyProjectileMoveSprite_;
    if (spriteSheet.isNull()) {
        return;
    }

    const int frameCount = enemyProjectileExploding_
                               ? 7
                               : ((enemyProjectileType_ == EnemyType::FIRE_WORM ||
                                   enemyProjectileType_ == EnemyType::FLYING_DEMON) ? 6 : 4);
    const int frameWidth = qMax(1, spriteSheet.width() / qMax(1, frameCount));
    const int frameHeight = spriteSheet.height();
    const int frameIndex = qBound(0, enemyProjectileFrame_, frameCount - 1);
    const QPixmap sprite = spriteSheet.copy(frameIndex * frameWidth, 0, frameWidth, frameHeight);

    const qreal arenaHeight = qMax(1.0, static_cast<qreal>(height() - 160));
    const qreal desiredEnemyHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
    const qreal spriteScale = (enemyProjectileType_ == EnemyType::FIRE_WORM) ? 0.26
                             : (enemyProjectileType_ == EnemyType::FLYING_DEMON) ? 0.23
                                                                                  : 0.22;
    const qreal scale = desiredEnemyHeight * spriteScale / qMax(1, sprite.height());
    const qreal w = sprite.width() * scale;
    const qreal h = sprite.height() * scale;

    painter.save();
    if (!enemyProjectileFacingRight_ && !enemyProjectileExploding_) {
        painter.translate(enemyProjectileX_, 0.0);
        painter.scale(-1.0, 1.0);
        painter.translate(-enemyProjectileX_, 0.0);
    }

    painter.drawPixmap(QRectF(enemyProjectileX_ - w * 0.5, enemyProjectileY_ - h * 0.5, w, h),
                       sprite, QRectF(0, 0, sprite.width(), sprite.height()));
    painter.restore();
}

void BattleWidget::drawHUD(QPainter &painter) {
    if (!gameManager_) {
        return;
    }

    const Player *player = gameManager_->getPlayer();
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    if (!player || !enemy) {
        return;
    }

    const int outerMargin = 28;
    const int hudY = 18;
    const int panelWidth = qMin(360, qMax(260, (width() - 260) / 2));
    const int panelHeight = 74;
    const int centerWidth = 184;
    const QRectF playerPanel(outerMargin, hudY, panelWidth, panelHeight);
    const QRectF enemyPanel(width() - outerMargin - panelWidth, hudY, panelWidth, panelHeight);
    const QRectF scorePanel((width() - centerWidth) / 2.0, hudY - 2.0, centerWidth, panelHeight + 8.0);

    auto drawPanel = [&](const QRectF &panelRect,
                         bool leftAligned,
                         const QString &title,
                         const QString &subtitle,
                         int hp,
                         int maxHp,
                         double displayRatio,
                         const QColor &accentStart,
                         const QColor &accentEnd) {
        QPainterPath panelPath;
        panelPath.addRoundedRect(panelRect, 18, 18);

        painter.save();
        painter.setPen(Qt::NoPen);
        painter.fillPath(panelPath.translated(0, 4), QColor(0, 0, 0, 55));
        painter.fillPath(panelPath, QColor(34, 20, 13, 225));
        painter.setPen(QPen(QColor("#D4AF37"), 2));
        painter.drawPath(panelPath);

        QRectF headerStrip = panelRect.adjusted(12, 10, -12, -42);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 16));
        painter.drawRoundedRect(headerStrip, 10, 10);

        QFont titleFont("Showcard Gothic", 10);
        if (titleFont.family() != "Showcard Gothic") {
            titleFont = painter.font();
            titleFont.setPointSize(11);
            titleFont.setBold(true);
        }
        painter.setFont(titleFont);
        painter.setPen(QColor("#F7D774"));
        painter.drawText(panelRect.adjusted(16, 8, -16, 0),
                         leftAligned ? Qt::AlignLeft | Qt::AlignTop : Qt::AlignRight | Qt::AlignTop,
                         title);

        QFont subFont = painter.font();
        subFont.setFamily("Segoe UI");
        subFont.setPointSize(8);
        subFont.setBold(false);
        painter.setFont(subFont);
        painter.setPen(QColor("#C6A66A"));
        painter.drawText(panelRect.adjusted(16, 30, -16, 0),
                         leftAligned ? Qt::AlignLeft | Qt::AlignTop : Qt::AlignRight | Qt::AlignTop,
                         subtitle);

        const QRectF barRect(panelRect.x() + 16.0, panelRect.bottom() - 28.0, panelRect.width() - 32.0, 16.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(67, 44, 29, 235));
        painter.drawRoundedRect(barRect, 8, 8);

        const double clampedRatio = qBound(0.0, displayRatio, 1.0);
        QRectF fillRect = barRect.adjusted(2.0, 2.0, -2.0, -2.0);
        fillRect.setWidth(qMax(0.0, (barRect.width() - 4.0) * clampedRatio));
        if (!leftAligned) {
            fillRect.moveRight(barRect.right() - 2.0);
        }

        QLinearGradient fillGradient(fillRect.topLeft(), fillRect.topRight());
        fillGradient.setColorAt(0.0, accentStart);
        fillGradient.setColorAt(1.0, accentEnd);
        painter.setBrush(fillGradient);
        painter.drawRoundedRect(fillRect, 6, 6);

        painter.setPen(QPen(QColor(255, 255, 255, 28), 1));
        for (int i = 1; i < 8; ++i) {
            const qreal tickX = barRect.left() + (barRect.width() * i / 8.0);
            painter.drawLine(QPointF(tickX, barRect.top() + 2.0), QPointF(tickX, barRect.bottom() - 2.0));
        }

        QFont valueFont("Segoe UI", 8, QFont::Bold);
        painter.setFont(valueFont);
        painter.setPen(QColor("#FFF3C4"));
        painter.drawText(barRect, Qt::AlignCenter, QString("%1 / %2").arg(hp).arg(maxHp));
        painter.restore();
    };

    drawPanel(playerPanel,
              true,
              QString::fromStdString(player->getName()).toUpper(),
              healCooldown_ > 0.0 ? QString("Heal ready in %1s").arg(QString::number(healCooldown_, 'f', 1))
                                  : QString("Heal ready"),
              player->getHealth(),
              player->getMaxHealth(),
              playerHpDisplay_,
              QColor("#7CFF63"),
              QColor("#22C55E"));

    drawPanel(enemyPanel,
              false,
              QString::fromStdString(enemy->getName()).toUpper(),
              QString("Hostile target"),
              enemy->getHealth(),
              enemy->getMaxHealth(),
              enemyHpDisplay_,
              QColor("#FF9A56"),
              QColor("#DC2626"));

    QPainterPath scorePath;
    scorePath.addRoundedRect(scorePanel, 22, 22);
    painter.fillPath(scorePath.translated(0, 4), QColor(0, 0, 0, 60));
    painter.fillPath(scorePath, QColor(24, 13, 9, 232));
    painter.setPen(QPen(QColor("#D4AF37"), 2));
    painter.drawPath(scorePath);

    painter.setPen(QColor("#C6A66A"));
    QFont labelFont("Segoe UI", 8, QFont::DemiBold);
    painter.setFont(labelFont);
    painter.drawText(scorePanel.adjusted(0, 8, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "ARENA SCORE");

    QFont scoreFont("Showcard Gothic", 24);
    if (scoreFont.family() != "Showcard Gothic") {
        scoreFont = painter.font();
        scoreFont.setPointSize(24);
        scoreFont.setBold(true);
    }
    painter.setFont(scoreFont);
    painter.setPen(QColor("#FFD700"));
    painter.drawText(scorePanel.adjusted(0, 18, 0, -6), Qt::AlignCenter, QString::number(score_));

    QFont levelFont("Segoe UI", 8, QFont::Bold);
    painter.setFont(levelFont);
    painter.setPen(QColor("#F3D38C"));
    painter.drawText(scorePanel.adjusted(0, 0, 0, 8), Qt::AlignHCenter | Qt::AlignBottom,
                     QString("STAGE %1 / %2").arg(gameManager_->getCurrentLevel()).arg(gameManager_->getTotalLevels()));
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
                                            const QString &name, AnimatedCharacter *animChar, bool isPlayer) {
    if (!animChar) return;

    const double arenaHeight = height() - 160.0;
    double desiredVisibleHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
    bool flipSprite = false;

    if (isPlayer) {
        flipSprite = playerAnimChar_->isFacingLeft();
    } else {
        const Enemy *enemy = gameManager_ ? gameManager_->getCurrentEnemy() : nullptr;
        const EnemyType enemyType = enemy ? enemy->getEnemyType() : EnemyType::FIRE_WORM;
        if (enemyType == EnemyType::EVIL_WIZARD) {
            desiredVisibleHeight *= 1.2;
        }

        flipSprite = enemyAnimChar_->isFacingLeft();
        if (enemyType == EnemyType::FLYING_DEMON) {
            flipSprite = !flipSprite;
        }
    }

    const QPixmap frame = animChar->getCurrentFrame();
    if (frame.isNull()) {
        // Fallback drawing if animation fails
        drawFighter(painter, x, y, hp, maxHp, name, isPlayer, false);
        return;
    }

    AnimationManager *animManager = isPlayer ? playerAnimManager_ : enemyAnimManager_;
    QPixmap referenceFrame;
    if (animManager) {
        referenceFrame = animManager->getFrame(AnimationState::IDLE, 0);
    }

    if (referenceFrame.isNull()) {
        referenceFrame = frame;
    }

    const QRect visibleBounds = opaqueBounds(frame);
    const QRect referenceBounds = opaqueBounds(referenceFrame);
    const QRect src(0, 0, frame.width(), frame.height());
    const double referenceHeight = qMax(1, referenceBounds.height());
    const double scale = desiredVisibleHeight / referenceHeight;
    const double scaledWidth = src.width() * scale;
    const double scaledHeight = src.height() * scale;

    const double visibleCenterX = visibleBounds.x() + (visibleBounds.width() / 2.0);
    const double visibleBottomY = visibleBounds.y() + visibleBounds.height();
    const double drawX = x - (visibleCenterX * scale);
    const double drawY = y - (visibleBottomY * scale);

    QRectF destRect(drawX, drawY, scaledWidth, scaledHeight);

    // Flip horizontally if needed
    if (flipSprite) {
        painter.save();
        painter.translate(destRect.center().x(), 0);
        painter.scale(-1, 1);
        painter.translate(-destRect.center().x(), 0);
        painter.drawPixmap(destRect.toRect(), frame, src);
        painter.restore();
    } else {
        painter.drawPixmap(destRect.toRect(), frame, src);
    }

    // Name label
    painter.setPen(QColor("#D4AF37"));
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(x - 50, y + 5, 100, 20), Qt::AlignCenter, name);
}
