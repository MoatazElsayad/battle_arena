#include "BattleWidget.h"
#include "GameManager.h"
#include "LanSessionManager.h"
#include "Player.h"
#include "Enemy.h"
#include "SoundManager.h"
#include "AnimatedCharacter.h"
#include "AnimationManager.h"
#include "InputHandler.h"
#include <QBuffer>
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
#include <QHash>
#include <QImage>
#include <QTransform>
#include <array>

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

QString playerProfilePath(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN:
            return QStringLiteral("assets/players/Arcen/profile.png");
        case PlayerType::DEMON_SLAYER:
            return QStringLiteral("assets/players/Demon_Slayer/profile.png");
        case PlayerType::FANTASY_WARRIOR:
            return QStringLiteral("assets/players/Fantasy_Warrior/profile.png");
        case PlayerType::HUNTRESS:
            return QStringLiteral("assets/players/Huntress/profile.png");
        case PlayerType::KNIGHT:
            return QStringLiteral("assets/players/Knight/profile.png");
        case PlayerType::MARTIAL:
            return QStringLiteral("assets/players/Martial/profile.png");
        case PlayerType::MARTIAL_HERO:
            return QStringLiteral("assets/players/Martial_Hero/profile.png");
        case PlayerType::MEDIEVAL_WARRIOR:
            return QStringLiteral("assets/players/Medieval_Warrior/profile.png");
        case PlayerType::WIZARD:
            return QStringLiteral("assets/players/Wizard/profile.png");
    }
    return QString();
}

int arcenArrowDamage(int baseDamage, AnimationState attackState) {
    double multiplier = 0.85;
    if (attackState == AnimationState::ATTACK2) {
        multiplier = 1.00;
    } else if (attackState == AnimationState::ATTACK3) {
        multiplier = 1.15;
    }
    return qMax(1, static_cast<int>(baseDamage * multiplier));
}

QString enemyProfilePath(EnemyType type) {
    switch (type) {
        case EnemyType::FIRE_WORM:
            return QStringLiteral("assets/enemies/Fire_Worm/profile.png");
        case EnemyType::FIRE_WIZARD:
            return QStringLiteral("assets/enemies/Fire_Wizard/profile.png");
        case EnemyType::FLYING_DEMON:
            return QStringLiteral("assets/enemies/Flying_Demon/profile.png");
        case EnemyType::NIGHTWEAVER:
            return QStringLiteral("assets/enemies/Nightweaver/profile.png");
        case EnemyType::EVIL_WIZARD:
            return QStringLiteral("assets/enemies/Evil_Wizard/profile.png");
        case EnemyType::BLACK_WEREWOLF:
        case EnemyType::RED_WEREWOLF:
        case EnemyType::WHITE_WEREWOLF:
            return QStringLiteral("assets/beasts/werewolf/profile.png");
        case EnemyType::ZOMBIE_1:
        case EnemyType::ZOMBIE_2:
        case EnemyType::ZOMBIE_3:
        case EnemyType::ZOMBIE_4:
        case EnemyType::ADVANCED_ZOMBIE_1:
        case EnemyType::ADVANCED_ZOMBIE_2:
        case EnemyType::ADVANCED_ZOMBIE_3:
            return QStringLiteral("assets/beasts/zombies/Zombie_1/Idle.png");
    }
    return QString();
}

QPixmap loadPortraitPixmap(const QStringList& relativePaths) {
    for (const QString& relativePath : relativePaths) {
        const QString resolved = resolveAssetPath(relativePath);
        if (resolved.isEmpty()) {
            continue;
        }

        const QPixmap portrait(resolved);
        if (!portrait.isNull()) {
            return portrait;
        }
    }

    return QPixmap();
}

void drawPortraitBadge(QPainter& painter,
                       const QRectF& rect,
                       const QPixmap& portrait,
                       const QColor& rimColor,
                       const QString& fallbackLabel) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 40));
    painter.drawEllipse(rect.adjusted(-8.0, -4.0, 8.0, 10.0));

    const QRectF shadowRect = rect.adjusted(-2.0, 4.0, 2.0, 8.0);
    painter.setBrush(QColor(0, 0, 0, 78));
    painter.drawEllipse(shadowRect);

    const QRectF outerRing = rect.adjusted(-3.0, -3.0, 3.0, 3.0);
    painter.setBrush(QColor(32, 18, 11, 235));
    painter.setPen(QPen(rimColor, 3.1));
    painter.drawEllipse(outerRing);

    QPainterPath clipPath;
    clipPath.addEllipse(rect);
    painter.setClipPath(clipPath);

    if (!portrait.isNull()) {
        const QPixmap scaled = portrait.scaled(rect.size().toSize(),
                                               Qt::KeepAspectRatioByExpanding,
                                               Qt::SmoothTransformation);
        const QRectF sourceRect((scaled.width() - rect.width()) * 0.5,
                                (scaled.height() - rect.height()) * 0.5,
                                rect.width(),
                                rect.height());
        painter.drawPixmap(rect, scaled, sourceRect);
    } else {
        QLinearGradient fallbackGradient(rect.topLeft(), rect.bottomRight());
        fallbackGradient.setColorAt(0.0, QColor("#4B2D1A"));
        fallbackGradient.setColorAt(1.0, QColor("#8C5E2D"));
        painter.setBrush(fallbackGradient);
        painter.drawEllipse(rect);

        painter.setClipping(false);
        painter.setPen(QColor("#FFF4D2"));
        QFont fallbackFont("Segoe UI", qMax(10, static_cast<int>(rect.height() * 0.35)), QFont::Black);
        painter.setFont(fallbackFont);
        painter.drawText(rect.toRect(), Qt::AlignCenter, fallbackLabel.left(2).toUpper());
        painter.restore();
        return;
    }

    painter.setClipping(false);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 255, 255, 84), 1.6));
    painter.drawEllipse(rect.adjusted(1.0, 1.0, -1.0, -1.0));

    painter.restore();
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

QPixmap loadExhibitionArenaBackground(const QString& arenaName) {
    const QString lowered = arenaName.trimmed().toLower();

    QString relativePath = QStringLiteral("assets/backgrounds/Level_1.png");
    if (lowered == QStringLiteral("ember court")) {
        relativePath = QStringLiteral("assets/backgrounds/Level_2.png");
    } else if (lowered == QStringLiteral("forest temple")) {
        relativePath = QStringLiteral("assets/backgrounds/Level_3.png");
    } else if (lowered == QStringLiteral("night fortress")) {
        relativePath = QStringLiteral("assets/backgrounds/Level_4.png");
    } else if (lowered == QStringLiteral("lava pit")) {
        relativePath = QStringLiteral("assets/backgrounds/Level_5.png");
    } else if (lowered == QStringLiteral("sky ruins")) {
        relativePath = QStringLiteral("assets/backgrounds/Level_6.png");
    }

    const QString resolved = resolveAssetPath(relativePath);
    if (resolved.isEmpty()) {
        return QPixmap();
    }

    return QPixmap(resolved);
}

QPixmap firstFrameFromSheet(const QString& relativePath, int frameCount) {
    static QHash<QString, QPixmap> cache;
    const QString key = relativePath + "|" + QString::number(frameCount);
    if (cache.contains(key)) {
        return cache.value(key);
    }

    const QString resolved = resolveAssetPath(relativePath);
    if (resolved.isEmpty()) {
        return QPixmap();
    }

    const QPixmap sheet(resolved);
    if (sheet.isNull() || frameCount <= 0) {
        return QPixmap();
    }

    const int frameWidth = qMax(1, sheet.width() / frameCount);
    const QPixmap frame = sheet.copy(0, 0, frameWidth, sheet.height());
    cache.insert(key, frame);
    return frame;
}

QPixmap frameFromSheet(const QString& relativePath, int frameCount, double seconds, int frameSpeedMs) {
    static QHash<QString, QPixmap> sheetCache;
    static QHash<QString, QPixmap> frameCache;
    const QString resolved = resolveAssetPath(relativePath);
    if (resolved.isEmpty() || frameCount <= 0) {
        return QPixmap();
    }

    if (!sheetCache.contains(resolved)) {
        sheetCache.insert(resolved, QPixmap(resolved));
    }

    const QPixmap sheet = sheetCache.value(resolved);
    if (sheet.isNull()) {
        return QPixmap();
    }

    const int frameWidth = qMax(1, sheet.width() / frameCount);
    const int frameIndex = qBound(0,
                                  static_cast<int>((seconds * 1000.0) / qMax(1, frameSpeedMs)) % frameCount,
                                  frameCount - 1);
    const QString frameKey = resolved + "|" + QString::number(frameCount) + "|" + QString::number(frameIndex);
    if (frameCache.contains(frameKey)) {
        return frameCache.value(frameKey);
    }

    const QPixmap frame = sheet.copy(frameIndex * frameWidth, 0, frameWidth, sheet.height());
    frameCache.insert(frameKey, frame);
    return frame;
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

    static QHash<quint64, QRect> cache;
    const quint64 key = px.cacheKey();
    if (key != 0) {
        const auto it = cache.constFind(key);
        if (it != cache.cend()) {
            return it.value();
        }
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
        const QRect fullBounds(0, 0, w, h);
        if (key != 0) {
            cache.insert(key, fullBounds);
        }
        return fullBounds;
    }

    const QRect bounds(minX, minY, maxX - minX + 1, maxY - minY + 1);
    if (key != 0) {
        cache.insert(key, bounds);
    }
    return bounds;
}

QString playerTypeLabel(PlayerType type) {
    return QString::fromStdString(InputHandler::playerTypeToDisplayName(type));
}

QString enemyTypeLabel(EnemyType type) {
    switch (type) {
        case EnemyType::FIRE_WORM: return "Fire Worm";
        case EnemyType::FIRE_WIZARD: return "Fire Wizard";
        case EnemyType::FLYING_DEMON: return "Flying Demon";
        case EnemyType::NIGHTWEAVER: return "Nightweaver";
        case EnemyType::EVIL_WIZARD: return "Evil Wizard";
        case EnemyType::BLACK_WEREWOLF: return "Black Werewolf";
        case EnemyType::RED_WEREWOLF: return "Red Werewolf";
        case EnemyType::WHITE_WEREWOLF: return "White Werewolf";
        case EnemyType::ZOMBIE_1: return "Zombie 1";
        case EnemyType::ZOMBIE_2: return "Zombie 2";
        case EnemyType::ZOMBIE_3: return "Zombie 3";
        case EnemyType::ZOMBIE_4: return "Zombie 4";
        case EnemyType::ADVANCED_ZOMBIE_1: return "Advanced Zombie 1";
        case EnemyType::ADVANCED_ZOMBIE_2: return "Advanced Zombie 2";
        case EnemyType::ADVANCED_ZOMBIE_3: return "Advanced Zombie 3";
    }
    return "Enemy";
}

HighlightAttackType highlightAttackTypeForAnimation(AnimationState state) {
    switch (state) {
        case AnimationState::ATTACK2:
            return HighlightAttackType::Attack2;
        case AnimationState::ATTACK3:
            return HighlightAttackType::Attack3;
        case AnimationState::ATTACK1:
        default:
            return HighlightAttackType::Attack1;
    }
}

void drawGroundedSprite(QPainter& painter,
                        const QPixmap& sprite,
                        qreal centerX,
                        qreal groundY,
                        qreal targetHeight,
                        bool flip = false,
                        qreal opacity = 1.0) {
    if (sprite.isNull() || targetHeight <= 0.0) {
        return;
    }

    const QRect visibleBounds = opaqueBounds(sprite);
    const qreal scale = targetHeight / qMax(1, visibleBounds.height());
    const qreal scaledWidth = sprite.width() * scale;
    const qreal scaledHeight = sprite.height() * scale;
    const qreal visibleCenterX = visibleBounds.x() + visibleBounds.width() * 0.5;
    const qreal visibleBottomY = visibleBounds.y() + visibleBounds.height();
    const QRectF destRect(centerX - visibleCenterX * scale,
                          groundY - visibleBottomY * scale,
                          scaledWidth,
                          scaledHeight);

    painter.save();
    painter.setOpacity(opacity);
    if (flip) {
        painter.translate(destRect.center().x(), 0.0);
        painter.scale(-1.0, 1.0);
        painter.translate(-destRect.center().x(), 0.0);
    }
    painter.drawPixmap(destRect.toRect(), sprite, QRect(0, 0, sprite.width(), sprite.height()));
    painter.restore();
}
}

BattleWidget::BattleWidget(QWidget *parent)
    : QWidget(parent),
      gameManager_(nullptr),
      lanSessionManager_(nullptr),
      soundManager_(nullptr),
      playerAnimChar_(new AnimatedCharacter(this)),
      enemyAnimChar_(new AnimatedCharacter(this)),
    playerAnimManager_(new AnimationManager()),
      enemyAnimManager_(new AnimationManager()),
      movingLeft_(false),
      movingRight_(false),
      keyboardMovingLeft_(false),
      keyboardMovingRight_(false),
      controllerMovingLeft_(false),
      controllerMovingRight_(false),
      attackPressed_(false),
      healPressed_(false),
      pausePressed_(false),
      battleActive_(false),
      playerCooldown_(0.0),
      enemyCooldown_(0.0),
      healCooldown_(0.0),
      battleEndDelay_(0.0),
      introLockTime_(0.0),
      levelTransitionTime_(0.0),
      levelTransitionStartPlayerX_(0.0),
      levelTransitionStartKingX_(0.0),
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
    arcenProjectileAttackType_(HighlightAttackType::Attack1),
    enemyProjectileActive_(false),
    enemyProjectileExploding_(false),
    enemyProjectileUsesArcenArrow_(false),
    enemyProjectileFacingRight_(false),
    enemyProjectileType_(EnemyType::FIRE_WORM),
    enemyProjectileDamage_(0),
    enemyProjectileX_(0.0),
    enemyProjectileY_(0.0),
    enemyProjectileSpeed_(720.0),
    enemyProjectileAnimTime_(0.0),
    enemyProjectileFrame_(0),
    enemyProjectileExplosionTime_(0.0),
      statusMessage_("Keyboard: A/D move, J/K/L attack, H heal. Controller: left stick move, Square/Triangle/Circle attack, R1 heal."),
      statusDisplayTime_(0.0),
      queuedPlayerAttackState_(AnimationState::ATTACK1),
      levelTransitionActive_(false),
      finalRescueTransitionActive_(false),
      playerHpDisplay_(1.0),
      enemyHpDisplay_(1.0),
      arenaGroundBaseColor_(QColor("#5A3A23")),
      score_(0),
      duelRemoteScore_(0),
      lanBridgeActive_(false),
      lanHostAuthority_(false),
      lanGuestStateSeen_(false),
      lanResolvedWinner_(LanCombatWinner::NONE),
      lastPredictedLocalLanInputBits_(0),
      lastSentLanInputBits_(0),
      lastRemoteLanInputBits_(0),
      lanStateTick_(0),
      enemyHealCooldown_(0.0),
      enemyAiDecision_(),
      enemyAiDecisionTimer_(0.0),
      enemyAiPlayerAttackMemory_(0.0),
      enemyAiPlayerMissMemory_(0.0),
      enemyAiPlayerHealMemory_(0.0),
      enemyAiEnemyDamageMemory_(0.0),
      enemyAiLastEnemyHp_(0),
      enemyAiLastAttack_(AnimationState::ATTACK1),
      zombieSpawnDelay_(0.0),
      zombieIntroTime_(0.0),
      zombieIntroActive_(false),
      zombieEnteringFromLeft_(false),
      zombieCityCleaned_(false),
      zombieLevelTransitionPending_(false),
      highlightFrameAccumulator_(0.0),
      highlightPostFramesRemaining_(0) {
    // Sound teammate:
    // Most combat SFX will be triggered from this class.

    arenaBackgroundPlaceholder_ = loadArenaBackgroundForLevel(1);
    arenaGroundBaseColor_ = sampleGroundColorFromBackground(arenaBackgroundPlaceholder_);
    levelTransitionPortalSpriteSheet_ =
        QPixmap(resolveAssetPath("assets/objects/magic_portal/Sprites/Isometric_Portal.png"));
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
    if (gameManager_ && gameManager_->isZombieMode()) {
        QString backgroundPath;
        if (zombieIntroActive_) {
            backgroundPath = QStringLiteral("assets/backgrounds/intro.png");
        } else if (zombieCityCleaned_) {
            backgroundPath = QStringLiteral("assets/backgrounds/city.png");
        } else {
            backgroundPath = gameManager_->getCurrentLevel() >= 2
                ? QStringLiteral("assets/backgrounds/zombie_lvl2.png")
                : QStringLiteral("assets/backgrounds/zombie_lvl1.png");
        }

        const QString resolved = resolveAssetPath(backgroundPath);
        if (!resolved.isEmpty()) {
            const QPixmap zombieBackdrop(resolved);
            if (!zombieBackdrop.isNull()) {
                arenaBackgroundPlaceholder_ = zombieBackdrop;
                arenaGroundBaseColor_ = sampleGroundColorFromBackground(arenaBackgroundPlaceholder_);
                return;
            }
        }
    }

    if (gameManager_ && gameManager_->isLanDuel()) {
        const QString duelBackdropPath = resolveAssetPath("assets/backgrounds/city.png");
        if (!duelBackdropPath.isEmpty()) {
            const QPixmap duelBackdrop(duelBackdropPath);
            if (!duelBackdrop.isNull()) {
                arenaBackgroundPlaceholder_ = duelBackdrop;
                arenaGroundBaseColor_ = sampleGroundColorFromBackground(arenaBackgroundPlaceholder_);
                return;
            }
        }
    }

    if (gameManager_ && gameManager_->isDuelMode()) {
        const QPixmap duelBackdrop = loadExhibitionArenaBackground(gameManager_->getDuelConfig().selectedArena);
        if (!duelBackdrop.isNull()) {
            arenaBackgroundPlaceholder_ = duelBackdrop;
            arenaGroundBaseColor_ = sampleGroundColorFromBackground(arenaBackgroundPlaceholder_);
            return;
        }
    }

    const int level = gameManager_ ? qMax(1, gameManager_->getCurrentLevel()) : 1;
    arenaBackgroundPlaceholder_ = loadArenaBackgroundForLevel(level);
    arenaGroundBaseColor_ = sampleGroundColorFromBackground(arenaBackgroundPlaceholder_);
}

void BattleWidget::loadBattleProfilePortraits() {
    playerProfilePortrait_ = QPixmap();
    enemyProfilePortrait_ = QPixmap();

    if (!gameManager_) {
        return;
    }

    const Player *player = gameManager_->getPlayer();
    const Enemy *enemy = gameManager_->getCurrentEnemy();

    if (player) {
        const QString playerProfile = playerProfilePath(player->getPlayerType());
        const QString playerImage = playerProfile.isEmpty()
            ? QString()
            : QString(playerProfile).replace(QStringLiteral("profile.png"), QStringLiteral("image.png"));
        playerProfilePortrait_ = loadPortraitPixmap({
            playerProfile,
            playerImage,
            QStringLiteral("assets/story/warrior/image.png")
        });
    }

    if (gameManager_->isLanDuel()) {
        const QString rivalProfile = playerProfilePath(gameManager_->getLanOpponentPlayerType());
        const QString rivalImage = rivalProfile.isEmpty()
            ? QString()
            : QString(rivalProfile).replace(QStringLiteral("profile.png"), QStringLiteral("image.png"));
        enemyProfilePortrait_ = loadPortraitPixmap({
            rivalProfile,
            rivalImage,
            QStringLiteral("assets/story/warrior/image.png")
        });
        return;
    }

    if (gameManager_->isDuelMode()
        && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE) {
        const QString rivalProfile = playerProfilePath(gameManager_->getLanOpponentPlayerType());
        const QString rivalImage = rivalProfile.isEmpty()
            ? QString()
            : QString(rivalProfile).replace(QStringLiteral("profile.png"), QStringLiteral("image.png"));
        enemyProfilePortrait_ = loadPortraitPixmap({
            rivalProfile,
            rivalImage,
            QStringLiteral("assets/story/warrior/image.png")
        });
        return;
    }

    if (enemy) {
        const QString enemyProfile = enemyProfilePath(enemy->getEnemyType());
        const QString enemyImage = enemyProfile.isEmpty()
            ? QString()
            : QString(enemyProfile).replace(QStringLiteral("profile.png"), QStringLiteral("image.png"));
        enemyProfilePortrait_ = loadPortraitPixmap({
            enemyProfile,
            enemyImage,
            QStringLiteral("assets/beasts/werewolf/profile.png"),
            QStringLiteral("assets/beasts/werewolf/image.png")
        });
    }
}

void BattleWidget::ensureArcenArrowAssetsLoaded() {
    if (!arcenArrowSprite_.isNull() || !arcenArrowMoveSprite_.isNull()) {
        return;
    }

    arcenArrowSprite_ = QPixmap(resolveAssetPath("assets/players/Arcen/Sprites/Arrow/Static.png"));
    arcenArrowMoveSprite_ = QPixmap(resolveAssetPath("assets/players/Arcen/Sprites/Arrow/Move.png"));
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

void BattleWidget::setLanSessionManager(LanSessionManager *manager) {
    lanSessionManager_ = manager;
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

    const bool zombieMode = gameManager_->isZombieMode();
    const bool playZombieIntro = zombieMode && gameManager_->getCurrentLevel() == 1;
    resetZombieModeState();
    zombieIntroActive_ = playZombieIntro;
    zombieIntroTime_ = playZombieIntro ? 10.0 : 0.0;
    refreshArenaBackground();
    loadPrototypeAnimations();
    loadBattleProfilePortraits();
    
    // Reset animation states
    playerAnimChar_->reset();
    enemyAnimChar_->reset();
    playerAnimChar_->setAnimationState(AnimationState::IDLE);
    enemyAnimChar_->setAnimationState(AnimationState::IDLE);
    
    battleActive_ = true;
    levelTransitionActive_ = false;
    finalRescueTransitionActive_ = false;
    battleEndDelay_ = 0.0;
    introLockTime_ = zombieMode
        ? (playZombieIntro ? 10.0 : 4.2)
        : BATTLE_COUNTDOWN_DURATION;
    levelTransitionTime_ = 0.0;
    levelTransitionStartPlayerX_ = 0.0;
    levelTransitionStartKingX_ = 0.0;
    clearLocalInputState();
    pausePressed_ = false;
    duelRemoteScore_ = 0;
    lanGuestStateSeen_ = false;
    lanResolvedWinner_ = LanCombatWinner::NONE;
    lastPredictedLocalLanInputBits_ = 0;
    lastSentLanInputBits_ = 0;
    lastRemoteLanInputBits_ = 0;
    lanStateTick_ = 0;
    playerCooldown_ = 0.0;
    enemyCooldown_ = introLockTime_ + 0.45;
    enemyHealCooldown_ = 0.0;
    enemyAiDecision_ = FighterAiDecision();
    enemyAiDecisionTimer_ = 0.0;
    enemyAiPlayerAttackMemory_ = 0.0;
    enemyAiPlayerMissMemory_ = 0.0;
    enemyAiPlayerHealMemory_ = 0.0;
    enemyAiEnemyDamageMemory_ = 0.0;
    enemyAiLastEnemyHp_ = 0;
    enemyAiLastAttack_ = AnimationState::ATTACK1;
    const double arenaLeft = ARENA_LEFT_X + 70.0;
    const double arenaRight = qMax(arenaLeft + 220.0, double(width()) - ARENA_RIGHT_MARGIN - 70.0);
    playerX_ = arenaLeft;
    enemyX_ = arenaRight;
    score_ = 0;
    lanBridgeActive_ = gameManager_->isLanDuel() && lanSessionManager_;
    lanHostAuthority_ = lanBridgeActive_ && lanSessionManager_->snapshot().localRole == LanRole::HOST;
    if (gameManager_->isLanDuel() && (!lanSessionManager_ || lanSessionManager_->snapshot().localRole == LanRole::NONE)) {
        battleActive_ = false;
        lanBridgeActive_ = false;
        lanHostAuthority_ = false;
        statusMessage_ = QStringLiteral("LAN bridge is not attached. Return to Arena Link and re-prime the duel.");
        statusDisplayTime_ = 5.0;
        update();
        return;
    }
    if (lanBridgeActive_) {
        lanSessionManager_->beginCombatBridge();
    }
    if (playerAnimChar_) {
        playerAnimChar_->setFacingLeft(enemyX_ < playerX_);
    }
    if (enemyAnimChar_) {
        enemyAnimChar_->setFacingLeft(playerX_ < enemyX_);
    }
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    if (enemy) {
        enemyAiLastEnemyHp_ = enemy->getHealth();
        if (zombieMode) {
            statusMessage_ = playZombieIntro
                ? QStringLiteral("The city is full of zombies. Watch the streets...")
                : QString("Zombie Outbreak %1/2 - infected entering from the city edge.")
                      .arg(gameManager_->getCurrentLevel());
        } else if (gameManager_->isLanDuel()) {
            statusMessage_ = QString("Arena Link Duel - %1 enters the arena!")
                                 .arg(QString::fromStdString(enemy->getName()));
        } else if (gameManager_->isDuelMode()) {
            statusMessage_ = QString("1v1 Exhibition - %1 enters the arena!")
                                 .arg(QString::fromStdString(enemy->getName()));
        } else {
            statusMessage_ = QString("Stage %1/%2 - %3 enters the arena!")
                                 .arg(gameManager_->getCurrentLevel())
                                 .arg(gameManager_->getTotalLevels())
                                 .arg(QString::fromStdString(enemy->getName()));
        }
    } else {
        statusMessage_ = "Battle started!";
    }
    statusDisplayTime_ = 2.0;
    enemyProjectileActive_ = false;
    enemyProjectileExploding_ = false;
    enemyProjectileUsesArcenArrow_ = false;
    enemyProjectileAnimTime_ = 0.0;
    enemyProjectileFrame_ = 0;
    enemyProjectileExplosionTime_ = 0.0;
    arcenProjectileAttackType_ = HighlightAttackType::Attack1;
    if (zombieMode && !playZombieIntro) {
        prepareZombieSpawn(true);
    }

    const Player *player = gameManager_->getPlayer();
    levelBattleReport_ = ChronicleBattleReport();
    if (player) {
        levelBattleReport_.playerName = QString::fromStdString(player->getName());
        levelBattleReport_.playerType = playerTypeLabel(player->getPlayerType());
        levelBattleReport_.playerHp = player->getHealth();
        levelBattleReport_.playerMaxHp = player->getMaxHealth();
    }
    if (enemy) {
        levelBattleReport_.defeatedEnemyName = QString::fromStdString(enemy->getName());
        const bool duelPlayerRival = gameManager_->isLanDuel()
            || (gameManager_->isDuelMode()
                && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE);
        levelBattleReport_.defeatedEnemyType = duelPlayerRival
            ? playerTypeLabel(gameManager_->getLanOpponentPlayerType())
            : enemyTypeLabel(enemy->getEnemyType());
        levelBattleReport_.enemyMaxHp = enemy->getMaxHealth();
    }
    levelBattleReport_.completedLevel = gameManager_->getCurrentLevel();
    levelBattleReport_.totalLevels = gameManager_->getTotalLevels();
    levelBattleReport_.currentScore = gameManager_->getCurrentScore();
    levelBattleReport_.victory = false;
    levelBattleReport_.campaignComplete = false;

    remoteBattleReport_ = ChronicleBattleReport();
    if (enemy) {
        remoteBattleReport_.playerName = QString::fromStdString(enemy->getName());
        const bool duelPlayerRival = gameManager_->isLanDuel()
            || (gameManager_->isDuelMode()
                && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE);
        remoteBattleReport_.playerType = duelPlayerRival
            ? playerTypeLabel(gameManager_->getLanOpponentPlayerType())
            : enemyTypeLabel(enemy->getEnemyType());
        remoteBattleReport_.playerHp = enemy->getHealth();
        remoteBattleReport_.playerMaxHp = enemy->getMaxHealth();
    }
    if (player) {
        remoteBattleReport_.defeatedEnemyName = QString::fromStdString(player->getName());
        remoteBattleReport_.defeatedEnemyType = playerTypeLabel(player->getPlayerType());
        remoteBattleReport_.enemyMaxHp = player->getMaxHealth();
    }
    remoteBattleReport_.completedLevel = gameManager_->getCurrentLevel();
    remoteBattleReport_.totalLevels = gameManager_->getTotalLevels();
    remoteBattleReport_.currentScore = 0;
    remoteBattleReport_.victory = false;
    remoteBattleReport_.campaignComplete = false;
    resetHighlightReplay();
    
    elapsedTimer_.start();
    levelClock_.start();
    frameTimer_.start();
}

void BattleWidget::startLevelTransition() {
    if (!gameManager_ || !gameManager_->getPlayer()) {
        return;
    }

    battleActive_ = false;
    levelTransitionActive_ = true;
    finalRescueTransitionActive_ = gameManager_->hasCompletedCampaign() && gameManager_->isFinalKingStage();
    levelTransitionTime_ = 0.0;
    levelTransitionStartPlayerX_ = playerX_;
    levelTransitionStartKingX_ = width() * 0.90;
    battleEndDelay_ = 0.0;
    introLockTime_ = 0.0;
    clearLocalInputState();
    arcenProjectileActive_ = false;
    enemyProjectileActive_ = false;
    enemyProjectileExploding_ = false;
    const QString playerName = QString::fromStdString(gameManager_->getPlayer()->getName());
    statusMessage_ = finalRescueTransitionActive_
        ? QString("%1: I saved you, King. Let's go back!").arg(playerName)
        : QStringLiteral("Stage cleared! Enter the portal!");
    statusDisplayTime_ = finalRescueTransitionActive_ ? 7.6 : LEVEL_TRANSITION_DURATION;

    if (playerAnimChar_) {
        playerAnimChar_->reset();
        playerAnimChar_->setFacingLeft(false);
        playerAnimChar_->setAnimationState(finalRescueTransitionActive_ ? AnimationState::IDLE : AnimationState::RUN);
    }
    if (enemyAnimChar_) {
        enemyAnimChar_->setAnimationState(AnimationState::DEATH);
    }

    elapsedTimer_.restart();
    frameTimer_.start();
    update();
}

void BattleWidget::stopBattle() {
    battleActive_ = false;
    levelTransitionActive_ = false;
    finalRescueTransitionActive_ = false;
    pausePressed_ = false;
    frameTimer_.stop();
    if (lanSessionManager_) {
        lanSessionManager_->endCombatBridge();
    }
}

void BattleWidget::pauseBattle() {
    if (pausePressed_) {
        return;
    }

    if (!battleActive_ && !levelTransitionActive_) {
        return;
    }

    pausePressed_ = true;
    clearLocalInputState();
    frameTimer_.stop();
    update();
}

void BattleWidget::resumeBattle() {
    if (!pausePressed_) {
        return;
    }

    pausePressed_ = false;
    if (battleActive_ || levelTransitionActive_) {
        elapsedTimer_.restart();
        frameTimer_.start();
    }
    setFocus(Qt::OtherFocusReason);
    update();
}

bool BattleWidget::isBattleRunning() const {
    return battleActive_ || levelTransitionActive_;
}

bool BattleWidget::isPaused() const {
    return pausePressed_;
}

quint8 BattleWidget::currentLanInputBits() const {
    quint8 inputBits = 0;
    if (movingLeft_) {
        inputBits |= LanInputMoveLeft;
    }
    if (movingRight_) {
        inputBits |= LanInputMoveRight;
    }
    if (attackPressed_) {
        switch (queuedPlayerAttackState_) {
            case AnimationState::ATTACK2:
                inputBits |= LanInputAttack2;
                break;
            case AnimationState::ATTACK3:
                inputBits |= LanInputAttack3;
                break;
            case AnimationState::ATTACK1:
            default:
                inputBits |= LanInputAttack1;
                break;
        }
    }
    if (healPressed_) {
        inputBits |= LanInputHeal;
    }
    if (pausePressed_) {
        inputBits |= LanInputPause;
    }
    return inputBits;
}

void BattleWidget::syncHealthToTarget(Character* character, int targetHp) const {
    if (!character) {
        return;
    }

    const int clampedHp = qBound(0, targetHp, character->getMaxHealth());
    const int delta = character->getHealth() - clampedHp;
    if (delta != 0) {
        character->takeDamage(delta);
    }
}

double BattleWidget::mirrorArenaX(double x) const {
    return width() - x;
}

bool BattleWidget::isLocalLanWinner(LanCombatWinner winner) const {
    if (winner == LanCombatWinner::NONE) {
        return false;
    }

    if (lanHostAuthority_) {
        return winner == LanCombatWinner::HOST;
    }

    if (lanSessionManager_ && lanSessionManager_->snapshot().localRole == LanRole::GUEST) {
        return winner == LanCombatWinner::GUEST;
    }

    return false;
}

void BattleWidget::applyCombatantStats(const LanCombatantStats& localStats,
                                       const LanCombatantStats& enemyStats,
                                       bool localVictory,
                                       double battleDurationSeconds,
                                       int currentScore) {
    levelBattleReport_.damageDealt = localStats.damageDealt;
    levelBattleReport_.damageTaken = localStats.damageTaken;
    levelBattleReport_.healsUsed = localStats.healsUsed;
    levelBattleReport_.playerAttacks = localStats.attacks;
    levelBattleReport_.playerHits = localStats.hits;
    levelBattleReport_.playerMisses = localStats.misses;
    levelBattleReport_.projectilesFired = localStats.projectilesFired;
    levelBattleReport_.projectilesHit = localStats.projectilesHit;
    levelBattleReport_.enemyHits = enemyStats.hits;
    levelBattleReport_.battleDurationSeconds = battleDurationSeconds;
    levelBattleReport_.currentScore = currentScore;
    levelBattleReport_.victory = localVictory;
}

void BattleWidget::applyGuestCombatState(const LanCombatState& state) {
    if (!gameManager_) {
        return;
    }

    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    if (!player || !enemy) {
        return;
    }

    const bool hadGuestState = lanGuestStateSeen_;
    lanGuestStateSeen_ = true;
    battleActive_ = state.battleActive;
    battleEndDelay_ = 0.0;
    introLockTime_ = state.introLockTime;
    const double guestTargetX = mirrorArenaX(state.guestX);
    const double hostTargetX = mirrorArenaX(state.hostX);
    if (!hadGuestState || std::abs(playerX_ - guestTargetX) > 140.0 || std::abs(enemyX_ - hostTargetX) > 140.0) {
        playerX_ = guestTargetX;
        enemyX_ = hostTargetX;
    } else {
        const bool localMovementActive = movingLeft_ != movingRight_;
        const double localBlend = localMovementActive ? 0.16 : 0.38;
        playerX_ += (guestTargetX - playerX_) * localBlend;
        enemyX_ += (hostTargetX - enemyX_) * 0.42;
    }
    score_ = state.guestStats.score;
    duelRemoteScore_ = state.hostStats.score;
    playerCooldown_ = state.guestAttackCooldown;
    enemyCooldown_ = state.hostAttackCooldown;
    healCooldown_ = state.guestHealCooldown;
    enemyHealCooldown_ = state.hostHealCooldown;

    syncHealthToTarget(player, state.guestHp);
    syncHealthToTarget(enemy, state.hostHp);

    if (playerAnimChar_) {
        playerAnimChar_->setFacingLeft(!state.guestFacingLeft);
        const bool forceAuthoritativeLocalAnim =
            (!movingLeft_ && !movingRight_ && !attackPressed_ && !healPressed_) ||
            state.guestAnimation == AnimationState::HURT ||
            state.guestAnimation == AnimationState::DEATH;
        if (forceAuthoritativeLocalAnim) {
            playerAnimChar_->setAnimationState(state.guestAnimation);
        }
    }
    if (enemyAnimChar_) {
        enemyAnimChar_->setFacingLeft(!state.hostFacingLeft);
        enemyAnimChar_->setAnimationState(state.hostAnimation);
    }

    const bool localIsArcen = gameManager_->getSelectedPlayerType() == PlayerType::ARCEN;
    arcenProjectileActive_ = localIsArcen && state.guestProjectileActive;
    arcenProjectileFacingRight_ = !state.guestProjectileFacingRight;
    const double guestProjectileTargetX = mirrorArenaX(state.guestProjectileX);
    if (!hadGuestState || !arcenProjectileActive_) {
        arcenProjectileX_ = guestProjectileTargetX;
        arcenProjectileY_ = state.guestProjectileY;
    } else {
        arcenProjectileX_ += (guestProjectileTargetX - arcenProjectileX_) * 0.32;
        arcenProjectileY_ += (state.guestProjectileY - arcenProjectileY_) * 0.32;
    }
    arcenProjectileFrame_ = state.guestProjectileFrame;

    enemyProjectileActive_ = state.hostProjectileActive;
    enemyProjectileUsesArcenArrow_ = gameManager_->getLanOpponentPlayerType() == PlayerType::ARCEN;
    enemyProjectileExploding_ = false;
    enemyProjectileFacingRight_ = !state.hostProjectileFacingRight;
    enemyProjectileType_ = EnemyType::FIRE_WORM;
    const double hostProjectileTargetX = mirrorArenaX(state.hostProjectileX);
    if (!hadGuestState || !enemyProjectileActive_) {
        enemyProjectileX_ = hostProjectileTargetX;
        enemyProjectileY_ = state.hostProjectileY;
    } else {
        enemyProjectileX_ += (hostProjectileTargetX - enemyProjectileX_) * 0.44;
        enemyProjectileY_ += (state.hostProjectileY - enemyProjectileY_) * 0.44;
    }
    enemyProjectileFrame_ = state.hostProjectileFrame;

    if (!state.statusMessage.trimmed().isEmpty()) {
        statusMessage_ = state.statusMessage;
        statusDisplayTime_ = qMax(statusDisplayTime_, state.statusSeconds);
    } else {
        statusDisplayTime_ = qMax(0.0, statusDisplayTime_);
    }

    applyCombatantStats(state.guestStats,
                        state.hostStats,
                        state.winner == LanCombatWinner::GUEST,
                        state.battleDurationSeconds,
                        state.guestStats.score);

    if (state.winner != LanCombatWinner::NONE) {
        lanResolvedWinner_ = state.winner;
    }
}

void BattleWidget::pushHostCombatState(bool finished) {
    if (!lanBridgeActive_ || !lanHostAuthority_ || !lanSessionManager_ || !gameManager_) {
        return;
    }

    const Player *player = gameManager_->getPlayer();
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    if (!player || !enemy) {
        return;
    }

    LanCombatState state;
    state.tick = ++lanStateTick_;
    state.battleActive = !finished && battleActive_;
    state.matchFinished = finished;
    state.winner = finished
        ? (player->isAlive() ? LanCombatWinner::HOST : LanCombatWinner::GUEST)
        : (!player->isAlive() ? LanCombatWinner::GUEST
           : !enemy->isAlive() ? LanCombatWinner::HOST
                               : LanCombatWinner::NONE);
    state.battleDurationSeconds = levelClock_.isValid() ? levelClock_.elapsed() / 1000.0 : 0.0;
    state.introLockTime = introLockTime_;
    state.hostX = playerX_;
    state.guestX = enemyX_;
    state.hostHp = player->getHealth();
    state.guestHp = enemy->getHealth();
    state.hostFacingLeft = playerAnimChar_ ? playerAnimChar_->isFacingLeft() : (enemyX_ < playerX_);
    state.guestFacingLeft = enemyAnimChar_ ? enemyAnimChar_->isFacingLeft() : (playerX_ < enemyX_);
    state.hostAnimation = playerAnimChar_ ? playerAnimChar_->getCurrentState() : AnimationState::IDLE;
    state.guestAnimation = enemyAnimChar_ ? enemyAnimChar_->getCurrentState() : AnimationState::IDLE;
    state.hostAttackCooldown = playerCooldown_;
    state.guestAttackCooldown = enemyCooldown_;
    state.hostHealCooldown = healCooldown_;
    state.guestHealCooldown = enemyHealCooldown_;
    state.hostProjectileActive = arcenProjectileActive_;
    state.hostProjectileFacingRight = arcenProjectileFacingRight_;
    state.hostProjectileX = arcenProjectileX_;
    state.hostProjectileY = arcenProjectileY_;
    state.hostProjectileFrame = arcenProjectileFrame_;
    state.guestProjectileActive = enemyProjectileActive_;
    state.guestProjectileExploding = enemyProjectileExploding_;
    state.guestProjectileFacingRight = enemyProjectileFacingRight_;
    state.guestProjectileType = enemyProjectileType_;
    state.guestProjectileX = enemyProjectileX_;
    state.guestProjectileY = enemyProjectileY_;
    state.guestProjectileFrame = enemyProjectileFrame_;
    state.statusMessage = statusDisplayTime_ > 0.0 ? statusMessage_ : QString();
    state.statusSeconds = statusDisplayTime_;
    state.hostStats.score = score_;
    state.hostStats.damageDealt = levelBattleReport_.damageDealt;
    state.hostStats.damageTaken = levelBattleReport_.damageTaken;
    state.hostStats.healsUsed = levelBattleReport_.healsUsed;
    state.hostStats.attacks = levelBattleReport_.playerAttacks;
    state.hostStats.hits = levelBattleReport_.playerHits;
    state.hostStats.misses = levelBattleReport_.playerMisses;
    state.hostStats.projectilesFired = levelBattleReport_.projectilesFired;
    state.hostStats.projectilesHit = levelBattleReport_.projectilesHit;
    state.guestStats.score = duelRemoteScore_;
    state.guestStats.damageDealt = remoteBattleReport_.damageDealt;
    state.guestStats.damageTaken = remoteBattleReport_.damageTaken;
    state.guestStats.healsUsed = remoteBattleReport_.healsUsed;
    state.guestStats.attacks = remoteBattleReport_.playerAttacks;
    state.guestStats.hits = remoteBattleReport_.playerHits;
    state.guestStats.misses = remoteBattleReport_.playerMisses;
    state.guestStats.projectilesFired = remoteBattleReport_.projectilesFired;
    state.guestStats.projectilesHit = remoteBattleReport_.projectilesHit;
    if (state.winner != LanCombatWinner::NONE) {
        lanResolvedWinner_ = state.winner;
    }
    lanSessionManager_->publishCombatState(state);
}

void BattleWidget::updateRemoteLanMovement(double dt, quint8 remoteInputBits) {
    const bool moveWorldLeft = (remoteInputBits & LanInputMoveRight) != 0;
    const bool moveWorldRight = (remoteInputBits & LanInputMoveLeft) != 0;
    const double moveAmount = MOVE_SPEED * dt;

    if (moveWorldLeft && !moveWorldRight) {
        enemyX_ = qMax(ARENA_LEFT_X + 10.0, enemyX_ - moveAmount);
    } else if (moveWorldRight && !moveWorldLeft) {
        enemyX_ = qMin(double(width()) - ARENA_RIGHT_MARGIN - 10.0, enemyX_ + moveAmount);
    }

    if (enemyAnimChar_) {
        const AnimationState st = enemyAnimChar_->getCurrentState();
        const bool locked = (st == AnimationState::ATTACK1 || st == AnimationState::ATTACK2 ||
                             st == AnimationState::ATTACK3 || st == AnimationState::STRONG_ATTACK ||
                             st == AnimationState::HURT || st == AnimationState::DEATH);
        if (!locked) {
            enemyAnimChar_->setAnimationState((moveWorldLeft || moveWorldRight)
                ? AnimationState::RUN
                : AnimationState::IDLE);
        }
    }
}

void BattleWidget::tryRemoteLanHeal() {
    if (enemyHealCooldown_ > 0.0 || !gameManager_) {
        return;
    }

    const Player *player = gameManager_->getPlayer();
    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    if (!player || !player->isAlive() || !enemy || !enemy->isAlive()) {
        return;
    }

    enemy->takeDamage(-25);
    enemyHealCooldown_ = 5.0;
    ++remoteBattleReport_.healsUsed;
    remoteBattleReport_.playerHp = enemy->getHealth();
    statusMessage_ = QString("%1 uses heal.").arg(QString::fromStdString(enemy->getName()));
    statusDisplayTime_ = 1.2;
}

void BattleWidget::tryRemoteLanAttack(AnimationState attackState) {
    if (enemyCooldown_ > 0.0 || !gameManager_) {
        return;
    }

    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    if (!player || !enemy || !player->isAlive() || !enemy->isAlive()) {
        return;
    }

    if (enemyAnimChar_) {
        enemyAnimChar_->setAnimationState(attackState);
    }

    ++remoteBattleReport_.playerAttacks;
    const PlayerType remoteType = gameManager_->getLanOpponentPlayerType();
    if (remoteType == PlayerType::ARCEN) {
        const int damage = arcenArrowDamage(enemy->calculateDamage(), attackState);

        if (!enemyProjectileActive_) {
            spawnEnemyProjectile(enemy->getEnemyType(), damage);
            ++remoteBattleReport_.projectilesFired;
            statusMessage_ = QString("%1 fires a ranged attack!").arg(QString::fromStdString(enemy->getName()));
            statusDisplayTime_ = 0.9;
        } else {
            ++remoteBattleReport_.playerMisses;
            statusMessage_ = QString("%1's projectile lane is still occupied.").arg(QString::fromStdString(enemy->getName()));
            statusDisplayTime_ = 0.9;
        }

        enemyCooldown_ = PLAYER_ATTACK_COOLDOWN;
        return;
    }

    const double distToPlayer = std::abs(playerX_ - enemyX_);
    int damage = enemy->calculateDamage();
    if (attackState == AnimationState::ATTACK2) {
        damage = static_cast<int>(damage * 1.2);
    } else if (attackState == AnimationState::ATTACK3) {
        damage = static_cast<int>(damage * 1.35);
    }

    if (distToPlayer <= ATTACK_RANGE) {
        player->takeDamage(damage);
        duelRemoteScore_ += damage * 10;
        remoteBattleReport_.damageDealt += qMax(0, damage);
        ++remoteBattleReport_.playerHits;
        levelBattleReport_.damageTaken += qMax(0, damage);
        ++levelBattleReport_.enemyHits;

        if (playerAnimChar_) {
            playerAnimChar_->takeDamage();
        }

        statusMessage_ = QString("%1 lands %2 damage!").arg(QString::fromStdString(enemy->getName())).arg(damage);
        statusDisplayTime_ = 1.2;

        if (!player->isAlive()) {
            statusMessage_ = "Defeat! You were defeated!";
            statusDisplayTime_ = 3.0;
            if (playerAnimChar_) {
                playerAnimChar_->kill();
            }
        }
    } else {
        ++remoteBattleReport_.playerMisses;
        statusMessage_ = QString("%1 misses the strike.").arg(QString::fromStdString(enemy->getName()));
        statusDisplayTime_ = 0.9;
    }

    enemyCooldown_ = PLAYER_ATTACK_COOLDOWN;
}

ChronicleBattleReport BattleWidget::levelBattleReport() const {
    ChronicleBattleReport report = levelBattleReport_;
    const Player *player = gameManager_ ? gameManager_->getPlayer() : nullptr;
    const Enemy *enemy = gameManager_ ? gameManager_->getCurrentEnemy() : nullptr;

    if (player) {
        report.playerHp = player->getHealth();
        report.playerMaxHp = player->getMaxHealth();
        report.playerName = QString::fromStdString(player->getName());
        report.playerType = playerTypeLabel(player->getPlayerType());
    }
    if (enemy) {
        report.defeatedEnemyName = QString::fromStdString(enemy->getName());
        const bool duelPlayerRival = gameManager_
            && (gameManager_->isLanDuel()
                || (gameManager_->isDuelMode()
                    && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE));
        report.defeatedEnemyType = duelPlayerRival
            ? playerTypeLabel(gameManager_->getLanOpponentPlayerType())
            : enemyTypeLabel(enemy->getEnemyType());
        report.enemyMaxHp = enemy->getMaxHealth();
    }
    if (gameManager_) {
        report.completedLevel = gameManager_->getCurrentLevel();
        report.totalLevels = gameManager_->getTotalLevels();
        report.currentScore = gameManager_->isLanDuel() ? score_ : gameManager_->getCurrentScore();
        report.victory = gameManager_->isLanDuel() && lanResolvedWinner_ != LanCombatWinner::NONE
            ? isLocalLanWinner(lanResolvedWinner_)
            : (player && player->isAlive() && enemy && !enemy->isAlive());
        report.campaignComplete = gameManager_->hasCompletedCampaign();
    }
    report.battleDurationSeconds = levelClock_.isValid() ? levelClock_.elapsed() / 1000.0 : report.battleDurationSeconds;
    return report;
}

std::optional<CombatHighlightSnapshot> BattleWidget::levelBattleHighlight() const {
    return highlightTracker_.snapshot();
}

void BattleWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderBattleScene(painter, true);

    if (highlightTracker_.needsCapture()) {
        captureHighlightFrame(true);
    }
}

void BattleWidget::renderBattleScene(QPainter& painter,
                                     bool includeTransientOverlays,
                                     bool focusHighlightCombatants) {
    drawArenaBackground(painter);
    const double floorY = groundY();

    if (gameManager_ && gameManager_->isZombieMode() && zombieIntroActive_) {
        drawZombieIntroScene(painter);
        return;
    }
    
    if (!gameManager_ || !gameManager_->getPlayer() || !gameManager_->getCurrentEnemy()) {
        painter.setPen(QColor("#D4AF37"));
        painter.drawText(rect(), Qt::AlignCenter, "Initializing battle...");
        return;
    }
    
    const Player *player = gameManager_->getPlayer();
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    
    if (levelTransitionActive_) {
        drawLevelTransitionPortal(painter);
        if (finalRescueTransitionActive_) {
            drawFinalRescueTransition(painter);
        }
        drawFighterWithAnimation(painter, playerX_, floorY, player->getHealth(), player->getMaxHealth(),
                                 QString::fromStdString(player->getName()), playerAnimChar_, true);
    } else {
        double highlightShift = 0.0;
        if (focusHighlightCombatants) {
            const double guard = qMax(120.0, width() * 0.16);
            const double desiredCenter = width() * 0.5;
            const double combatCenter = (playerX_ + enemyX_) * 0.5;
            highlightShift = desiredCenter - combatCenter;

            const double shiftedLeft = qMin(playerX_ + highlightShift, enemyX_ + highlightShift);
            const double shiftedRight = qMax(playerX_ + highlightShift, enemyX_ + highlightShift);
            if (shiftedLeft < guard) {
                highlightShift += guard - shiftedLeft;
            }
            if (shiftedRight > width() - guard) {
                highlightShift -= shiftedRight - (width() - guard);
            }
        }

        // Draw fighters with animation support - Draw enemy first so player is on top
        drawFinalKingStageScene(painter);
        painter.save();
        painter.translate(highlightShift, 0.0);
        drawFighterWithAnimation(painter, enemyX_, floorY, enemy->getHealth(), enemy->getMaxHealth(),
                                 QString::fromStdString(enemy->getName()), enemyAnimChar_, false);
        drawFighterWithAnimation(painter, playerX_, floorY, player->getHealth(), player->getMaxHealth(),
                                 QString::fromStdString(player->getName()), playerAnimChar_, true);

        drawArcenProjectile(painter);
        drawEnemyProjectile(painter);
        painter.restore();
    }
    
    // Draw HUD and status
    drawHUD(painter);
    drawStatus(painter);
    if (includeTransientOverlays) {
        drawFinalRescueDialogue(painter);
        drawCountdownOverlay(painter);
        drawZombieCityClearedOverlay(painter);
    }
}

void BattleWidget::captureHighlightFrame(bool focusHighlightCombatants) {
    if (width() <= 0 || height() <= 0) {
        return;
    }

    QPixmap capture(size());
    capture.fill(Qt::transparent);

    {
        QPainter capturePainter(&capture);
        capturePainter.setRenderHint(QPainter::Antialiasing, true);
        renderBattleScene(capturePainter, false, focusHighlightCombatants);
    }

    QImage image = capture.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    if (image.width() > 1280) {
        image = image.scaledToWidth(1280, Qt::SmoothTransformation);
    }

    QByteArray imageBytes;
    QBuffer buffer(&imageBytes);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        return;
    }

    highlightTracker_.completeCapture(imageBytes, QStringLiteral("image/png"), QDateTime::currentDateTimeUtc());
}

QImage BattleWidget::captureHighlightReplayFrame() const {
    if (width() <= 0 || height() <= 0) {
        return QImage();
    }

    QPixmap capture(size());
    capture.fill(Qt::transparent);

    {
        QPainter capturePainter(&capture);
        capturePainter.setRenderHint(QPainter::Antialiasing, true);
        const_cast<BattleWidget*>(this)->renderBattleScene(capturePainter, false, true);
    }

    QImage image = capture.toImage().convertToFormat(QImage::Format_RGB32);
    const double targetRatio = double(HIGHLIGHT_CLIP_FRAME_WIDTH) / double(HIGHLIGHT_CLIP_FRAME_HEIGHT);
    QRect cropRect = image.rect();
    if (image.height() > 0 && image.width() / double(image.height()) > targetRatio) {
        const int cropWidth = qMax(1, int(image.height() * targetRatio));
        cropRect = QRect((image.width() - cropWidth) / 2, 0, cropWidth, image.height());
    } else if (image.width() > 0) {
        const int cropHeight = qMax(1, int(image.width() / targetRatio));
        cropRect = QRect(0, qMax(0, (image.height() - cropHeight) / 2), image.width(), qMin(cropHeight, image.height()));
    }

    return image.copy(cropRect).scaled(HIGHLIGHT_CLIP_FRAME_WIDTH,
                                      HIGHLIGHT_CLIP_FRAME_HEIGHT,
                                      Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation);
}

QByteArray BattleWidget::buildHighlightClipSheet(QString* outMimeType) const {
    if (activeHighlightClipFrames_.isEmpty()) {
        return QByteArray();
    }

    QVector<QImage> frames = activeHighlightClipFrames_;
    const QImage fallbackFrame = frames.last().isNull() ? captureHighlightReplayFrame() : frames.last();
    while (frames.size() < HIGHLIGHT_CLIP_FRAME_COUNT && !fallbackFrame.isNull()) {
        frames.append(fallbackFrame);
    }
    if (frames.size() > HIGHLIGHT_CLIP_FRAME_COUNT) {
        frames = frames.mid(frames.size() - HIGHLIGHT_CLIP_FRAME_COUNT);
    }

    const int rows = qMax(1, (HIGHLIGHT_CLIP_FRAME_COUNT + HIGHLIGHT_CLIP_COLUMNS - 1) / HIGHLIGHT_CLIP_COLUMNS);
    QImage sheet(HIGHLIGHT_CLIP_FRAME_WIDTH * HIGHLIGHT_CLIP_COLUMNS,
                 HIGHLIGHT_CLIP_FRAME_HEIGHT * rows,
                 QImage::Format_RGB32);
    sheet.fill(QColor("#120b08"));

    {
        QPainter painter(&sheet);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        for (int i = 0; i < frames.size(); ++i) {
            const QImage frame = frames[i].isNull()
                ? fallbackFrame
                : frames[i].scaled(HIGHLIGHT_CLIP_FRAME_WIDTH,
                                   HIGHLIGHT_CLIP_FRAME_HEIGHT,
                                   Qt::IgnoreAspectRatio,
                                   Qt::SmoothTransformation);
            const int col = i % HIGHLIGHT_CLIP_COLUMNS;
            const int row = i / HIGHLIGHT_CLIP_COLUMNS;
            painter.drawImage(QRect(col * HIGHLIGHT_CLIP_FRAME_WIDTH,
                                    row * HIGHLIGHT_CLIP_FRAME_HEIGHT,
                                    HIGHLIGHT_CLIP_FRAME_WIDTH,
                                    HIGHLIGHT_CLIP_FRAME_HEIGHT),
                              frame);
        }
    }

    auto saveSheet = [&](const char* format, int quality, const QString& mimeType) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        if (!buffer.open(QIODevice::WriteOnly) || !sheet.save(&buffer, format, quality)) {
            return QByteArray();
        }
        if (outMimeType) {
            *outMimeType = mimeType;
        }
        return bytes;
    };

    for (const char* format : {"JPEG", "JPG"}) {
        for (int quality : {72, 64, 56, 48}) {
            QByteArray bytes = saveSheet(format, quality, QStringLiteral("image/jpeg"));
            if (!bytes.isEmpty() && bytes.size() <= 3800 * 1024) {
                return bytes;
            }
        }
    }

    QByteArray pngBytes = saveSheet("PNG", -1, QStringLiteral("image/png"));
    if (!pngBytes.isEmpty()) {
        if (pngBytes.size() > 3800 * 1024) {
            qWarning() << "BattleWidget: highlight clip PNG exceeds target size" << pngBytes.size();
        }
        return pngBytes;
    }

    qWarning() << "BattleWidget: failed to encode highlight replay sprite sheet";
    return QByteArray();
}

void BattleWidget::resetHighlightReplay() {
    highlightTracker_.reset();
    rollingHighlightFrames_.clear();
    activeHighlightClipFrames_.clear();
    highlightFrameAccumulator_ = 0.0;
    highlightPostFramesRemaining_ = 0;
}

void BattleWidget::recordHighlightReplayFrame(double dt) {
    if (!gameManager_ || gameManager_->isLanDuel() || width() <= 0 || height() <= 0) {
        return;
    }

    highlightFrameAccumulator_ += dt;
    const double interval = 1.0 / double(HIGHLIGHT_CLIP_FPS);
    if (highlightFrameAccumulator_ < interval) {
        return;
    }
    highlightFrameAccumulator_ = std::fmod(highlightFrameAccumulator_, interval);

    const QImage frame = captureHighlightReplayFrame();
    if (frame.isNull()) {
        return;
    }

    rollingHighlightFrames_.append(frame);
    while (rollingHighlightFrames_.size() > HIGHLIGHT_CLIP_FRAME_COUNT) {
        rollingHighlightFrames_.removeFirst();
    }

    if (highlightPostFramesRemaining_ > 0 && !activeHighlightClipFrames_.isEmpty()) {
        activeHighlightClipFrames_.append(frame);
        --highlightPostFramesRemaining_;
        if (highlightPostFramesRemaining_ <= 0 || activeHighlightClipFrames_.size() >= HIGHLIGHT_CLIP_FRAME_COUNT) {
            finalizeHighlightClip();
        }
    }
}

void BattleWidget::beginHighlightClipCapture(const QImage& impactFrame) {
    activeHighlightClipFrames_.clear();

    const int start = qMax(0, rollingHighlightFrames_.size() - HIGHLIGHT_CLIP_PRE_FRAMES);
    for (int i = start; i < rollingHighlightFrames_.size(); ++i) {
        activeHighlightClipFrames_.append(rollingHighlightFrames_[i]);
    }
    if (!impactFrame.isNull()) {
        activeHighlightClipFrames_.append(impactFrame);
    }

    while (activeHighlightClipFrames_.size() > HIGHLIGHT_CLIP_FRAME_COUNT) {
        activeHighlightClipFrames_.removeFirst();
    }

    highlightPostFramesRemaining_ = qMax(0, HIGHLIGHT_CLIP_FRAME_COUNT - activeHighlightClipFrames_.size());
}

void BattleWidget::finalizeHighlightClip() {
    if (activeHighlightClipFrames_.isEmpty()) {
        return;
    }

    if (activeHighlightClipFrames_.size() < HIGHLIGHT_CLIP_FRAME_COUNT) {
        QImage fallback = activeHighlightClipFrames_.last();
        if (fallback.isNull()) {
            fallback = captureHighlightReplayFrame();
        }
        while (activeHighlightClipFrames_.size() < HIGHLIGHT_CLIP_FRAME_COUNT && !fallback.isNull()) {
            activeHighlightClipFrames_.append(fallback);
        }
    }

    QString clipMimeType = QStringLiteral("image/jpeg");
    const QByteArray sheetBytes = buildHighlightClipSheet(&clipMimeType);
    if (sheetBytes.isEmpty()) {
        return;
    }

    highlightTracker_.completeClip(sheetBytes,
                                   clipMimeType,
                                   QStringLiteral("sprite_sheet_v1"),
                                   HIGHLIGHT_CLIP_FRAME_COUNT,
                                   HIGHLIGHT_CLIP_FPS,
                                   HIGHLIGHT_CLIP_FRAME_WIDTH,
                                   HIGHLIGHT_CLIP_FRAME_HEIGHT,
                                   HIGHLIGHT_CLIP_DURATION_SECONDS);
    highlightPostFramesRemaining_ = 0;
}

CombatHighlightCandidate BattleWidget::buildHighlightCandidate(HighlightAttackType attackType,
                                                               int damage,
                                                               bool wasProjectile,
                                                               int playerHpBefore,
                                                               int enemyHpBefore,
                                                               int enemyHpAfter) const {
    CombatHighlightCandidate candidate;
    candidate.attackType = attackType;
    candidate.damage = qMax(0, damage);
    candidate.wasProjectile = wasProjectile;
    candidate.wasFinisher = enemyHpAfter <= 0;
    candidate.playerHpBefore = qMax(0, playerHpBefore);
    candidate.playerHpAfter = gameManager_ && gameManager_->getPlayer() ? gameManager_->getPlayer()->getHealth() : qMax(0, playerHpBefore);
    candidate.playerMaxHp = gameManager_ && gameManager_->getPlayer() ? gameManager_->getPlayer()->getMaxHealth() : 0;
    candidate.enemyHpBefore = qMax(0, enemyHpBefore);
    candidate.enemyHpAfter = qMax(0, enemyHpAfter);
    return candidate;
}

bool BattleWidget::considerPlayerHighlight(const CombatHighlightCandidate& candidate) {
    if (!gameManager_ || gameManager_->isLanDuel()) {
        return false;
    }

    if (highlightTracker_.considerCandidate(candidate)) {
        update();
        return true;
    }

    return false;
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
    if (finalRescueTransitionActive_) {
        return;
    }

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
                     gameManager_ && gameManager_->isLanDuel()
                         ? "Arena Link Live Duel  |  A/D: Move  |  J/K/L: Attack  |  H: Heal  |  ESC: Pause"
                         : "A/D: Move  |  J/K/L: Attack 1/2/3  |  W: Jump  |  H: Heal  |  ESC: Pause");
}

void BattleWidget::drawFinalRescueDialogue(QPainter &painter) {
    if (!finalRescueTransitionActive_ || !gameManager_ || !gameManager_->getPlayer()) {
        return;
    }

    const double w = width();
    const double h = height();
    const double runStart = 4.4;
    const double transitionDuration = 7.6;
    const double runProgress = qBound(0.0, (levelTransitionTime_ - runStart) / qMax(0.1, transitionDuration - runStart), 1.0);
    const double kingX = levelTransitionStartKingX_ + (w * 0.99 - levelTransitionStartKingX_) * runProgress;
    const QString playerName = QString::fromStdString(gameManager_->getPlayer()->getName());

    auto drawSpeechBubble = [&](const QRectF& requestedRect, const QString& speaker, const QString& line, bool tailLeft) {
        const double bubbleWidth = requestedRect.width();
        const QRectF bubbleRect(qBound(24.0, requestedRect.left(), w - bubbleWidth - 24.0),
                                requestedRect.top(),
                                bubbleWidth,
                                requestedRect.height());

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QColor(235, 204, 132, 190));
        painter.setBrush(QColor(17, 11, 9, 218));
        painter.drawRoundedRect(bubbleRect, 14.0, 14.0);

        QPainterPath tail;
        const double tailX = tailLeft ? bubbleRect.left() + bubbleRect.width() * 0.24
                                      : bubbleRect.right() - bubbleRect.width() * 0.24;
        tail.moveTo(tailX - 12.0, bubbleRect.bottom() - 1.0);
        tail.lineTo(tailX + 12.0, bubbleRect.bottom() - 1.0);
        tail.lineTo(tailLeft ? tailX - 28.0 : tailX + 28.0, bubbleRect.bottom() + 22.0);
        tail.closeSubpath();
        painter.drawPath(tail);

        QFont speakerFont("Segoe UI", qMax(10, width() / 116), QFont::Black);
        painter.setFont(speakerFont);
        painter.setPen(QColor("#F4D895"));
        const QRectF speakerRect = bubbleRect.adjusted(14.0, 8.0, -14.0, -bubbleRect.height() * 0.58);
        painter.drawText(speakerRect, Qt::AlignLeft | Qt::AlignVCenter, speaker);

        QFont speechFont("Segoe UI", qMax(11, width() / 104), QFont::DemiBold);
        painter.setFont(speechFont);
        painter.setPen(QColor("#FFF3CF"));
        painter.drawText(bubbleRect.adjusted(14.0, bubbleRect.height() * 0.36, -14.0, -10.0),
                         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                         line);
        painter.restore();
    };

    if (levelTransitionTime_ < 2.25) {
        drawSpeechBubble(QRectF(playerX_ - w * 0.08, h * 0.14, w * 0.27, h * 0.105),
                         playerName,
                         QStringLiteral("I saved you, King. Let's go back!"),
                         true);
        return;
    }

    if (levelTransitionTime_ < runStart) {
        drawSpeechBubble(QRectF(kingX - w * 0.25, h * 0.14, w * 0.27, h * 0.105),
                         QStringLiteral("King"),
                         QStringLiteral("Thank you, brave gladiator. Lead the way!"),
                         false);
        return;
    }

    const QRectF captionCard(w * 0.16, h * 0.885, w * 0.68, h * 0.058);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QColor(212, 160, 74, 145));
    painter.setBrush(QColor(19, 12, 10, 188));
    painter.drawRoundedRect(captionCard, 12.0, 12.0);
    painter.setPen(QColor("#FFF1CC"));
    QFont bodyFont("Segoe UI", qMax(11, width() / 95), QFont::DemiBold);
    painter.setFont(bodyFont);
    painter.drawText(captionCard.toRect(),
                     Qt::AlignCenter,
                     QStringLiteral("The king and gladiator enter the portal together."));
    painter.restore();
}

void BattleWidget::drawCountdownOverlay(QPainter &painter) {
    if (!battleActive_ || levelTransitionActive_ || introLockTime_ <= 0.0) {
        return;
    }

    const Player *player = gameManager_ ? gameManager_->getPlayer() : nullptr;
    const Enemy *enemy = gameManager_ ? gameManager_->getCurrentEnemy() : nullptr;
    if (!player || !enemy) {
        return;
    }

    const double remaining = qBound(0.0, introLockTime_, BATTLE_COUNTDOWN_DURATION);
    QString countdownText;
    if (remaining > 2.25) {
        countdownText = "3";
    } else if (remaining > 1.35) {
        countdownText = "2";
    } else if (remaining > 0.45) {
        countdownText = "1";
    } else {
        countdownText = "FIGHT!";
    }

    const bool isFight = countdownText == "FIGHT!";
    const double segment = isFight ? qBound(0.0, (0.45 - remaining) / 0.45, 1.0)
                                   : std::fmod(BATTLE_COUNTDOWN_DURATION - remaining, 0.9) / 0.9;
    const int titleSize = isFight
        ? qMax(48, static_cast<int>(height() * (0.084 + segment * 0.012)))
        : qMax(86, static_cast<int>(height() * (0.13 + (1.0 - segment) * 0.018)));

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), QColor(9, 5, 3, 124));

    const QString playerName = QString::fromStdString(player->getName()).toUpper();
    const QString enemyName = QString::fromStdString(enemy->getName()).toUpper();
    const bool isLanDuel = gameManager_ && gameManager_->isLanDuel();
    const qreal cardLift = isFight ? 0.0 : (1.0 - qBound(0.0, remaining / BATTLE_COUNTDOWN_DURATION, 1.0)) * 10.0;
    const QRectF leftZone(width() * 0.10, height() * 0.3 + cardLift, width() * 0.27, height() * 0.34);
    const QRectF rightZone(width() * 0.63, height() * 0.3 + cardLift, width() * 0.27, height() * 0.34);

    auto fittedFaceoffFont = [&](const QString& text, qreal maxWidth, int maxPointSize, int minPointSize) {
        QFont font("Showcard Gothic", maxPointSize);
        if (font.family() != "Showcard Gothic") {
            font = painter.font();
            font.setBold(true);
        }

        for (int pointSize = maxPointSize; pointSize >= minPointSize; --pointSize) {
            font.setPointSize(pointSize);
            QFontMetricsF metrics(font);
            if (metrics.horizontalAdvance(text) <= maxWidth) {
                break;
            }
        }
        return font;
    };

    auto drawIntroPortrait = [&](const QRectF& zoneRect,
                                 const QPixmap& portrait,
                                 const QColor& accentColor,
                                 const QString& fighterName) {
        painter.save();
        const qreal portraitSize = qMin<qreal>(224.0, qMax<qreal>(164.0, height() * 0.196));
        const QRectF portraitRect(zoneRect.center().x() - portraitSize * 0.5,
                                  zoneRect.y() + 4.0,
                                  portraitSize,
                                  portraitSize);
        drawPortraitBadge(painter, portraitRect, portrait, accentColor, fighterName.left(2));

        const QRectF nameRect(zoneRect.center().x() - zoneRect.width() * 0.46,
                              portraitRect.bottom() + 18.0,
                              zoneRect.width() * 0.92,
                              zoneRect.height() - portraitRect.height() - 18.0);
        QFont nameFont = fittedFaceoffFont(fighterName,
                                           nameRect.width(),
                                           qMax(22, static_cast<int>(height() * 0.040)),
                                           qMax(14, static_cast<int>(height() * 0.023)));
        painter.setFont(nameFont);
        painter.setPen(QColor(0, 0, 0, 165));
        painter.drawText(nameRect.translated(0.0, 3.0),
                         Qt::AlignHCenter | Qt::AlignTop,
                         fighterName);
        painter.setPen(QColor("#FFF0C8"));
        painter.drawText(nameRect,
                         Qt::AlignHCenter | Qt::AlignTop,
                         fighterName);
        painter.restore();
    };

    drawIntroPortrait(leftZone,
                      playerProfilePortrait_,
                      QColor("#33D17A"),
                      playerName);
    drawIntroPortrait(rightZone,
                      enemyProfilePortrait_,
                      QColor("#F97316"),
                      enemyName);

    const QRectF plateRect(width() * 0.33, height() * 0.33, width() * 0.34, height() * 0.24);
    QPainterPath plate;
    plate.addRoundedRect(plateRect, 28, 28);

    QLinearGradient plateGradient(plateRect.topLeft(), plateRect.bottomRight());
    plateGradient.setColorAt(0.0, QColor(38, 22, 12, 224));
    plateGradient.setColorAt(0.48, QColor(74, 39, 18, 232));
    plateGradient.setColorAt(1.0, QColor(114, 28, 18, 225));
    painter.fillPath(plate, plateGradient);
    painter.setPen(QPen(QColor(218, 167, 48, 215), 3));
    painter.drawPath(plate);

    QFont labelFont("Segoe UI", qMax(12, static_cast<int>(height() * 0.022)), QFont::Black);
    labelFont.setLetterSpacing(QFont::AbsoluteSpacing, 4);
    painter.setFont(labelFont);
    painter.setPen(QColor("#F6D47A"));
    painter.drawText(plateRect.adjusted(0, 18, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "GET READY");

    QFont countdownFont("Segoe UI", titleSize, QFont::Black);
    countdownFont.setLetterSpacing(QFont::AbsoluteSpacing, isFight ? 7 : 1);
    painter.setFont(countdownFont);
    const QRectF textRect = plateRect.adjusted(10, 34, -10, -34);

    painter.setPen(QColor(0, 0, 0, 185));
    painter.drawText(textRect.translated(5, 7), Qt::AlignCenter, countdownText);

    QLinearGradient textGradient(textRect.topLeft(), textRect.bottomLeft());
    textGradient.setColorAt(0.0, QColor("#FFF0A6"));
    textGradient.setColorAt(0.55, isFight ? QColor("#FF3B25") : QColor("#FFD22E"));
    textGradient.setColorAt(1.0, isFight ? QColor("#96130F") : QColor("#A85A12"));
    painter.setPen(QPen(QBrush(textGradient), 2));
    painter.drawText(textRect, Qt::AlignCenter, countdownText);

    QFont hintFont("Segoe UI", qMax(10, static_cast<int>(height() * 0.019)), QFont::DemiBold);
    painter.setFont(hintFont);
    painter.setPen(QColor(255, 230, 160, 210));
    painter.drawText(plateRect.adjusted(20, 0, -20, -18),
                     Qt::AlignHCenter | Qt::AlignBottom,
                     isFight
                         ? (isLanDuel
                                ? "Fight with honor."
                                : (gameManager_ && gameManager_->isDuelMode()
                                       ? "Settle the duel."
                                       : "Defend the king."))
                         : (isLanDuel
                                ? "Profile locked. Arena syncing."
                                : (gameManager_ && gameManager_->isDuelMode()
                                       ? "Matchup locked. Arena chosen."
                                       : "Prepare your stance.")));

    painter.restore();
}

void BattleWidget::drawZombieIntroScene(QPainter &painter) {
    if (!gameManager_ || !gameManager_->isZombieMode()) {
        return;
    }

    const double elapsed = qBound(0.0, 10.0 - zombieIntroTime_, 10.0);
    const double progress = elapsed / 10.0;
    const double floorY = groundY() + 8.0;

    painter.save();
    painter.fillRect(rect(), QColor(4, 3, 3, 76));

    struct IntroZombie {
        int index;
        bool exitsLeft;
        double lane;
        double scale;
    };

    const std::array<IntroZombie, 4> zombies = {{
        {1, true, 0.34, 0.86},
        {2, true, 0.45, 0.92},
        {3, false, 0.56, 0.90},
        {4, false, 0.66, 0.96},
    }};

    for (const IntroZombie& zombie : zombies) {
        const QString path = resolveAssetPath(QStringLiteral("assets/beasts/zombies/Zombie_%1/Walk.png").arg(zombie.index));
        QPixmap sheet(path);
        if (sheet.isNull()) {
            continue;
        }

        const int frameCount = zombie.index == 4 ? 12 : 10;
        const int frameWidth = qMax(1, sheet.width() / frameCount);
        const int frameIndex = static_cast<int>(elapsed * 8.0 + zombie.index * 2) % frameCount;
        QPixmap frame = sheet.copy(frameIndex * frameWidth, 0, frameWidth, sheet.height());
        if (!zombie.exitsLeft) {
            frame = frame.transformed(QTransform().scale(-1, 1));
        }

        const double gatherX = width() * zombie.lane;
        const double entranceX = zombie.exitsLeft ? -120.0 - zombie.index * 18.0 : width() + 120.0 + zombie.index * 18.0;
        const double exitX = zombie.exitsLeft ? -150.0 - zombie.index * 26.0 : width() + 150.0 + zombie.index * 26.0;

        double x = gatherX;
        if (progress < 0.60) {
            const double t = qBound(0.0, progress / 0.60, 1.0);
            x = entranceX + (gatherX - entranceX) * t;
        } else {
            const double t = qBound(0.0, (progress - 0.60) / 0.40, 1.0);
            x = gatherX + (exitX - gatherX) * t;
        }

        const double targetHeight = qMax(112.0, height() * 0.24 * zombie.scale);
        const double targetWidth = targetHeight * (double(frame.width()) / qMax(1, frame.height()));
        QRectF target(x - targetWidth * 0.5,
                      floorY - targetHeight - (zombie.index % 2) * 10.0,
                      targetWidth,
                      targetHeight);

        painter.setOpacity(0.95);
        painter.drawPixmap(target, frame, frame.rect());
    }

    painter.setOpacity(1.0);
    painter.fillRect(rect(), QColor(0, 0, 0, 58));

    const QRectF panel(width() * 0.20, height() * 0.12, width() * 0.60, height() * 0.18);
    QPainterPath panelPath;
    panelPath.addRoundedRect(panel, 22.0, 22.0);
    painter.fillPath(panelPath, QColor(18, 8, 7, 202));
    painter.setPen(QPen(QColor(211, 158, 38, 210), 2));
    painter.drawPath(panelPath);

    QFont titleFont("Showcard Gothic", qMax(26, static_cast<int>(height() * 0.052)), QFont::Black);
    painter.setFont(titleFont);
    painter.setPen(QColor("#FFD21F"));
    painter.drawText(panel.adjusted(16, 12, -16, -panel.height() * 0.42),
                     Qt::AlignCenter,
                     QStringLiteral("THE CITY IS FULL OF ZOMBIES"));

    QFont bodyFont("Segoe UI", qMax(12, static_cast<int>(height() * 0.022)), QFont::DemiBold);
    painter.setFont(bodyFont);
    painter.setPen(QColor("#FFE6B0"));
    painter.drawText(panel.adjusted(26, panel.height() * 0.50, -26, -12),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     progress < 0.60
                         ? QStringLiteral("Four infected enter the ruined streets.")
                         : QStringLiteral("Two vanish left, two vanish right. Clear them one by one."));

    painter.restore();
}

void BattleWidget::drawZombieCityClearedOverlay(QPainter &painter) {
    if (!gameManager_ || !gameManager_->isZombieMode() || !zombieCityCleaned_) {
        return;
    }

    const double fade = qBound(0.0, battleEndDelay_ / 3.2, 1.0);
    painter.save();
    painter.fillRect(rect(), QColor(6, 16, 13, static_cast<int>(72 + fade * 56)));

    const QRectF panel(width() * 0.22, height() * 0.30, width() * 0.56, height() * 0.22);
    QPainterPath path;
    path.addRoundedRect(panel, 24.0, 24.0);
    painter.fillPath(path, QColor(10, 24, 18, 218));
    painter.setPen(QPen(QColor(80, 220, 150, 210), 2));
    painter.drawPath(path);

    QFont titleFont("Showcard Gothic", qMax(30, static_cast<int>(height() * 0.06)), QFont::Black);
    painter.setFont(titleFont);
    painter.setPen(QColor("#B7FFD1"));
    painter.drawText(panel.adjusted(18, 20, -18, -panel.height() * 0.42),
                     Qt::AlignCenter,
                     QStringLiteral("CITY ZOMBIE FREE"));

    QFont bodyFont("Segoe UI", qMax(13, static_cast<int>(height() * 0.024)), QFont::DemiBold);
    painter.setFont(bodyFont);
    painter.setPen(QColor("#F5FFE7"));
    painter.drawText(panel.adjusted(28, panel.height() * 0.54, -28, -18),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     QStringLiteral("The last infected falls. The streets are clean again."));
    painter.restore();
}

void BattleWidget::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;

    lanBridgeActive_ = gameManager_ && gameManager_->isLanDuel() && lanSessionManager_;
    lanHostAuthority_ = lanBridgeActive_ && lanSessionManager_
        && lanSessionManager_->snapshot().localRole == LanRole::HOST;

    const PlayerType playerType = selectedPlayerType();

    auto rejectAction = [this](const QString &text) {
        statusMessage_ = text;
        statusDisplayTime_ = 1.2;
    };
    
    switch (event->key()) {
        case Qt::Key_A:
        case Qt::Key_Left:
            keyboardMovingLeft_ = true;
            refreshCombinedMovementInput();
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            keyboardMovingRight_ = true;
            refreshCombinedMovementInput();
            break;
        case Qt::Key_J:
        case Qt::Key_Space:
            if (!queuePlayerAttack(AnimationState::ATTACK1, false)) {
                rejectAction("This character cannot use Attack 1");
            }
            break;
        case Qt::Key_K:
            if (!queuePlayerAttack(AnimationState::ATTACK2, true)) {
                rejectAction("This character has no Attack 2");
            }
            break;
        case Qt::Key_L:
            if (!queuePlayerAttack(AnimationState::ATTACK3, true)) {
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

    if (battleActive_ && lanBridgeActive_ && !lanHostAuthority_ && lanSessionManager_) {
        const quint8 inputBits = currentLanInputBits();
        lastSentLanInputBits_ = inputBits;
        lanSessionManager_->sendCombatInput(inputBits);
    }
}

void BattleWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;

    lanBridgeActive_ = gameManager_ && gameManager_->isLanDuel() && lanSessionManager_;
    lanHostAuthority_ = lanBridgeActive_ && lanSessionManager_
        && lanSessionManager_->snapshot().localRole == LanRole::HOST;
    
    switch (event->key()) {
        case Qt::Key_A:
        case Qt::Key_Left:
            keyboardMovingLeft_ = false;
            refreshCombinedMovementInput();
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            keyboardMovingRight_ = false;
            refreshCombinedMovementInput();
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

    if (battleActive_ && lanBridgeActive_ && !lanHostAuthority_ && lanSessionManager_) {
        const quint8 inputBits = currentLanInputBits();
        lastSentLanInputBits_ = inputBits;
        lanSessionManager_->sendCombatInput(inputBits);
    }
}

void BattleWidget::advanceFrame() {
    if (!gameManager_) return;

    lanBridgeActive_ = gameManager_->isLanDuel() && lanSessionManager_;
    lanHostAuthority_ = lanBridgeActive_ && lanSessionManager_
        && lanSessionManager_->snapshot().localRole == LanRole::HOST;

    const double dt = qMin(0.033, elapsedTimer_.restart() / 1000.0);
    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());

    if (!player || !enemy) return;

    if (levelTransitionActive_) {
        levelTransitionTime_ += dt;
        statusDisplayTime_ = qMax(0.0, statusDisplayTime_ - dt);

        const double runEndX = width() * 1.01;
        const double finalDialogueTime = 4.4;
        const double finalTransitionDuration = 7.6;
        const double transitionDuration = finalRescueTransitionActive_ ? finalTransitionDuration : LEVEL_TRANSITION_DURATION;
        const double progress = finalRescueTransitionActive_
            ? qBound(0.0, (levelTransitionTime_ - finalDialogueTime) / qMax(0.1, transitionDuration - finalDialogueTime), 1.0)
            : qBound(0.0, levelTransitionTime_ / qMax(0.1, LEVEL_TRANSITION_DURATION), 1.0);
        playerX_ = levelTransitionStartPlayerX_ + (runEndX - levelTransitionStartPlayerX_) * progress;

        if (playerAnimChar_) {
            playerAnimChar_->setFacingLeft(false);
            playerAnimChar_->setAnimationState(finalRescueTransitionActive_ && levelTransitionTime_ < finalDialogueTime
                                                   ? AnimationState::IDLE
                                                   : AnimationState::RUN);
        }

        if (finalRescueTransitionActive_) {
            const QString playerName = QString::fromStdString(player->getName());
            if (levelTransitionTime_ < 2.25) {
                statusMessage_ = QString("%1: I saved you, King. Let's go back!").arg(playerName);
            } else if (levelTransitionTime_ < finalDialogueTime) {
                statusMessage_ = "King: Thank you, brave gladiator. Lead the way!";
            } else {
                statusMessage_ = "The king and gladiator enter the portal together.";
            }
        }

        update();

        if (levelTransitionTime_ >= transitionDuration) {
            levelTransitionActive_ = false;
            finalRescueTransitionActive_ = false;
            frameTimer_.stop();
            emit levelTransitionFinished();
        }
        return;
    }

    if (!battleActive_) return;

    updateControllerInput();

    if (lanBridgeActive_ && !lanHostAuthority_) {
        quint8 inputBits = currentLanInputBits();
        if (lanSessionManager_) {
            lastSentLanInputBits_ = inputBits;
            lanSessionManager_->sendCombatInput(inputBits);

            const LanCombatState state = lanSessionManager_->latestCombatState();
            if (state.tick > 0) {
                applyGuestCombatState(state);
            }
        }

        if (introLockTime_ <= 0.0) {
            updatePlayerMovement(dt);
            if (playerAnimChar_) {
                playerAnimChar_->setFacingLeft(enemyX_ < playerX_);
            }
            if (enemyAnimChar_) {
                enemyAnimChar_->setFacingLeft(playerX_ < enemyX_);
            }

            const quint8 pressedThisFrame = static_cast<quint8>(inputBits & ~lastPredictedLocalLanInputBits_);
            if (player->isAlive() && enemy->isAlive()) {
                if ((pressedThisFrame & LanInputHeal) != 0 && healCooldown_ <= 0.0) {
                    tryPlayerHeal();
                }

                AnimationState predictedAttackState = AnimationState::IDLE;
                if ((pressedThisFrame & LanInputAttack1) != 0) {
                    predictedAttackState = AnimationState::ATTACK1;
                } else if ((pressedThisFrame & LanInputAttack2) != 0) {
                    predictedAttackState = AnimationState::ATTACK2;
                } else if ((pressedThisFrame & LanInputAttack3) != 0) {
                    predictedAttackState = AnimationState::ATTACK3;
                }

                if (predictedAttackState != AnimationState::IDLE && playerCooldown_ <= 0.0) {
                    if (playerAnimChar_) {
                        playerAnimChar_->setAnimationState(predictedAttackState);
                    }
                    if (soundManager_) {
                        soundManager_->playAttack();
                    }

                    const PlayerType playerType = gameManager_->getSelectedPlayerType();
                    if (playerType == PlayerType::ARCEN && !arcenProjectileActive_) {
                        const int damage = arcenArrowDamage(player->calculateDamage(), predictedAttackState);
                        spawnArcenProjectile(damage);
                        statusMessage_ = "Arrow fired!";
                        statusDisplayTime_ = 0.7;
                    } else if (playerType == PlayerType::ARCEN) {
                        statusMessage_ = "Arrow already in flight";
                        statusDisplayTime_ = 0.7;
                    } else {
                        statusMessage_ = "Strike committed";
                        statusDisplayTime_ = 0.55;
                    }

                    playerCooldown_ = PLAYER_ATTACK_COOLDOWN;
                }
            }

            updateArcenProjectile(dt);
            updateEnemyProjectile(dt);
        }
        lastPredictedLocalLanInputBits_ = inputBits;
        attackPressed_ = false;
        healPressed_ = false;

        const Player *p = gameManager_->getPlayer();
        const Enemy *e = gameManager_->getCurrentEnemy();
        if (p) {
            const double target = p->getMaxHealth() > 0 ? static_cast<double>(p->getHealth()) / p->getMaxHealth() : 0.0;
            playerHpDisplay_ += (target - playerHpDisplay_) * qMin(1.0, 5.0 * dt);
        }
        if (e) {
            const double target = e->getMaxHealth() > 0 ? static_cast<double>(e->getHealth()) / e->getMaxHealth() : 0.0;
            enemyHpDisplay_ += (target - enemyHpDisplay_) * qMin(1.0, 5.0 * dt);
        }

        statusDisplayTime_ = qMax(0.0, statusDisplayTime_ - dt);
        update();

        if (lanGuestStateSeen_ && !battleActive_) {
            emit battleFinished();
        }
        return;
    }
    
    // Let death animations play before leaving battle.
    if (!player->isAlive() || !enemy->isAlive()) {
        if (gameManager_->isZombieMode() && zombieCityCleaned_) {
            battleEndDelay_ -= dt;
            if (battleEndDelay_ <= 0.0) {
                battleActive_ = false;
                emit battleFinished();
            }
            recordHighlightReplayFrame(dt);
            update();
            return;
        }

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
            if (gameManager_->isZombieMode() && player->isAlive() && !enemy->isAlive()) {
                const int clearedLevel = gameManager_->getCurrentLevel();
                gameManager_->addScore(25 + gameManager_->getCurrentLevel() * 8);
                if (gameManager_->advanceZombieWave()) {
                    Enemy *nextZombie = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
                    if (nextZombie) {
                        if (clearedLevel == 1 && gameManager_->getCurrentLevel() == 2) {
                            zombieLevelTransitionPending_ = true;
                            battleActive_ = false;
                            statusMessage_ = QStringLiteral("Level 1 cleared. Advanced infected are ahead.");
                            statusDisplayTime_ = 2.5;
                            pushHostCombatState(true);
                            update();
                            emit battleFinished();
                            return;
                        }

                        battleEndDelay_ = 0.0;
                        zombieSpawnDelay_ = 5.0;
                        enemyProjectileActive_ = false;
                        enemyProjectileExploding_ = false;
                        enemyProjectileAnimTime_ = 0.0;
                        enemyProjectileFrame_ = 0;
                        enemyProjectileExplosionTime_ = 0.0;
                        enemyHpDisplay_ = nextZombie->getMaxHealth() > 0
                            ? static_cast<double>(nextZombie->getHealth()) / nextZombie->getMaxHealth()
                            : 1.0;
                        prepareZombieSpawn(false);
                        statusMessage_ = gameManager_->getCurrentLevel() == 2
                            ? QStringLiteral("Advanced infected are breaking through...")
                            : QStringLiteral("Another infected is shambling into the street...");
                        statusDisplayTime_ = 2.4;
                        update();
                        return;
                    }
                }

                completeZombieOutbreak();
                return;
            }

            if (player->isAlive() && !enemy->isAlive() && gameManager_->advanceFinalGuardianWave()) {
                Enemy *nextGuardian = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
                if (nextGuardian) {
                    loadEnemyAnimations(nextGuardian->getEnemyType());
                    loadBattleProfilePortraits();
                    enemyX_ = qMax(ARENA_LEFT_X + 260.0, double(width()) - ARENA_RIGHT_MARGIN - 110.0);
                    enemyCooldown_ = 0.85;
                    introLockTime_ = 0.0;
                    battleEndDelay_ = 0.0;
                    enemyProjectileActive_ = false;
                    enemyProjectileExploding_ = false;
                    enemyProjectileAnimTime_ = 0.0;
                    enemyProjectileFrame_ = 0;
                    enemyProjectileExplosionTime_ = 0.0;
                    enemyAiDecision_ = FighterAiDecision();
                    enemyAiDecisionTimer_ = 0.0;
                    enemyAiPlayerAttackMemory_ = 0.0;
                    enemyAiPlayerMissMemory_ = 0.0;
                    enemyAiPlayerHealMemory_ = 0.0;
                    enemyAiEnemyDamageMemory_ = 0.0;
                    enemyAiLastEnemyHp_ = nextGuardian->getHealth();
                    enemyAiLastAttack_ = AnimationState::ATTACK1;
                    statusMessage_ = QString("%1 lunges forward to guard the king!")
                                         .arg(QString::fromStdString(nextGuardian->getName()));
                    statusDisplayTime_ = 2.0;
                    if (enemyAnimChar_) {
                        enemyAnimChar_->reset();
                        enemyAnimChar_->setFacingLeft(playerX_ < enemyX_);
                        enemyAnimChar_->setAnimationState(AnimationState::RUN);
                    }
                    update();
                    return;
                }
            }
            battleActive_ = false;
            pushHostCombatState(true);
            emit battleFinished();
        }
        pushHostCombatState(false);
        recordHighlightReplayFrame(dt);
        update();
        return;
    }
    
    // Update cooldowns
    playerCooldown_ = qMax(0.0, playerCooldown_ - dt);
    enemyCooldown_ = qMax(0.0, enemyCooldown_ - dt);
    healCooldown_ = qMax(0.0, healCooldown_ - dt);
    enemyHealCooldown_ = qMax(0.0, enemyHealCooldown_ - dt);
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

    if (introLockTime_ > 0.0) {
        attackPressed_ = false;
        healPressed_ = false;
        if (gameManager_->isZombieMode() && zombieIntroActive_) {
            zombieIntroTime_ = qMax(0.0, zombieIntroTime_ - dt);
            if (zombieIntroTime_ > 6.0) {
                statusMessage_ = QStringLiteral("The city is full of zombies...");
            } else if (zombieIntroTime_ > 2.2) {
                statusMessage_ = QStringLiteral("Four infected split into the alleys.");
            } else {
                statusMessage_ = QStringLiteral("Level 1 begins. Clear the first street.");
            }
            statusDisplayTime_ = 1.0;
            if (zombieIntroTime_ <= 0.0) {
                zombieIntroActive_ = false;
                zombieSpawnDelay_ = 2.2;
                refreshArenaBackground();
                prepareZombieSpawn(true);
            }
        } else if (gameManager_->isZombieMode()) {
            updateZombieEntry(dt);
        }
        if (playerAnimChar_) {
            playerAnimChar_->setAnimationState(AnimationState::IDLE);
            playerAnimChar_->setFacingLeft(enemyX_ < playerX_);
        }
        if (enemyAnimChar_) {
            enemyAnimChar_->setAnimationState(AnimationState::IDLE);
            enemyAnimChar_->setFacingLeft(playerX_ < enemyX_);
        }
        pushHostCombatState(false);
        update();
        return;
    }

    if (gameManager_->isZombieMode() && zombieSpawnDelay_ > 0.0) {
        zombieSpawnDelay_ = qMax(0.0, zombieSpawnDelay_ - dt);
        attackPressed_ = false;
        healPressed_ = false;
        updateZombieEntry(dt);
        if (zombieSpawnDelay_ <= 0.0) {
            statusMessage_ = QStringLiteral("Zombie released. Hold the street.");
            statusDisplayTime_ = 1.5;
            enemyCooldown_ = 0.6;
        }
        pushHostCombatState(false);
        recordHighlightReplayFrame(dt);
        update();
        return;
    }

    // Player movement
    updatePlayerMovement(dt);

    quint8 remoteInputBits = 0;
    if (lanBridgeActive_ && lanSessionManager_) {
        if (lanHostAuthority_) {
            const qint64 lastRemoteInputMs = lanSessionManager_->latestRemoteCombatInputReceivedMs();
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            const bool remoteInputFresh = lastRemoteInputMs > 0 && (nowMs - lastRemoteInputMs) <= 180;
            remoteInputBits = remoteInputFresh
                ? lanSessionManager_->latestRemoteCombatInput().inputBits
                : 0;
            updateRemoteLanMovement(dt, remoteInputBits);
        }
    } else {
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

    if (player->isAlive() && enemy->isAlive()) {
        if (lanBridgeActive_ && lanHostAuthority_) {
            const quint8 pressedThisFrame = static_cast<quint8>(remoteInputBits & ~lastRemoteLanInputBits_);
            if ((pressedThisFrame & LanInputHeal) != 0) {
                tryRemoteLanHeal();
            }
            if ((pressedThisFrame & LanInputAttack1) != 0) {
                tryRemoteLanAttack(AnimationState::ATTACK1);
            } else if ((pressedThisFrame & LanInputAttack2) != 0) {
                tryRemoteLanAttack(AnimationState::ATTACK2);
            } else if ((pressedThisFrame & LanInputAttack3) != 0) {
                tryRemoteLanAttack(AnimationState::ATTACK3);
            }
            lastRemoteLanInputBits_ = remoteInputBits;
        } else if (enemyCooldown_ <= 0.0) {
            tryEnemyAttack(dt);
        }
    }

    updateArcenProjectile(dt);
    updateEnemyProjectile(dt);
    pushHostCombatState(false);
    recordHighlightReplayFrame(dt);
    update();
}

void BattleWidget::drawLevelTransitionPortal(QPainter &painter) {
    if (levelTransitionPortalSpriteSheet_.isNull()) {
        return;
    }

    const int frameCount = 6;
    const int frameWidth = qMax(1, levelTransitionPortalSpriteSheet_.width() / frameCount);
    const int frameHeight = levelTransitionPortalSpriteSheet_.height();
    const int frameIndex = qBound(0, static_cast<int>(levelTransitionTime_ / 0.11) % frameCount, frameCount - 1);
    const QPixmap frame = levelTransitionPortalSpriteSheet_.copy(frameIndex * frameWidth, 0, frameWidth, frameHeight);

    const QRect visibleBounds = opaqueBounds(frame);
    const qreal desiredHeight = qMax(150.0, height() * 0.40);
    const qreal scale = desiredHeight / qMax(1, visibleBounds.height());
    const qreal scaledWidth = frame.width() * scale;
    const qreal scaledHeight = frame.height() * scale;
    const qreal visibleCenterX = visibleBounds.x() + visibleBounds.width() * 0.5;
    const qreal visibleBottomY = visibleBounds.y() + visibleBounds.height();
    const qreal portalCenterX = width() * 0.98;
    const qreal portalGroundY = groundY() + height() * 0.012;
    const qreal drawX = portalCenterX - visibleCenterX * scale;
    const qreal drawY = portalGroundY - visibleBottomY * scale;
    const QRect targetRect = QRectF(drawX, drawY, scaledWidth, scaledHeight).toRect();

    painter.save();
    painter.translate(targetRect.center().x(), 0.0);
    painter.scale(-1.0, 1.0);
    painter.translate(-targetRect.center().x(), 0.0);
    painter.drawPixmap(targetRect, frame, QRectF(0, 0, frame.width(), frame.height()));
    painter.restore();
}

void BattleWidget::drawFinalRescueTransition(QPainter &painter) {
    if (!finalRescueTransitionActive_) {
        return;
    }

    const double floorY = groundY();
    const double runStart = 4.4;
    const double transitionDuration = 7.6;
    const double runProgress = qBound(0.0, (levelTransitionTime_ - runStart) / qMax(0.1, transitionDuration - runStart), 1.0);
    const double kingX = levelTransitionStartKingX_ + (width() * 0.99 - levelTransitionStartKingX_) * runProgress;
    const bool kingRunning = levelTransitionTime_ >= runStart;
    const double kingOpacity = 1.0 - qBound(0.0, (levelTransitionTime_ - 7.0) / 0.45, 1.0);
    const double stageHeight = qMax(180.0, (height() - 160.0) * 0.34);
    const double kingHeight = stageHeight * 0.95;

    const QPixmap kingSprite = kingRunning
        ? frameFromSheet("assets/story/king_1/Sprites/Run.png", 8, levelTransitionTime_ - runStart, 110)
        : frameFromSheet("assets/story/king_1/Sprites/Idle.png", 8, levelTransitionTime_, 170);

    drawGroundedSprite(painter, kingSprite, kingX, floorY - 6.0, kingHeight, false, kingOpacity);
}

FighterAiContext BattleWidget::buildEnemyAiContext(double distance) const {
    FighterAiContext context;
    context.distance = distance;
    context.attackRange = ATTACK_RANGE;
    context.difficulty = gameManager_ ? gameManager_->getDifficulty() : DifficultyLevel::NORMAL;

    const Player *player = gameManager_ ? gameManager_->getPlayer() : nullptr;
    const Enemy *enemy = gameManager_ ? gameManager_->getCurrentEnemy() : nullptr;
    if (player) {
        context.playerHp = player->getHealth();
        context.playerMaxHp = player->getMaxHealth();
    }
    if (enemy) {
        context.enemyHp = enemy->getHealth();
        context.enemyMaxHp = enemy->getMaxHealth();
        context.enemyType = enemy->getEnemyType();
    }

    context.opponentIsPlayerRival = gameManager_
        && (gameManager_->isLanDuel()
            || (gameManager_->isDuelMode()
                && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE));
    if (context.opponentIsPlayerRival && gameManager_) {
        context.rivalPlayerType = gameManager_->getLanOpponentPlayerType();
    }

    const FighterAiProfile profile = FighterAiBrain::profileForContext(context);
    context.projectileReady = profile.hasProjectile && !enemyProjectileActive_;
    context.enemyHealReady = enemyHealCooldown_ <= 0.0;
    context.playerRecentlyAttacked = enemyAiPlayerAttackMemory_ > 0.0;
    context.playerRecentlyMissed = enemyAiPlayerMissMemory_ > 0.0;
    context.playerRecentlyHealed = enemyAiPlayerHealMemory_ > 0.0;
    context.enemyRecentlyDamaged = enemyAiEnemyDamageMemory_ > 0.0;
    return context;
}

void BattleWidget::updateEnemyAiMemory(double dt) {
    enemyAiDecisionTimer_ = qMax(0.0, enemyAiDecisionTimer_ - dt);
    enemyAiPlayerAttackMemory_ = qMax(0.0, enemyAiPlayerAttackMemory_ - dt);
    enemyAiPlayerMissMemory_ = qMax(0.0, enemyAiPlayerMissMemory_ - dt);
    enemyAiPlayerHealMemory_ = qMax(0.0, enemyAiPlayerHealMemory_ - dt);
    enemyAiEnemyDamageMemory_ = qMax(0.0, enemyAiEnemyDamageMemory_ - dt);

    const Enemy *enemy = gameManager_ ? gameManager_->getCurrentEnemy() : nullptr;
    if (!enemy || !enemy->isAlive()) {
        enemyAiLastEnemyHp_ = enemy ? enemy->getHealth() : 0;
        return;
    }

    const int currentHp = enemy->getHealth();
    if (enemyAiLastEnemyHp_ > 0 && currentHp < enemyAiLastEnemyHp_) {
        enemyAiEnemyDamageMemory_ = 2.0;
        enemyAiDecisionTimer_ = 0.0;
    }
    enemyAiLastEnemyHp_ = currentHp;
}

void BattleWidget::refreshEnemyAiDecision(double dt, double distance) {
    updateEnemyAiMemory(dt);
    if (lanBridgeActive_ || enemyAiDecisionTimer_ > 0.0) {
        return;
    }

    enemyAiDecision_ = FighterAiBrain::chooseDecision(buildEnemyAiContext(distance));
    enemyAiDecisionTimer_ = qMax(0.12, enemyAiDecision_.decisionDuration);
    enemyAiLastAttack_ = enemyAiDecision_.attackState;
}

void BattleWidget::resetZombieModeState() {
    zombieSpawnDelay_ = 0.0;
    zombieIntroTime_ = 0.0;
    zombieIntroActive_ = false;
    zombieEnteringFromLeft_ = false;
    zombieCityCleaned_ = false;
    zombieLevelTransitionPending_ = false;
}

void BattleWidget::prepareZombieSpawn(bool firstSpawn) {
    if (!gameManager_ || !gameManager_->isZombieMode()) {
        return;
    }

    zombieCityCleaned_ = false;
    refreshArenaBackground();
    Enemy *currentEnemy = gameManager_->getCurrentEnemy();
    if (!currentEnemy) {
        return;
    }
    loadEnemyAnimations(currentEnemy->getEnemyType());
    loadBattleProfilePortraits();

    zombieEnteringFromLeft_ = zombieEntryFromLeftForEnemy(currentEnemy->getEnemyType());
    enemyX_ = zombieEnteringFromLeft_ ? ARENA_LEFT_X - 100.0 : double(width()) + 100.0;
    enemyCooldown_ = (firstSpawn ? introLockTime_ : zombieSpawnDelay_) + 0.65;
    enemyAiDecision_ = FighterAiDecision();
    enemyAiDecisionTimer_ = 0.0;
    enemyAiPlayerAttackMemory_ = 0.0;
    enemyAiPlayerMissMemory_ = 0.0;
    enemyAiPlayerHealMemory_ = 0.0;
    enemyAiEnemyDamageMemory_ = 0.0;
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    enemyAiLastEnemyHp_ = enemy ? enemy->getHealth() : 0;
    enemyAiLastAttack_ = AnimationState::ATTACK1;
    if (enemyAnimChar_) {
        enemyAnimChar_->reset();
        enemyAnimChar_->setAnimationState(AnimationState::RUN);
        enemyAnimChar_->setFacingLeft(!zombieEnteringFromLeft_);
    }
    statusMessage_ = firstSpawn
        ? QStringLiteral("The infected are entering the city...")
        : QStringLiteral("Another infected breaks through the edge.");
    statusDisplayTime_ = firstSpawn ? 3.4 : 2.0;
}

bool BattleWidget::zombieEntryFromLeftForEnemy(EnemyType type) const {
    switch (type) {
        case EnemyType::ZOMBIE_1:
        case EnemyType::ZOMBIE_2:
        case EnemyType::ADVANCED_ZOMBIE_1:
        case EnemyType::ADVANCED_ZOMBIE_2:
            return true;
        case EnemyType::ZOMBIE_3:
        case EnemyType::ZOMBIE_4:
        case EnemyType::ADVANCED_ZOMBIE_3:
            return false;
        default:
            return true;
    }
}

void BattleWidget::updateZombieEntry(double dt) {
    if (!gameManager_ || !gameManager_->isZombieMode()) {
        return;
    }

    const double arenaLeft = ARENA_LEFT_X + 85.0;
    const double arenaRight = qMax(arenaLeft + 260.0, double(width()) - ARENA_RIGHT_MARGIN - 90.0);
    const double targetX = zombieEnteringFromLeft_ ? arenaLeft : arenaRight;
    const double direction = targetX > enemyX_ ? 1.0 : -1.0;
    const double step = 190.0 * dt;
    if (std::abs(targetX - enemyX_) <= step) {
        enemyX_ = targetX;
    } else {
        enemyX_ += direction * step;
    }

    if (enemyAnimChar_) {
        enemyAnimChar_->setAnimationState(AnimationState::RUN);
        enemyAnimChar_->setFacingLeft(!zombieEnteringFromLeft_);
    }
}

void BattleWidget::completeZombieOutbreak() {
    zombieCityCleaned_ = true;
    battleEndDelay_ = 3.2;
    refreshArenaBackground();
    statusMessage_ = QStringLiteral("The city is cleaned from zombies.");
    statusDisplayTime_ = 3.2;
    if (playerAnimChar_) {
        playerAnimChar_->setAnimationState(AnimationState::IDLE);
    }
    update();
}

PlayerType BattleWidget::selectedPlayerType() const {
    return gameManager_ ? gameManager_->getSelectedPlayerType() : PlayerType::KNIGHT;
}

void BattleWidget::refreshCombinedMovementInput() {
    movingLeft_ = keyboardMovingLeft_ || controllerMovingLeft_;
    movingRight_ = keyboardMovingRight_ || controllerMovingRight_;
}

void BattleWidget::clearLocalInputState() {
    keyboardMovingLeft_ = false;
    keyboardMovingRight_ = false;
    controllerMovingLeft_ = false;
    controllerMovingRight_ = false;
    movingLeft_ = false;
    movingRight_ = false;
    attackPressed_ = false;
    healPressed_ = false;
    controllerInput_.resetTransientState();
}

bool BattleWidget::queuePlayerAttack(AnimationState attackState, bool showQueuedStatus) {
    PlayerAction action = PlayerAction::ATTACK1;
    QString unavailableMessage = QStringLiteral("This character cannot use Attack 1");
    QString queuedMessage;

    if (attackState == AnimationState::ATTACK2) {
        action = PlayerAction::ATTACK2;
        unavailableMessage = QStringLiteral("This character has no Attack 2");
        queuedMessage = QStringLiteral("Attack 2 queued");
    } else if (attackState == AnimationState::ATTACK3) {
        action = PlayerAction::ATTACK3;
        unavailableMessage = QStringLiteral("This character has no Attack 3");
        queuedMessage = QStringLiteral("Attack 3 queued");
    }

    if (!InputHandler::canPerformAction(selectedPlayerType(), action)) {
        statusMessage_ = unavailableMessage;
        statusDisplayTime_ = 1.2;
        return false;
    }

    attackPressed_ = true;
    queuedPlayerAttackState_ = attackState;
    if (showQueuedStatus && !queuedMessage.isEmpty()) {
        statusMessage_ = queuedMessage;
        statusDisplayTime_ = 0.8;
    }
    return true;
}

void BattleWidget::updateControllerInput() {
    controllerInput_.poll();
    controllerMovingLeft_ = controllerInput_.moveLeft();
    controllerMovingRight_ = controllerInput_.moveRight();
    refreshCombinedMovementInput();

    if (!controllerInput_.isAvailable()) {
        return;
    }

    if (controllerInput_.wasPressed(ControllerButton::Pause)) {
        emit pauseRequested();
        return;
    }

    if (!battleActive_ || pausePressed_) {
        return;
    }

    if (controllerInput_.wasPressed(ControllerButton::Attack1)) {
        queuePlayerAttack(AnimationState::ATTACK1, false);
    } else if (controllerInput_.wasPressed(ControllerButton::Attack2)) {
        queuePlayerAttack(AnimationState::ATTACK2, true);
    } else if (controllerInput_.wasPressed(ControllerButton::Attack3)) {
        queuePlayerAttack(AnimationState::ATTACK3, true);
    }

    if (controllerInput_.wasPressed(ControllerButton::Jump)) {
        if (InputHandler::canPerformAction(selectedPlayerType(), PlayerAction::JUMP)) {
            statusMessage_ = "Jump action triggered";
            statusDisplayTime_ = 0.8;
        } else {
            statusMessage_ = "This character cannot jump";
            statusDisplayTime_ = 1.2;
        }
    }

    if (controllerInput_.wasPressed(ControllerButton::Heal)) {
        healPressed_ = true;
    }
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
                if (st != AnimationState::RUN && soundManager_) {
                    soundManager_->playRun();
                }
                playerAnimChar_->setAnimationState(AnimationState::RUN);
            } else {
                playerAnimChar_->setAnimationState(AnimationState::IDLE);
            }
        }
    }
}

void BattleWidget::updateEnemyAI(double dt) {
    double distToPlayer = playerX_ - enemyX_;
    const double absDistToPlayer = std::abs(distToPlayer);
    refreshEnemyAiDecision(dt, absDistToPlayer);

    double moveAmount = 150.0 * dt; // Enemy is slightly slower
    const bool duelPlayerRival = gameManager_
        && (gameManager_->isLanDuel()
            || (gameManager_->isDuelMode()
                && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE));
    if (duelPlayerRival) {
        switch (gameManager_->getLanOpponentPlayerType()) {
            case PlayerType::HUNTRESS:
            case PlayerType::MARTIAL:
            case PlayerType::MARTIAL_HERO:
                moveAmount = 178.0 * dt;
                break;
            case PlayerType::ARCEN:
            case PlayerType::WIZARD:
                moveAmount = 142.0 * dt;
                break;
            case PlayerType::KNIGHT:
            case PlayerType::MEDIEVAL_WARRIOR:
                moveAmount = 148.0 * dt;
                break;
            default:
                moveAmount = 160.0 * dt;
                break;
        }
    }

    moveAmount *= enemyAiDecision_.moveSpeedMultiplier;

    bool enemyMoved = false;
    auto approachPlayer = [&](double amount) {
        if (distToPlayer > 0) {
            const double next = qMin(double(width()) - 60.0, enemyX_ + amount);
            enemyMoved = enemyMoved || std::abs(next - enemyX_) > 0.1;
            enemyX_ = next;
        } else {
            const double next = qMax(60.0, enemyX_ - amount);
            enemyMoved = enemyMoved || std::abs(next - enemyX_) > 0.1;
            enemyX_ = next;
        }
    };
    auto retreatFromPlayer = [&](double amount) {
        if (distToPlayer > 0) {
            const double next = qMax(60.0, enemyX_ - amount);
            enemyMoved = enemyMoved || std::abs(next - enemyX_) > 0.1;
            enemyX_ = next;
        } else {
            const double next = qMin(double(width()) - 60.0, enemyX_ + amount);
            enemyMoved = enemyMoved || std::abs(next - enemyX_) > 0.1;
            enemyX_ = next;
        }
    };

    const FighterAiProfile profile = FighterAiBrain::profileForContext(buildEnemyAiContext(absDistToPlayer));
    switch (enemyAiDecision_.state) {
        case FighterAiState::Approach:
        case FighterAiState::Punish:
            approachPlayer(moveAmount);
            break;
        case FighterAiState::Retreat:
            retreatFromPlayer(moveAmount);
            break;
        case FighterAiState::Bait: {
            const double baitRange = ATTACK_RANGE + 38.0;
            if (absDistToPlayer < baitRange - 12.0) {
                retreatFromPlayer(moveAmount * 0.78);
            } else if (absDistToPlayer > baitRange + 46.0) {
                approachPlayer(moveAmount * 0.52);
            }
            break;
        }
        case FighterAiState::HoldRange:
        case FighterAiState::ProjectileAttack:
            if (absDistToPlayer < profile.idealMinRange) {
                retreatFromPlayer(moveAmount * 0.72);
            } else if (absDistToPlayer > profile.idealMaxRange) {
                approachPlayer(moveAmount * 0.62);
            }
            break;
        case FighterAiState::MeleeAttack:
            if (absDistToPlayer > ATTACK_RANGE * 0.88) {
                approachPlayer(moveAmount * 0.42);
            }
            break;
        case FighterAiState::Recover:
            break;
    }

    if (enemyAnimChar_) {
        const AnimationState st = enemyAnimChar_->getCurrentState();
        const bool locked = (st == AnimationState::ATTACK1 || st == AnimationState::ATTACK2 ||
                             st == AnimationState::ATTACK3 || st == AnimationState::STRONG_ATTACK ||
                             st == AnimationState::HURT || st == AnimationState::DEATH);
        if (!locked) {
            if (enemyMoved) {
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

    if (!player || !enemy || !player->isAlive() || !enemy->isAlive()) return;

    ++levelBattleReport_.playerAttacks;
    enemyAiPlayerAttackMemory_ = 1.0;

    // Trigger player attack animation regardless of distance
    if (playerAnimChar_) {
        playerAnimChar_->setAnimationState(queuedPlayerAttackState_);
    }

    const PlayerType playerType = gameManager_ ? gameManager_->getSelectedPlayerType() : PlayerType::KNIGHT;
    if (playerType == PlayerType::ARCEN) {
        if (soundManager_) {
            soundManager_->playAttack();
        }
        const int damage = arcenArrowDamage(player->calculateDamage(), queuedPlayerAttackState_);

        if (!arcenProjectileActive_) {
            spawnArcenProjectile(damage);
            statusMessage_ = "Arrow fired!";
            statusDisplayTime_ = 0.7;
        } else {
            statusMessage_ = "Arrow already in flight";
            statusDisplayTime_ = 0.7;
            ++levelBattleReport_.playerMisses;
            enemyAiPlayerMissMemory_ = 1.2;
        }

        playerCooldown_ = PLAYER_ATTACK_COOLDOWN;
        return;
    }

    double distToEnemy = std::abs(enemyX_ - playerX_);
    if (soundManager_) {
        soundManager_->playAttack();
    }
    
    if (distToEnemy <= ATTACK_RANGE) {
        int damage = player->calculateDamage();
        if (queuedPlayerAttackState_ == AnimationState::ATTACK2) {
            damage = static_cast<int>(damage * 1.2);
        } else if (queuedPlayerAttackState_ == AnimationState::ATTACK3) {
            damage = static_cast<int>(damage * 1.35);
        }

        const int playerHpBefore = player->getHealth();
        const int enemyHpBefore = enemy->getHealth();
        enemy->takeDamage(damage);
        score_ += damage * 10;
        levelBattleReport_.damageDealt += qMax(0, damage);
        ++levelBattleReport_.playerHits;
        if (gameManager_ && gameManager_->isLanDuel() && lanHostAuthority_) {
            remoteBattleReport_.damageTaken += qMax(0, damage);
        }
        const bool highlightAccepted =
            considerPlayerHighlight(buildHighlightCandidate(highlightAttackTypeForAnimation(queuedPlayerAttackState_),
                                                           damage,
                                                           false,
                                                           playerHpBefore,
                                                           enemyHpBefore,
                                                           enemy->getHealth()));
        
        if (soundManager_) soundManager_->playHit();
        // Trigger enemy hurt animation
        if (enemyAnimChar_) {
            enemyAnimChar_->takeDamage();
        }
        
        statusMessage_ = QString("Hit! Dealt %1 damage!").arg(damage);
        statusDisplayTime_ = 1.0;

        if (highlightAccepted) {
            captureHighlightFrame(true);
            beginHighlightClipCapture(captureHighlightReplayFrame());
        }
        
        if (!enemy->isAlive()) {

            if (soundManager_) soundManager_->playEnemyDeath();
            statusMessage_ = "Victory! Enemy defeated!";
            statusDisplayTime_ = 3.0;
            if (enemyAnimChar_) {
                enemyAnimChar_->kill();
            }
        }
    } else {
        statusMessage_ = "Miss! Too far away!";
        statusDisplayTime_ = 1.0;
        ++levelBattleReport_.playerMisses;
        enemyAiPlayerMissMemory_ = 1.35;
    }

    playerCooldown_ = PLAYER_ATTACK_COOLDOWN;
}

void BattleWidget::tryEnemyAttack(double dt) {
    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    
    if (!player || !enemy || !player->isAlive() || !enemy->isAlive()) return;
    
    const EnemyType enemyType = enemy->getEnemyType();
    double distToPlayer = std::abs(playerX_ - enemyX_);
    refreshEnemyAiDecision(0.0, distToPlayer);
    const FighterAiContext aiContext = buildEnemyAiContext(distToPlayer);
    const FighterAiProfile aiProfile = FighterAiBrain::profileForContext(aiContext);

    const bool demonSlayerFireball = aiContext.opponentIsPlayerRival &&
                                     aiContext.rivalPlayerType == PlayerType::DEMON_SLAYER;
    const bool prefersProjectile = (enemyType == EnemyType::FIRE_WORM ||
                                    enemyType == EnemyType::NIGHTWEAVER ||
                                    enemyType == EnemyType::FLYING_DEMON ||
                                    demonSlayerFireball ||
                                    (aiContext.opponentIsPlayerRival &&
                                     aiContext.rivalPlayerType == PlayerType::ARCEN));
    double projectileRange = ATTACK_RANGE + 180.0;
    if (enemyType == EnemyType::FIRE_WORM) {
        projectileRange = ATTACK_RANGE + 360.0;
    } else if (enemyType == EnemyType::NIGHTWEAVER) {
        projectileRange = ATTACK_RANGE + 260.0;
    } else if (enemyType == EnemyType::FLYING_DEMON) {
        projectileRange = ATTACK_RANGE + 240.0;
    } else if (aiContext.opponentIsPlayerRival && aiContext.rivalPlayerType == PlayerType::ARCEN) {
        projectileRange = (ATTACK_RANGE + 430.0) * 3.0;
    } else if (demonSlayerFireball) {
        projectileRange = ATTACK_RANGE + 330.0;
    }

    const bool shouldUseProjectile =
        enemyAiDecision_.wantsProjectile &&
        prefersProjectile &&
        !enemyProjectileActive_ &&
        distToPlayer <= projectileRange &&
        distToPlayer > ATTACK_RANGE * 0.65 &&
        aiProfile.hasProjectile;

    if (shouldUseProjectile) {
        int damage = enemy->calculateDamage();
        if (enemyType == EnemyType::NIGHTWEAVER) {
            damage = static_cast<int>(damage * 1.15);
        } else if (enemyType == EnemyType::FLYING_DEMON) {
            damage = static_cast<int>(damage * 1.1);
        } else if (demonSlayerFireball) {
            damage = static_cast<int>(damage * 1.12);
        } else if (aiContext.opponentIsPlayerRival && aiContext.rivalPlayerType == PlayerType::ARCEN) {
            damage = arcenArrowDamage(damage, enemyAiDecision_.attackState);
        }

        if (enemyAnimChar_) {
            enemyAnimChar_->setAnimationState(AnimationState::ATTACK2);
        }

        spawnEnemyProjectile(enemyType, damage);
        enemyCooldown_ = (ENEMY_ATTACK_COOLDOWN + 0.25) * enemyAiDecision_.attackCooldownMultiplier;
        if (gameManager_ && gameManager_->isLanDuel()) {
            statusMessage_ = QString("%1 launches a ranged strike!")
                                 .arg(QString::fromStdString(enemy->getName()));
        } else if (aiContext.opponentIsPlayerRival && aiContext.rivalPlayerType == PlayerType::ARCEN) {
            statusMessage_ = QString("%1 fires an arrow!").arg(QString::fromStdString(enemy->getName()));
        } else if (demonSlayerFireball) {
            statusMessage_ = QString("%1 throws a fireball!").arg(QString::fromStdString(enemy->getName()));
        } else {
            statusMessage_ = enemyType == EnemyType::FIRE_WORM ? "Fire Worm launches a fireball!"
                          : enemyType == EnemyType::FLYING_DEMON ? "Flying Demon hurls a hellfire orb!"
                                                                 : "Nightweaver fires a shadow bolt!";
        }
        statusDisplayTime_ = 1.3;
        return;
    }

    if (enemyAiDecision_.wantsHeal &&
        enemyHealCooldown_ <= 0.0 &&
        enemy->getHealth() < enemy->getMaxHealth()) {
        const double difficultyScale =
            aiContext.difficulty == DifficultyLevel::HARD ? 0.20 :
            aiContext.difficulty == DifficultyLevel::EASY ? 0.13 : 0.16;
        const int healAmount = qBound(12, static_cast<int>(enemy->getMaxHealth() * difficultyScale), 34);
        enemy->takeDamage(-healAmount);
        enemyHealCooldown_ = 6.5;
        enemyCooldown_ = (ENEMY_ATTACK_COOLDOWN * 0.65) * enemyAiDecision_.attackCooldownMultiplier;
        enemyAiEnemyDamageMemory_ = 0.0;
        enemyAiLastEnemyHp_ = enemy->getHealth();
        if (enemyAnimChar_) {
            enemyAnimChar_->setAnimationState(AnimationState::IDLE);
        }
        statusMessage_ = QString("%1 steps back and heals %2 HP.")
                             .arg(QString::fromStdString(enemy->getName()))
                             .arg(healAmount);
        statusDisplayTime_ = 1.3;
        return;
    }

    if (!enemyAiDecision_.wantsAttack &&
        enemyAiDecision_.state != FighterAiState::MeleeAttack &&
        enemyAiDecision_.state != FighterAiState::Punish) {
        enemyCooldown_ = 0.28 * enemyAiDecision_.attackCooldownMultiplier;
        return;
    }

    if (distToPlayer <= ATTACK_RANGE) {
        int damage = enemy->calculateDamage();
        AnimationState enemyAttackAnim = enemyAiDecision_.attackState;
        if (enemyAttackAnim == AnimationState::ATTACK2) {
            damage = static_cast<int>(damage * 1.15);
        } else if (enemyAttackAnim == AnimationState::ATTACK3) {
            damage = static_cast<int>(damage * 1.3);
        }

        player->takeDamage(damage);
        if (soundManager_) {
            soundManager_->playHit();
        }
        
        // Trigger enemy attack animation
        if (enemyAnimChar_) {
            enemyAnimChar_->setAnimationState(enemyAttackAnim);
        }
        
        // Trigger player hurt animation
        if (playerAnimChar_) {
            playerAnimChar_->takeDamage();
        }
        
        enemyCooldown_ = ENEMY_ATTACK_COOLDOWN * enemyAiDecision_.attackCooldownMultiplier;
        statusMessage_ = (gameManager_ && gameManager_->isDuelMode())
            ? QString("%1 lands %2 damage!").arg(QString::fromStdString(enemy->getName())).arg(damage)
            : QString("Enemy deals %1 damage!").arg(damage);
        statusDisplayTime_ = 1.5;
        
        if (!player->isAlive()) {
            if (soundManager_) soundManager_->playDeath();

            statusMessage_ = "Defeat! You were defeated!";
            statusDisplayTime_ = 3.0;
            if (playerAnimChar_) {
                playerAnimChar_->kill();
            }
        }
    } else {
        // Enemy moves closer instead of attacking
        enemyCooldown_ = 0.3 * enemyAiDecision_.attackCooldownMultiplier;
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
            ensureArcenArrowAssetsLoaded();
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

void BattleWidget::loadOpponentCharacterAnimations(PlayerType type) {
    if (!enemyAnimManager_) return;

    enemyFallbackSprite_ = QPixmap();
    QString basePath;

    switch (type) {
        case PlayerType::KNIGHT:
            basePath = resolveAssetPath("assets/players/Knight/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 7, basePath + "/IDLE.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/RUN.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/ATTACK 1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 5, basePath + "/ATTACK 2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, basePath + "/ATTACK 3.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 12, basePath + "/DEATH.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/HURT.png", false, 90);
            break;

        case PlayerType::DEMON_SLAYER:
            basePath = resolveAssetPath("assets/players/Demon_Slayer/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 4, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 4, basePath + "/Attack1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 4, basePath + "/Attack2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 4, basePath + "/Attack2.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 7, basePath + "/Death.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take hit.png", false, 90);
            enemyProjectileMoveSprite_ = QPixmap(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Fire Ball/Move.png"));
            enemyProjectileExplodeSprite_ = QPixmap(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Fire Ball/Explosion.png"));
            break;

        case PlayerType::FANTASY_WARRIOR:
            basePath = resolveAssetPath("assets/players/Fantasy_Warrior/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 7, basePath + "/Attack1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 7, basePath + "/Attack2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 8, basePath + "/Attack3.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 7, basePath + "/Death.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take hit.png", false, 90);
            break;

        case PlayerType::HUNTRESS:
            basePath = resolveAssetPath("assets/players/Huntress/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 5, basePath + "/Attack1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 5, basePath + "/Attack2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 7, basePath + "/Attack3.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 8, basePath + "/Death.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take hit.png", false, 90);
            break;

        case PlayerType::ARCEN:
            basePath = resolveAssetPath("assets/players/Arcen/Sprites/Character");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/Attack.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 6, basePath + "/Attack.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, basePath + "/Attack.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 10, basePath + "/Death.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Get Hit.png", false, 90);
            break;

        case PlayerType::MARTIAL:
            basePath = resolveAssetPath("assets/players/Martial/Sprite");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 7, basePath + "/Attack1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 6, basePath + "/Attack2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 9, basePath + "/Attack3.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 11, basePath + "/Death.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Take Hit.png", false, 90);
            break;

        case PlayerType::MARTIAL_HERO:
            basePath = resolveAssetPath("assets/players/Martial_Hero/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/Attack1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 6, basePath + "/Attack2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, basePath + "/Attack2.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 6, basePath + "/Death.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/Take Hit.png", false, 90);
            break;

        case PlayerType::MEDIEVAL_WARRIOR:
            basePath = resolveAssetPath("assets/players/Medieval_Warrior/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 10, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 6, basePath + "/Run.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 4, basePath + "/Attack1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 4, basePath + "/Attack2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 5, basePath + "/Attack3.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 9, basePath + "/Death.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 3, basePath + "/Get Hit.png", false, 90);
            break;

        case PlayerType::WIZARD:
            basePath = resolveAssetPath("assets/players/Wizard/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 6, basePath + "/Idle.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/Run.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 8, basePath + "/Attack1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 8, basePath + "/Attack2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 8, basePath + "/Attack2.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 7, basePath + "/Death.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/Hit.png", false, 90);
            break;

        default:
            basePath = resolveAssetPath("assets/players/Knight/Sprites");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 7, basePath + "/IDLE.png", true, 150);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 8, basePath + "/RUN.png", true, 95);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/ATTACK 1.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 5, basePath + "/ATTACK 2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 6, basePath + "/ATTACK 3.png", false, 60);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 12, basePath + "/DEATH.png", false, 110);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/HURT.png", false, 90);
            break;
    }

    QString idlePath;
    int idleFrames = 7;
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
        enemyFallbackSprite_ = idleSheet.copy(0, 0, frameWidth, idleSheet.height());
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

        case EnemyType::ZOMBIE_1:
        case EnemyType::ZOMBIE_2:
        case EnemyType::ZOMBIE_3:
        case EnemyType::ZOMBIE_4: {
            const int zombieIndex = type == EnemyType::ZOMBIE_1 ? 1
                                  : type == EnemyType::ZOMBIE_2 ? 2
                                  : type == EnemyType::ZOMBIE_3 ? 3
                                                                : 4;
            basePath = resolveAssetPath(QString("assets/beasts/zombies/Zombie_%1").arg(zombieIndex));
            const int idleCount = zombieIndex == 4 ? 7 : 6;
            const int walkCount = zombieIndex == 4 ? 12 : 10;
            const int attackCount = zombieIndex == 3 ? 4 : (zombieIndex == 4 ? 10 : 5);
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, idleCount, basePath + "/Idle.png", true, 145);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, walkCount, basePath + "/Walk.png", true, 90);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, attackCount, basePath + "/Attack.png", false, 62);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, attackCount, basePath + "/Attack.png", false, 62);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, attackCount, basePath + "/Attack.png", false, 62);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 5, basePath + "/Dead.png", false, 120);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 4, basePath + "/Hurt.png", false, 90);
            idlePath = basePath + "/Idle.png";
            idleFrames = idleCount;
            break;
        }

        case EnemyType::ADVANCED_ZOMBIE_1:
        case EnemyType::ADVANCED_ZOMBIE_2:
        case EnemyType::ADVANCED_ZOMBIE_3: {
            const int zombieIndex = type == EnemyType::ADVANCED_ZOMBIE_1 ? 1
                                  : type == EnemyType::ADVANCED_ZOMBIE_2 ? 2
                                                                         : 3;
            basePath = resolveAssetPath(QString("assets/beasts/zombies/Advanced_Zombie_%1").arg(zombieIndex));
            const int idleCount = zombieIndex == 1 ? 5 : (zombieIndex == 2 ? 9 : 8);
            const int runCount = zombieIndex == 1 ? 7 : 8;
            const int hurtCount = zombieIndex == 2 ? 5 : 3;
            const int attack1Count = zombieIndex == 3 ? 5 : 4;
            const int attack2Count = 4;
            const int attack3Count = zombieIndex == 3 ? 5 : 4;
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, idleCount, basePath + "/Idle.png", true, 145);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, runCount, basePath + "/Run.png", true, 82);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, attack1Count, basePath + "/Attack_1.png", false, 56);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, attack2Count, basePath + "/Attack_2.png", false, 62);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, attack3Count, basePath + "/Attack_3.png", false, 58);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 5, basePath + "/Dead.png", false, 120);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, hurtCount, basePath + "/Hurt.png", false, 90);
            idlePath = basePath + "/Idle.png";
            idleFrames = idleCount;
            break;
        }

        case EnemyType::BLACK_WEREWOLF:
            basePath = resolveAssetPath("assets/beasts/werewolf/Black_Werewolf");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 120);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 9, basePath + "/Run.png", true, 88);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/Attack_1.png", false, 62);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 4, basePath + "/Attack_2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 5, basePath + "/Attack_3.png", false, 66);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 2, basePath + "/Dead.png", false, 180);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 2, basePath + "/Hurt.png", false, 110);
            idlePath = basePath + "/Idle.png";
            idleFrames = 8;
            break;

        case EnemyType::RED_WEREWOLF:
            basePath = resolveAssetPath("assets/beasts/werewolf/Red_Werewolf");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 120);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 9, basePath + "/Run.png", true, 88);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/Attack_1.png", false, 62);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 4, basePath + "/Attack_2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 5, basePath + "/Attack_3.png", false, 66);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 2, basePath + "/Dead.png", false, 180);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 2, basePath + "/Hurt.png", false, 110);
            idlePath = basePath + "/Idle.png";
            idleFrames = 8;
            break;

        case EnemyType::WHITE_WEREWOLF:
            basePath = resolveAssetPath("assets/beasts/werewolf/White_Werewolf");
            enemyAnimManager_->loadAnimation(AnimationState::IDLE, 8, basePath + "/Idle.png", true, 120);
            enemyAnimManager_->loadAnimation(AnimationState::RUN, 9, basePath + "/Run.png", true, 88);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK1, 6, basePath + "/Attack_1.png", false, 62);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK2, 4, basePath + "/Attack_2.png", false, 70);
            enemyAnimManager_->loadAnimation(AnimationState::ATTACK3, 5, basePath + "/Attack_3.png", false, 66);
            enemyAnimManager_->loadAnimation(AnimationState::DEATH, 2, basePath + "/Dead.png", false, 180);
            enemyAnimManager_->loadAnimation(AnimationState::HURT, 2, basePath + "/Hurt.png", false, 110);
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

void BattleWidget::drawFinalKingStageScene(QPainter &painter) {
    if (!gameManager_ || !gameManager_->isFinalKingStage()) {
        return;
    }

    const Enemy *enemy = gameManager_->getCurrentEnemy();
    if (!enemy) {
        return;
    }

    const double floorY = groundY();
    const double stageHeight = qMax(180.0, (height() - 160.0) * 0.34);
    const double kingHeight = stageHeight * 0.95;
    const double wolfHeight = stageHeight * 0.954;
    const double kingX = width() * 0.90;

    const QPixmap kingSprite = firstFrameFromSheet("assets/story/king_1/Sprites/Idle.png", 8);
    drawGroundedSprite(painter, kingSprite, kingX, floorY - 6.0, kingHeight, false, 0.92);

    const auto drawGuardianIfWaiting = [&](EnemyType type, qreal x) {
        if (enemy->getEnemyType() == type) {
            return;
        }

        QPixmap sprite;
        switch (type) {
            case EnemyType::BLACK_WEREWOLF:
                sprite = firstFrameFromSheet("assets/beasts/werewolf/Black_Werewolf/Idle.png", 8);
                break;
            case EnemyType::RED_WEREWOLF:
                sprite = firstFrameFromSheet("assets/beasts/werewolf/Red_Werewolf/Idle.png", 8);
                break;
            case EnemyType::WHITE_WEREWOLF:
                sprite = firstFrameFromSheet("assets/beasts/werewolf/White_Werewolf/Idle.png", 8);
                break;
            default:
                break;
        }

        drawGroundedSprite(painter, sprite, x, floorY, wolfHeight, true, 0.96);
    };

    if (enemy->getEnemyType() == EnemyType::BLACK_WEREWOLF) {
        drawGuardianIfWaiting(EnemyType::RED_WEREWOLF, width() * 0.78);
        drawGuardianIfWaiting(EnemyType::WHITE_WEREWOLF, width() * 0.86);
    } else if (enemy->getEnemyType() == EnemyType::RED_WEREWOLF) {
        drawGuardianIfWaiting(EnemyType::WHITE_WEREWOLF, width() * 0.84);
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
        if (gameManager_->isLanDuel()) {
            loadOpponentCharacterAnimations(gameManager_->getLanOpponentPlayerType());
        } else if (gameManager_->isDuelMode()
                   && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE) {
            loadOpponentCharacterAnimations(gameManager_->getLanOpponentPlayerType());
        }
    }

    if (player->getPlayerType() == PlayerType::ARCEN ||
        ((gameManager_->isLanDuel()
          || (gameManager_->isDuelMode()
              && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE))
         && gameManager_->getLanOpponentPlayerType() == PlayerType::ARCEN)) {
        ensureArcenArrowAssetsLoaded();
    }
}

void BattleWidget::spawnArcenProjectile(int damage) {
    arcenProjectileActive_ = true;
    arcenProjectileDamage_ = qMax(1, damage);
    ++levelBattleReport_.projectilesFired;
    arcenProjectileAttackType_ = highlightAttackTypeForAnimation(queuedPlayerAttackState_);
    arcenProjectileFacingRight_ = enemyX_ >= playerX_;
    arcenProjectileAnimTime_ = 0.0;
    arcenProjectileFrame_ = 0;

    const qreal arenaHeight = qMax(1.0, static_cast<qreal>(height() - 160));
    const qreal desiredPlayerHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
    arcenProjectileY_ = groundY() - desiredPlayerHeight * ARCEN_ARROW_LAUNCH_HEIGHT_RATIO;
    arcenProjectileX_ = playerX_ + (arcenProjectileFacingRight_ ? 45.0 : -45.0);

    if (soundManager_) soundManager_->playProjectile();

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

    if (lanBridgeActive_ && !lanHostAuthority_) {
        if (arcenProjectileX_ < ARENA_LEFT_X || arcenProjectileX_ > (width() - ARENA_RIGHT_MARGIN)) {
            arcenProjectileActive_ = false;
            arcenProjectileDamage_ = 0;
        }
        return;
    }

    if (arcenProjectileX_ < ARENA_LEFT_X || arcenProjectileX_ > (width() - ARENA_RIGHT_MARGIN)) {
        arcenProjectileActive_ = false;
        ++levelBattleReport_.playerMisses;
        enemyAiPlayerMissMemory_ = 1.35;
        return;
    }

    if (!gameManager_) {
        arcenProjectileActive_ = false;
        return;
    }

    const Player *player = gameManager_->getPlayer();
    if (gameManager_->isLanDuel() && (!player || !player->isAlive())) {
        arcenProjectileActive_ = false;
        arcenProjectileDamage_ = 0;
        return;
    }

    Enemy *enemy = const_cast<Enemy*>(gameManager_->getCurrentEnemy());
    if (!enemy || !enemy->isAlive()) {
        arcenProjectileActive_ = false;
        arcenProjectileDamage_ = 0;
        return;
    }

    const double hitDistance = std::abs(arcenProjectileX_ - enemyX_);
    if (hitDistance <= ARCEN_PROJECTILE_HIT_WIDTH) {
        const int playerHpBefore = player ? player->getHealth() : 0;
        const int enemyHpBefore = enemy->getHealth();
        enemy->takeDamage(arcenProjectileDamage_);
        score_ += arcenProjectileDamage_ * 10;
        levelBattleReport_.damageDealt += qMax(0, arcenProjectileDamage_);
        ++levelBattleReport_.playerHits;
        ++levelBattleReport_.projectilesHit;
        if (soundManager_) {
            soundManager_->playHit();
        }

        if (enemyAnimChar_) {
            enemyAnimChar_->takeDamage();
        }

        const bool highlightAccepted =
            considerPlayerHighlight(buildHighlightCandidate(arcenProjectileAttackType_,
                                                           arcenProjectileDamage_,
                                                           true,
                                                           playerHpBefore,
                                                           enemyHpBefore,
                                                           enemy->getHealth()));

        statusMessage_ = QString("Arrow hit! Dealt %1 damage!").arg(arcenProjectileDamage_);
        statusDisplayTime_ = 1.0;

        if (highlightAccepted) {
            captureHighlightFrame(true);
            beginHighlightClipCapture(captureHighlightReplayFrame());
        }

        if (!enemy->isAlive()) {
            if (soundManager_) soundManager_->playEnemyDeath();

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
    const bool opponentUsesPlayerRival = gameManager_
        && (gameManager_->isLanDuel()
            || (gameManager_->isDuelMode()
                && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE));
    const bool remoteArcenProjectile = opponentUsesPlayerRival
        && gameManager_->getLanOpponentPlayerType() == PlayerType::ARCEN
        && (!gameManager_->isLanDuel() || lanHostAuthority_);
    const bool remoteDemonSlayerFireball = opponentUsesPlayerRival
        && gameManager_->getLanOpponentPlayerType() == PlayerType::DEMON_SLAYER
        && (!gameManager_->isLanDuel() || lanHostAuthority_);

    enemyProjectileActive_ = true;
    enemyProjectileExploding_ = false;
    enemyProjectileUsesArcenArrow_ = remoteArcenProjectile;
    enemyProjectileType_ = type;
    enemyProjectileDamage_ = qMax(1, damage);
    enemyProjectileFacingRight_ = playerX_ >= enemyX_;
    enemyProjectileAnimTime_ = 0.0;
    enemyProjectileFrame_ = 0;
    enemyProjectileExplosionTime_ = 0.0;
    enemyProjectileSpeed_ = remoteArcenProjectile ? 900.0
        : (type == EnemyType::FIRE_WORM) ? 520.0
        : (type == EnemyType::FLYING_DEMON) ? 640.0
        : remoteDemonSlayerFireball ? 620.0
                                            : 760.0;

    const qreal arenaHeight = qMax(1.0, static_cast<qreal>(height() - 160));
    const qreal desiredEnemyHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
    const qreal projectileHeightRatio = remoteArcenProjectile ? ARCEN_ARROW_LAUNCH_HEIGHT_RATIO
                                    : (type == EnemyType::FIRE_WORM) ? 0.28
                                    : (type == EnemyType::FLYING_DEMON) ? 0.72
                                    : remoteDemonSlayerFireball ? 0.62
                                                                        : 0.68;
    enemyProjectileY_ = groundY() - desiredEnemyHeight * projectileHeightRatio;
    enemyProjectileX_ = enemyX_ + (enemyProjectileFacingRight_ ? 46.0 : -46.0);
    if (soundManager_) soundManager_->playProjectile();
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
        const bool demonSlayerFireball = gameManager_
            && (gameManager_->isLanDuel()
                || (gameManager_->isDuelMode()
                    && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE))
            && gameManager_->getLanOpponentPlayerType() == PlayerType::DEMON_SLAYER;
        const int maxMoveFrames = enemyProjectileUsesArcenArrow_ ? 2
                               : (enemyProjectileType_ == EnemyType::FIRE_WORM ||
                                   enemyProjectileType_ == EnemyType::FLYING_DEMON ||
                                   demonSlayerFireball) ? 6 : 4;
        enemyProjectileFrame_ = (enemyProjectileFrame_ + 1) % maxMoveFrames;
    }

    const double dir = enemyProjectileFacingRight_ ? 1.0 : -1.0;
    enemyProjectileX_ += dir * enemyProjectileSpeed_ * dt;

    if (lanBridgeActive_ && !lanHostAuthority_) {
        if (enemyProjectileX_ < ARENA_LEFT_X || enemyProjectileX_ > (width() - ARENA_RIGHT_MARGIN)) {
            enemyProjectileActive_ = false;
            enemyProjectileExploding_ = false;
        }
        return;
    }

    Enemy *enemy = gameManager_ ? const_cast<Enemy*>(gameManager_->getCurrentEnemy()) : nullptr;
    Player *player = gameManager_ ? const_cast<Player*>(gameManager_->getPlayer()) : nullptr;
    if (gameManager_ && gameManager_->isLanDuel() && (!enemy || !enemy->isAlive())) {
        enemyProjectileActive_ = false;
        enemyProjectileExploding_ = false;
        return;
    }
    if (!player || !player->isAlive()) {
        enemyProjectileActive_ = false;
        return;
    }

    if (enemyProjectileX_ < ARENA_LEFT_X || enemyProjectileX_ > (width() - ARENA_RIGHT_MARGIN)) {
        if (gameManager_ && gameManager_->isLanDuel() && lanHostAuthority_) {
            ++remoteBattleReport_.playerMisses;
        }
        enemyProjectileActive_ = false;
        return;
    }

    const double hitDistance = std::abs(enemyProjectileX_ - playerX_);
    if (hitDistance <= ENEMY_PROJECTILE_HIT_WIDTH) {
        const bool demonSlayerFireball = gameManager_
            && (gameManager_->isLanDuel()
                || (gameManager_->isDuelMode()
                    && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE))
            && gameManager_->getLanOpponentPlayerType() == PlayerType::DEMON_SLAYER;
        player->takeDamage(enemyProjectileDamage_);
        if (soundManager_) {
            soundManager_->playHit();
        }
        if (playerAnimChar_) {
            playerAnimChar_->takeDamage();
        }

        statusMessage_ = enemyProjectileUsesArcenArrow_
                             ? QString("Arrow hit! Took %1 damage!").arg(enemyProjectileDamage_)
                             : (enemyProjectileType_ == EnemyType::FIRE_WORM ||
                                enemyProjectileType_ == EnemyType::FLYING_DEMON ||
                                demonSlayerFireball)
                                   ? QString("Fireball hit! Took %1 damage!").arg(enemyProjectileDamage_)
                                   : QString("Shadow bolt hit! Took %1 damage!").arg(enemyProjectileDamage_);
        statusDisplayTime_ = 1.2;

        enemyProjectileExploding_ = !enemyProjectileUsesArcenArrow_ && !enemyProjectileExplodeSprite_.isNull();
        enemyProjectileFrame_ = 0;
        enemyProjectileAnimTime_ = 0.0;
        enemyProjectileExplosionTime_ = 0.0;

        if (!enemyProjectileExploding_) {
            enemyProjectileActive_ = false;
        }

        if (!player->isAlive()) {
            if (soundManager_) soundManager_->playDeath();

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

    if (enemyProjectileUsesArcenArrow_) {
        ensureArcenArrowAssetsLoaded();

        QPixmap spriteSheet = arcenArrowMoveSprite_.isNull() ? arcenArrowSprite_ : arcenArrowMoveSprite_;
        if (spriteSheet.isNull()) {
            return;
        }

        QPixmap sprite;
        if (!arcenArrowMoveSprite_.isNull()) {
            const int frameWidth = qMax(1, spriteSheet.width() / 2);
            const int frameHeight = spriteSheet.height();
            const int x = qBound(0, enemyProjectileFrame_ * frameWidth, spriteSheet.width() - frameWidth);
            sprite = spriteSheet.copy(x, 0, frameWidth, frameHeight);
        } else {
            sprite = spriteSheet;
        }

        const qreal arenaHeight = qMax(1.0, static_cast<qreal>(height() - 160));
        const qreal desiredEnemyHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
        QPixmap arcenReferenceFrame;
        if (enemyAnimManager_) {
            arcenReferenceFrame = enemyAnimManager_->getFrame(AnimationState::IDLE, 0);
        }
        if (arcenReferenceFrame.isNull()) {
            arcenReferenceFrame = sprite;
        }

        const QRect referenceBounds = opaqueBounds(arcenReferenceFrame);
        const qreal referenceVisibleHeight = qMax(1, referenceBounds.height());
        const qreal scale = desiredEnemyHeight / referenceVisibleHeight;
        const qreal w = sprite.width() * scale;
        const qreal h = sprite.height() * scale;

        painter.save();
        if (!enemyProjectileFacingRight_) {
            painter.translate(enemyProjectileX_, 0.0);
            painter.scale(-1.0, 1.0);
            painter.translate(-enemyProjectileX_, 0.0);
        }

        painter.drawPixmap(QRectF(enemyProjectileX_ - w * 0.5, enemyProjectileY_ - h * 0.5, w, h),
                           sprite, QRectF(0, 0, sprite.width(), sprite.height()));
        painter.restore();
        return;
    }

    QPixmap spriteSheet = enemyProjectileExploding_ ? enemyProjectileExplodeSprite_ : enemyProjectileMoveSprite_;
    if (spriteSheet.isNull()) {
        return;
    }

    const bool demonSlayerFireball = gameManager_
        && (gameManager_->isLanDuel()
            || (gameManager_->isDuelMode()
                && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE))
        && gameManager_->getLanOpponentPlayerType() == PlayerType::DEMON_SLAYER;
    const int frameCount = enemyProjectileExploding_
                               ? 7
                               : ((enemyProjectileType_ == EnemyType::FIRE_WORM ||
                                   enemyProjectileType_ == EnemyType::FLYING_DEMON ||
                                   demonSlayerFireball) ? 6 : 4);
    const int frameWidth = qMax(1, spriteSheet.width() / qMax(1, frameCount));
    const int frameHeight = spriteSheet.height();
    const int frameIndex = qBound(0, enemyProjectileFrame_, frameCount - 1);
    const QPixmap sprite = spriteSheet.copy(frameIndex * frameWidth, 0, frameWidth, frameHeight);

    const qreal arenaHeight = qMax(1.0, static_cast<qreal>(height() - 160));
    const qreal desiredEnemyHeight = arenaHeight * FIGHTER_VISIBLE_HEIGHT_RATIO;
    const qreal spriteScale = (enemyProjectileType_ == EnemyType::FIRE_WORM) ? 0.26
                             : (enemyProjectileType_ == EnemyType::FLYING_DEMON) ? 0.23
                             : demonSlayerFireball ? 0.24
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
    const int panelWidth = qMin(448, qMax(320, (width() - 200) / 2));
    const int panelHeight = 102;
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
                         const QPixmap &portrait,
                         const QString &portraitFallback,
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

        const qreal portraitSize = 112.0;
        const qreal portraitInset = -24.0;
        const qreal portraitFloatY = -12.0;
        const QRectF portraitRect = leftAligned
            ? QRectF(panelRect.x() + portraitInset, panelRect.y() + portraitFloatY, portraitSize, portraitSize)
            : QRectF(panelRect.right() - portraitInset - portraitSize, panelRect.y() + portraitFloatY, portraitSize, portraitSize);

        const qreal contentLeft = leftAligned ? (portraitRect.right() + 18.0) : (panelRect.x() + 18.0);
        const qreal contentRight = leftAligned ? (panelRect.right() - 18.0) : (portraitRect.left() - 18.0);
        const QRectF headerStrip(contentLeft, panelRect.y() + 12.0, qMax(0.0, contentRight - contentLeft), 30.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 16));
        painter.drawRoundedRect(headerStrip, 10, 10);
        drawPortraitBadge(painter, portraitRect, portrait, accentEnd.lighter(118), portraitFallback);

        auto fittedHudTitleFont = [&](const QString& text, qreal maxWidth) {
            QFont titleFont("Showcard Gothic", 10);
            if (titleFont.family() != "Showcard Gothic") {
                titleFont = painter.font();
                titleFont.setBold(true);
            }

            for (int pointSize = 10; pointSize >= 7; --pointSize) {
                titleFont.setPointSize(pointSize);
                QFontMetricsF metrics(titleFont);
                if (metrics.horizontalAdvance(text) <= maxWidth) {
                    break;
                }
            }
            return titleFont;
        };
        const qreal headerWidth = qMax(0.0, contentRight - contentLeft);
        QFont titleFont = fittedHudTitleFont(title, headerWidth);
        painter.setFont(titleFont);
        painter.setPen(QColor("#F7D774"));
        painter.drawText(QRectF(contentLeft, panelRect.y() + 10.0, headerWidth, 20.0),
                         leftAligned ? Qt::AlignLeft | Qt::AlignTop : Qt::AlignRight | Qt::AlignTop,
                         title);

        QFont subFont = painter.font();
        subFont.setFamily("Segoe UI");
        subFont.setPointSize(8);
        subFont.setBold(false);
        painter.setFont(subFont);
        painter.setPen(QColor("#C6A66A"));
        painter.drawText(QRectF(contentLeft, panelRect.y() + 34.0, qMax(0.0, contentRight - contentLeft), 16.0),
                         leftAligned ? Qt::AlignLeft | Qt::AlignTop : Qt::AlignRight | Qt::AlignTop,
                         subtitle);

        const QRectF barRect(contentLeft, panelRect.bottom() - 21.0, qMax(0.0, contentRight - contentLeft), 16.0);
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
              playerProfilePortrait_,
              QString::fromStdString(player->getName()),
              QColor("#7CFF63"),
              QColor("#22C55E"));

    drawPanel(enemyPanel,
              false,
              QString::fromStdString(enemy->getName()).toUpper(),
              gameManager_->isLanDuel()
                  ? QString("Linked rival")
                  : (gameManager_->isDuelMode() ? QString("Exhibition rival") : QString("Hostile target")),
              enemy->getHealth(),
              enemy->getMaxHealth(),
              enemyHpDisplay_,
              enemyProfilePortrait_,
              QString::fromStdString(enemy->getName()),
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
    painter.drawText(scorePanel.adjusted(0, 8, 0, 0),
                     Qt::AlignHCenter | Qt::AlignTop,
                     gameManager_->isLanDuel()
                         ? "ARENA LINK DUEL"
                         : (gameManager_->isDuelMode() ? "1V1 EXHIBITION" : "ARENA SCORE"));

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
                     (gameManager_->isLanDuel() || gameManager_->isDuelMode())
                         ? QStringLiteral("ROUND 1 / 1")
                         : QString("STAGE %1 / %2").arg(gameManager_->getCurrentLevel()).arg(gameManager_->getTotalLevels()));
}

void BattleWidget::tryPlayerHeal() {
    if (healCooldown_ > 0.0) return;
    
    Player *player = const_cast<Player*>(gameManager_->getPlayer());
    if (!player || !player->isAlive()) return;
    
    player->takeDamage(-25); // Heal 25 HP
    healCooldown_ = 5.0; // 5 second cooldown
    ++levelBattleReport_.healsUsed;
    enemyAiPlayerHealMemory_ = 1.4;
    if (soundManager_) {
        soundManager_->playHeal();
    }
    
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
        } else if (enemyType == EnemyType::BLACK_WEREWOLF ||
                   enemyType == EnemyType::RED_WEREWOLF ||
                   enemyType == EnemyType::WHITE_WEREWOLF) {
            desiredVisibleHeight *= 0.9;
        } else if (enemyType == EnemyType::ZOMBIE_1 ||
                   enemyType == EnemyType::ZOMBIE_2 ||
                   enemyType == EnemyType::ZOMBIE_3 ||
                   enemyType == EnemyType::ZOMBIE_4) {
            desiredVisibleHeight *= 0.88;
        } else if (enemyType == EnemyType::ADVANCED_ZOMBIE_1 ||
                   enemyType == EnemyType::ADVANCED_ZOMBIE_2 ||
                   enemyType == EnemyType::ADVANCED_ZOMBIE_3) {
            desiredVisibleHeight *= 0.96;
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
