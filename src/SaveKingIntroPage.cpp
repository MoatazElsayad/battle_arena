#include "SaveKingIntroPage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QHash>
#include <QTransform>
#include <QVector>
#include <algorithm>
#include <cmath>

namespace {

struct SpriteClip {
    QVector<QPixmap> frames;
    int frameSpeed = 100;
    bool loops = true;
};

QRect opaqueBounds(const QPixmap& pixmap) {
    if (pixmap.isNull()) {
        return QRect();
    }

    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int minX = image.width();
    int minY = image.height();
    int maxX = -1;
    int maxY = -1;

    for (int y = 0; y < image.height(); ++y) {
        const QRgb* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(row[x]) > 10) {
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }

    if (maxX < minX || maxY < minY) {
        return QRect(0, 0, image.width(), image.height());
    }

    return QRect(QPoint(minX, minY), QPoint(maxX, maxY));
}

QString resolveAssetPath(const QString& relativePath) {
    if (relativePath.isEmpty()) {
        return QString();
    }

    const QStringList candidates = {
        QDir::current().filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../") + relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../") + relativePath)
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }

    return QString();
}

qreal clamp01(qreal value) {
    return qBound<qreal>(0.0, value, 1.0);
}

qreal ramp(qreal seconds, qreal start, qreal duration) {
    if (duration <= 0.0) {
        return (seconds >= start) ? 1.0 : 0.0;
    }
    return clamp01((seconds - start) / duration);
}

SpriteClip loadClip(const QString& spriteSheetPath, int frameCount, bool loops = true, int frameSpeed = 100) {
    SpriteClip clip;
    clip.frameSpeed = frameSpeed;
    clip.loops = loops;

    if (spriteSheetPath.isEmpty() || frameCount <= 0) {
        return clip;
    }

    QPixmap spriteSheet(spriteSheetPath);
    if (spriteSheet.isNull()) {
        return clip;
    }

    const int frameWidth = spriteSheet.width() / frameCount;
    const int frameHeight = spriteSheet.height();
    for (int i = 0; i < frameCount; ++i) {
        clip.frames.append(spriteSheet.copy(i * frameWidth, 0, frameWidth, frameHeight));
    }

    return clip;
}

QPixmap frameForTime(const SpriteClip& clip, qreal seconds) {
    if (clip.frames.isEmpty()) {
        return QPixmap();
    }

    if (seconds <= 0.0) {
        return clip.frames.first();
    }

    int frameIndex = static_cast<int>((seconds * 1000.0) / std::max(1, clip.frameSpeed));
    if (clip.loops) {
        frameIndex %= clip.frames.size();
    } else {
        frameIndex = std::min(frameIndex, static_cast<int>(clip.frames.size()) - 1);
    }

    return clip.frames[frameIndex];
}

void drawPixelSprite(QPainter& painter,
                     const QPixmap& pixmap,
                     const QPointF& anchor,
                     qreal targetHeight,
                     qreal opacity = 1.0,
                     bool center = true,
                     bool mirrored = false) {
    if (pixmap.isNull() || targetHeight <= 0.0) {
        return;
    }

    QPixmap scaled = pixmap.scaledToHeight(static_cast<int>(targetHeight),
                                          Qt::FastTransformation);
    if (mirrored) {
        scaled = scaled.transformed(QTransform().scale(-1.0, 1.0));
    }

    const QRect visibleBounds = opaqueBounds(scaled);
    QPointF topLeft = anchor;
    if (center) {
        topLeft.rx() -= visibleBounds.center().x();
        topLeft.ry() -= (visibleBounds.bottom() + 1);
    }

    painter.save();
    painter.setOpacity(opacity);
    painter.drawPixmap(topLeft, scaled);
    painter.restore();
}

void drawClipSprite(QPainter& painter,
                    const SpriteClip& clip,
                    qreal clipSeconds,
                    const QPointF& anchor,
                    qreal targetHeight,
                    qreal opacity = 1.0,
                    bool center = true,
                    bool mirrored = false) {
    drawPixelSprite(painter,
                    frameForTime(clip, clipSeconds),
                    anchor,
                    targetHeight,
                    opacity,
                    center,
                    mirrored);
}

QPixmap portraitPixmap(const QString& relativePath) {
    const QString resolved = resolveAssetPath(relativePath);
    if (resolved.isEmpty()) {
        return QPixmap();
    }
    return QPixmap(resolved);
}

const QPixmap& kingProfile() {
    static const QPixmap pixmap = portraitPixmap(QStringLiteral("assets/story/king_1/profile.png"));
    return pixmap;
}

const QPixmap& nightweaverProfile() {
    static const QPixmap pixmap = portraitPixmap(QStringLiteral("assets/enemies/Nightweaver/profile.png"));
    return pixmap;
}

QPixmap gladiatorProfile(PlayerType type) {
    QString path;

    switch (type) {
        case PlayerType::ARCEN:
            path = QStringLiteral("assets/players/Arcen/profile.png");
            break;
        case PlayerType::DEMON_SLAYER:
            path = QStringLiteral("assets/players/Demon_Slayer/profile.png");
            break;
        case PlayerType::FANTASY_WARRIOR:
            path = QStringLiteral("assets/players/Fantasy_Warrior/profile.png");
            break;
        case PlayerType::HUNTRESS:
            path = QStringLiteral("assets/players/Huntress/profile.png");
            break;
        case PlayerType::KNIGHT:
            path = QStringLiteral("assets/players/Knight/profile.png");
            break;
        case PlayerType::MARTIAL:
            path = QStringLiteral("assets/players/Martial/profile.png");
            break;
        case PlayerType::MARTIAL_HERO:
            path = QStringLiteral("assets/players/Martial_Hero/profile.png");
            break;
        case PlayerType::MEDIEVAL_WARRIOR:
            path = QStringLiteral("assets/players/Medieval_Warrior/profile.png");
            break;
        case PlayerType::WIZARD:
            path = QStringLiteral("assets/players/Wizard/profile.png");
            break;
    }

    return portraitPixmap(path);
}

void drawDialogueCard(QPainter& painter,
                      const QRectF& bubbleRect,
                      const QString& text,
                      const QPixmap& portrait = QPixmap()) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QColor(235, 204, 132, 190));
    painter.setBrush(QColor(17, 11, 9, 212));
    painter.drawRoundedRect(bubbleRect, 14.0, 14.0);

    QRectF textRect = bubbleRect.adjusted(12.0, 8.0, -12.0, -8.0);
    if (!portrait.isNull()) {
        const qreal portraitSize = bubbleRect.height() - 16.0;
        const QRectF portraitRect(bubbleRect.x() + 8.0,
                                  bubbleRect.y() + 8.0,
                                  portraitSize,
                                  portraitSize);

        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 70));
        painter.drawEllipse(portraitRect.adjusted(-2.0, 3.0, 2.0, 5.0));
        painter.setBrush(QColor(43, 28, 18, 240));
        painter.setPen(QPen(QColor("#E4B35B"), 2.0));
        painter.drawEllipse(portraitRect.adjusted(-2.0, -2.0, 2.0, 2.0));

        QPainterPath clipPath;
        clipPath.addEllipse(portraitRect);
        painter.setClipPath(clipPath);
        const QPixmap scaled = portrait.scaled(portraitRect.size().toSize(),
                                               Qt::KeepAspectRatioByExpanding,
                                               Qt::SmoothTransformation);
        const QRectF sourceRect((scaled.width() - portraitRect.width()) * 0.5,
                                (scaled.height() - portraitRect.height()) * 0.5,
                                portraitRect.width(),
                                portraitRect.height());
        painter.drawPixmap(portraitRect, scaled, sourceRect);
        painter.restore();

        textRect.adjust(portraitSize + 12.0, 0.0, 0.0, 0.0);
    }

    painter.setPen(QColor("#FFF3CF"));
    QFont speechFont("Segoe UI", qMax(10, painter.viewport().width() / 120), QFont::Bold);
    painter.setFont(speechFont);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    painter.restore();
}

const SpriteClip& warriorIdleClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/story/warrior/Sprites/Idle.png"), 6, true, 160);
    return clip;
}

const SpriteClip& warriorRunClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/story/warrior/Sprites/Run.png"), 8, true, 95);
    return clip;
}

const SpriteClip& warriorAttackClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/story/warrior/Sprites/Attack1.png"), 4, false, 85);
    return clip;
}

const SpriteClip& warriorHitClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/story/warrior/Sprites/Hit.png"), 3, false, 100);
    return clip;
}

const SpriteClip& warriorDeathClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/story/warrior/Sprites/Death.png"), 9, false, 110);
    return clip;
}

const SpriteClip& kingIdleClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/story/king_1/Sprites/Idle.png"), 8, true, 170);
    return clip;
}

const SpriteClip& kingRunClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/story/king_1/Sprites/Run.png"), 8, true, 110);
    return clip;
}

const SpriteClip& rescuedKingIdleClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/story/king_2/Sprites/Idle.png"), 6, true, 170);
    return clip;
}

const SpriteClip& portalClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/objects/magic_portal/Sprites/Isometric_Portal.png"), 6, true, 135);
    return clip;
}

const SpriteClip& nightweaverRunClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Nightweaver/Sprites/Run.png"), 8, true, 100);
    return clip;
}

const SpriteClip& nightweaverIdleClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Nightweaver/Sprites/Idle.png"), 10, true, 150);
    return clip;
}

const SpriteClip& nightweaverAttackClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Nightweaver/Sprites/Attack.png"), 13, false, 55);
    return clip;
}

const SpriteClip& fireWormWalkClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Worm/Walk.png"), 9, true, 105);
    return clip;
}

const SpriteClip& fireWormIdleClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Worm/Idle.png"), 9, true, 140);
    return clip;
}

const SpriteClip& fireWormAttackClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Fire_Worm/Sprites/Worm/Attack.png"), 16, false, 55);
    return clip;
}

const SpriteClip& evilWizardRunClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Evil_Wizard/Sprites/Run.png"), 8, true, 100);
    return clip;
}

const SpriteClip& evilWizardIdleClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Evil_Wizard/Sprites/Idle.png"), 8, true, 150);
    return clip;
}

const SpriteClip& evilWizardAttackClip() {
    static const SpriteClip clip = loadClip(resolveAssetPath("assets/enemies/Evil_Wizard/Sprites/Attack1.png"), 8, false, 60);
    return clip;
}

struct PlayerSceneClips {
    SpriteClip idle;
    SpriteClip run;
};

PlayerSceneClips makePlayerSceneClips(PlayerType type) {
    QString basePath;
    QString idlePath;
    QString runPath;
    int idleFrames = 0;
    int runFrames = 0;

    switch (type) {
        case PlayerType::KNIGHT:
            basePath = resolveAssetPath("assets/players/Knight/Sprites");
            idlePath = basePath + "/IDLE.png";
            runPath = basePath + "/RUN.png";
            idleFrames = 7;
            runFrames = 8;
            break;
        case PlayerType::DEMON_SLAYER:
            basePath = resolveAssetPath("assets/players/Demon_Slayer/Sprites");
            idlePath = basePath + "/Idle.png";
            runPath = basePath + "/Run.png";
            idleFrames = 4;
            runFrames = 8;
            break;
        case PlayerType::FANTASY_WARRIOR:
            basePath = resolveAssetPath("assets/players/Fantasy_Warrior/Sprites");
            idlePath = basePath + "/Idle.png";
            runPath = basePath + "/Run.png";
            idleFrames = 10;
            runFrames = 8;
            break;
        case PlayerType::HUNTRESS:
            basePath = resolveAssetPath("assets/players/Huntress/Sprites");
            idlePath = basePath + "/Idle.png";
            runPath = basePath + "/Run.png";
            idleFrames = 8;
            runFrames = 8;
            break;
        case PlayerType::ARCEN:
            basePath = resolveAssetPath("assets/players/Arcen/Sprites/Character");
            idlePath = basePath + "/Idle.png";
            runPath = basePath + "/Run.png";
            idleFrames = 10;
            runFrames = 8;
            break;
        case PlayerType::MARTIAL:
            basePath = resolveAssetPath("assets/players/Martial/Sprite");
            idlePath = basePath + "/Idle.png";
            runPath = basePath + "/Run.png";
            idleFrames = 10;
            runFrames = 8;
            break;
        case PlayerType::MARTIAL_HERO:
            basePath = resolveAssetPath("assets/players/Martial_Hero/Sprites");
            idlePath = basePath + "/Idle.png";
            runPath = basePath + "/Run.png";
            idleFrames = 8;
            runFrames = 8;
            break;
        case PlayerType::MEDIEVAL_WARRIOR:
            basePath = resolveAssetPath("assets/players/Medieval_Warrior/Sprites");
            idlePath = basePath + "/Idle.png";
            runPath = basePath + "/Run.png";
            idleFrames = 10;
            runFrames = 6;
            break;
        case PlayerType::WIZARD:
            basePath = resolveAssetPath("assets/players/Wizard/Sprites");
            idlePath = basePath + "/Idle.png";
            runPath = basePath + "/Run.png";
            idleFrames = 6;
            runFrames = 8;
            break;
        default:
            basePath = resolveAssetPath("assets/players/Knight/Sprites");
            idlePath = basePath + "/IDLE.png";
            runPath = basePath + "/RUN.png";
            idleFrames = 7;
            runFrames = 8;
            break;
    }

    PlayerSceneClips clips;
    clips.idle = loadClip(idlePath, idleFrames, true, 150);
    clips.run = loadClip(runPath, runFrames, true, 95);
    return clips;
}

const PlayerSceneClips& playerSceneClips(PlayerType type) {
    static QHash<int, PlayerSceneClips> cache;
    const int key = static_cast<int>(type);
    if (!cache.contains(key)) {
        cache.insert(key, makePlayerSceneClips(type));
    }
    return cache[key];
}

} // namespace

SaveKingIntroPage::SaveKingIntroPage(QWidget* parent)
    : QWidget(parent),
      sceneActive_(false),
      finishEmitted_(false),
      sceneMode_(SceneMode::Intro),
      selectedGladiatorType_(PlayerType::KNIGHT),
      selectedGladiatorName_(QStringLiteral("Gladiator")) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: #100C0B;");

    frameTimer_.setInterval(16);
    connect(&frameTimer_, &QTimer::timeout, this, &SaveKingIntroPage::advanceScene);

    ensureAssetsLoaded();
}

void SaveKingIntroPage::setSelectedGladiator(PlayerType type, const QString& displayName) {
    selectedGladiatorType_ = type;
    selectedGladiatorName_ = displayName.trimmed().isEmpty() ? QStringLiteral("Gladiator") : displayName.trimmed();
}

void SaveKingIntroPage::startScene() {
    startIntroScene();
}

void SaveKingIntroPage::startIntroScene() {
    startScene(SceneMode::Intro);
}

void SaveKingIntroPage::startRescueEndingScene() {
    startScene(SceneMode::RescueEnding);
}

void SaveKingIntroPage::startScene(SceneMode mode) {
    ensureAssetsLoaded();
    sceneMode_ = mode;
    finishEmitted_ = false;
    sceneActive_ = true;
    sceneClock_.restart();
    frameTimer_.start();
    setFocus();
    update();
}

void SaveKingIntroPage::stopScene() {
    frameTimer_.stop();
    sceneActive_ = false;
    update();
}

SaveKingIntroPage::SceneMode SaveKingIntroPage::sceneMode() const {
    return sceneMode_;
}

void SaveKingIntroPage::advanceScene() {
    if (!sceneActive_) {
        return;
    }

    const qreal seconds = sceneClock_.elapsed() / 1000.0;
    if (seconds >= sceneDuration() && !finishEmitted_) {
        finishEmitted_ = true;
        sceneActive_ = false;
        frameTimer_.stop();
        emit sceneFinished();
        return;
    }

    update();
}

void SaveKingIntroPage::keyPressEvent(QKeyEvent* event) {
    if (event && event->key() == Qt::Key_Space) {
        if (!finishEmitted_) {
            finishEmitted_ = true;
            sceneActive_ = false;
            frameTimer_.stop();
            emit sceneFinished();
        }
        return;
    }

    QWidget::keyPressEvent(event);
}

void SaveKingIntroPage::ensureAssetsLoaded() {
    if (background_.isNull()) {
        background_.load(resolveAssetPath("assets/backgrounds/Kidnap.png"));
    }
    if (rescueBackground_.isNull()) {
        rescueBackground_.load(resolveAssetPath("assets/backgrounds/Kidnap.png"));
    }
}

qreal SaveKingIntroPage::sceneDuration() const {
    return sceneMode_ == SceneMode::Intro ? 36.4 : 6.4;
}

QString SaveKingIntroPage::currentTitle(qreal seconds) const {
    if (sceneMode_ == SceneMode::RescueEnding) {
        if (seconds < 3.4) {
            return QStringLiteral("ACT III  THE RETURN");
        }
        if (seconds < 5.3) {
            return QStringLiteral("THE KING IS SAVED");
        }
        return QStringLiteral("SAVE THE KINGS");
    }

    if (seconds < 5.0) {
        return QStringLiteral("THE PEACEFUL COURTYARD");
    }
    if (seconds < 10.2) {
        return QStringLiteral("THE PORTAL OPENS");
    }
    if (seconds < 18.0) {
        return QStringLiteral("THE LAST STAND");
    }
    if (seconds < 23.8) {
        return QStringLiteral("THE THREAT");
    }
    if (seconds < 31.2) {
        return QStringLiteral("THE ABDUCTION");
    }
    if (seconds < 33.6) {
        return QStringLiteral("THE OATH");
    }
    return QStringLiteral("THE PURSUIT");
}

QString SaveKingIntroPage::currentCaption(qreal seconds) const {
    if (sceneMode_ == SceneMode::RescueEnding) {
        if (seconds < 3.6) {
            return QStringLiteral("%1 and the king return to the courtyard.").arg(selectedGladiatorName_);
        }
        if (seconds < 5.2) {
            return QStringLiteral("The courtyard is safe again.");
        }
        return QStringLiteral("The king is safe.");
    }

    if (seconds < 5.0) {
        return QStringLiteral("In a city of gladiators, the king was standing safely.");
    }
    if (seconds < 10.2) {
        return QStringLiteral("However, a portal appears from nowhere.");
    }
    if (seconds < 17.8) {
        return QStringLiteral("Nightweaver, Fire Worm, and Evil Wizard strike the royal guard.");
    }
    if (seconds < 20.8) {
        return QStringLiteral("Nightweaver: Come with us, King, or we kill you here.");
    }
    if (seconds < 24.2) {
        return QStringLiteral("King: Our gladiators will find me.");
    }
    if (seconds < 31.2) {
        return QStringLiteral("The invaders drag the king into the portal.");
    }
    if (seconds < 33.6) {
        return QStringLiteral("%1: I will save the king.").arg(selectedGladiatorName_);
    }
    return QStringLiteral("%1 charges into the portal.").arg(selectedGladiatorName_);
}

void SaveKingIntroPage::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    ensureAssetsLoaded();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    const qreal seconds = sceneActive_ ? sceneClock_.elapsed() / 1000.0 : 0.0;
    const QRect fullRect = rect();
    QPixmap activeBackground = background_;
    if (sceneMode_ == SceneMode::RescueEnding) {
        activeBackground = rescueBackground_;
    }
    if (!activeBackground.isNull()) {
        painter.drawPixmap(fullRect, activeBackground);
    } else {
        painter.fillRect(fullRect, QColor("#1C1412"));
    }

    QLinearGradient atmosphere(fullRect.topLeft(), QPointF(fullRect.left(), fullRect.bottom()));
    atmosphere.setColorAt(0.0, QColor(18, 12, 10, 80));
    atmosphere.setColorAt(1.0, QColor(7, 5, 4, 170));
    painter.fillRect(fullRect, atmosphere);

    const qreal w = width();
    const qreal h = height();
    const qreal groundLine = h * 0.885;
    const QRectF floorBand(0.0, h * 0.84, w, h * 0.16);

    QLinearGradient floorGradient(floorBand.topLeft(), floorBand.bottomLeft());
    floorGradient.setColorAt(0.0, QColor(42, 28, 18, 30));
    floorGradient.setColorAt(0.08, QColor(88, 62, 38, 125));
    floorGradient.setColorAt(1.0, QColor(15, 11, 10, 228));
    painter.fillRect(floorBand, floorGradient);

    painter.save();
    painter.setPen(QPen(QColor(225, 183, 104, 120), 3));
    painter.drawLine(QPointF(0.0, groundLine), QPointF(w, groundLine));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(18, 10, 8, 44));
    painter.drawEllipse(QRectF(w * 0.10, groundLine - h * 0.035, w * 0.80, h * 0.07));
    painter.restore();

    if (sceneMode_ == SceneMode::RescueEnding) {
        const PlayerSceneClips& heroClips = playerSceneClips(selectedGladiatorType_);
        const qreal phaseSeconds = seconds;
        const qreal portalX = w * 0.98;
        const qreal portalY = groundLine + h * 0.012;
        const qreal portalAlpha = (1.0 - ramp(phaseSeconds, 4.1, 1.5)) * (0.90 + 0.10 * std::sin(phaseSeconds * 4.6));
        drawClipSprite(painter, portalClip(), std::max<qreal>(0.0, phaseSeconds), QPointF(portalX, portalY), h * 0.40, portalAlpha, true, true);

        const qreal heroReturn = ramp(phaseSeconds, 0.4, 2.0);
        const qreal kingReturn = ramp(phaseSeconds, 0.9, 2.1);
        const qreal guardReturn = ramp(phaseSeconds, 3.3, 1.3);

        const qreal heroX = w * 0.90 + (w * 0.64 - w * 0.90) * heroReturn;
        const qreal kingX = w * 0.98 + (w * 0.75 - w * 0.98) * kingReturn;
        const qreal kingOpacity = clamp01(kingReturn + 0.1);
        const bool heroStillMoving = heroReturn < 0.995;

        drawClipSprite(painter,
                       heroStillMoving ? heroClips.run : heroClips.idle,
                       heroStillMoving ? std::max<qreal>(0.0, phaseSeconds - 0.4) : std::max<qreal>(0.0, phaseSeconds - 2.4),
                       QPointF(heroX, groundLine),
                       h * 0.32,
                       clamp01(heroReturn + 0.15),
                       true,
                       true);
        drawClipSprite(painter,
                       kingReturn < 0.995 ? kingRunClip() : kingIdleClip(),
                       kingReturn < 0.995 ? std::max<qreal>(0.0, phaseSeconds - 0.9) : phaseSeconds,
                       QPointF(kingX, groundLine),
                       h * 0.30,
                       kingOpacity,
                       true,
                       true);
        drawClipSprite(painter, warriorIdleClip(), phaseSeconds, QPointF(w * 0.23, groundLine), h * 0.27, guardReturn, true, false);
        drawClipSprite(painter, warriorIdleClip(), phaseSeconds + 0.4, QPointF(w * 0.34, groundLine), h * 0.27, guardReturn, true, false);
    } else {
        const PlayerSceneClips& heroClips = playerSceneClips(selectedGladiatorType_);
        const qreal kingStandX = w * 0.16;
        const qreal knight1StandX = w * 0.25;
        const qreal knight2StandX = w * 0.37;
        const qreal knight3StandX = w * 0.49;
        const qreal portalX = w * 0.98;
        const qreal portalY = groundLine + h * 0.012;

        const qreal portalAppearStart = 5.0;
        const qreal fightStart = 11.0;
        const qreal dialogueStart = 18.0;
        const qreal kingReplyStart = 20.8;
        const qreal abductionStart = 24.2;
        const qreal abductionProgress = ramp(seconds, abductionStart, 4.2);
        const qreal heroEnterStart = 30.0;
        const qreal heroIdleStart = 32.0;
        const qreal heroPortalStart = 33.8;

        const qreal kingThreatX = w * 0.21;
        const qreal kingPortalX = w * 0.82;
        const qreal kingX = (seconds < dialogueStart)
            ? kingStandX
            : kingThreatX + (kingPortalX - kingThreatX) * abductionProgress;
        const bool kingRunning = seconds >= abductionStart;
        const qreal kingOpacity = 1.0 - ramp(seconds, 28.9, 0.7);
        drawClipSprite(painter,
                       kingRunning ? kingRunClip() : kingIdleClip(),
                       kingRunning ? std::max<qreal>(0.0, seconds - abductionStart) : std::fmod(std::max<qreal>(0.0, seconds), 1.6),
                       QPointF(kingX, groundLine),
                       h * 0.38,
                       kingOpacity,
                       true,
                       false);

        const qreal portalAlpha = ramp(seconds, portalAppearStart, 0.9) * (0.92 + 0.08 * std::sin(seconds * 5.4));
        drawClipSprite(painter, portalClip(), std::max<qreal>(0.0, seconds - portalAppearStart), QPointF(portalX, portalY), h * 0.40, portalAlpha, true, true);

        const qreal knight1DeathStart = 14.2;
        const qreal knight2DeathStart = 15.1;
        const qreal knight3DeathStart = 16.0;
        const qreal knightFadeOutA = 1.0 - ramp(seconds, knight1DeathStart + 1.5, 0.9);
        const qreal knightFadeOutB = 1.0 - ramp(seconds, knight2DeathStart + 1.5, 0.9);
        const qreal knightFadeOutC = 1.0 - ramp(seconds, knight3DeathStart + 1.5, 0.9);

        if (knightFadeOutA > 0.0) {
            if (seconds < fightStart) {
                drawClipSprite(painter, warriorIdleClip(), seconds, QPointF(knight1StandX, groundLine), h * 0.31, knightFadeOutA, true, false);
            } else if (seconds < 12.4) {
                drawClipSprite(painter, warriorRunClip(), seconds - fightStart, QPointF(knight1StandX + w * 0.02, groundLine), h * 0.34, knightFadeOutA, true, false);
            } else if (seconds < knight1DeathStart) {
                drawClipSprite(painter, warriorAttackClip(), std::fmod(std::max(0.0, seconds - 12.4), 0.55), QPointF(knight1StandX + w * 0.03, groundLine), h * 0.31, knightFadeOutA, true, false);
            } else if (seconds < knight1DeathStart + 0.45) {
                drawClipSprite(painter, warriorHitClip(), seconds - knight1DeathStart, QPointF(knight1StandX + w * 0.03, groundLine), h * 0.31, knightFadeOutA, true, false);
            } else {
                drawClipSprite(painter, warriorDeathClip(), seconds - (knight1DeathStart + 0.2), QPointF(knight1StandX + w * 0.03, groundLine), h * 0.34, knightFadeOutA, true, false);
            }
        }

        if (knightFadeOutB > 0.0) {
            if (seconds < fightStart + 0.2) {
                drawClipSprite(painter, warriorIdleClip(), seconds + 0.35, QPointF(knight2StandX, groundLine), h * 0.31, knightFadeOutB, true, false);
            } else if (seconds < 12.9) {
                drawClipSprite(painter, warriorRunClip(), seconds - (fightStart + 0.2), QPointF(knight2StandX + w * 0.015, groundLine), h * 0.34, knightFadeOutB, true, false);
            } else if (seconds < knight2DeathStart) {
                drawClipSprite(painter, warriorAttackClip(), std::fmod(std::max(0.0, seconds - 12.9), 0.55), QPointF(knight2StandX + w * 0.02, groundLine), h * 0.31, knightFadeOutB, true, false);
            } else if (seconds < knight2DeathStart + 0.45) {
                drawClipSprite(painter, warriorHitClip(), seconds - knight2DeathStart, QPointF(knight2StandX + w * 0.02, groundLine), h * 0.31, knightFadeOutB, true, false);
            } else {
                drawClipSprite(painter, warriorDeathClip(), seconds - (knight2DeathStart + 0.18), QPointF(knight2StandX + w * 0.02, groundLine), h * 0.34, knightFadeOutB, true, false);
            }
        }

        if (knightFadeOutC > 0.0) {
            if (seconds < fightStart + 0.4) {
                drawClipSprite(painter, warriorIdleClip(), seconds + 0.6, QPointF(knight3StandX, groundLine), h * 0.31, knightFadeOutC, true, false);
            } else if (seconds < 13.4) {
                drawClipSprite(painter, warriorRunClip(), seconds - (fightStart + 0.4), QPointF(knight3StandX + w * 0.01, groundLine), h * 0.34, knightFadeOutC, true, false);
            } else if (seconds < knight3DeathStart) {
                drawClipSprite(painter, warriorAttackClip(), std::fmod(std::max(0.0, seconds - 13.4), 0.55), QPointF(knight3StandX + w * 0.015, groundLine), h * 0.31, knightFadeOutC, true, false);
            } else if (seconds < knight3DeathStart + 0.45) {
                drawClipSprite(painter, warriorHitClip(), seconds - knight3DeathStart, QPointF(knight3StandX + w * 0.015, groundLine), h * 0.31, knightFadeOutC, true, false);
            } else {
                drawClipSprite(painter, warriorDeathClip(), seconds - (knight3DeathStart + 0.16), QPointF(knight3StandX + w * 0.015, groundLine), h * 0.34, knightFadeOutC, true, false);
            }
        }

        const qreal enemyNightEnter = ramp(seconds, 7.2, 1.9);
        const qreal enemyWormEnter = ramp(seconds, 7.6, 2.1);
        const qreal enemyWizardEnter = ramp(seconds, 8.0, 2.4);
        const qreal threatApproach = ramp(seconds, dialogueStart, 1.8);

        const qreal nightFightX = w * 0.56;
        const qreal wormFightX = w * 0.46;
        const qreal wizardFightX = w * 0.66;
        const qreal nightThreatX = w * 0.49;
        const qreal wormThreatX = w * 0.39;
        const qreal wizardThreatX = w * 0.60;
        const qreal nightPortalX = kingX + w * 0.08;
        const qreal wormPortalX = kingX - w * 0.02;
        const qreal wizardPortalX = kingX + w * 0.17;

        const qreal nightBaseX = (portalX + w * 0.02) + (nightFightX - (portalX + w * 0.02)) * enemyNightEnter;
        const qreal wormBaseX = (portalX + w * 0.01) + (wormFightX - (portalX + w * 0.01)) * enemyWormEnter;
        const qreal wizardBaseX = (portalX + w * 0.03) + (wizardFightX - (portalX + w * 0.03)) * enemyWizardEnter;

        const qreal nightThreatBlend = nightBaseX + (nightThreatX - nightBaseX) * threatApproach;
        const qreal wormThreatBlend = wormBaseX + (wormThreatX - wormBaseX) * threatApproach;
        const qreal wizardThreatBlend = wizardBaseX + (wizardThreatX - wizardBaseX) * threatApproach;

        const qreal nightX = nightThreatBlend + (nightPortalX - nightThreatBlend) * abductionProgress;
        const qreal wormX = wormThreatBlend + (wormPortalX - wormThreatBlend) * abductionProgress;
        const qreal wizardX = wizardThreatBlend + (wizardPortalX - wizardThreatBlend) * abductionProgress;

        const qreal enemyAlpha = ramp(seconds, 7.2, 0.7) * (1.0 - ramp(seconds, 28.9, 0.7));
        const bool fightPhase = seconds >= fightStart && seconds < dialogueStart;
        const bool threatPhase = seconds >= dialogueStart && seconds < abductionStart;
        const bool kidnappingPhase = seconds >= abductionStart;

        const qreal fightPairPull = ramp(seconds, fightStart, 1.0) * (1.0 - ramp(seconds, dialogueStart - 0.3, 0.5));
        const qreal nightPairX = nightX + (knight3StandX + w * 0.055 - nightX) * fightPairPull;
        const qreal wormPairX = wormX + (knight2StandX + w * 0.055 - wormX) * fightPairPull;
        const qreal wizardPairX = wizardX + (knight1StandX + w * 0.06 - wizardX) * fightPairPull;

        drawClipSprite(painter,
                       fightPhase ? fireWormAttackClip() : (threatPhase ? fireWormIdleClip() : fireWormWalkClip()),
                       fightPhase ? std::fmod(std::max(0.0, seconds - fightStart), 0.88)
                                  : threatPhase ? std::fmod(std::max<qreal>(0.0, seconds - dialogueStart), 1.2)
                                                : std::max<qreal>(0.0, seconds - 7.6),
                       QPointF(fightPhase ? wormPairX : wormX, groundLine),
                       h * 0.27,
                       enemyAlpha,
                       true,
                       !kidnappingPhase);
        drawClipSprite(painter,
                       fightPhase ? nightweaverAttackClip() : (threatPhase ? nightweaverIdleClip() : nightweaverRunClip()),
                       fightPhase ? std::fmod(std::max(0.0, seconds - (fightStart + 0.2)), 0.72)
                                  : threatPhase ? std::fmod(std::max<qreal>(0.0, seconds - dialogueStart), 1.4)
                                                : std::max<qreal>(0.0, seconds - 7.2),
                       QPointF(fightPhase ? nightPairX : nightX, groundLine),
                       h * 0.39,
                       enemyAlpha,
                       true,
                       !kidnappingPhase);
        drawClipSprite(painter,
                       fightPhase ? evilWizardAttackClip() : (threatPhase ? evilWizardIdleClip() : evilWizardRunClip()),
                       fightPhase ? std::fmod(std::max(0.0, seconds - (fightStart + 0.5)), 0.78)
                                  : threatPhase ? std::fmod(std::max<qreal>(0.0, seconds - dialogueStart), 1.2)
                                                : std::max<qreal>(0.0, seconds - 8.0),
                       QPointF(fightPhase ? wizardPairX : wizardX, groundLine),
                       h * 0.43,
                       enemyAlpha,
                       true,
                       !kidnappingPhase);

        const qreal heroEnterProgress = ramp(seconds, heroEnterStart, heroIdleStart - heroEnterStart);
        const qreal heroPortalProgress = ramp(seconds, heroPortalStart, sceneDuration() - heroPortalStart - 0.6);
        const qreal heroStartX = -w * 0.10;
        const qreal heroIdleX = w * 0.20;
        const qreal heroPortalX = w * 0.82;
        qreal heroX = heroStartX;
        if (seconds >= heroEnterStart && seconds < heroIdleStart) {
            heroX = heroStartX + (heroIdleX - heroStartX) * heroEnterProgress;
        } else if (seconds >= heroIdleStart) {
            heroX = heroIdleX + (heroPortalX - heroIdleX) * heroPortalProgress;
        }

        const bool heroVisible = seconds >= heroEnterStart;
        const bool heroIdlePhase = seconds >= heroIdleStart && seconds < heroPortalStart;
        const bool heroPortalPhase = seconds >= heroPortalStart;
        const qreal heroOpacity = heroVisible ? (1.0 - ramp(seconds, sceneDuration() - 0.8, 0.8)) : 0.0;
        if (heroVisible) {
            drawClipSprite(painter,
                           heroIdlePhase ? heroClips.idle : heroClips.run,
                           heroIdlePhase ? std::max<qreal>(0.0, seconds - heroIdleStart)
                                         : std::max<qreal>(0.0, seconds - heroEnterStart),
                           QPointF(heroX, groundLine),
                           h * 0.37,
                           heroOpacity,
                           true,
                           false);
        }

        if (threatPhase) {
            if (seconds < kingReplyStart) {
                drawDialogueCard(painter,
                                 QRectF(w * 0.53, h * 0.14, w * 0.24, h * 0.09),
                                 QStringLiteral("Nightweaver:\nCome with us, or you die."),
                                 nightweaverProfile());
            } else {
                drawDialogueCard(painter,
                                 QRectF(w * 0.10, h * 0.16, w * 0.27, h * 0.09),
                                 QStringLiteral("King:\nOur gladiators will find me."),
                                 kingProfile());
            }
        } else if (seconds >= heroIdleStart && seconds < heroPortalStart) {
            drawDialogueCard(painter,
                             QRectF(w * 0.11, h * 0.15, w * 0.22, h * 0.085),
                             QStringLiteral("I will save the king."),
                             gladiatorProfile(selectedGladiatorType_));
        }
    }

    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(225, 190, 112, 28));
    painter.drawEllipse(QRectF(w * 0.08, groundLine - h * 0.045, w * 0.82, h * 0.07));
    painter.restore();

    painter.fillRect(QRect(0, 0, width(), static_cast<int>(h * 0.075)), QColor(5, 4, 4, 200));
    painter.fillRect(QRect(0, static_cast<int>(h * 0.90), width(), static_cast<int>(h * 0.10)), QColor(5, 4, 4, 215));

    painter.setPen(QColor("#F4D895"));
    QFont titleFont("Segoe UI", qMax(16, width() / 62), QFont::Black);
    painter.setFont(titleFont);
    painter.drawText(QRect(static_cast<int>(w * 0.05), static_cast<int>(h * 0.042), static_cast<int>(w * 0.90), static_cast<int>(h * 0.07)),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     currentTitle(seconds));

    QRectF captionCard(w * 0.16, h * 0.905, w * 0.68, h * 0.055);
    painter.save();
    painter.setPen(QColor(212, 160, 74, 135));
    painter.setBrush(QColor(19, 12, 10, 180));
    painter.drawRoundedRect(captionCard, 12.0, 12.0);
    painter.restore();

    painter.setPen(QColor("#FFF1CC"));
    QFont bodyFont("Segoe UI", qMax(11, width() / 95), QFont::DemiBold);
    painter.setFont(bodyFont);
    painter.drawText(QRect(static_cast<int>(captionCard.left()),
                           static_cast<int>(captionCard.top()),
                           static_cast<int>(captionCard.width()),
                           static_cast<int>(captionCard.height())),
                     Qt::AlignCenter,
                     currentCaption(seconds));

    const qreal fadeIn = 1.0 - clamp01(seconds / 0.65);
    const qreal fadeOut = ramp(seconds, sceneDuration() - 0.85, 0.85);
    const qreal overlayOpacity = std::max(fadeIn, fadeOut);
    if (overlayOpacity > 0.0) {
        painter.fillRect(fullRect, QColor(2, 2, 2, static_cast<int>(240 * overlayOpacity)));
    }
}
