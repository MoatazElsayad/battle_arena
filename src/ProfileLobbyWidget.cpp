#include "ProfileLobbyWidget.h"

#include "FighterAiProfile.h"
#include "GameManager.h"
#include "InputHandler.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QEasingCurve>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QComboBox>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyle>
#include <QStyleOption>
#include <QToolButton>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace {

constexpr qreal kLobbyPreviewScaleMultiplier = 1.836;
constexpr qreal kLobbyPreviewFloorLiftRatio = 0.13;
constexpr int kCharacterUnlockRevealMs = 3600;
constexpr int kPreviewSceneWidth = 1200;
constexpr int kPreviewSceneHeight = 1280;
const QString kPlayableLobbyMode = QStringLiteral("Save the Kings");
const QString kExhibitionLobbyMode = QStringLiteral("1v1 Exhibition");
const QString kZombieLobbyMode = QStringLiteral("Zombie");
const QString kLanLobbyMode = QStringLiteral("LAN Battle");
constexpr int kZombieUnlockTier = 5; // Elite Knight
const QStringList kDuelArenaChoices = {
    QStringLiteral("Colosseum"),
    QStringLiteral("Ember Court"),
    QStringLiteral("Forest Temple"),
    QStringLiteral("Night Fortress"),
    QStringLiteral("Lava Pit"),
    QStringLiteral("Sky Ruins")
};

struct LobbyAnimationSpec {
    QString idlePath;
    int idleFrameCount;
    QString attackPath;
    int attackFrameCount;
};

qreal smoothStep(qreal value) {
    const qreal t = qBound(0.0, value, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

qreal scaledBetween(qreal start, qreal end, qreal value) {
    if (qFuzzyCompare(start, end)) {
        return value >= end ? 1.0 : 0.0;
    }
    return qBound(0.0, (value - start) / (end - start), 1.0);
}

LobbyAnimationSpec animationSpecForCharacterName(const QString& name) {
    const QString characterName = name.trimmed().toLower();
    if (characterName.contains("arcen")) {
        return {"Sprites/Character/Idle.png", 10, "Sprites/Character/Attack.png", 6};
    }
    if (characterName.contains("demon slayer")) {
        return {"Sprites/Idle.png", 4, "Sprites/Attack1.png", 4};
    }
    if (characterName.contains("fantasy")) {
        return {"Sprites/Idle.png", 10, "Sprites/Attack1.png", 7};
    }
    if (characterName.contains("huntress")) {
        return {"Sprites/Idle.png", 8, "Sprites/Attack1.png", 5};
    }
    if (characterName.contains("knight")) {
        return {"Sprites/IDLE.png", 7, "Sprites/ATTACK 1.png", 6};
    }
    if (characterName.contains("martial hero")) {
        return {"Sprites/Idle.png", 8, "Sprites/Attack1.png", 6};
    }
    if (characterName.contains("martial")) {
        return {"Sprite/Idle.png", 10, "Sprite/Attack1.png", 7};
    }
    if (characterName.contains("medieval")) {
        return {"Sprites/Idle.png", 10, "Sprites/Attack1.png", 7};
    }
    if (characterName.contains("wizard")) {
        return {"Sprites/Idle.png", 6, "Sprites/Attack1.png", 8};
    }
    return {QString(), 1, QString(), 0};
}

QString resolveAssetPath(const QString& relativePath) {
    if (relativePath.isEmpty()) {
        return QString();
    }

    const QFileInfo directInfo(relativePath);
    if (directInfo.isAbsolute() && directInfo.exists()) {
        return directInfo.absoluteFilePath();
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

    return QDir::cleanPath(relativePath);
}

QPixmap circularPortraitPixmap(const QPixmap& source, const QSize& targetSize) {
    if (source.isNull() || !targetSize.isValid()) {
        return QPixmap();
    }

    const int side = qMin(targetSize.width(), targetSize.height());
    if (side <= 0) {
        return QPixmap();
    }

    QPixmap result(side, side);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath clipPath;
    clipPath.addEllipse(QRectF(0, 0, side, side));
    painter.setClipPath(clipPath);

    const QPixmap scaled = source.scaled(side, side, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QRect sourceRect((scaled.width() - side) / 2,
                           (scaled.height() - side) / 2,
                           side,
                           side);
    painter.drawPixmap(QRect(0, 0, side, side), scaled, sourceRect);

    painter.setClipping(false);
    const QRectF outerRing(1.5, 1.5, side - 3.0, side - 3.0);
    painter.setPen(QPen(QColor("#D4A017"), qMax(2.0, side / 28.0)));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(outerRing);

    const QRectF innerHighlight(4.5, 4.5, side - 9.0, side - 9.0);
    painter.setPen(QPen(QColor(255, 244, 210, 110), qMax(1.0, side / 72.0)));
    painter.drawEllipse(innerHighlight);
    painter.end();

    return result;
}

QString profilePortraitPathForCharacter(const QString& name, const QString& imagePath) {
    const QString resolvedImagePath = resolveAssetPath(imagePath);
    if (!resolvedImagePath.isEmpty()) {
        const QFileInfo imageInfo(resolvedImagePath);
        const QString siblingProfile = imageInfo.dir().filePath(QStringLiteral("profile.png"));
        if (QFileInfo::exists(siblingProfile)) {
            return siblingProfile;
        }
    }

    const QString lowered = name.trimmed().toLower();
    if (lowered.contains("arcen")) return resolveAssetPath(QStringLiteral("assets/players/Arcen/profile.png"));
    if (lowered.contains("demon slayer")) return resolveAssetPath(QStringLiteral("assets/players/Demon_Slayer/profile.png"));
    if (lowered.contains("fantasy")) return resolveAssetPath(QStringLiteral("assets/players/Fantasy_Warrior/profile.png"));
    if (lowered.contains("huntress")) return resolveAssetPath(QStringLiteral("assets/players/Huntress/profile.png"));
    if (lowered.contains("knight")) return resolveAssetPath(QStringLiteral("assets/players/Knight/profile.png"));
    if (lowered.contains("martial hero")) return resolveAssetPath(QStringLiteral("assets/players/Martial_Hero/profile.png"));
    if (lowered.contains("martial")) return resolveAssetPath(QStringLiteral("assets/players/Martial/profile.png"));
    if (lowered.contains("medieval")) return resolveAssetPath(QStringLiteral("assets/players/Medieval_Warrior/profile.png"));
    if (lowered.contains("wizard")) return resolveAssetPath(QStringLiteral("assets/players/Wizard/profile.png"));
    return QString();
}

QString rankNameForScore(int score) {
    return QString::fromStdString(GameManager::calculateRankFromScore(qMax(0, score)));
}

QString canonicalRankName(const QString& rawRank, int score) {
    static const QStringList ranks = {
        QStringLiteral("Wanderer"),
        QStringLiteral("Squire"),
        QStringLiteral("Gladiator"),
        QStringLiteral("Knight"),
        QStringLiteral("Elite Knight"),
        QStringLiteral("Warlord"),
        QStringLiteral("Champion"),
        QStringLiteral("High Champion"),
        QStringLiteral("Legend"),
        QStringLiteral("Immortal")
    };

    const QString cleaned = rawRank.trimmed();
    for (const QString& rank : ranks) {
        if (cleaned.compare(rank, Qt::CaseInsensitive) == 0) {
            return rank;
        }
    }

    return rankNameForScore(score);
}

QString rankBadgePath(const QString& rankName) {
    QString fileName = rankName.trimmed();
    fileName.replace(QLatin1Char(' '), QLatin1Char('_'));
    if (fileName.isEmpty()) {
        fileName = QStringLiteral("Wanderer");
    }

    return resolveAssetPath(QStringLiteral("assets/ranks/%1.png").arg(fileName));
}

QPixmap rankBadgePixmap(const QString& rankName, int side) {
    const QString path = rankBadgePath(rankName);
    if (path.isEmpty() || side <= 0) {
        return QPixmap();
    }

    static QHash<QString, QPixmap> cache;
    const QString key = QStringLiteral("%1|%2").arg(path).arg(side);
    const auto cached = cache.constFind(key);
    if (cached != cache.constEnd()) {
        return cached.value();
    }

    const QPixmap badge(path);
    if (badge.isNull()) {
        return QPixmap();
    }

    const QPixmap scaled = badge.scaled(QSize(side, side), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    cache.insert(key, scaled);
    return scaled;
}

QVector<PlayerType> rosterByUnlockTier() {
    return {
        PlayerType::KNIGHT,
        PlayerType::MEDIEVAL_WARRIOR,
        PlayerType::MARTIAL_HERO,
        PlayerType::MARTIAL,
        PlayerType::FANTASY_WARRIOR,
        PlayerType::DEMON_SLAYER,
        PlayerType::HUNTRESS,
        PlayerType::ARCEN,
        PlayerType::WIZARD
    };
}

int rankTierForName(const QString& rankName) {
    const QString rank = rankName.trimmed().toLower();
    if (rank == QStringLiteral("squire")) return 2;
    if (rank == QStringLiteral("gladiator")) return 3;
    if (rank == QStringLiteral("knight")) return 4;
    if (rank == QStringLiteral("elite knight")) return 5;
    if (rank == QStringLiteral("warlord")) return 6;
    if (rank == QStringLiteral("champion")) return 7;
    if (rank == QStringLiteral("high champion")) return 8;
    if (rank == QStringLiteral("legend")) return 9;
    if (rank == QStringLiteral("immortal")) return 10;
    return 1;
}

QString rankNameForUnlockTier(int tier) {
    switch (tier) {
        case 2: return QStringLiteral("Squire");
        case 3: return QStringLiteral("Gladiator");
        case 4: return QStringLiteral("Knight");
        case 5: return QStringLiteral("Elite Knight");
        case 6: return QStringLiteral("Warlord");
        case 7: return QStringLiteral("Champion");
        case 8: return QStringLiteral("High Champion");
        case 9: return QStringLiteral("Legend");
        case 10: return QStringLiteral("Immortal");
        case 1:
        default:
            return QStringLiteral("Wanderer");
    }
}

QPixmap lockBadgePixmap(int side) {
    const int s = qMax(24, side);
    QPixmap pix(s, s);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF outer(1.0, 1.0, s - 2.0, s - 2.0);
    painter.setPen(QPen(QColor(255, 226, 168, 160), qMax(1.2, s / 20.0)));
    painter.setBrush(QColor(20, 14, 10, 220));
    painter.drawEllipse(outer);

    const qreal shackleW = s * 0.34;
    const qreal shackleH = s * 0.25;
    const QRectF shackle((s - shackleW) * 0.5, s * 0.25, shackleW, shackleH);
    painter.setPen(QPen(QColor("#F4D895"), qMax(1.4, s / 18.0)));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(shackle, 0, 180 * 16);

    const QRectF body(s * 0.30, s * 0.43, s * 0.40, s * 0.30);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#D4A017"));
    painter.drawRoundedRect(body, s * 0.06, s * 0.06);

    painter.setBrush(QColor(35, 23, 12, 210));
    painter.drawEllipse(QRectF(s * 0.46, s * 0.53, s * 0.08, s * 0.08));
    painter.end();
    return pix;
}

class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent = nullptr, int margin = 0, int hSpacing = 12, int vSpacing = 12)
        : QLayout(parent), hSpace_(hSpacing), vSpace_(vSpacing) {
        setContentsMargins(margin, margin, margin, margin);
    }

    ~FlowLayout() override {
        QLayoutItem* item;
        while ((item = takeAt(0)) != nullptr) {
            delete item;
        }
    }

    void addItem(QLayoutItem* item) override {
        items_.append(item);
    }

    int count() const override {
        return items_.size();
    }

    QLayoutItem* itemAt(int index) const override {
        if (index < 0 || index >= items_.size()) {
            return nullptr;
        }
        return items_.at(index);
    }

    QLayoutItem* takeAt(int index) override {
        if (index < 0 || index >= items_.size()) {
            return nullptr;
        }
        return items_.takeAt(index);
    }

    Qt::Orientations expandingDirections() const override {
        return {};
    }

    bool hasHeightForWidth() const override {
        return true;
    }

    int heightForWidth(int width) const override {
        return doLayout(QRect(0, 0, width, 0), true);
    }

    QSize minimumSize() const override {
        QSize size;
        for (const QLayoutItem* item : items_) {
            size = size.expandedTo(item->minimumSize());
        }
        const QMargins margins = contentsMargins();
        size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
        return size;
    }

    QSize sizeHint() const override {
        return minimumSize();
    }

    void setGeometry(const QRect& rect) override {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

private:
    int doLayout(const QRect& rect, bool testOnly) const {
        const QMargins margins = contentsMargins();
        const QRect area = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());

        int x = area.x();
        int y = area.y();
        int lineHeight = 0;

        for (QLayoutItem* item : items_) {
            const QSize hint = item->sizeHint();
            const int nextX = x + hint.width() + hSpace_;

            if (nextX - hSpace_ > area.right() && lineHeight > 0) {
                x = area.x();
                y += lineHeight + vSpace_;
                lineHeight = 0;
            }

            if (!testOnly) {
                item->setGeometry(QRect(QPoint(x, y), hint));
            }

            x += hint.width() + hSpace_;
            lineHeight = qMax(lineHeight, hint.height());
        }

        return (y + lineHeight - rect.y()) + margins.bottom();
    }

    QList<QLayoutItem*> items_;
    int hSpace_;
    int vSpace_;
};

QPixmap createGoldGlowPixmap(int size) {
    QPixmap glow(size, size);
    glow.fill(Qt::transparent);

    QPainter painter(&glow);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRadialGradient grad(QPointF(size / 2.0, size / 2.0), size / 2.0);
    grad.setColorAt(0.0, QColor(212, 160, 23, 120));
    grad.setColorAt(0.55, QColor(212, 160, 23, 34));
    grad.setColorAt(1.0, QColor(212, 160, 23, 0));

    painter.setPen(Qt::NoPen);
    painter.setBrush(grad);
    painter.drawEllipse(QRectF(0, 0, size, size));

    return glow;
}

QPixmap createCinematicBackdrop(const QSize& size) {
    QPixmap bg(size);
    bg.fill(Qt::transparent);

    QPainter p(&bg);
    p.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient grad(0, 0, 0, size.height());
    grad.setColorAt(0.0, QColor(40, 28, 22));
    grad.setColorAt(0.38, QColor(28, 21, 18));
    grad.setColorAt(1.0, QColor(16, 13, 11));
    p.fillRect(bg.rect(), grad);

    QRadialGradient upperGlow(QPointF(size.width() * 0.5, size.height() * 0.14), size.width() * 0.34);
    upperGlow.setColorAt(0.0, QColor(214, 169, 80, 70));
    upperGlow.setColorAt(0.42, QColor(214, 169, 80, 18));
    upperGlow.setColorAt(1.0, QColor(214, 169, 80, 0));
    p.fillRect(bg.rect(), upperGlow);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(180, 140, 90, 16));
    p.drawEllipse(QRectF(size.width() * 0.10, size.height() * 0.60, size.width() * 0.80, size.height() * 0.24));
    p.setBrush(QColor(220, 70, 42, 12));
    p.drawEllipse(QRectF(size.width() * 0.58, size.height() * 0.70, size.width() * 0.22, size.height() * 0.08));

    p.setBrush(QColor(255, 255, 255, 6));
    p.drawRoundedRect(QRectF(size.width() * 0.12, size.height() * 0.10, size.width() * 0.76, size.height() * 0.05), 24, 24);

    p.setBrush(QColor(212, 160, 23, 16));
    p.drawRect(QRectF(0, size.height() * 0.72, size.width(), 2));

    return bg;
}

QPixmap coverScaledPixmap(const QPixmap& source, const QSize& targetSize) {
    if (source.isNull() || !targetSize.isValid()) {
        return QPixmap();
    }

    const QPixmap scaled = source.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QRect cropRect((scaled.width() - targetSize.width()) / 2,
                         (scaled.height() - targetSize.height()) / 2,
                         targetSize.width(),
                         targetSize.height());
    return scaled.copy(cropRect);
}

QString previewBackgroundPathForCharacter(const QString& name, const QString& imagePath) {
    const QString resolvedImagePath = resolveAssetPath(imagePath);
    if (!resolvedImagePath.isEmpty()) {
        const QFileInfo imageInfo(resolvedImagePath);
        const QString siblingPreview = imageInfo.dir().filePath(QStringLiteral("preview_bg.png"));
        if (QFileInfo::exists(siblingPreview)) {
            return siblingPreview;
        }
    }

    const QString lowered = name.trimmed().toLower();
    if (lowered.contains("arcen")) return resolveAssetPath(QStringLiteral("assets/players/Arcen/preview_bg.png"));
    if (lowered.contains("demon slayer")) return resolveAssetPath(QStringLiteral("assets/players/Demon_Slayer/preview_bg.png"));
    if (lowered.contains("fantasy")) return resolveAssetPath(QStringLiteral("assets/players/Fantasy_Warrior/preview_bg.png"));
    if (lowered.contains("huntress")) return resolveAssetPath(QStringLiteral("assets/players/Huntress/preview_bg.png"));
    if (lowered.contains("knight")) return resolveAssetPath(QStringLiteral("assets/players/Knight/preview_bg.png"));
    if (lowered.contains("martial hero")) return resolveAssetPath(QStringLiteral("assets/players/Martial_Hero/preview_bg.png"));
    if (lowered.contains("martial")) return resolveAssetPath(QStringLiteral("assets/players/Martial/preview_bg.png"));
    if (lowered.contains("medieval")) return resolveAssetPath(QStringLiteral("assets/players/Medieval_Warrior/preview_bg.png"));
    if (lowered.contains("wizard")) return resolveAssetPath(QStringLiteral("assets/players/Wizard/preview_bg.png"));
    return QString();
}

QPixmap previewBackgroundPixmapForCharacter(const QString& name, const QString& imagePath, const QSize& targetSize) {
    const QString backgroundPath = previewBackgroundPathForCharacter(name, imagePath);
    if (!backgroundPath.isEmpty() && QFileInfo::exists(backgroundPath)) {
        const QPixmap customBackground(backgroundPath);
        const QPixmap fitted = coverScaledPixmap(customBackground, targetSize);
        if (!fitted.isNull()) {
            return fitted;
        }
    }

    return createCinematicBackdrop(targetSize);
}

QPixmap createCharacterPlaceholder(const QSize& size, const QString& name) {
    QPixmap pix(size);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#6B4B2D"));
    painter.drawRoundedRect(QRectF(10, 30, size.width() - 20, size.height() - 50), 20, 20);

    painter.setBrush(QColor("#8A6742"));
    painter.drawEllipse(QRectF(size.width() * 0.28, 4, size.width() * 0.44, size.width() * 0.44));

    painter.setPen(QColor("#F5E6B8"));
    painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
    painter.drawText(QRect(0, size.height() - 42, size.width(), 36), Qt::AlignCenter, name.isEmpty() ? "Character" : name);

    return pix;
}

struct CharacterLobbyProfile {
    QString description;
    QStringList abilities;
    int attack = 50;
    int heal = 20;
    int mobility = 45;
    int control = 40;
};

PlayerType playerTypeForLobbyName(const QString& name) {
    const QString lowered = name.trimmed().toLower();
    if (lowered.contains("arcen")) return PlayerType::ARCEN;
    if (lowered.contains("demon slayer")) return PlayerType::DEMON_SLAYER;
    if (lowered.contains("fantasy")) return PlayerType::FANTASY_WARRIOR;
    if (lowered.contains("huntress")) return PlayerType::HUNTRESS;
    if (lowered.contains("martial hero")) return PlayerType::MARTIAL_HERO;
    if (lowered.contains("martial")) return PlayerType::MARTIAL;
    if (lowered.contains("medieval")) return PlayerType::MEDIEVAL_WARRIOR;
    if (lowered.contains("wizard")) return PlayerType::WIZARD;
    return PlayerType::KNIGHT;
}

QString personalityLabel(FighterAiPersonality personality) {
    switch (personality) {
        case FighterAiPersonality::Balanced: return QStringLiteral("Balanced");
        case FighterAiPersonality::Berserker: return QStringLiteral("Berserker");
        case FighterAiPersonality::Duelist: return QStringLiteral("Duelist");
        case FighterAiPersonality::Tank: return QStringLiteral("Tank");
        case FighterAiPersonality::Skirmisher: return QStringLiteral("Skirmisher");
        case FighterAiPersonality::Zoner: return QStringLiteral("Zoner");
        case FighterAiPersonality::Trickster: return QStringLiteral("Trickster");
        case FighterAiPersonality::Caster: return QStringLiteral("Caster");
    }
    return QStringLiteral("Balanced");
}

QString profileDescriptionFor(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN:
            return QStringLiteral("A precision archer who holds long lanes with arrows that trade damage for reach.");
        case PlayerType::DEMON_SLAYER:
            return QStringLiteral("An infernal berserker who now pressures from fireball range before committing.");
        case PlayerType::FANTASY_WARRIOR:
            return QStringLiteral("A reliable duelist with balanced pressure, combo variety, and steady defense.");
        case PlayerType::HUNTRESS:
            return QStringLiteral("A fast skirmisher built around spacing, retreats, and clean re-entry attacks.");
        case PlayerType::KNIGHT:
            return QStringLiteral("A sturdy tank with disciplined melee, strong defense, and composed recovery.");
        case PlayerType::MARTIAL:
            return QStringLiteral("A close-range duelist who chains fast attacks and punishes missed openings.");
        case PlayerType::MARTIAL_HERO:
            return QStringLiteral("A mobile duelist with quick burst windows and flexible close combat.");
        case PlayerType::MEDIEVAL_WARRIOR:
            return QStringLiteral("A grounded bruiser who absorbs pressure and answers with measured heavy attacks.");
        case PlayerType::WIZARD:
            return QStringLiteral("A caster-style fighter who controls mid-range with deliberate attack timing.");
    }
    return QStringLiteral("A battle-ready contender prepared to adapt to any arena challenge.");
}

QString attackPatternFor(PlayerType type, int attackSlots) {
    switch (type) {
        case PlayerType::ARCEN:
            return QStringLiteral("%1 attack states with a real, lower-damage arrow projectile.").arg(attackSlots);
        case PlayerType::DEMON_SLAYER:
            return QStringLiteral("%1 attack states with fireball pressure from range.").arg(attackSlots);
        case PlayerType::WIZARD:
            return QStringLiteral("%1 attack states; Attack 3 reuses the second spell animation.").arg(attackSlots);
        case PlayerType::MARTIAL_HERO:
            return QStringLiteral("%1 attack states; Attack 3 reuses the second combo animation.").arg(attackSlots);
        default:
            return QStringLiteral("%1 usable melee attack states.").arg(attackSlots);
    }
}

QString projectileLineFor(PlayerType type, const FighterAiProfile& profile) {
    if (!profile.hasProjectile) {
        return QStringLiteral("Projectile: none in the current combat build.");
    }
    if (type == PlayerType::ARCEN) {
        return QStringLiteral("Projectile: arrow, triple-range reach with reduced hit damage.");
    }
    if (type == PlayerType::DEMON_SLAYER) {
        return QStringLiteral("Projectile: fireball, used before approaching.");
    }
    return QStringLiteral("Projectile: enabled by fighter profile.");
}

QString projectileShortLabelFor(PlayerType type, const FighterAiProfile& profile) {
    if (!profile.hasProjectile) {
        return QStringLiteral("Melee");
    }
    if (type == PlayerType::ARCEN) {
        return QStringLiteral("Arrow");
    }
    if (type == PlayerType::DEMON_SLAYER) {
        return QStringLiteral("Fireball");
    }
    return QStringLiteral("Projectile");
}

QString tacticalNoteFor(PlayerType type, const FighterAiProfile& profile) {
    if (type == PlayerType::ARCEN) {
        return QStringLiteral("Keeps long lanes with lighter arrow damage, then resets spacing.");
    }
    if (type == PlayerType::DEMON_SLAYER) {
        return QStringLiteral("Pressures from fireball range before rushing into heavy close attacks.");
    }

    switch (profile.personality) {
        case FighterAiPersonality::Berserker:
            return QStringLiteral("Wants fast pressure, but can be punished after overcommitting.");
        case FighterAiPersonality::Tank:
            return QStringLiteral("Absorbs pressure well and answers with steady close-range hits.");
        case FighterAiPersonality::Skirmisher:
            return QStringLiteral("Steps in and out often, looking for clean re-entry attacks.");
        case FighterAiPersonality::Zoner:
            return QStringLiteral("Controls space first and attacks when the opponent enters range.");
        case FighterAiPersonality::Trickster:
            return QStringLiteral("Baits whiffs, delays entries, and punishes careless approaches.");
        case FighterAiPersonality::Caster:
            return QStringLiteral("Prefers deliberate timing and mid-range control over frantic trades.");
        case FighterAiPersonality::Duelist:
            return QStringLiteral("Balanced footwork with sharp punish windows and combo pressure.");
        case FighterAiPersonality::Balanced:
        default:
            return QStringLiteral("Reliable pressure with balanced range, recovery, and control.");
    }
}

CharacterLobbyProfile profileForCharacter(const QString& name, const QString& specialMove) {
    const PlayerType type = playerTypeForLobbyName(name);
    const FighterAiProfile profile = fighterAiProfileFor(type);
    const CharacterFeatureSet features = InputHandler::getCharacterFeatures(type);
    const int attackSlots = qBound(1, qMax(profile.attackCount, features.attackOptions), 3);

    QStringList abilities = {
        attackPatternFor(type, attackSlots),
        QStringLiteral("Personality: %1.").arg(personalityLabel(profile.personality)),
        projectileLineFor(type, profile),
        QStringLiteral("Ideal range: %1-%2 px.").arg(static_cast<int>(profile.idealMinRange)).arg(static_cast<int>(profile.idealMaxRange)),
        QStringLiteral("Unlock tier: %1; ability tier: %2.").arg(profile.unlockTier).arg(profile.abilityTier)
    };
    if (!specialMove.trimmed().isEmpty() && specialMove != abilities.first()) {
        abilities.insert(1, QStringLiteral("Style note: %1").arg(specialMove.trimmed()));
    }

    const int attack = qBound(20, static_cast<int>(42.0 + profile.aggression * 30.0 +
                                                   profile.comboBias * 18.0 +
                                                   profile.projectileBias * 12.0 +
                                                   profile.abilityTier * 2.0), 100);
    const int recovery = qBound(12, static_cast<int>(24.0 + profile.defense * 34.0 +
                                                     profile.retreatBias * 24.0), 100);
    const int mobility = qBound(18, static_cast<int>(28.0 + profile.movementBias * 24.0 +
                                                     profile.spacing * 30.0 +
                                                     profile.retreatBias * 16.0), 100);
    const int control = qBound(18, static_cast<int>(28.0 + profile.spacing * 36.0 +
                                                    profile.projectileBias * 24.0 +
                                                    profile.punishBias * 18.0), 100);

    return {
        profileDescriptionFor(type),
        abilities,
        attack,
        recovery,
        mobility,
        control
    };
}

QRect visibleBoundsForSheet(const QImage& sheetImage, int frameWidth, int frameHeight, int frameCount) {
    int minX = frameWidth;
    int minY = frameHeight;
    int maxX = -1;
    int maxY = -1;

    for (int frame = 0; frame < frameCount; ++frame) {
        const int xOffset = frame * frameWidth;
        for (int y = 0; y < frameHeight; ++y) {
            for (int x = 0; x < frameWidth; ++x) {
                if (qAlpha(sheetImage.pixel(xOffset + x, y)) > 8) {
                    minX = qMin(minX, x);
                    minY = qMin(minY, y);
                    maxX = qMax(maxX, x);
                    maxY = qMax(maxY, y);
                }
            }
        }
    }

    if (maxX < minX || maxY < minY) {
        return QRect(0, 0, frameWidth, frameHeight);
    }

    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1)
        .adjusted(-1, -1, 1, 1)
        .intersected(QRect(0, 0, frameWidth, frameHeight));
}

QVector<QPixmap> extractFramesFromSheet(const QString& spritePath, int frameCount) {
    QVector<QPixmap> frames;
    if (spritePath.isEmpty() || !QFileInfo::exists(spritePath) || frameCount <= 0) {
        return frames;
    }

    const QPixmap spriteSheet(spritePath);
    if (spriteSheet.isNull()) {
        return frames;
    }

    const int safeFrameCount = qMax(1, frameCount);
    const int frameWidth = spriteSheet.width() / safeFrameCount;
    const int frameHeight = spriteSheet.height();
    if (frameWidth <= 0 || frameHeight <= 0) {
        return frames;
    }

    const QImage sheetImage = spriteSheet.toImage().convertToFormat(QImage::Format_ARGB32);
    const QRect cropRect = visibleBoundsForSheet(sheetImage, frameWidth, frameHeight, safeFrameCount);

    for (int i = 0; i < safeFrameCount; ++i) {
        frames.append(spriteSheet.copy(i * frameWidth + cropRect.x(),
                                       cropRect.y(),
                                       cropRect.width(),
                                       cropRect.height()));
    }

    return frames;
}

} // namespace

ProfileLobbyWidget::ProfileLobbyWidget(QWidget* parent)
    : QWidget(parent),
      logoLabel_(nullptr),
      usernameLabel_(nullptr),
      scoreLabel_(nullptr),
      badgeLabel_(nullptr),
      avatarLabel_(nullptr),
      settingsButton_(nullptr),
      settingsMenu_(nullptr),
      previewEyebrowLabel_(nullptr),
      previewTitleLabel_(nullptr),
      previewModeChipLabel_(nullptr),
      lobbySummaryCharacterLabel_(nullptr),
      lobbySummaryRankLabel_(nullptr),
      lobbySummaryScoreLabel_(nullptr),
      lobbySummaryBadgeLabel_(nullptr),
      rankUpgradeOverlay_(nullptr),
      rankUpgradePanel_(nullptr),
      rankUpgradeEyebrowLabel_(nullptr),
      rankUpgradeTitleLabel_(nullptr),
      rankUpgradeBodyLabel_(nullptr),
      rankUpgradeBadgeRowWidget_(nullptr),
      rankUpgradeArrowLabel_(nullptr),
      rankUpgradeOldRankLabel_(nullptr),
      rankUpgradeNewRankLabel_(nullptr),
      rankUpgradeOldBadgeLabel_(nullptr),
      rankUpgradeNewBadgeLabel_(nullptr),
      rankUpgradeHintLabel_(nullptr),
      characterUnlockView_(nullptr),
      characterUnlockScene_(nullptr),
      characterUnlockGlowItem_(nullptr),
      characterUnlockPortraitItem_(nullptr),
      characterUnlockFighterItem_(nullptr),
      previewPortraitLabel_(nullptr),
      previewDescriptionLabel_(nullptr),
      previewMoveLabel_(nullptr),
      previewAbilitiesLabel_(nullptr),
      previewRoleChipLabel_(nullptr),
      previewAttacksChipLabel_(nullptr),
      previewRangeChipLabel_(nullptr),
      previewProjectileChipLabel_(nullptr),
      previewUnlockChipLabel_(nullptr),
      previewHintLabel_(nullptr),
      attackPowerBar_(nullptr),
      healPowerBar_(nullptr),
      mobilityPowerBar_(nullptr),
      controlPowerBar_(nullptr),
      characterView_(nullptr),
      previewScene_(nullptr),
      sceneBackgroundItem_(nullptr),
      sceneLockDimItem_(nullptr),
      characterItem_(nullptr),
      glowItem_(nullptr),
      characterLockItem_(nullptr),
      characterLockTextItem_(nullptr),
      fallbackTextItem_(nullptr),
      modeScrollArea_(nullptr),
      modeContainer_(nullptr),
      modeLayout_(nullptr),
      changeCharacterButton_(nullptr),
      enterArenaButton_(nullptr),
      actionSummaryLabel_(nullptr),
      actionHintLabel_(nullptr),
      duelSetupPanel_(nullptr),
      opponentModePicker_(nullptr),
      opponentCategoryPicker_(nullptr),
      opponentPicker_(nullptr),
      arenaPicker_(nullptr),
      duelSetupLabel_(nullptr),
      hoverGlowAnimation_(nullptr),
      rankUpgradeOverlayAnimation_(nullptr),
      characterUnlockRevealAnimation_(nullptr),
      enterArenaGlowEffect_(nullptr),
      idleAnimationTimer_(new QTimer(this)),
      idleFrameIndex_(0),
      showcasingAttack_(false),
      idleShowcaseElapsedMs_(0),
      rankUpgradeOverlayClosing_(false),
      showingCharacterUnlockPopup_(false),
      characterUnlockClaimReady_(true) {
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("profileLobbyRoot");
    setCursor(Qt::ArrowCursor);

    userProfile_ = {"Player_01", 0, "Wanderer", QString()};
    selectedCharacter_ = {"Demon Slayer", QString(), "Infernal blade style with ranged fireball pressure."};
    selectedModeName_ = kPlayableLobbyMode;
    duelSetup_ = {"Random", "Player", QString(), "Colosseum"};

    idleAnimationTimer_->setInterval(110);
    connect(idleAnimationTimer_, &QTimer::timeout, this, &ProfileLobbyWidget::advanceIdleAnimation);

    setupUi();
    refreshProfileUi();
    refreshCharacterPreview();
    refreshModeSelectionUi();
}

ProfileLobbyWidget::~ProfileLobbyWidget() = default;

void ProfileLobbyWidget::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(18);

    setupHeader(rootLayout);

    auto* contentRow = new QWidget(this);
    auto* contentLayout = new QHBoxLayout(contentRow);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(18);

    setupCharacterPreview(contentLayout);
    setupModeCarousel(contentLayout);
    rootLayout->addWidget(contentRow, 1);

    setupBottomBar(rootLayout);

    rootLayout->setStretch(0, 0);
    rootLayout->setStretch(1, 1);
    rootLayout->setStretch(2, 0);

    setLayout(rootLayout);
    setupRankUpgradeOverlay();
}

void ProfileLobbyWidget::setupRankUpgradeOverlay() {
    rankUpgradeOverlay_ = new QWidget(this);
    rankUpgradeOverlay_->setObjectName("rankUpgradeOverlay");
    rankUpgradeOverlay_->setAttribute(Qt::WA_StyledBackground, true);
    rankUpgradeOverlay_->setFocusPolicy(Qt::StrongFocus);
    rankUpgradeOverlay_->setStyleSheet(
        "QWidget#rankUpgradeOverlay {"
        " background: rgba(5, 4, 3, 152);"
        "}"
    );
    rankUpgradeOverlay_->hide();
    rankUpgradeOverlay_->installEventFilter(this);

    auto* overlayEffect = new QGraphicsOpacityEffect(rankUpgradeOverlay_);
    overlayEffect->setOpacity(0.0);
    rankUpgradeOverlay_->setGraphicsEffect(overlayEffect);

    auto* overlayLayout = new QVBoxLayout(rankUpgradeOverlay_);
    overlayLayout->setContentsMargins(36, 36, 36, 36);
    overlayLayout->setAlignment(Qt::AlignCenter);

    rankUpgradePanel_ = new QFrame(rankUpgradeOverlay_);
    rankUpgradePanel_->setObjectName("rankUpgradePanel");
    rankUpgradePanel_->setMinimumWidth(680);
    rankUpgradePanel_->setMaximumWidth(800);
    rankUpgradePanel_->setStyleSheet(
        "QFrame#rankUpgradePanel {"
        " background: rgba(26, 17, 12, 224);"
        " border: 1px solid rgba(226, 170, 81, 0.66);"
        " border-radius: 28px;"
        "}"
        "QLabel#rankUpgradeEyebrow { color:rgba(245,213,143,0.78); font:800 11px 'Segoe UI'; letter-spacing:2px; }"
        "QLabel#rankUpgradeTitle { color:#FFF0C6; font:900 34px 'Segoe UI'; letter-spacing:1.3px; }"
        "QLabel#rankUpgradeBody { color:rgba(245,230,184,0.88); font:14px 'Segoe UI'; }"
        "QLabel#rankUpgradeOldBadge, QLabel#rankUpgradeNewBadge {"
        " color:rgba(245,230,184,0.82);"
        " background: rgba(8, 7, 6, 0.34);"
        " border: 1px solid rgba(212, 160, 23, 0.24);"
        " border-radius: 22px;"
        "}"
        "QLabel#rankUpgradeOldRank {"
        " color:rgba(245,230,184,0.72);"
        " font:900 15px 'Segoe UI';"
        " letter-spacing:1.1px;"
        "}"
        "QLabel#rankUpgradeNewRank {"
        " color:#FFD36A;"
        " font:900 24px 'Segoe UI';"
        " letter-spacing:1.1px;"
        "}"
        "QLabel#rankUpgradeArrow { color:#E5B95B; font:900 28px 'Segoe UI'; }"
        "QLabel#rankUpgradeHint {"
        " color:#FFF2D4;"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #8C1111, stop:1 #E1502C);"
        " border: 2px solid rgba(255, 187, 140, 0.88);"
        " border-radius: 14px;"
        " padding: 12px 18px;"
        " font:800 14px 'Segoe UI';"
        "}"
    );
    rankUpgradePanel_->installEventFilter(this);

    auto* shadow = new QGraphicsDropShadowEffect(rankUpgradePanel_);
    shadow->setBlurRadius(38.0);
    shadow->setOffset(0.0, 18.0);
    shadow->setColor(QColor(0, 0, 0, 168));
    rankUpgradePanel_->setGraphicsEffect(shadow);

    auto* panelLayout = new QVBoxLayout(rankUpgradePanel_);
    panelLayout->setContentsMargins(38, 32, 38, 32);
    panelLayout->setSpacing(16);

    rankUpgradeEyebrowLabel_ = new QLabel("NEW TITLE UNLOCKED", rankUpgradePanel_);
    rankUpgradeEyebrowLabel_->setObjectName("rankUpgradeEyebrow");
    rankUpgradeEyebrowLabel_->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(rankUpgradeEyebrowLabel_);

    rankUpgradeTitleLabel_ = new QLabel("RANK UPGRADED", rankUpgradePanel_);
    rankUpgradeTitleLabel_->setObjectName("rankUpgradeTitle");
    rankUpgradeTitleLabel_->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(rankUpgradeTitleLabel_);

    rankUpgradeBadgeRowWidget_ = new QWidget(rankUpgradePanel_);
    auto* badgeRow = new QHBoxLayout(rankUpgradeBadgeRowWidget_);
    badgeRow->setContentsMargins(0, 0, 0, 0);
    badgeRow->setSpacing(22);

    auto* oldRankStack = new QVBoxLayout();
    oldRankStack->setSpacing(8);
    rankUpgradeOldBadgeLabel_ = new QLabel(rankUpgradePanel_);
    rankUpgradeOldBadgeLabel_->setObjectName("rankUpgradeOldBadge");
    rankUpgradeOldBadgeLabel_->setFixedSize(138, 138);
    rankUpgradeOldBadgeLabel_->setAlignment(Qt::AlignCenter);
    rankUpgradeOldBadgeLabel_->setScaledContents(false);
    oldRankStack->addWidget(rankUpgradeOldBadgeLabel_, 0, Qt::AlignHCenter);

    rankUpgradeOldRankLabel_ = new QLabel(rankUpgradePanel_);
    rankUpgradeOldRankLabel_->setObjectName("rankUpgradeOldRank");
    rankUpgradeOldRankLabel_->setAlignment(Qt::AlignCenter);
    oldRankStack->addWidget(rankUpgradeOldRankLabel_);
    badgeRow->addLayout(oldRankStack, 1);

    rankUpgradeArrowLabel_ = new QLabel("->", rankUpgradePanel_);
    rankUpgradeArrowLabel_->setObjectName("rankUpgradeArrow");
    rankUpgradeArrowLabel_->setAlignment(Qt::AlignCenter);
    badgeRow->addWidget(rankUpgradeArrowLabel_, 0, Qt::AlignVCenter);

    auto* newRankStack = new QVBoxLayout();
    newRankStack->setSpacing(8);
    rankUpgradeNewBadgeLabel_ = new QLabel(rankUpgradePanel_);
    rankUpgradeNewBadgeLabel_->setObjectName("rankUpgradeNewBadge");
    rankUpgradeNewBadgeLabel_->setFixedSize(198, 198);
    rankUpgradeNewBadgeLabel_->setAlignment(Qt::AlignCenter);
    rankUpgradeNewBadgeLabel_->setScaledContents(false);
    newRankStack->addWidget(rankUpgradeNewBadgeLabel_, 0, Qt::AlignHCenter);

    rankUpgradeNewRankLabel_ = new QLabel(rankUpgradePanel_);
    rankUpgradeNewRankLabel_->setObjectName("rankUpgradeNewRank");
    rankUpgradeNewRankLabel_->setAlignment(Qt::AlignCenter);
    newRankStack->addWidget(rankUpgradeNewRankLabel_);
    badgeRow->addLayout(newRankStack, 1);

    panelLayout->addWidget(rankUpgradeBadgeRowWidget_);

    characterUnlockView_ = new QGraphicsView(rankUpgradePanel_);
    characterUnlockView_->setFixedSize(620, 360);
    characterUnlockView_->setFrameShape(QFrame::NoFrame);
    characterUnlockView_->setRenderHint(QPainter::Antialiasing, true);
    characterUnlockView_->setRenderHint(QPainter::SmoothPixmapTransform, true);
    characterUnlockView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    characterUnlockView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    characterUnlockView_->setStyleSheet("background: transparent; border: 0;");
    characterUnlockView_->hide();

    characterUnlockScene_ = new QGraphicsScene(characterUnlockView_);
    characterUnlockScene_->setSceneRect(0, 0, 620, 360);
    characterUnlockScene_->setBackgroundBrush(Qt::transparent);
    characterUnlockView_->setScene(characterUnlockScene_);

    characterUnlockGlowItem_ = characterUnlockScene_->addPixmap(createGoldGlowPixmap(310));
    characterUnlockGlowItem_->setZValue(-2.0);
    characterUnlockGlowItem_->setOpacity(0.0);

    characterUnlockPortraitItem_ = characterUnlockScene_->addPixmap(QPixmap());
    characterUnlockPortraitItem_->setZValue(2.0);
    auto* portraitShadow = new QGraphicsDropShadowEffect(this);
    portraitShadow->setBlurRadius(44.0);
    portraitShadow->setOffset(0.0, 18.0);
    portraitShadow->setColor(QColor(0, 0, 0, 170));
    characterUnlockPortraitItem_->setGraphicsEffect(portraitShadow);

    characterUnlockFighterItem_ = characterUnlockScene_->addPixmap(QPixmap());
    characterUnlockFighterItem_->setZValue(3.0);
    characterUnlockFighterItem_->setOpacity(0.0);

    panelLayout->addWidget(characterUnlockView_, 0, Qt::AlignHCenter);

    rankUpgradeBodyLabel_ = new QLabel(rankUpgradePanel_);
    rankUpgradeBodyLabel_->setObjectName("rankUpgradeBody");
    rankUpgradeBodyLabel_->setWordWrap(true);
    rankUpgradeBodyLabel_->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(rankUpgradeBodyLabel_);

    rankUpgradeHintLabel_ = new QLabel("PRESS SPACE TO CLAIM", rankUpgradePanel_);
    rankUpgradeHintLabel_->setObjectName("rankUpgradeHint");
    rankUpgradeHintLabel_->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(rankUpgradeHintLabel_);

    overlayLayout->addWidget(rankUpgradePanel_, 0, Qt::AlignCenter);

    rankUpgradeOverlayAnimation_ = new QVariantAnimation(this);
    rankUpgradeOverlayAnimation_->setDuration(260);
    rankUpgradeOverlayAnimation_->setEasingCurve(QEasingCurve::OutCubic);
    connect(rankUpgradeOverlayAnimation_, &QVariantAnimation::valueChanged, this, [overlayEffect](const QVariant& value) {
        overlayEffect->setOpacity(value.toReal());
    });
    connect(rankUpgradeOverlayAnimation_, &QVariantAnimation::finished, this, [this]() {
        if (rankUpgradeOverlayClosing_ && rankUpgradeOverlay_) {
            rankUpgradeOverlay_->hide();
            if (characterUnlockRevealAnimation_) {
                characterUnlockRevealAnimation_->stop();
            }
            if (characterUnlockView_) {
                characterUnlockView_->hide();
            }
            if (rankUpgradeBadgeRowWidget_) {
                rankUpgradeBadgeRowWidget_->show();
            }
            showingCharacterUnlockPopup_ = false;
            characterUnlockClaimReady_ = true;
            if (!pendingCharacterUnlockName_.trimmed().isEmpty()) {
                const QString characterName = pendingCharacterUnlockName_;
                const QString rankName = pendingCharacterUnlockRank_;
                const QString imagePath = pendingCharacterUnlockImagePath_;
                pendingCharacterUnlockName_.clear();
                pendingCharacterUnlockRank_.clear();
                pendingCharacterUnlockImagePath_.clear();
                QTimer::singleShot(90, this, [this, characterName, rankName, imagePath]() {
                    showCharacterUnlockPopup(characterName, rankName, imagePath);
                });
            }
        }
    });

    characterUnlockRevealAnimation_ = new QVariantAnimation(this);
    characterUnlockRevealAnimation_->setDuration(kCharacterUnlockRevealMs);
    characterUnlockRevealAnimation_->setStartValue(0.0);
    characterUnlockRevealAnimation_->setEndValue(1.0);
    characterUnlockRevealAnimation_->setEasingCurve(QEasingCurve::Linear);
    connect(characterUnlockRevealAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        updateCharacterUnlockReveal(value.toReal());
    });
    connect(characterUnlockRevealAnimation_, &QVariantAnimation::finished, this, [this]() {
        updateCharacterUnlockReveal(1.0);
        characterUnlockClaimReady_ = true;
        if (rankUpgradeHintLabel_) {
            rankUpgradeHintLabel_->show();
        }
        if (rankUpgradeOverlay_) {
            rankUpgradeOverlay_->setFocus(Qt::OtherFocusReason);
        }
    });

    syncRankUpgradeOverlay();
}

void ProfileLobbyWidget::prepareCharacterUnlockReveal(const QString& characterName, const QString& imagePath) {
    characterUnlockIdleFrames_.clear();

    if (!characterUnlockScene_ || !characterUnlockPortraitItem_ || !characterUnlockFighterItem_) {
        return;
    }

    const QString resolvedImagePath = resolveAssetPath(imagePath);
    const QString portraitPath = profilePortraitPathForCharacter(characterName, resolvedImagePath);
    QPixmap portrait(!portraitPath.isEmpty() ? portraitPath : resolvedImagePath);
    QPixmap portraitPixmap = circularPortraitPixmap(portrait, QSize(250, 250));
    if (portraitPixmap.isNull()) {
        portraitPixmap = createCharacterPlaceholder(QSize(250, 250), characterName);
    }

    characterUnlockPortraitItem_->setPixmap(portraitPixmap);
    characterUnlockPortraitItem_->setTransformOriginPoint(characterUnlockPortraitItem_->boundingRect().center());

    const LobbyAnimationSpec spec = animationSpecForCharacterName(characterName);
    const QFileInfo imageInfo(resolvedImagePath);
    const QString idleSpritePath = spec.idlePath.isEmpty()
        ? QString()
        : imageInfo.dir().filePath(spec.idlePath);
    characterUnlockIdleFrames_ = extractFramesFromSheet(idleSpritePath, spec.idleFrameCount);

    QPixmap fighterFrame = characterUnlockIdleFrames_.isEmpty()
        ? QPixmap(resolvedImagePath)
        : characterUnlockIdleFrames_.first();
    if (fighterFrame.isNull()) {
        fighterFrame = createCharacterPlaceholder(QSize(220, 260), characterName);
    }

    characterUnlockFighterItem_->setPixmap(fighterFrame);
    characterUnlockFighterItem_->setTransformOriginPoint(characterUnlockFighterItem_->boundingRect().center());
    updateCharacterUnlockReveal(0.0);
}

void ProfileLobbyWidget::updateCharacterUnlockReveal(qreal progress) {
    if (!characterUnlockScene_ || !characterUnlockPortraitItem_ || !characterUnlockFighterItem_) {
        return;
    }

    const QRectF sceneRect = characterUnlockScene_->sceneRect();
    const QPointF portraitCenter(sceneRect.width() * 0.32, sceneRect.height() * 0.52);
    const QPointF fighterCenter(sceneRect.width() * 0.70, sceneRect.height() * 0.57);

    if (characterUnlockGlowItem_) {
        const qreal glowRamp = smoothStep(scaledBetween(0.0, 0.30, progress));
        const qreal glowOpacity = 0.26 + glowRamp * 0.42;
        characterUnlockGlowItem_->setOpacity(glowOpacity);
        characterUnlockGlowItem_->setScale(0.90 + smoothStep(progress) * 0.18);
        const QRectF glowBounds = characterUnlockGlowItem_->boundingRect();
        characterUnlockGlowItem_->setPos(fighterCenter.x() - glowBounds.width() * characterUnlockGlowItem_->scale() * 0.5,
                                         fighterCenter.y() - glowBounds.height() * characterUnlockGlowItem_->scale() * 0.5);
    }

    const QRectF portraitBounds = characterUnlockPortraitItem_->boundingRect();
    const qreal portraitIn = smoothStep(scaledBetween(0.0, 0.25, progress));
    const qreal portraitScale = 0.78 + portraitIn * 0.22;
    characterUnlockPortraitItem_->setOpacity(qBound(0.0, portraitIn, 1.0));
    characterUnlockPortraitItem_->setScale(portraitScale);
    characterUnlockPortraitItem_->setPos(portraitCenter.x() - portraitBounds.width() * 0.5,
                                         portraitCenter.y() - portraitBounds.height() * 0.5);
    characterUnlockPortraitItem_->setVisible(characterUnlockPortraitItem_->opacity() > 0.02);

    if (!characterUnlockIdleFrames_.isEmpty()) {
        const qreal revealProgress = scaledBetween(0.24, 1.0, progress);
        const int frameIndex = qBound(0,
                                      static_cast<int>((revealProgress * kCharacterUnlockRevealMs) / 110.0) % characterUnlockIdleFrames_.size(),
                                      characterUnlockIdleFrames_.size() - 1);
        characterUnlockFighterItem_->setPixmap(characterUnlockIdleFrames_.at(frameIndex));
    }

    const QRectF fighterBounds = characterUnlockFighterItem_->boundingRect();
    const qreal fighterIn = smoothStep(scaledBetween(0.18, 0.48, progress));
    const qreal maxFighterWidth = sceneRect.width() * 0.34;
    const qreal maxFighterHeight = sceneRect.height() * 0.72;
    const qreal fighterScale = qMin(maxFighterWidth / qMax(1.0, fighterBounds.width()),
                                    maxFighterHeight / qMax(1.0, fighterBounds.height()));
    characterUnlockFighterItem_->setOpacity(fighterIn);
    characterUnlockFighterItem_->setScale(fighterScale * (0.92 + fighterIn * 0.08));
    characterUnlockFighterItem_->setPos(fighterCenter.x() - fighterBounds.width() * 0.5,
                                        sceneRect.height() * 0.84 - fighterBounds.height() * 0.5);
    characterUnlockFighterItem_->setVisible(fighterIn > 0.02);
}

void ProfileLobbyWidget::showRankUpgradePopup(const QString& previousRank, const QString& newRank) {
    if (!rankUpgradeOverlay_ || previousRank.trimmed().isEmpty() || newRank.trimmed().isEmpty()
        || previousRank.compare(newRank, Qt::CaseInsensitive) == 0) {
        return;
    }

    const QString oldRank = previousRank.trimmed();
    const QString upgradedRank = newRank.trimmed();
    showingCharacterUnlockPopup_ = false;
    characterUnlockClaimReady_ = true;
    if (characterUnlockRevealAnimation_) {
        characterUnlockRevealAnimation_->stop();
    }
    if (characterUnlockView_) {
        characterUnlockView_->hide();
    }
    if (rankUpgradeBadgeRowWidget_) {
        rankUpgradeBadgeRowWidget_->show();
    }
    if (rankUpgradeEyebrowLabel_) {
        rankUpgradeEyebrowLabel_->setText(QStringLiteral("NEW TITLE UNLOCKED"));
    }
    if (rankUpgradeHintLabel_) {
        rankUpgradeHintLabel_->show();
    }

    rankUpgradeOldRankLabel_->setText(oldRank.toUpper());
    rankUpgradeNewRankLabel_->setText(upgradedRank.toUpper());
    rankUpgradeOldRankLabel_->show();
    rankUpgradeNewRankLabel_->show();
    rankUpgradeOldBadgeLabel_->show();
    rankUpgradeNewBadgeLabel_->show();
    if (rankUpgradeArrowLabel_) {
        rankUpgradeArrowLabel_->show();
    }
    rankUpgradeTitleLabel_->setText(QStringLiteral("RANK UPGRADED"));
    rankUpgradeHintLabel_->setText(QStringLiteral("PRESS SPACE TO CLAIM"));
    rankUpgradeBodyLabel_->setText(QStringLiteral("Your arena record has advanced from %1 to %2. Claim the title and carry it into the next battle.")
                                       .arg(oldRank, upgradedRank));

    const QPixmap oldBadge = rankBadgePixmap(oldRank, 126);
    rankUpgradeOldBadgeLabel_->setPixmap(oldBadge);
    rankUpgradeOldBadgeLabel_->setText(oldBadge.isNull() ? oldRank : QString());

    const QPixmap newBadge = rankBadgePixmap(upgradedRank, 186);
    rankUpgradeNewBadgeLabel_->setPixmap(newBadge);
    rankUpgradeNewBadgeLabel_->setText(newBadge.isNull() ? upgradedRank : QString());

    syncRankUpgradeOverlay();
    rankUpgradeOverlayClosing_ = false;
    rankUpgradeOverlay_->show();
    rankUpgradeOverlay_->raise();
    rankUpgradeOverlay_->setFocus(Qt::OtherFocusReason);

    if (rankUpgradeOverlayAnimation_) {
        rankUpgradeOverlayAnimation_->stop();
        rankUpgradeOverlayAnimation_->setStartValue(0.0);
        rankUpgradeOverlayAnimation_->setEndValue(1.0);
        rankUpgradeOverlayAnimation_->start();
    }
}

void ProfileLobbyWidget::showCharacterUnlockPopup(const QString& characterName,
                                                  const QString& rankName,
                                                  const QString& imagePath) {
    if (!rankUpgradeOverlay_ || characterName.trimmed().isEmpty()) {
        return;
    }

    if (rankUpgradeOverlay_->isVisible() && !rankUpgradeOverlayClosing_) {
        pendingCharacterUnlockName_ = characterName;
        pendingCharacterUnlockRank_ = rankName;
        pendingCharacterUnlockImagePath_ = imagePath;
        return;
    }

    displayCharacterUnlockPopup(characterName, rankName, imagePath);
}

void ProfileLobbyWidget::displayCharacterUnlockPopup(const QString& characterName,
                                                     const QString& rankName,
                                                     const QString& imagePath) {
    if (!rankUpgradeOverlay_ || characterName.trimmed().isEmpty()) {
        return;
    }

    const QString fighterName = characterName.trimmed();
    const QString unlockedRank = rankName.trimmed();
    showingCharacterUnlockPopup_ = true;
    characterUnlockClaimReady_ = true;

    if (rankUpgradeOverlayAnimation_) {
        rankUpgradeOverlayAnimation_->stop();
    }
    if (auto* overlayEffect = qobject_cast<QGraphicsOpacityEffect*>(rankUpgradeOverlay_->graphicsEffect())) {
        overlayEffect->setOpacity(1.0);
    }

    if (rankUpgradeEyebrowLabel_) {
        rankUpgradeEyebrowLabel_->setText(QStringLiteral("NEW GLADIATOR UNLOCKED"));
    }
    if (rankUpgradeBadgeRowWidget_) {
        rankUpgradeBadgeRowWidget_->hide();
    }
    if (rankUpgradeOldRankLabel_) {
        rankUpgradeOldRankLabel_->clear();
    }
    if (rankUpgradeNewRankLabel_) {
        rankUpgradeNewRankLabel_->clear();
    }
    if (rankUpgradeOldBadgeLabel_) {
        rankUpgradeOldBadgeLabel_->clear();
    }
    if (rankUpgradeNewBadgeLabel_) {
        rankUpgradeNewBadgeLabel_->clear();
    }
    if (characterUnlockView_) {
        characterUnlockView_->show();
    }

    rankUpgradeTitleLabel_->setText(QStringLiteral("CONGRATS, %1 IS YOURS").arg(fighterName.toUpper()));
    rankUpgradeBodyLabel_->setText(
        unlockedRank.isEmpty()
            ? QStringLiteral("%1 has joined your roster. Their profile portrait and idle stance are now available in the lobby.")
                  .arg(fighterName)
            : QStringLiteral("%1 has joined your roster at %2 rank. Their profile portrait and idle stance are now available in the lobby.")
                  .arg(fighterName, unlockedRank));
    rankUpgradeHintLabel_->setText(QStringLiteral("PRESS SPACE TO CLAIM FIGHTER"));
    rankUpgradeHintLabel_->hide();

    prepareCharacterUnlockReveal(fighterName, imagePath);
    if (characterUnlockRevealAnimation_) {
        characterUnlockRevealAnimation_->stop();
        characterUnlockRevealAnimation_->setStartValue(0.0);
        characterUnlockRevealAnimation_->setEndValue(1.0);
        characterUnlockRevealAnimation_->setDuration(kCharacterUnlockRevealMs);
    }
    updateCharacterUnlockReveal(1.0);

    syncRankUpgradeOverlay();
    rankUpgradeOverlayClosing_ = false;
    rankUpgradeOverlay_->show();
    rankUpgradeOverlay_->raise();
    rankUpgradeOverlay_->setFocus(Qt::OtherFocusReason);

    if (rankUpgradeHintLabel_) {
        rankUpgradeHintLabel_->show();
    }
}

void ProfileLobbyWidget::claimRankUpgradePopup() {
    if (!rankUpgradeOverlay_ || !rankUpgradeOverlay_->isVisible()) {
        return;
    }
    if (showingCharacterUnlockPopup_ && !characterUnlockClaimReady_) {
        return;
    }

    if (!showingCharacterUnlockPopup_ && !pendingCharacterUnlockName_.trimmed().isEmpty()) {
        const QString characterName = pendingCharacterUnlockName_;
        const QString rankName = pendingCharacterUnlockRank_;
        const QString imagePath = pendingCharacterUnlockImagePath_;
        pendingCharacterUnlockName_.clear();
        pendingCharacterUnlockRank_.clear();
        pendingCharacterUnlockImagePath_.clear();
        if (rankUpgradeOverlayAnimation_) {
            rankUpgradeOverlayAnimation_->stop();
        }
        rankUpgradeOverlayClosing_ = false;
        displayCharacterUnlockPopup(characterName, rankName, imagePath);
        return;
    }

    rankUpgradeOverlayClosing_ = true;
    if (characterUnlockRevealAnimation_) {
        characterUnlockRevealAnimation_->stop();
    }
    if (rankUpgradeOverlayAnimation_) {
        rankUpgradeOverlayAnimation_->stop();
        rankUpgradeOverlayAnimation_->setStartValue(1.0);
        rankUpgradeOverlayAnimation_->setEndValue(0.0);
        rankUpgradeOverlayAnimation_->start();
        return;
    }

    rankUpgradeOverlay_->hide();
}

void ProfileLobbyWidget::syncRankUpgradeOverlay() {
    if (!rankUpgradeOverlay_) {
        return;
    }

    rankUpgradeOverlay_->setGeometry(rect());
}

void ProfileLobbyWidget::setupHeader(QBoxLayout* rootLayout) {
    auto* header = new QFrame(this);
    header->setObjectName("lobbyHeader");
    header->setFixedHeight(112);
    header->setStyleSheet(
        "QFrame#lobbyHeader {"
        " background: rgba(20,15,12,0.80);"
        " border: 1px solid rgba(212,160,23,0.38);"
        " border-radius: 18px;"
        "}"
    );

    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 8, 24, 8);
    headerLayout->setSpacing(22);

    auto* brandWrap = new QWidget(header);
    brandWrap->setFixedWidth(320);
    auto* brandLayout = new QHBoxLayout(brandWrap);
    brandLayout->setContentsMargins(0, 0, 0, 0);

    logoLabel_ = new QLabel(brandWrap);
    logoLabel_->setFixedSize(300, 96);
    logoLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    const QPixmap logoPixmap(resolveAssetPath("assets/backgrounds/logo.png"));
    if (!logoPixmap.isNull()) {
        logoLabel_->setPixmap(logoPixmap.scaled(300, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        logoLabel_->setStyleSheet("background: transparent;");
    } else {
        logoLabel_->setText("GLADIATORS");
        logoLabel_->setStyleSheet("color:#F0C96C; font: 900 34px 'Segoe UI'; letter-spacing: 2px;");
    }
    brandLayout->addWidget(logoLabel_, 0, Qt::AlignLeft | Qt::AlignVCenter);

    headerLayout->addWidget(brandWrap, 0, Qt::AlignVCenter | Qt::AlignLeft);

    auto* centerWrap = new QWidget(header);
    auto* centerLayout = new QVBoxLayout(centerWrap);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(4);

    auto* subtitleLabel = new QLabel("Forge your fighter, choose your ruleset, enter the arena.", centerWrap);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "color:#F7E4B4;"
        "font: italic 800 20px 'Georgia';"
        "letter-spacing:0.7px;"
        "background:transparent;"
    );
    centerLayout->addWidget(subtitleLabel, 0, Qt::AlignCenter);

    auto* subtitleAccent = new QLabel("Choose with purpose. Fight with honor.", centerWrap);
    subtitleAccent->setAlignment(Qt::AlignCenter);
    subtitleAccent->setStyleSheet(
        "color:rgba(212,160,23,0.72);"
        "font:700 10px 'Segoe UI';"
        "letter-spacing:2.4px;"
        "background:transparent;"
    );
    centerLayout->addWidget(subtitleAccent, 0, Qt::AlignCenter);

    headerLayout->addWidget(centerWrap, 1, Qt::AlignCenter);

    auto* rightWrap = new QWidget(header);
    rightWrap->setFixedWidth(320);
    auto* rightLayout = new QHBoxLayout(rightWrap);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);
    rightLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    usernameLabel_ = new QLabel(rightWrap);
    usernameLabel_->setCursor(Qt::PointingHandCursor);
    usernameLabel_->setStyleSheet(
        "color:#FFF2CF; font: 900 17px 'Segoe UI';"
        "padding:9px 18px; border-radius:17px;"
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 rgba(80,55,28,0.92), stop:1 rgba(37,28,21,0.88));"
        "border:1px solid rgba(226,176,82,0.48);"
        "letter-spacing:0.3px;"
    );
    usernameLabel_->installEventFilter(this);
    rightLayout->addWidget(usernameLabel_);

    avatarLabel_ = new QLabel(rightWrap);
    avatarLabel_->setFixedSize(46, 46);
    avatarLabel_->setAlignment(Qt::AlignCenter);
    avatarLabel_->setStyleSheet(
        "background:#3B2A1D; border:2px solid #D4A017; border-radius:23px;"
    );
    rightLayout->addWidget(avatarLabel_);

    settingsButton_ = new QToolButton(rightWrap);
    settingsButton_->setText(QString::fromUtf8("\xE2\x9A\x99"));
    settingsButton_->setToolTip("Settings");
    settingsButton_->setCursor(Qt::PointingHandCursor);
    settingsButton_->setFixedSize(46, 46);
    settingsButton_->setStyleSheet(
        "QToolButton {"
        " color:#FFF0C6; font:900 24px 'Segoe UI Symbol';"
        " border:1px solid rgba(226,176,82,0.56); border-radius:23px;"
        " background:rgba(212,160,23,0.12);"
        "}"
        "QToolButton:hover { background: rgba(212,160,23,0.24); border-color:#E0B35A; }"
        "QToolButton:pressed { background: rgba(140,90,24,0.38); }"
    );
    connect(settingsButton_, &QToolButton::clicked, this, [this]() {
        emit settingsActionTriggered(QStringLiteral("Settings"));
    });
    rightLayout->addWidget(settingsButton_);

    headerLayout->addWidget(rightWrap, 0, Qt::AlignRight);
    rootLayout->addWidget(header);
}

void ProfileLobbyWidget::setupCharacterPreview(QBoxLayout* rootLayout) {
    auto* previewFrame = new QFrame(this);
    previewFrame->setObjectName("characterPreviewFrame");
    previewFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewFrame->setStyleSheet(
        "QFrame#characterPreviewFrame {"
        " background: rgba(18,14,12,0.72);"
        " border:1px solid rgba(212,160,23,0.24);"
        " border-radius:24px;"
        "}"
    );

    auto* previewLayout = new QVBoxLayout(previewFrame);
    previewLayout->setContentsMargins(22, 18, 22, 18);
    previewLayout->setSpacing(12);

    auto* previewHeader = new QWidget(previewFrame);
    auto* previewHeaderLayout = new QHBoxLayout(previewHeader);
    previewHeaderLayout->setContentsMargins(0, 0, 0, 0);
    previewHeaderLayout->setSpacing(12);

    previewEyebrowLabel_ = new QLabel("FEATURED FIGHTER", previewHeader);
    previewEyebrowLabel_->setStyleSheet(
        "color:#F4D895; font:700 11px 'Segoe UI'; letter-spacing:1px;"
        "padding:6px 10px; border-radius:10px;"
        "background:rgba(212,160,23,0.12); border:1px solid rgba(212,160,23,0.22);"
    );
    previewHeaderLayout->addWidget(previewEyebrowLabel_, 0, Qt::AlignLeft);

    previewHeaderLayout->addStretch(1);

    previewModeChipLabel_ = new QLabel("DUEL", previewHeader);
    previewModeChipLabel_->setStyleSheet(
        "color:#FFF2CF; font:700 11px 'Segoe UI'; letter-spacing:1px;"
        "padding:6px 10px; border-radius:10px;"
        "background:rgba(124,22,22,0.45); border:1px solid rgba(219,100,81,0.36);"
    );
    previewHeaderLayout->addWidget(previewModeChipLabel_, 0, Qt::AlignRight);
    previewLayout->addWidget(previewHeader);

    auto* contentRow = new QWidget(previewFrame);
    auto* contentLayout = new QHBoxLayout(contentRow);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);

    auto* plainPanel = new QFrame(contentRow);
    plainPanel->setObjectName("fighterPlainPanel");
    plainPanel->setMinimumWidth(300);
    plainPanel->setStyleSheet(
        "QFrame#fighterPlainPanel {"
        " background: rgba(20,16,14,0.56);"
        " border:1px solid rgba(212,160,23,0.10);"
        " border-radius:18px;"
        "}"
    );

    auto* plainLayout = new QVBoxLayout(plainPanel);
    plainLayout->setContentsMargins(16, 16, 16, 16);
    plainLayout->setSpacing(12);

    auto* summaryEyebrow = new QLabel("LOBBY PROFILE", plainPanel);
    summaryEyebrow->setStyleSheet(
        "color:#F4D895; font:700 11px 'Segoe UI'; letter-spacing:1px;"
        "padding:6px 10px; border-radius:10px;"
        "background:rgba(212,160,23,0.12); border:1px solid rgba(212,160,23,0.22);"
    );
    plainLayout->addWidget(summaryEyebrow, 0, Qt::AlignLeft);

    const auto createSummaryValue = [plainPanel, plainLayout](const QString& title, QLabel** outLabel) {
        auto* block = new QWidget(plainPanel);
        block->setStyleSheet("background:transparent; border:none;");
        auto* blockLayout = new QVBoxLayout(block);
        blockLayout->setContentsMargins(0, 0, 0, 0);
        blockLayout->setSpacing(3);

        auto* titleLabel = new QLabel(title, block);
        titleLabel->setStyleSheet(
            "color:rgba(244,216,149,0.76);"
            "font:800 10px 'Segoe UI';"
            "letter-spacing:1px;"
            "background:transparent;"
            "border:none;"
        );
        blockLayout->addWidget(titleLabel);

        auto* valueLabel = new QLabel(block);
        valueLabel->setWordWrap(true);
        valueLabel->setStyleSheet(
            "color:#FFF0C6;"
            "font:800 15px 'Segoe UI';"
            "background:transparent;"
            "border:none;"
        );
        blockLayout->addWidget(valueLabel);

        plainLayout->addWidget(block);
        *outLabel = valueLabel;
    };

    plainLayout->addSpacing(4);

    auto* badgeTitle = new QLabel("RANK BADGE", plainPanel);
    badgeTitle->setStyleSheet(
        "color:rgba(244,216,149,0.76);"
        "font:800 10px 'Segoe UI';"
        "letter-spacing:1px;"
        "background:transparent;"
        "border:none;"
    );
    plainLayout->addWidget(badgeTitle);
    plainLayout->addSpacing(6);

    lobbySummaryBadgeLabel_ = new QLabel(plainPanel);
    lobbySummaryBadgeLabel_->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    lobbySummaryBadgeLabel_->setMinimumHeight(255);
    lobbySummaryBadgeLabel_->setScaledContents(false);
    lobbySummaryBadgeLabel_->setStyleSheet(
        "color:rgba(245,230,184,0.80);"
        "font:700 12px 'Segoe UI';"
        "letter-spacing:0.4px;"
        "background:transparent;"
        "border:none;"
        "padding:0px;"
    );
    plainLayout->addWidget(lobbySummaryBadgeLabel_);
    plainLayout->addSpacing(8);

    createSummaryValue("RANK", &lobbySummaryRankLabel_);
    createSummaryValue("TOTAL SCORE", &lobbySummaryScoreLabel_);
    createSummaryValue("CURRENT FIGHTER", &lobbySummaryCharacterLabel_);
    if (lobbySummaryRankLabel_) {
        lobbySummaryRankLabel_->setStyleSheet(
            "color:#FFD36A;"
            "font:900 26px 'Segoe UI';"
            "background:transparent;"
            "border:none;"
        );
    }
    if (lobbySummaryScoreLabel_) {
        lobbySummaryScoreLabel_->setStyleSheet(
            "color:#F0B45B;"
            "font:900 16px 'Segoe UI';"
            "background:transparent;"
            "border:none;"
        );
    }
    if (lobbySummaryCharacterLabel_) {
        lobbySummaryCharacterLabel_->setStyleSheet(
            "color:#FFF0C6;"
            "font:800 15px 'Segoe UI';"
            "background:transparent;"
            "border:none;"
        );
    }

    plainLayout->addStretch(1);
    contentLayout->addWidget(plainPanel, 3);

    auto* infoPanel = new QFrame(contentRow);
    infoPanel->setObjectName("fighterInfoPanel");
    infoPanel->setStyleSheet(
        "QFrame#fighterInfoPanel {"
        " background: rgba(22,18,15,0.86);"
        " border:1px solid rgba(212,160,23,0.18);"
        " border-radius:18px;"
        "}"
    );

    auto* infoLayout = new QVBoxLayout(infoPanel);
    infoLayout->setContentsMargins(20, 18, 20, 18);
    infoLayout->setSpacing(10);

    previewPortraitLabel_ = new QLabel(infoPanel);
    previewPortraitLabel_->setFixedSize(180, 180);
    previewPortraitLabel_->setAlignment(Qt::AlignCenter);
    infoLayout->addWidget(previewPortraitLabel_, 0, Qt::AlignHCenter);

    previewDescriptionLabel_ = new QLabel("A battle-ready contender prepared to adapt to any arena challenge.", infoPanel);
    previewDescriptionLabel_->setWordWrap(true);
    previewDescriptionLabel_->setStyleSheet("color:#E4D2AA; font:13px 'Segoe UI'; line-height: 1.35em;");
    infoLayout->addWidget(previewDescriptionLabel_);

    auto* moveTitleLabel = new QLabel("COMBAT READOUT", infoPanel);
    moveTitleLabel->setStyleSheet("color:rgba(244,216,149,0.82); font:700 11px 'Segoe UI'; letter-spacing: 0.8px;");
    infoLayout->addWidget(moveTitleLabel);

    previewMoveLabel_ = new QLabel("Adaptive combat pattern", infoPanel);
    previewMoveLabel_->setWordWrap(true);
    previewMoveLabel_->setStyleSheet(
        "color:#FFE8B0; font:800 13px 'Segoe UI';"
        "background: rgba(212,160,23,0.10);"
        "border:1px solid rgba(212,160,23,0.20);"
        "border-radius:10px;"
        "padding:8px 10px;"
    );
    infoLayout->addWidget(previewMoveLabel_);

    auto* quickStatsGrid = new QGridLayout();
    quickStatsGrid->setContentsMargins(0, 0, 0, 0);
    quickStatsGrid->setHorizontalSpacing(8);
    quickStatsGrid->setVerticalSpacing(8);

    const auto makeQuickChip = [infoPanel](const QString& title) {
        auto* chip = new QLabel(infoPanel);
        chip->setMinimumHeight(46);
        chip->setAlignment(Qt::AlignCenter);
        chip->setTextFormat(Qt::RichText);
        chip->setStyleSheet(
            "QLabel {"
            " color:#FFF0C6;"
            " background: rgba(255,255,255,0.055);"
            " border:1px solid rgba(212,160,23,0.16);"
            " border-radius:10px;"
            " padding:6px 8px;"
            "}"
        );
        chip->setText(QStringLiteral(
            "<span style='font-size:9px; font-weight:800; color:rgba(244,216,149,0.76); letter-spacing:1px;'>%1</span><br>"
            "<span style='font-size:13px; font-weight:900; color:#FFF0C6;'>--</span>")
            .arg(title.toUpper()));
        return chip;
    };

    previewRoleChipLabel_ = makeQuickChip(QStringLiteral("Role"));
    previewAttacksChipLabel_ = makeQuickChip(QStringLiteral("Attacks"));
    previewRangeChipLabel_ = makeQuickChip(QStringLiteral("Range"));
    previewProjectileChipLabel_ = makeQuickChip(QStringLiteral("Reach"));
    previewUnlockChipLabel_ = makeQuickChip(QStringLiteral("Unlock"));
    quickStatsGrid->addWidget(previewRoleChipLabel_, 0, 0);
    quickStatsGrid->addWidget(previewAttacksChipLabel_, 0, 1);
    quickStatsGrid->addWidget(previewRangeChipLabel_, 1, 0);
    quickStatsGrid->addWidget(previewProjectileChipLabel_, 1, 1);
    quickStatsGrid->addWidget(previewUnlockChipLabel_, 2, 0, 1, 2);
    infoLayout->addLayout(quickStatsGrid);

    previewAbilitiesLabel_ = new QLabel("Flexible role coverage", infoPanel);
    previewAbilitiesLabel_->setWordWrap(true);
    previewAbilitiesLabel_->setStyleSheet("color:#D9C7A0; font:12px 'Segoe UI'; line-height: 1.35em;");
    infoLayout->addWidget(previewAbilitiesLabel_);

    auto* statTitleLabel = new QLabel("COMBAT STATS", infoPanel);
    statTitleLabel->setStyleSheet("color:rgba(244,216,149,0.82); font:700 11px 'Segoe UI'; letter-spacing: 0.8px;");
    infoLayout->addWidget(statTitleLabel);

    const auto createStatRow = [infoPanel, infoLayout](const QString& title, const QString& accent, QProgressBar** outBar) {
        auto* row = new QWidget(infoPanel);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(9);

        auto* label = new QLabel(title, row);
        label->setFixedWidth(86);
        label->setStyleSheet("color:#F1E0B8; font:800 11px 'Segoe UI'; letter-spacing:0.4px;");
        rowLayout->addWidget(label);

        auto* bar = new QProgressBar(row);
        bar->setTextVisible(true);
        bar->setRange(0, 100);
        bar->setFormat(QStringLiteral("%v"));
        bar->setFixedHeight(15);
        bar->setStyleSheet(QString(
            "QProgressBar {"
            " background: rgba(255,255,255,0.08);"
            " border:1px solid rgba(212,160,23,0.14);"
            " border-radius:7px;"
            " color:#FFF0C6;"
            " font:800 10px 'Segoe UI';"
            " text-align:center;"
            "}"
            "QProgressBar::chunk {"
            " background:%1;"
            " border-radius:7px;"
            "}"
        ).arg(accent));
        rowLayout->addWidget(bar, 1);
        infoLayout->addWidget(row);
        *outBar = bar;
    };

    createStatRow("Attack Power", "#BE3A2C", &attackPowerBar_);
    createStatRow("Recovery", "#2F8A63", &healPowerBar_);
    createStatRow("Mobility", "#2D77B2", &mobilityPowerBar_);
    createStatRow("Control", "#8B5AB8", &controlPowerBar_);

    auto* hintTitleLabel = new QLabel("MODE NOTE", infoPanel);
    hintTitleLabel->setStyleSheet("color:rgba(244,216,149,0.82); font:700 11px 'Segoe UI'; letter-spacing: 0.8px;");
    infoLayout->addWidget(hintTitleLabel);

    previewHintLabel_ = new QLabel("Choose a mode and enter the arena.", infoPanel);
    previewHintLabel_->setWordWrap(true);
    previewHintLabel_->setStyleSheet("color:#D9C7A0; font:12px 'Segoe UI';");
    infoLayout->addWidget(previewHintLabel_);
    infoLayout->addStretch(1);

    contentLayout->addWidget(infoPanel, 4);

    auto* visualPanel = new QFrame(contentRow);
    visualPanel->setObjectName("fighterVisualPanel");
    visualPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    visualPanel->setStyleSheet(
        "QFrame#fighterVisualPanel {"
        " background: rgba(16,12,10,0.38);"
        " border:1px solid rgba(212,160,23,0.12);"
        " border-radius:20px;"
        "}"
    );

    auto* visualLayout = new QVBoxLayout(visualPanel);
    visualLayout->setContentsMargins(12, 12, 12, 12);
    visualLayout->setSpacing(10);

    previewTitleLabel_ = new QLabel("Your Fighter", visualPanel);
    previewTitleLabel_->setStyleSheet("color:#FFF0C6; font: 800 30px 'Segoe UI'; letter-spacing: 0.7px;");
    previewTitleLabel_->setAlignment(Qt::AlignCenter);
    visualLayout->addWidget(previewTitleLabel_, 0, Qt::AlignTop);

    characterView_ = new QGraphicsView(visualPanel);
    characterView_->setFrameShape(QFrame::NoFrame);
    characterView_->setRenderHint(QPainter::Antialiasing, true);
    characterView_->setRenderHint(QPainter::SmoothPixmapTransform, true);
    characterView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    characterView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    characterView_->setStyleSheet("background: transparent;");
    characterView_->setCursor(Qt::ArrowCursor);
    characterView_->viewport()->setCursor(Qt::ArrowCursor);

    previewScene_ = new QGraphicsScene(characterView_);
    previewScene_->setSceneRect(0, 0, kPreviewSceneWidth, kPreviewSceneHeight);
    previewScene_->setBackgroundBrush(QColor(40, 30, 25));
    characterView_->setScene(previewScene_);

    sceneBackgroundItem_ = previewScene_->addPixmap(createCinematicBackdrop(QSize(kPreviewSceneWidth, kPreviewSceneHeight)));
    sceneBackgroundItem_->setZValue(-10.0);

    sceneLockDimItem_ = previewScene_->addRect(previewScene_->sceneRect(),
                                               Qt::NoPen,
                                               QColor(0, 0, 0, 178));
    sceneLockDimItem_->setZValue(-6.0);
    sceneLockDimItem_->setVisible(false);

    glowItem_ = previewScene_->addPixmap(createGoldGlowPixmap(820));
    glowItem_->setOpacity(0.54);
    glowItem_->setZValue(-2.0);

    characterItem_ = previewScene_->addPixmap(createCharacterPlaceholder(QSize(460, 700), selectedCharacter_.name));
    characterItem_->setZValue(1.0);

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(32);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(212, 160, 23, 90));
    characterItem_->setGraphicsEffect(shadow);

    characterLockItem_ = previewScene_->addPixmap(lockBadgePixmap(112));
    characterLockItem_->setZValue(4.0);
    characterLockItem_->setVisible(false);

    characterLockTextItem_ = previewScene_->addText(QString(), QFont("Segoe UI", 22, QFont::Black));
    characterLockTextItem_->setDefaultTextColor(QColor("#FFE6A6"));
    characterLockTextItem_->setTextWidth(520);
    characterLockTextItem_->setZValue(5.0);
    characterLockTextItem_->setVisible(false);

    fallbackTextItem_ = previewScene_->addText("Character image not found", QFont("Segoe UI", 16, QFont::Bold));
    fallbackTextItem_->setDefaultTextColor(QColor("#D4A017"));
    fallbackTextItem_->setZValue(2.0);
    fallbackTextItem_->setVisible(false);

    visualLayout->addWidget(characterView_, 1);
    contentLayout->addWidget(visualPanel, 6);

    previewLayout->addWidget(contentRow, 1);
    rootLayout->addWidget(previewFrame, 7);
}

void ProfileLobbyWidget::setupModeCarousel(QBoxLayout* rootLayout) {
    // 1v1 teammate:
    // Turn "1v1 Fight (Duel)" into a real mode here.
    // Add duel-specific setup controls near this mode:
    // - random opponent vs manual opponent
    // - manual opponent picker
    // - background picker
    // Sound teammate:
    // Mode-card select sounds belong around this section and the mode click flow.
    auto* modeSection = new QFrame(this);
    modeSection->setObjectName("modeSection");
    modeSection->setMinimumWidth(320);
    modeSection->setMaximumWidth(420);
    modeSection->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    modeSection->setStyleSheet(
        "QFrame#modeSection {"
        " background: rgba(18,14,12,0.68);"
        " border:1px solid rgba(212,160,23,0.20);"
        " border-radius:22px;"
        "}"
    );

    auto* sectionLayout = new QVBoxLayout(modeSection);
    sectionLayout->setContentsMargins(20, 18, 20, 18);
    sectionLayout->setSpacing(14);

    auto* sectionHeader = new QWidget(modeSection);
    auto* sectionHeaderLayout = new QHBoxLayout(sectionHeader);
    sectionHeaderLayout->setContentsMargins(0, 0, 0, 0);
    sectionHeaderLayout->setSpacing(12);

    const QString shieldIconPath = resolveAssetPath("assets/icons/shield.png");
    auto* headerShield = new QLabel(sectionHeader);
    headerShield->setFixedSize(34, 34);
    headerShield->setAlignment(Qt::AlignCenter);
    const QPixmap headerShieldPix(shieldIconPath);
    if (!headerShieldPix.isNull()) {
        headerShield->setPixmap(headerShieldPix.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        headerShield->setStyleSheet(
            "background:rgba(212,160,23,0.14);"
            "border:1px solid rgba(212,160,23,0.34);"
            "border-radius:17px;");
        sectionHeaderLayout->addWidget(headerShield, 0, Qt::AlignTop);
    } else {
        headerShield->deleteLater();
    }

    auto* titleStack = new QWidget(sectionHeader);
    auto* titleStackLayout = new QVBoxLayout(titleStack);
    titleStackLayout->setContentsMargins(0, 0, 0, 0);
    titleStackLayout->setSpacing(2);

    auto* sectionTitle = new QLabel("Select Game Mode", titleStack);
    sectionTitle->setStyleSheet("color:#FFF0C6; font:800 20px 'Segoe UI';");
    titleStackLayout->addWidget(sectionTitle);

    auto* sectionSubtitle = new QLabel("Pick the type of fight you want to step into next.", titleStack);
    sectionSubtitle->setStyleSheet("color:rgba(245,230,184,0.72); font:12px 'Segoe UI';");
    titleStackLayout->addWidget(sectionSubtitle);
    sectionHeaderLayout->addWidget(titleStack, 1);

    sectionLayout->addWidget(sectionHeader);

    modeScrollArea_ = new QScrollArea(modeSection);
    modeScrollArea_->setWidgetResizable(true);
    modeScrollArea_->setFrameShape(QFrame::NoFrame);
    modeScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    modeScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    modeScrollArea_->setStyleSheet(
        "QScrollArea { background: rgba(26,20,15,0.72); border: 1px solid rgba(212,160,23,0.14); border-radius: 16px; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }"
        "QScrollBar:vertical { width: 10px; margin: 14px 2px 14px 0; background: transparent; }"
        "QScrollBar::handle:vertical { background: rgba(212,160,23,0.34); border-radius: 5px; min-height: 32px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
    );
    modeScrollArea_->viewport()->setStyleSheet("background: transparent; border: none;");
    modeScrollArea_->setMinimumHeight(230);

    modeContainer_ = new QWidget(modeScrollArea_);
    modeContainer_->setAttribute(Qt::WA_StyledBackground, true);
    modeContainer_->setStyleSheet("background: transparent;");
    auto* flow = new FlowLayout(modeContainer_, 4, 0, 16);
    modeLayout_ = flow;

    const QList<GameMode> modes = {
        {kExhibitionLobbyMode, shieldIconPath, "Classic exhibition duels with dedicated rival setup, manual or random matchups, and selectable arena themes.", false},
        {kPlayableLobbyMode, shieldIconPath, "The current playable campaign build: defend the ruler through enemy stages.", true},
        {kZombieLobbyMode, shieldIconPath, "Survive two infected waves, push through advanced zombies, and reclaim the city.", false},
        {kLanLobbyMode, shieldIconPath, "Host or join a nearby LAN challenger. Session sync, ready states, and live duel handoff are now wired.", false}
    };

    for (const GameMode& mode : modes) {
        QWidget* card = createModeCard(mode);
        flow->addWidget(card);
        modeCards_.insert(mode.name, card);
        availableModes_.insert(mode.name, mode);
    }

    modeContainer_->setLayout(flow);
    modeScrollArea_->setWidget(modeContainer_);
    sectionLayout->addWidget(modeScrollArea_);
    setupDuelSetupPanel(sectionLayout);
    rootLayout->addWidget(modeSection, 3);
}

void ProfileLobbyWidget::setupDuelSetupPanel(QBoxLayout* rootLayout) {
    duelSetupPanel_ = new QFrame(this);
    duelSetupPanel_->setObjectName("duelSetupPanel");
    duelSetupPanel_->setVisible(false);
    duelSetupPanel_->setStyleSheet(
        "QFrame#duelSetupPanel {"
        " background: rgba(24,18,14,0.88);"
        " border:1px solid rgba(212,160,23,0.24);"
        " border-radius:16px;"
        "}");

    auto* layout = new QVBoxLayout(duelSetupPanel_);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    duelSetupLabel_ = new QLabel("DUEL SETUP", duelSetupPanel_);
    duelSetupLabel_->setStyleSheet("color:#F8E6B1; font:800 14px 'Segoe UI';");
    layout->addWidget(duelSetupLabel_);

    opponentModePicker_ = new QComboBox(duelSetupPanel_);
    opponentModePicker_->addItems({QStringLiteral("Random"), QStringLiteral("Manual")});
    layout->addWidget(opponentModePicker_);

    opponentCategoryPicker_ = new QComboBox(duelSetupPanel_);
    opponentCategoryPicker_->addItems({QStringLiteral("Player"), QStringLiteral("Enemy")});
    layout->addWidget(opponentCategoryPicker_);

    opponentPicker_ = new QComboBox(duelSetupPanel_);
    layout->addWidget(opponentPicker_);

    arenaPicker_ = new QComboBox(duelSetupPanel_);
    arenaPicker_->addItems(kDuelArenaChoices);
    layout->addWidget(arenaPicker_);

    refreshDuelSetupControls();

    connect(opponentModePicker_, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        duelSetup_.opponentMode = text;
        refreshDuelSetupControls();
    });
    connect(opponentCategoryPicker_, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        duelSetup_.opponentCategory = text;
        refreshDuelSetupControls();
    });
    connect(opponentPicker_, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        duelSetup_.selectedOpponent = text;
        refreshLobbyContext();
    });
    connect(arenaPicker_, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        duelSetup_.selectedArena = text;
        refreshLobbyContext();
    });

    rootLayout->addWidget(duelSetupPanel_);
}

void ProfileLobbyWidget::setupBottomBar(QBoxLayout* rootLayout) {
    // 1v1 teammate:
    // Keep the duel CTA flow here simple:
    // selecting 1v1 should prepare a one-match setup, not campaign progression.
    // Sound teammate:
    // Lobby button sounds belong here:
    // - browse fighters
    // - enter arena
    auto* bottomWrap = new QFrame(this);
    bottomWrap->setObjectName("lobbyActionDock");
    bottomWrap->setStyleSheet(
        "QFrame#lobbyActionDock {"
        " background: rgba(18,14,12,0.84);"
        " border:1px solid rgba(212,160,23,0.22);"
        " border-radius:22px;"
        "}"
    );

    auto* bottomLayout = new QHBoxLayout(bottomWrap);
    bottomLayout->setContentsMargins(20, 16, 20, 16);
    bottomLayout->setSpacing(16);

    auto* summaryWrap = new QWidget(bottomWrap);
    auto* summaryLayout = new QVBoxLayout(summaryWrap);
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->setSpacing(3);

    actionSummaryLabel_ = new QLabel("Queue for Save the King", summaryWrap);
    actionSummaryLabel_->setStyleSheet("color:#FFF0C6; font:800 18px 'Segoe UI';");
    summaryLayout->addWidget(actionSummaryLabel_);

    actionHintLabel_ = new QLabel("Defend the ruler in the current ready combat build.", summaryWrap);
    actionHintLabel_->setStyleSheet("color:rgba(245,230,184,0.72); font:12px 'Segoe UI';");
    actionHintLabel_->setWordWrap(true);
    summaryLayout->addWidget(actionHintLabel_);

    bottomLayout->addWidget(summaryWrap, 1);

    changeCharacterButton_ = new QPushButton("Browse Gladiators", bottomWrap);
    changeCharacterButton_->setCursor(Qt::PointingHandCursor);
    changeCharacterButton_->setMinimumHeight(52);
    changeCharacterButton_->setStyleSheet(
        "QPushButton {"
        " background: rgba(58,42,27,0.92);"
        " border:1px solid rgba(212,160,23,0.30);"
        " border-radius:14px;"
        " color:#F0DEB2;"
        " font:700 15px 'Segoe UI';"
        " padding:12px 20px;"
        "}"
        "QPushButton:hover { background: rgba(82,58,35,0.95); }"
    );
    connect(changeCharacterButton_, &QPushButton::clicked, this, &ProfileLobbyWidget::changeCharacterClicked);
    // Sound teammate:
    // Play browse/change-character click here when wiring sounds.

    enterArenaButton_ = new QPushButton("ENTER ARENA", bottomWrap);
    enterArenaButton_->setCursor(Qt::PointingHandCursor);
    enterArenaButton_->setMinimumHeight(66);
    enterArenaButton_->setMinimumWidth(320);
    enterArenaButton_->setStyleSheet(
        "QPushButton {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #6E0D0D, stop:0.45 #A81616, stop:1 #D84321);"
        " border:2px solid #D86A55;"
        " border-radius:16px;"
        " color:#FFE6E6;"
        " font:800 21px 'Segoe UI';"
        " letter-spacing: 1px;"
        " padding:16px 36px;"
        "}"
        "QPushButton:hover { border:2px solid #FFB299; }"
        "QPushButton:pressed {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #4F0A0A, stop:1 #91241B);"
        "}"
        "QPushButton:disabled {"
        " background: rgba(32,25,20,0.78);"
        " border:2px solid rgba(245,210,142,0.26);"
        " color:rgba(245,230,184,0.52);"
        "}"
    );

    enterArenaGlowEffect_ = new QGraphicsDropShadowEffect(this);
    enterArenaGlowEffect_->setColor(QColor(255, 69, 38, 210));
    enterArenaGlowEffect_->setOffset(0, 0);
    enterArenaGlowEffect_->setBlurRadius(28);
    enterArenaButton_->setGraphicsEffect(enterArenaGlowEffect_);

    hoverGlowAnimation_ = new QVariantAnimation(this);
    hoverGlowAnimation_->setStartValue(24.0);
    hoverGlowAnimation_->setEndValue(60.0);
    hoverGlowAnimation_->setDuration(900);
    hoverGlowAnimation_->setEasingCurve(QEasingCurve::InOutSine);
    hoverGlowAnimation_->setLoopCount(-1);
    connect(hoverGlowAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        if (enterArenaGlowEffect_) {
            enterArenaGlowEffect_->setBlurRadius(value.toReal());
        }
    });

    enterArenaButton_->installEventFilter(this);
    connect(enterArenaButton_, &QPushButton::clicked, this, [this]() {
        // Sound teammate:
        // Play confirm/click or coming-soon sound here.
        emit enterArenaClicked(selectedModeName_);
    });

    bottomLayout->addWidget(changeCharacterButton_, 0, Qt::AlignRight);
    bottomLayout->addWidget(enterArenaButton_, 0, Qt::AlignRight);
    rootLayout->addWidget(bottomWrap);
}

QWidget* ProfileLobbyWidget::createModeCard(const GameMode& mode) {
    auto* card = new QFrame(modeContainer_);
    card->setObjectName("modeCard");
    card->setProperty("modeName", mode.name);
    card->setProperty("selected", false);
    card->setCursor(Qt::PointingHandCursor);
    card->setFixedSize(286, 166);

    const QString accent = modeAccentFor(mode.name);
    const QString tagText = modeShortTagFor(mode.name).toUpper();

    card->setStyleSheet(QString(
        "QFrame#modeCard {"
        " background: rgba(255,255,255,0.03);"
        " border:1px solid rgba(212,160,23,0.18);"
        " border-top:4px solid %1;"
        " border-radius:16px;"
        "}"
        "QFrame#modeCard[selected='true'] {"
        " border:2px solid #D4A017;"
        " background: rgba(212,160,23,0.16);"
        " border-top:4px solid %1;"
        "}"
    ).arg(accent));

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(8);

    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(10);

    auto* icon = new QLabel(card);
    icon->setFixedSize(34, 34);
    icon->setAlignment(Qt::AlignCenter);
    bool hasIconPixmap = false;
    if (!mode.iconPath.isEmpty()) {
        QPixmap iconPix(mode.iconPath);
        if (!iconPix.isNull()) {
            icon->setPixmap(iconPix.scaled(26, 26, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            icon->setStyleSheet("background: rgba(212,160,23,0.14); border-radius: 17px;");
            hasIconPixmap = true;
        }
    }
    if (!hasIconPixmap) {
        icon->setText(mode.name.left(1));
        icon->setStyleSheet(QString("background:%1; border-radius:17px; color:white; font:700 14px 'Segoe UI';").arg(accent));
    }
    topRow->addWidget(icon, 0, Qt::AlignTop);

    auto* title = new QLabel(mode.name, card);
    title->setWordWrap(true);
    title->setStyleSheet("color:#F0DEB2; font:700 14px 'Segoe UI';");
    topRow->addWidget(title, 1);

    auto* tag = new QLabel(tagText, card);
    tag->setStyleSheet(QString(
        "color:#FCE9BC; font:700 10px 'Segoe UI'; letter-spacing:0.8px;"
        "padding:4px 8px; border-radius:9px;"
        "background:%1; border:1px solid rgba(255,255,255,0.14);"
    ).arg(accent));
    topRow->addWidget(tag, 0, Qt::AlignTop);
    cardLayout->addLayout(topRow);

    auto* desc = new QLabel(mode.description, card);
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#D3C09A; font:11px 'Segoe UI';");
    cardLayout->addWidget(desc);
    cardLayout->addStretch(1);

    auto* footerRow = new QHBoxLayout();
    footerRow->setSpacing(8);

    if (mode.recommended) {
        auto* badge = new QLabel("Recommended", card);
        badge->setStyleSheet(
            "background: rgba(212,160,23,0.24);"
            "border: 1px solid #D4A017;"
            "border-radius: 9px;"
            "padding: 3px 8px;"
            "color: #FFE6A6;"
            "font: 700 10px 'Segoe UI';"
        );
        footerRow->addWidget(badge, 0, Qt::AlignLeft);
    }

    footerRow->addStretch(1);

    auto* status = new QLabel("Select mode", card);
    status->setObjectName("modeStatusLabel");
    status->setStyleSheet("color:rgba(245,230,184,0.58); font:700 10px 'Segoe UI'; letter-spacing:0.7px;");
    footerRow->addWidget(status, 0, Qt::AlignRight);
    cardLayout->addLayout(footerRow);

    auto* effect = new QGraphicsDropShadowEffect(card);
    effect->setColor(QColor(212, 160, 23, 0));
    effect->setBlurRadius(0.0);
    effect->setOffset(0, 0);
    card->setGraphicsEffect(effect);

    cardGlowEffects_.insert(card, effect);
    cardBaseSizes_.insert(card, card->size());
    card->installEventFilter(this);

    return card;
}

QString ProfileLobbyWidget::badgeColorFor(const QString& badge) const {
    const QString lowered = badge.toLower();
    if (lowered.contains("immortal")) {
        return "#F7F0C8";
    }
    if (lowered.contains("legend")) {
        return "#E58E26";
    }
    if (lowered.contains("champion")) {
        return "#D4A017";
    }
    if (lowered.contains("warlord")) {
        return "#C0392B";
    }
    if (lowered.contains("knight")) {
        return "#7FB3D5";
    }
    if (lowered.contains("elite")) {
        return "#8E44AD";
    }
    if (lowered.contains("gladiator")) {
        return "#B9770E";
    }
    if (lowered.contains("squire")) {
        return "#9A7D0A";
    }
    if (lowered.contains("wanderer")) {
        return "#6C7A89";
    }
    if (lowered.contains("pro")) {
        return "#2E86DE";
    }
    return "#6C7A89";
}

QString ProfileLobbyWidget::modeAccentFor(const QString& modeName) const {
    if (modeName.contains("1v1", Qt::CaseInsensitive)) {
        return "#e00909";
    }
    if (modeName.contains("Kings", Qt::CaseInsensitive)) {
        return "#9600d1";
    }
    if (modeName.contains("Zombie", Qt::CaseInsensitive)) {
        return "#2B7D43";
    }
    if (modeName.contains("LAN", Qt::CaseInsensitive)) {
        return "#0C62B8";
    }
    return "#7A7A7A";
}

QString ProfileLobbyWidget::modeShortTagFor(const QString& modeName) const {
    if (modeName.contains("1v1", Qt::CaseInsensitive)) {
        return "Duel";
    }
    if (modeName.contains("Kings", Qt::CaseInsensitive)) {
        return "Defense";
    }
    if (modeName.contains("Zombie", Qt::CaseInsensitive)) {
        return "Zombie";
    }
    if (modeName.contains("LAN", Qt::CaseInsensitive)) {
        return "LAN";
    }
    return "Arena";
}

QString ProfileLobbyWidget::modeDescriptionFor(const QString& modeName) const {
    return availableModes_.contains(modeName) ? availableModes_.value(modeName).description : QString();
}

QStringList ProfileLobbyWidget::duelPlayerOpponentChoices() const {
    QStringList choices = {
        QStringLiteral("Arcen"),
        QStringLiteral("Demon Slayer"),
        QStringLiteral("Fantasy Warrior"),
        QStringLiteral("Huntress"),
        QStringLiteral("Knight"),
        QStringLiteral("Martial"),
        QStringLiteral("Martial Hero"),
        QStringLiteral("Medieval Warrior"),
        QStringLiteral("Wizard")
    };

    for (int i = 0; i < choices.size(); ++i) {
        if (choices.at(i).compare(selectedCharacter_.name.trimmed(), Qt::CaseInsensitive) == 0) {
            choices.removeAt(i);
            break;
        }
    }

    return choices;
}

QStringList ProfileLobbyWidget::duelEnemyOpponentChoices() const {
    return {
        QStringLiteral("Fire Worm"),
        QStringLiteral("Fire Wizard"),
        QStringLiteral("Flying Demon"),
        QStringLiteral("Nightweaver"),
        QStringLiteral("Evil Wizard"),
        QStringLiteral("Black Werewolf"),
        QStringLiteral("Red Werewolf"),
        QStringLiteral("White Werewolf")
    };
}

void ProfileLobbyWidget::refreshDuelSetupControls() {
    if (!opponentModePicker_ || !opponentCategoryPicker_ || !opponentPicker_ || !arenaPicker_) {
        return;
    }

    if (duelSetup_.opponentMode.trimmed().isEmpty()) {
        duelSetup_.opponentMode = QStringLiteral("Manual");
    }
    if (duelSetup_.opponentCategory.trimmed().isEmpty()) {
        duelSetup_.opponentCategory = QStringLiteral("Player");
    }
    if (duelSetup_.selectedArena.trimmed().isEmpty()) {
        duelSetup_.selectedArena = kDuelArenaChoices.first();
    }

    const bool blockMode = opponentModePicker_->blockSignals(true);
    const bool blockCategory = opponentCategoryPicker_->blockSignals(true);
    const bool blockOpponent = opponentPicker_->blockSignals(true);
    const bool blockArena = arenaPicker_->blockSignals(true);

    opponentModePicker_->setCurrentText(duelSetup_.opponentMode);
    opponentCategoryPicker_->setCurrentText(duelSetup_.opponentCategory);

    const QStringList opponentChoices = duelSetup_.opponentCategory.compare(QStringLiteral("Player"), Qt::CaseInsensitive) == 0
        ? duelPlayerOpponentChoices()
        : duelEnemyOpponentChoices();
    opponentPicker_->clear();
    opponentPicker_->addItems(opponentChoices);

    if (!opponentChoices.contains(duelSetup_.selectedOpponent, Qt::CaseInsensitive)) {
        duelSetup_.selectedOpponent = opponentChoices.isEmpty() ? QString() : opponentChoices.first();
    }
    if (!duelSetup_.selectedOpponent.isEmpty()) {
        opponentPicker_->setCurrentText(duelSetup_.selectedOpponent);
    }

    if (!kDuelArenaChoices.contains(duelSetup_.selectedArena, Qt::CaseInsensitive)) {
        duelSetup_.selectedArena = kDuelArenaChoices.first();
    }
    arenaPicker_->setCurrentText(duelSetup_.selectedArena);
    opponentPicker_->setVisible(duelSetup_.opponentMode.compare(QStringLiteral("Manual"), Qt::CaseInsensitive) == 0);

    opponentModePicker_->blockSignals(blockMode);
    opponentCategoryPicker_->blockSignals(blockCategory);
    opponentPicker_->blockSignals(blockOpponent);
    arenaPicker_->blockSignals(blockArena);

    refreshLobbyContext();
}

bool ProfileLobbyWidget::isPlayableMode(const QString& modeName) const {
    if (modeName.compare(kZombieLobbyMode, Qt::CaseInsensitive) == 0) {
        return isZombieModeUnlocked();
    }
    return modeName.compare(kPlayableLobbyMode, Qt::CaseInsensitive) == 0
        || modeName.compare(kExhibitionLobbyMode, Qt::CaseInsensitive) == 0
        || modeName.compare(kLanLobbyMode, Qt::CaseInsensitive) == 0;
}

bool ProfileLobbyWidget::isZombieModeUnlocked() const {
    const QString currentRank = canonicalRankName(userProfile_.badge, userProfile_.score);
    return rankTierForName(currentRank) >= kZombieUnlockTier;
}

void ProfileLobbyWidget::refreshProfileUi() {
    // Ranking teammate:
    // Extend this UI refresh to show progression values clearly in the lobby:
    // score, rank, and rating out of 5.
    const QString currentRank = canonicalRankName(userProfile_.badge, userProfile_.score);

    if (usernameLabel_) {
        usernameLabel_->setText(userProfile_.username);
    }
    if (scoreLabel_) {
        scoreLabel_->setText(QString("SCORE  %1").arg(userProfile_.score));
    }
    if (badgeLabel_) {
        const QString color = badgeColorFor(currentRank);
        badgeLabel_->setText(QString("RANK  %1").arg(currentRank));
        badgeLabel_->setStyleSheet(QString(
            "color:#F5E6B8; font:700 13px 'Segoe UI';"
            "padding:7px 12px; border-radius:11px;"
            "background: rgba(212,160,23,0.12);"
            "border:1px solid %1;"
        ).arg(color));
    }
    if (avatarLabel_) {
        QPixmap avatar;
        if (!userProfile_.avatarPath.isEmpty()) {
            avatar.load(userProfile_.avatarPath);
        }
        if (avatar.isNull()) {
            avatar = QPixmap(42, 42);
            avatar.fill(Qt::transparent);

            QPainter painter(&avatar);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#6E4A2E"));
            painter.drawEllipse(0, 0, 42, 42);
            painter.setPen(QColor("#F7E6BF"));
            painter.setFont(QFont("Segoe UI", 14, QFont::Bold));
            const QString initial = userProfile_.username.trimmed().isEmpty()
                ? "P"
                : userProfile_.username.trimmed().left(1).toUpper();
            painter.drawText(QRect(0, 0, 42, 42), Qt::AlignCenter, initial);
        }
        avatarLabel_->setPixmap(circularPortraitPixmap(avatar, QSize(42, 42)));
    }

    if (lobbySummaryRankLabel_) {
        lobbySummaryRankLabel_->setText(currentRank);
    }
    if (lobbySummaryScoreLabel_) {
        lobbySummaryScoreLabel_->setText(QString::number(qMax(0, userProfile_.score)));
    }
    if (lobbySummaryBadgeLabel_) {
        const int availableWidth = lobbySummaryBadgeLabel_->width() > 32
            ? lobbySummaryBadgeLabel_->width()
            : 240;
        const int side = qBound(210, qMin(availableWidth, 285), 285);
        const QPixmap badgePixmap = rankBadgePixmap(currentRank, side);
        if (!badgePixmap.isNull()) {
            lobbySummaryBadgeLabel_->setText(QString());
            lobbySummaryBadgeLabel_->setPixmap(badgePixmap);
            lobbySummaryBadgeLabel_->setToolTip(QStringLiteral("%1 rank badge").arg(currentRank));
        } else {
            lobbySummaryBadgeLabel_->setPixmap(QPixmap());
            lobbySummaryBadgeLabel_->setText(QStringLiteral("%1\nBadge Missing").arg(currentRank));
            lobbySummaryBadgeLabel_->setToolTip(QString());
        }
    }
    refreshCharacterLockState();
}

void ProfileLobbyWidget::refreshCharacterLockState() {
    if (!characterItem_) {
        return;
    }

    const QString currentRank = canonicalRankName(userProfile_.badge, userProfile_.score);
    const int rankTier = rankTierForName(currentRank);
    const PlayerType type = playerTypeForLobbyName(selectedCharacter_.name);
    const FighterAiProfile profile = fighterAiProfileFor(type);
    const bool locked = rankTier < profile.unlockTier;
    const QString unlockRank = rankNameForUnlockTier(profile.unlockTier);

    characterItem_->setOpacity(locked ? 0.68 : 1.0);
    if (sceneLockDimItem_) {
        sceneLockDimItem_->setVisible(locked);
        sceneLockDimItem_->setOpacity(locked ? 0.86 : 0.0);
    }
    if (glowItem_) {
        glowItem_->setOpacity(locked ? 0.30 : 0.54);
    }
    if (characterLockItem_) {
        characterLockItem_->setVisible(locked);
    }
    if (characterLockTextItem_) {
        characterLockTextItem_->setVisible(locked);
        characterLockTextItem_->setHtml(QStringLiteral(
            "<div align='center'>"
            "<span style='font-size:34px; font-weight:900; color:#FFE6A6;'>LOCKED</span><br/>"
            "<span style='font-size:20px; font-weight:800; color:#D9C7A0;'>Unlocks at %1</span>"
            "</div>").arg(unlockRank));
    }

    updatePreviewScale();
}

void ProfileLobbyWidget::refreshLobbyContext() {
    // 1v1 teammate:
    // Update lobby copy here to reflect duel setup choices:
    // chosen opponent mode, chosen opponent, and chosen background.
    const CharacterLobbyProfile profile = profileForCharacter(selectedCharacter_.name, selectedCharacter_.specialMoves);
    const PlayerType currentType = playerTypeForLobbyName(selectedCharacter_.name);
    const FighterAiProfile fighterProfile = fighterAiProfileFor(currentType);
    const CharacterFeatureSet featureSet = InputHandler::getCharacterFeatures(currentType);
    const int attackSlots = qBound(1, qMax(fighterProfile.attackCount, featureSet.attackOptions), 3);
    const QString currentRank = canonicalRankName(userProfile_.badge, userProfile_.score);
    const bool lockedCharacter = rankTierForName(currentRank) < fighterProfile.unlockTier;
    const QString unlockRank = rankNameForUnlockTier(fighterProfile.unlockTier);
    const auto setQuickChip = [](QLabel* label, const QString& title, const QString& value, const QString& color = QStringLiteral("#FFF0C6")) {
        if (!label) {
            return;
        }
        label->setText(QStringLiteral(
            "<span style='font-size:9px; font-weight:800; color:rgba(244,216,149,0.76); letter-spacing:1px;'>%1</span><br>"
            "<span style='font-size:13px; font-weight:900; color:%3;'>%2</span>")
            .arg(title.toUpper(), value.toHtmlEscaped(), color));
    };

    if (lobbySummaryCharacterLabel_) {
        lobbySummaryCharacterLabel_->setText(selectedCharacter_.name.trimmed().isEmpty()
            ? QStringLiteral("Unselected")
            : selectedCharacter_.name.trimmed());
    }

    if (duelSetupPanel_) {
        // 1v1 teammate:
        // Keep the original duel setup widget code alive, but move the visible
        // player-facing setup flow to the dedicated Exhibition page.
        duelSetupPanel_->setVisible(false);
    }

    if (previewModeChipLabel_) {
        previewModeChipLabel_->setText(modeShortTagFor(selectedModeName_).toUpper());
        previewModeChipLabel_->setStyleSheet(QString(
            "color:#FFF2CF; font:700 11px 'Segoe UI'; letter-spacing:1px;"
            "padding:6px 10px; border-radius:10px;"
            "background:%1; border:1px solid rgba(255,255,255,0.14);"
        ).arg(modeAccentFor(selectedModeName_)));
    }

    if (previewDescriptionLabel_) {
        previewDescriptionLabel_->setText(profile.description);
    }

    if (previewMoveLabel_) {
        previewMoveLabel_->setText(attackPatternFor(currentType, attackSlots));
    }

    setQuickChip(previewRoleChipLabel_,
                 QStringLiteral("Role"),
                 personalityLabel(fighterProfile.personality));
    setQuickChip(previewAttacksChipLabel_,
                 QStringLiteral("Attacks"),
                 QStringLiteral("%1 states").arg(attackSlots));
    setQuickChip(previewRangeChipLabel_,
                 QStringLiteral("Range"),
                 QStringLiteral("%1-%2 px")
                     .arg(static_cast<int>(fighterProfile.idealMinRange))
                     .arg(static_cast<int>(fighterProfile.idealMaxRange)));
    setQuickChip(previewProjectileChipLabel_,
                 QStringLiteral("Reach"),
                 projectileShortLabelFor(currentType, fighterProfile),
                 fighterProfile.hasProjectile ? QStringLiteral("#8DD9FF") : QStringLiteral("#FFF0C6"));
    setQuickChip(previewUnlockChipLabel_,
                 lockedCharacter ? QStringLiteral("Unlocks") : QStringLiteral("Unlocked"),
                 lockedCharacter ? unlockRank : QStringLiteral("Available"),
                 lockedCharacter ? QStringLiteral("#E9B86E") : QStringLiteral("#7FF0B2"));

    if (previewAbilitiesLabel_) {
        previewAbilitiesLabel_->setText(tacticalNoteFor(currentType, fighterProfile));
    }

    if (duelSetupLabel_ && duelSetupPanel_ && duelSetupPanel_->isVisible()) {
        const QString opponentLine = duelSetup_.opponentMode == "Manual"
            ? QString("%1 rival: %2").arg(duelSetup_.opponentCategory,
                                          duelSetup_.selectedOpponent.isEmpty() ? QStringLiteral("TBD") : duelSetup_.selectedOpponent)
            : QString("%1 rival: Random").arg(duelSetup_.opponentCategory);
        duelSetupLabel_->setText(QString("DUEL SETUP  •  %1  •  %2").arg(opponentLine, duelSetup_.selectedArena));
    }

    if (previewHintLabel_) {
        const QString description = modeDescriptionFor(selectedModeName_);
        if (lockedCharacter) {
            previewHintLabel_->setText(QStringLiteral("%1 is locked. Unlock this fighter at %2.")
                                           .arg(selectedCharacter_.name.trimmed(), unlockRank));
        } else if (isPlayableMode(selectedModeName_)) {
            if (selectedModeName_.compare(kExhibitionLobbyMode, Qt::CaseInsensitive) == 0) {
                previewHintLabel_->setText(QStringLiteral("Duel selected. Choose rival rules and arena in setup."));
            } else {
                previewHintLabel_->setText(description.isEmpty()
                    ? "Choose a mode and enter the arena."
                    : QString("%1 selected. %2").arg(modeShortTagFor(selectedModeName_), description));
            }
        } else if (selectedModeName_.compare(kZombieLobbyMode, Qt::CaseInsensitive) == 0) {
            previewHintLabel_->setText(QStringLiteral("Zombie mode unlocks at Elite Knight. Clear the city when your rank is ready."));
        } else {
            previewHintLabel_->setText(QString("%1 is coming soon. Save the Kings is ready now.")
                                           .arg(selectedModeName_));
        }
    }

    if (attackPowerBar_) {
        attackPowerBar_->setValue(profile.attack);
    }
    if (healPowerBar_) {
        healPowerBar_->setValue(profile.heal);
    }
    if (mobilityPowerBar_) {
        mobilityPowerBar_->setValue(profile.mobility);
    }
    if (controlPowerBar_) {
        controlPowerBar_->setValue(profile.control);
    }

    if (actionSummaryLabel_) {
        actionSummaryLabel_->setText(isPlayableMode(selectedModeName_)
            ? QString("Queue for %1").arg(selectedModeName_)
            : QString("%1").arg(selectedModeName_));
    }

    if (actionHintLabel_) {
        QString hint;
        if (lockedCharacter) {
            hint = QStringLiteral("%1 unlocks at %2. Browse another fighter to enter the arena now.")
                       .arg(selectedCharacter_.name.trimmed(), unlockRank);
        } else if (isPlayableMode(selectedModeName_)) {
            if (selectedModeName_.compare(kExhibitionLobbyMode, Qt::CaseInsensitive) == 0) {
                hint = QStringLiteral("Open duel setup next, then choose manual or random rival flow, browse opponents, and pick the battle theme.");
                if (!selectedCharacter_.name.trimmed().isEmpty()) {
                    hint = QString("%1 is ready. %2").arg(selectedCharacter_.name, hint);
                }
            } else {
                hint = modeDescriptionFor(selectedModeName_);
                if (hint.isEmpty()) {
                    hint = "Step into the next battle on your terms.";
                }
                if (!selectedCharacter_.name.trimmed().isEmpty()) {
                    hint = QString("%1 enters next. %2").arg(selectedCharacter_.name, hint);
                }
            }
        } else if (selectedModeName_.compare(kZombieLobbyMode, Qt::CaseInsensitive) == 0) {
            hint = QStringLiteral("Zombie unlocks at Elite Knight. Reach 1400 total score to open the outbreak run.");
        } else {
            hint = QString("%1 is coming soon. Select Save the Kings to enter the current combat experience.")
                       .arg(selectedModeName_);
        }
        actionHintLabel_->setText(hint);
    }

    if (enterArenaButton_) {
        enterArenaButton_->setEnabled(!lockedCharacter && isPlayableMode(selectedModeName_));
        enterArenaButton_->setCursor((!lockedCharacter && isPlayableMode(selectedModeName_))
            ? Qt::PointingHandCursor
            : Qt::ForbiddenCursor);
        if (lockedCharacter) {
            enterArenaButton_->setText(QStringLiteral("LOCKED"));
        } else if (selectedModeName_.compare(kZombieLobbyMode, Qt::CaseInsensitive) == 0 && !isZombieModeUnlocked()) {
            enterArenaButton_->setText(QStringLiteral("UNLOCKS AT ELITE KNIGHT"));
        } else if (!isPlayableMode(selectedModeName_)) {
            enterArenaButton_->setText("COMING SOON");
        } else if (selectedModeName_.compare(kExhibitionLobbyMode, Qt::CaseInsensitive) == 0) {
            enterArenaButton_->setText("OPEN DUEL SETUP");
        } else {
            enterArenaButton_->setText("ENTER ARENA");
        }
    }

    refreshCharacterLockState();
}

void ProfileLobbyWidget::refreshCharacterPreview() {
    if (!characterItem_) {
        return;
    }

    idleAnimationTimer_->stop();
    idleFrames_.clear();
    attackFrames_.clear();
    idleFrameIndex_ = 0;
    showcasingAttack_ = false;
    idleShowcaseElapsedMs_ = 0;

    const LobbyAnimationSpec spec = animationSpecForCharacterName(selectedCharacter_.name);

    const QString resolvedImagePath = resolveAssetPath(selectedCharacter_.imagePath);
    const QString resolvedPortraitPath = profilePortraitPathForCharacter(selectedCharacter_.name, selectedCharacter_.imagePath);
    const QFileInfo imageInfo(resolvedImagePath);
    const QDir baseDir = imageInfo.dir();
    const QString idleSpritePath = spec.idlePath.isEmpty() ? QString() : baseDir.filePath(spec.idlePath);
    const QString attackSpritePath = spec.attackPath.isEmpty() ? QString() : baseDir.filePath(spec.attackPath);

    if (sceneBackgroundItem_ && previewScene_) {
        const QSize sceneSize = previewScene_->sceneRect().size().toSize();
        sceneBackgroundItem_->setPixmap(previewBackgroundPixmapForCharacter(selectedCharacter_.name,
                                                                            selectedCharacter_.imagePath,
                                                                            sceneSize));
        sceneBackgroundItem_->setPos(0, 0);
    }

    if (previewPortraitLabel_) {
        QPixmap portrait(!resolvedPortraitPath.isEmpty() ? resolvedPortraitPath : resolvedImagePath);
        if (!portrait.isNull()) {
            previewPortraitLabel_->setPixmap(circularPortraitPixmap(portrait, previewPortraitLabel_->size()));
        } else {
            previewPortraitLabel_->setPixmap(QPixmap());
        }
    }

    idleFrames_ = extractFramesFromSheet(idleSpritePath, spec.idleFrameCount);
    attackFrames_ = extractFramesFromSheet(attackSpritePath, spec.attackFrameCount);
    if (!idleFrames_.isEmpty()) {
        characterItem_->setPixmap(idleFrames_.first());
        if (fallbackTextItem_) {
            fallbackTextItem_->setVisible(false);
        }
        idleAnimationTimer_->start();
        refreshPreviewTitle();
        refreshLobbyContext();
        updatePreviewScale();
        return;
    }

    const QPixmap fallback = createCharacterPlaceholder(QSize(460, 700), selectedCharacter_.name);
    if (fallbackTextItem_) {
        fallbackTextItem_->setVisible(true);
        fallbackTextItem_->setPlainText("Character image missing");
    }

    characterItem_->setPixmap(fallback);
    refreshPreviewTitle();
    refreshLobbyContext();
    updatePreviewScale();
}

void ProfileLobbyWidget::advanceIdleAnimation() {
    if (idleFrames_.isEmpty() || !characterItem_) {
        return;
    }

    if (showcasingAttack_) {
        if (attackFrames_.isEmpty()) {
            showcasingAttack_ = false;
            idleFrameIndex_ = 0;
            idleShowcaseElapsedMs_ = 0;
            characterItem_->setPixmap(idleFrames_.first());
            updatePreviewScale();
            return;
        }

        if (idleFrameIndex_ < attackFrames_.size()) {
            characterItem_->setPixmap(attackFrames_.at(idleFrameIndex_));
            ++idleFrameIndex_;
        }

        if (idleFrameIndex_ >= attackFrames_.size()) {
            showcasingAttack_ = false;
            idleFrameIndex_ = 0;
            idleShowcaseElapsedMs_ = 0;
        }

        updatePreviewScale();
        return;
    }

    idleFrameIndex_ = (idleFrameIndex_ + 1) % idleFrames_.size();
    idleShowcaseElapsedMs_ += idleAnimationTimer_->interval();
    characterItem_->setPixmap(idleFrames_.at(idleFrameIndex_));

    if (!attackFrames_.isEmpty() && idleShowcaseElapsedMs_ >= 5000) {
        showcasingAttack_ = true;
        idleFrameIndex_ = 0;
        characterItem_->setPixmap(attackFrames_.first());
        idleFrameIndex_ = 1;
    }

    updatePreviewScale();
}

void ProfileLobbyWidget::refreshModeSelectionUi() {
    for (auto it = modeCards_.begin(); it != modeCards_.end(); ++it) {
        QWidget* card = it.value();
        const bool selected = (it.key() == selectedModeName_);
        card->setProperty("selected", selected);
        card->style()->unpolish(card);
        card->style()->polish(card);
        card->update();

        if (auto* statusLabel = card->findChild<QLabel*>("modeStatusLabel")) {
            if (selected) {
                if (it.key().compare(kZombieLobbyMode, Qt::CaseInsensitive) == 0 && !isZombieModeUnlocked()) {
                    statusLabel->setText(QStringLiteral("Unlocks at Elite Knight"));
                } else {
                    statusLabel->setText(isPlayableMode(it.key()) ? "Selected" : "Coming soon");
                }
            } else {
                if (it.key().compare(kZombieLobbyMode, Qt::CaseInsensitive) == 0 && !isZombieModeUnlocked()) {
                    statusLabel->setText(QStringLiteral("Unlocks at Elite Knight"));
                } else {
                    statusLabel->setText(isPlayableMode(it.key()) ? "Select mode" : "Coming soon");
                }
            }
            statusLabel->setStyleSheet(selected
                ? "color:#FFE6A6; font:700 10px 'Segoe UI'; letter-spacing:0.7px;"
                : "color:rgba(245,230,184,0.58); font:700 10px 'Segoe UI'; letter-spacing:0.7px;");
        }
    }

    refreshLobbyContext();
}

void ProfileLobbyWidget::refreshPreviewTitle() {
    if (!previewTitleLabel_) {
        return;
    }

    if (!selectedCharacter_.name.trimmed().isEmpty()) {
        previewTitleLabel_->setText(selectedCharacter_.name);
    } else {
        previewTitleLabel_->setText("Your Fighter");
    }
}

void ProfileLobbyWidget::updatePreviewScale() {
    if (!characterView_ || !previewScene_ || !characterItem_ || !glowItem_) {
        return;
    }

    const QRectF sceneRect = previewScene_->sceneRect();
    const QPixmap pix = characterItem_->pixmap();
    if (pix.isNull()) {
        return;
    }

    glowItem_->setPos((sceneRect.width() - glowItem_->pixmap().width()) * 0.5,
                      sceneRect.height() * 0.67 - glowItem_->pixmap().height() * 0.5);

    const qreal targetHeight = characterView_->viewport()->height() * 0.774;
    const qreal targetWidth = characterView_->viewport()->width() * 0.63;
    const QPixmap scaleReference = idleFrames_.isEmpty() ? pix : idleFrames_.first();
    const qreal referenceWidth = qMax(1, scaleReference.width());
    const qreal referenceHeight = qMax(1, scaleReference.height());
    const qreal sx = targetWidth / referenceWidth;
    const qreal sy = targetHeight / referenceHeight;
    qreal scale = qMin(sx, sy) * kLobbyPreviewScaleMultiplier;

    if (!idleFrames_.isEmpty() && showcasingAttack_) {
        scale = sy * kLobbyPreviewScaleMultiplier;
    }

    characterItem_->setScale(scale);
    const QSizeF scaled(pix.width() * scale, pix.height() * scale);
    characterItem_->setPos((sceneRect.width() - scaled.width()) * 0.5,
                           sceneRect.height() - scaled.height() - sceneRect.height() * kLobbyPreviewFloorLiftRatio);

    if (characterLockItem_) {
        const QRectF lockBounds = characterLockItem_->boundingRect();
        characterLockItem_->setPos((sceneRect.width() - lockBounds.width()) * 0.5,
                                   sceneRect.height() * 0.37 - lockBounds.height() * 0.5);
    }

    if (characterLockTextItem_) {
        characterLockTextItem_->setPos((sceneRect.width() - characterLockTextItem_->textWidth()) * 0.5,
                                       sceneRect.height() * 0.47);
    }

    if (fallbackTextItem_) {
        fallbackTextItem_->setPos((sceneRect.width() - fallbackTextItem_->boundingRect().width()) * 0.5,
                                  sceneRect.height() * 0.85);
    }

    const QRectF focusRect = sceneRect;
    characterView_->fitInView(focusRect, Qt::KeepAspectRatio);
}

void ProfileLobbyWidget::updateModeCardHover(QWidget* card, bool hovered) {
    if (!card || !cardBaseSizes_.contains(card)) {
        return;
    }

    const QSize baseSize = cardBaseSizes_.value(card);
    const QSize hoverSize(static_cast<int>(baseSize.width() * 1.03), static_cast<int>(baseSize.height() * 1.03));
    card->setFixedSize(hovered ? hoverSize : baseSize);

    if (cardGlowEffects_.contains(card)) {
        QGraphicsDropShadowEffect* effect = cardGlowEffects_.value(card);
        if (hovered) {
            effect->setColor(QColor(212, 160, 23, 120));
            effect->setBlurRadius(18.0);
        } else {
            effect->setColor(QColor(212, 160, 23, 0));
            effect->setBlurRadius(0.0);
        }
    }

    if (modeContainer_) {
        modeContainer_->adjustSize();
    }
}

void ProfileLobbyWidget::setUserProfile(const UserProfile& profile) {
    userProfile_ = profile;
    refreshProfileUi();
}

void ProfileLobbyWidget::setSelectedCharacter(const Character& character) {
    selectedCharacter_ = character;
    refreshDuelSetupControls();
    refreshCharacterPreview();
    refreshCharacterLockState();
    refreshLobbyContext();
}

void ProfileLobbyWidget::setDuelSetup(const DuelSetup& setup) {
    duelSetup_ = setup;
    refreshDuelSetupControls();
}

void ProfileLobbyWidget::setSelectedMode(const QString& modeName) {
    // 1v1 teammate:
    // Selecting 1v1 should reveal and refresh the duel setup options.
    if (modeCards_.contains(modeName)) {
        selectedModeName_ = modeName;
        refreshModeSelectionUi();
    }
}

void ProfileLobbyWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
    gradient.setColorAt(0.0, QColor("#140F0C"));
    gradient.setColorAt(0.35, QColor("#21160F"));
    gradient.setColorAt(0.70, QColor("#120F0D"));
    gradient.setColorAt(1.0, QColor("#0D0B0A"));
    painter.fillRect(rect(), gradient);

    QRadialGradient goldGlow(width() * 0.50, height() * 0.10, width() * 0.42);
    goldGlow.setColorAt(0.0, QColor(214, 169, 80, 70));
    goldGlow.setColorAt(0.45, QColor(214, 169, 80, 18));
    goldGlow.setColorAt(1.0, QColor(214, 169, 80, 0));
    painter.fillRect(rect(), goldGlow);

    QRadialGradient emberGlow(width() * 0.86, height() * 0.78, width() * 0.34);
    emberGlow.setColorAt(0.0, QColor(168, 46, 24, 60));
    emberGlow.setColorAt(0.50, QColor(168, 46, 24, 14));
    emberGlow.setColorAt(1.0, QColor(168, 46, 24, 0));
    painter.fillRect(rect(), emberGlow);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 7));
    painter.drawRoundedRect(QRectF(width() * 0.06, height() * 0.08, width() * 0.88, height() * 0.12), 28, 28);
    painter.drawRoundedRect(QRectF(width() * 0.18, height() * 0.76, width() * 0.66, height() * 0.10), 26, 26);

    painter.setBrush(QColor(212, 160, 23, 18));
    painter.drawRect(QRectF(0, height() * 0.58, width(), 2));

    QLinearGradient vignette(0, 0, width(), height());
    vignette.setColorAt(0.0, QColor(0, 0, 0, 52));
    vignette.setColorAt(0.25, QColor(0, 0, 0, 0));
    vignette.setColorAt(0.75, QColor(0, 0, 0, 0));
    vignette.setColorAt(1.0, QColor(0, 0, 0, 68));
    painter.fillRect(rect(), vignette);

    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    QWidget::paintEvent(event);
}

void ProfileLobbyWidget::keyPressEvent(QKeyEvent* event) {
    if (rankUpgradeOverlay_ && rankUpgradeOverlay_->isVisible()) {
        if (event->key() == Qt::Key_Space) {
            claimRankUpgradePopup();
        }
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void ProfileLobbyWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updatePreviewScale();
    refreshProfileUi();
    syncRankUpgradeOverlay();
}

bool ProfileLobbyWidget::eventFilter(QObject* watched, QEvent* event) {
    if ((watched == rankUpgradeOverlay_ || watched == rankUpgradePanel_) && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (rankUpgradeOverlay_ && rankUpgradeOverlay_->isVisible()) {
            if (keyEvent->key() == Qt::Key_Space) {
                claimRankUpgradePopup();
            }
            return true;
        }
    }

    // 1v1 teammate:
    // Mode-card click handling already happens here.
    // Extend this flow if duel selection needs extra panel updates.
    if (watched == usernameLabel_ && event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            emit usernameEditRequested();
            return true;
        }
    }

    if (watched == enterArenaButton_) {
        if (event->type() == QEvent::Enter) {
            if (hoverGlowAnimation_) {
                hoverGlowAnimation_->start();
            }
        } else if (event->type() == QEvent::Leave) {
            if (hoverGlowAnimation_) {
                hoverGlowAnimation_->stop();
            }
            if (enterArenaGlowEffect_) {
                enterArenaGlowEffect_->setBlurRadius(28.0);
            }
        }
    }

    QWidget* modeCard = qobject_cast<QWidget*>(watched);
    if (modeCard && modeCard->property("modeName").isValid()) {
        if (event->type() == QEvent::Enter) {
            updateModeCardHover(modeCard, true);
        } else if (event->type() == QEvent::Leave) {
            updateModeCardHover(modeCard, false);
        } else if (event->type() == QEvent::MouseButtonPress) {
            selectedModeName_ = modeCard->property("modeName").toString();
            refreshModeSelectionUi();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ProfileLobbyWidget::updateProgression(const PlayerProgression& stats) {
    userProfile_.score = stats.totalScore;
    userProfile_.badge = QString::fromStdString(stats.currentRank);
    refreshProfileUi();
    refreshLobbyContext();

    if (actionHintLabel_) {
        const QString baseText = actionHintLabel_->text();
        actionHintLabel_->setText(QString("%1  |  Rating %2 / 5  |  %3W-%4L")
                                      .arg(baseText)
                                      .arg(stats.currentRating, 0, 'f', 1)
                                      .arg(stats.wins)
                                      .arg(stats.losses));
    }
}
