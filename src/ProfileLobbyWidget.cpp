#include "ProfileLobbyWidget.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QEasingCurve>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
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
const QString kPlayableLobbyMode = QStringLiteral("Save the Kings");
const QString kExhibitionLobbyMode = QStringLiteral("1v1 Exhibition");
const QString kZombieLobbyMode = QStringLiteral("Zombie");
const QString kLanLobbyMode = QStringLiteral("LAN Battle");
const QStringList kDuelArenaChoices = {
    QStringLiteral("Colosseum"),
    QStringLiteral("Ember Court"),
    QStringLiteral("Forest Temple"),
    QStringLiteral("Night Fortress"),
    QStringLiteral("Lava Pit"),
    QStringLiteral("Sky Ruins")
};

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

CharacterLobbyProfile profileForCharacter(const QString& name, const QString& specialMove) {
    const QString lowered = name.trimmed().toLower();

    if (lowered.contains("arcen")) {
        return {
            "A precision archer who controls space before opponents ever reach him.",
            {specialMove, "Charged ranged pressure", "Safe arena spacing"},
            72, 18, 68, 82
        };
    }
    if (lowered.contains("demon slayer")) {
        return {
            "An aggressive duelist built to overwhelm enemies with direct melee pressure.",
            {specialMove, "Close-range burst", "Relentless finisher chains"},
            88, 10, 58, 44
        };
    }
    if (lowered.contains("fantasy")) {
        return {
            "A reliable frontline fighter with balanced offense and a steady battle rhythm.",
            {specialMove, "Balanced sword play", "Stable arena presence"},
            70, 16, 56, 55
        };
    }
    if (lowered.contains("huntress")) {
        return {
            "A fast skirmisher who wins by movement, timing, and clean disengages.",
            {specialMove, "Agile repositioning", "Quick hit-and-run tempo"},
            64, 14, 86, 61
        };
    }
    if (lowered.contains("knight")) {
        return {
            "A disciplined defender with strong close combat fundamentals and composure.",
            {specialMove, "Armored pressure", "Solid frontline defense"},
            76, 12, 42, 60
        };
    }
    if (lowered.contains("martial hero")) {
        return {
            "A flashy combo specialist who spikes damage when momentum is on his side.",
            {specialMove, "Explosive combo routes", "Momentum-based offense"},
            84, 8, 74, 52
        };
    }
    if (lowered.contains("martial")) {
        return {
            "A rapid striker designed for chaining hits and never letting pressure drop.",
            {specialMove, "Rapid close-range strings", "Fast recovery tempo"},
            78, 6, 82, 48
        };
    }
    if (lowered.contains("medieval")) {
        return {
            "A grounded weapon master with heavier swings and deliberate battlefield control.",
            {specialMove, "Heavy disciplined strikes", "Measured mid-range control"},
            82, 12, 40, 57
        };
    }
    if (lowered.contains("wizard")) {
        return {
            "A mystical specialist who bends pace with controlled pressure and support potential.",
            {specialMove, "Arcane zone control", "High sustain potential"},
            60, 80, 46, 74
        };
    }

    return {
        "A battle-ready contender prepared to adapt to any arena challenge.",
        {specialMove.isEmpty() ? "Adaptive combat pattern" : specialMove, "Flexible role coverage"},
        60, 20, 50, 50
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
      previewPortraitLabel_(nullptr),
      previewDescriptionLabel_(nullptr),
      previewMoveLabel_(nullptr),
      previewAbilitiesLabel_(nullptr),
      previewHintLabel_(nullptr),
      attackPowerBar_(nullptr),
      healPowerBar_(nullptr),
      mobilityPowerBar_(nullptr),
      controlPowerBar_(nullptr),
      characterView_(nullptr),
      previewScene_(nullptr),
      sceneBackgroundItem_(nullptr),
      characterItem_(nullptr),
      glowItem_(nullptr),
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
      enterArenaGlowEffect_(nullptr),
      idleAnimationTimer_(new QTimer(this)),
      idleFrameIndex_(0),
      showcasingAttack_(false),
      idleShowcaseElapsedMs_(0) {
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("profileLobbyRoot");
    setCursor(Qt::ArrowCursor);

    userProfile_ = {"Player_01", 0, "Rookie", QString()};
    selectedCharacter_ = {"Demon Slayer", QString(), "Slash Combo"};
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
    headerLayout->setContentsMargins(24, 16, 24, 16);
    headerLayout->setSpacing(18);

    auto* brandWrap = new QWidget(header);
    auto* brandLayout = new QVBoxLayout(brandWrap);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(2);

    logoLabel_ = new QLabel(brandWrap);
    const QPixmap logoPixmap(resolveAssetPath("assets/backgrounds/logo.png"));
    if (!logoPixmap.isNull()) {
        logoLabel_->setPixmap(logoPixmap.scaled(250, 62, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        logoLabel_->setStyleSheet("background: transparent;");
    } else {
        logoLabel_->setText("GLADIATORS");
        logoLabel_->setStyleSheet("color:#F0C96C; font: 900 34px 'Segoe UI'; letter-spacing: 2px;");
    }
    brandLayout->addWidget(logoLabel_);

    auto* subtitleLabel = new QLabel("Forge your fighter, choose your ruleset, enter the arena.", brandWrap);
    subtitleLabel->setStyleSheet("color:rgba(245,230,184,0.78); font: 12px 'Segoe UI'; letter-spacing: 0.4px;");
    brandLayout->addWidget(subtitleLabel);

    headerLayout->addWidget(brandWrap, 1, Qt::AlignVCenter | Qt::AlignLeft);

    auto* centerWrap = new QWidget(header);
    auto* centerLayout = new QHBoxLayout(centerWrap);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(14);

    usernameLabel_ = new QLabel(centerWrap);
    usernameLabel_->setCursor(Qt::PointingHandCursor);
    usernameLabel_->setStyleSheet(
        "color:#FFF0C6; font: 700 16px 'Segoe UI';"
        "padding:7px 12px; border-radius:11px;"
        "background:rgba(212,160,23,0.10);"
        "border:1px solid rgba(212,160,23,0.24);"
    );
    usernameLabel_->installEventFilter(this);
    centerLayout->addWidget(usernameLabel_);

    scoreLabel_ = new QLabel(centerWrap);
    scoreLabel_->setStyleSheet(
        "color:#EED9A3; font:700 13px 'Segoe UI';"
        "padding:7px 12px; border-radius:11px;"
        "background:rgba(140,28,28,0.30);"
        "border:1px solid rgba(218,105,71,0.34);"
    );
    centerLayout->addWidget(scoreLabel_);

    badgeLabel_ = new QLabel(centerWrap);
    badgeLabel_->setStyleSheet(
        "color:#F5E6B8; font:700 13px 'Segoe UI';"
        "padding:7px 12px; border-radius:11px;"
        "background:rgba(212,160,23,0.14);"
    );
    centerLayout->addWidget(badgeLabel_);

    headerLayout->addWidget(centerWrap, 0, Qt::AlignCenter);

    auto* rightWrap = new QWidget(header);
    auto* rightLayout = new QHBoxLayout(rightWrap);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto* seasonLabel = new QLabel("SEASON I", rightWrap);
    seasonLabel->setStyleSheet(
        "color:#FCE8B2; font:700 11px 'Segoe UI';"
        "padding:6px 10px; border-radius:10px;"
        "background:rgba(44,69,110,0.34);"
        "border:1px solid rgba(123,169,255,0.35);"
    );
    rightLayout->addWidget(seasonLabel);

    avatarLabel_ = new QLabel(rightWrap);
    avatarLabel_->setFixedSize(46, 46);
    avatarLabel_->setAlignment(Qt::AlignCenter);
    avatarLabel_->setStyleSheet(
        "background:#3B2A1D; border:2px solid #D4A017; border-radius:23px;"
    );
    rightLayout->addWidget(avatarLabel_);

    settingsButton_ = new QToolButton(rightWrap);
    settingsButton_->setText("Menu");
    settingsButton_->setCursor(Qt::PointingHandCursor);
    settingsButton_->setMinimumSize(68, 38);
    settingsButton_->setStyleSheet(
        "QToolButton {"
        " color:#F0DFAB; font:700 12px 'Segoe UI';"
        " padding:0 12px;"
        " border:1px solid #7C5A24; border-radius:19px; background:rgba(212,160,23,0.10);"
        "}"
        "QToolButton:hover { background: rgba(212,160,23,0.20); }"
    );

    settingsMenu_ = new QMenu(settingsButton_);
    settingsMenu_->setStyleSheet(
        "QMenu { background:#1E1712; border:1px solid #6B4B24; color:#F3DFB3; }"
        "QMenu::item { padding:8px 16px; }"
        "QMenu::item:selected { background:#3A2A1A; }"
    );

    const QStringList actions = {"Settings", "Inventory", "Shop", "Crew"};
    for (const QString& actionText : actions) {
        QAction* action = settingsMenu_->addAction(actionText);
        connect(action, &QAction::triggered, this, [this, actionText]() {
            emit settingsActionTriggered(actionText);
        });
    }
    settingsButton_->setMenu(settingsMenu_);
    settingsButton_->setPopupMode(QToolButton::InstantPopup);
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
    plainPanel->setMinimumWidth(170);
    plainPanel->setStyleSheet(
        "QFrame#fighterPlainPanel {"
        " background: rgba(20,16,14,0.56);"
        " border:1px solid rgba(212,160,23,0.10);"
        " border-radius:18px;"
        "}"
    );
    contentLayout->addWidget(plainPanel, 2);

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
    infoLayout->setSpacing(12);

    previewPortraitLabel_ = new QLabel(infoPanel);
    previewPortraitLabel_->setFixedSize(128, 128);
    previewPortraitLabel_->setAlignment(Qt::AlignCenter);
    infoLayout->addWidget(previewPortraitLabel_, 0, Qt::AlignHCenter);

    previewDescriptionLabel_ = new QLabel("A battle-ready contender prepared to adapt to any arena challenge.", infoPanel);
    previewDescriptionLabel_->setWordWrap(true);
    previewDescriptionLabel_->setStyleSheet("color:#E4D2AA; font:13px 'Segoe UI'; line-height: 1.45em;");
    infoLayout->addWidget(previewDescriptionLabel_);

    auto* moveTitleLabel = new QLabel("Abilities", infoPanel);
    moveTitleLabel->setStyleSheet("color:rgba(244,216,149,0.82); font:700 11px 'Segoe UI'; letter-spacing: 0.8px;");
    infoLayout->addWidget(moveTitleLabel);

    previewMoveLabel_ = new QLabel("Adaptive combat pattern", infoPanel);
    previewMoveLabel_->setWordWrap(true);
    previewMoveLabel_->setStyleSheet("color:#F6E7BE; font:700 13px 'Segoe UI';");
    infoLayout->addWidget(previewMoveLabel_);

    previewAbilitiesLabel_ = new QLabel("Flexible role coverage", infoPanel);
    previewAbilitiesLabel_->setWordWrap(true);
    previewAbilitiesLabel_->setStyleSheet("color:#D9C7A0; font:12px 'Segoe UI'; line-height: 1.5em;");
    infoLayout->addWidget(previewAbilitiesLabel_);

    auto* statTitleLabel = new QLabel("Power Snapshot", infoPanel);
    statTitleLabel->setStyleSheet("color:rgba(244,216,149,0.82); font:700 11px 'Segoe UI'; letter-spacing: 0.8px;");
    infoLayout->addWidget(statTitleLabel);

    const auto createStatRow = [infoPanel, infoLayout](const QString& title, const QString& accent, QProgressBar** outBar) {
        auto* row = new QWidget(infoPanel);
        auto* rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(5);

        auto* label = new QLabel(title, row);
        label->setStyleSheet("color:#F1E0B8; font:700 11px 'Segoe UI'; letter-spacing:0.6px;");
        rowLayout->addWidget(label);

        auto* bar = new QProgressBar(row);
        bar->setTextVisible(false);
        bar->setRange(0, 100);
        bar->setFixedHeight(11);
        bar->setStyleSheet(QString(
            "QProgressBar {"
            " background: rgba(255,255,255,0.08);"
            " border:1px solid rgba(212,160,23,0.14);"
            " border-radius:5px;"
            "}"
            "QProgressBar::chunk {"
            " background:%1;"
            " border-radius:5px;"
            "}"
        ).arg(accent));
        rowLayout->addWidget(bar);
        infoLayout->addWidget(row);
        *outBar = bar;
    };

    createStatRow("Attack Power", "#BE3A2C", &attackPowerBar_);
    createStatRow("Heal Power", "#2F8A63", &healPowerBar_);
    createStatRow("Mobility", "#2D77B2", &mobilityPowerBar_);
    createStatRow("Control", "#8B5AB8", &controlPowerBar_);

    auto* hintTitleLabel = new QLabel("Arena Intel", infoPanel);
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
    previewScene_->setSceneRect(0, 0, 1600, 980);
    previewScene_->setBackgroundBrush(QColor(40, 30, 25));
    characterView_->setScene(previewScene_);

    sceneBackgroundItem_ = previewScene_->addPixmap(createCinematicBackdrop(QSize(1600, 980)));
    sceneBackgroundItem_->setZValue(-10.0);

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

    auto* rotationLabel = new QLabel("ARENA ROTATION", sectionHeader);
    rotationLabel->setStyleSheet(
        "color:#F8E6B1; font:700 11px 'Segoe UI'; letter-spacing:1px;"
        "padding:6px 10px; border-radius:10px;"
        "background:rgba(64,42,20,0.85); border:1px solid rgba(212,160,23,0.26);"
    );
    sectionHeaderLayout->addWidget(rotationLabel, 0, Qt::AlignTop);

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
        {kZombieLobbyMode, shieldIconPath, "Upcoming survival run against zombie waves and pressure battles.", false},
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
    if (lowered.contains("legend")) {
        return "#E58E26";
    }
    if (lowered.contains("pro")) {
        return "#2E86DE";
    }
    if (lowered.contains("elite")) {
        return "#8E44AD";
    }
    return "#6C7A89";
}

QString ProfileLobbyWidget::modeAccentFor(const QString& modeName) const {
    if (modeName.contains("1v1", Qt::CaseInsensitive)) {
        return "#A62626";
    }
    if (modeName.contains("Kings", Qt::CaseInsensitive)) {
        return "#275B9A";
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
    return modeName.compare(kPlayableLobbyMode, Qt::CaseInsensitive) == 0
        || modeName.compare(kExhibitionLobbyMode, Qt::CaseInsensitive) == 0
        || modeName.compare(kLanLobbyMode, Qt::CaseInsensitive) == 0;
}

void ProfileLobbyWidget::refreshProfileUi() {
    // Ranking teammate:
    // Extend this UI refresh to show progression values clearly in the lobby:
    // score, rank, and rating out of 5.
    if (usernameLabel_) {
        usernameLabel_->setText(userProfile_.username);
    }
    if (scoreLabel_) {
        scoreLabel_->setText(QString("SCORE  %1").arg(userProfile_.score));
    }
    if (badgeLabel_) {
        const QString color = badgeColorFor(userProfile_.badge);
        badgeLabel_->setText(QString("BADGE  %1").arg(userProfile_.badge));
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
}

void ProfileLobbyWidget::refreshLobbyContext() {
    // 1v1 teammate:
    // Update lobby copy here to reflect duel setup choices:
    // chosen opponent mode, chosen opponent, and chosen background.
    const CharacterLobbyProfile profile = profileForCharacter(selectedCharacter_.name, selectedCharacter_.specialMoves);

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
        const QString moveText = profile.abilities.isEmpty()
            ? "Adaptable close-range and spacing pressure."
            : profile.abilities.first();
        previewMoveLabel_->setText(moveText);
    }

    if (previewAbilitiesLabel_) {
        QStringList extraAbilities = profile.abilities;
        if (!extraAbilities.isEmpty()) {
            extraAbilities.removeFirst();
        }
        for (QString& ability : extraAbilities) {
            ability = QString("• %1").arg(ability);
        }
        previewAbilitiesLabel_->setText(extraAbilities.join("\n"));
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
        if (isPlayableMode(selectedModeName_)) {
            if (selectedModeName_.compare(kExhibitionLobbyMode, Qt::CaseInsensitive) == 0) {
                previewHintLabel_->setText(QStringLiteral("Duel mode selected. Open the exhibition setup page to choose your fighter, rival rules, and arena theme."));
            } else {
                previewHintLabel_->setText(description.isEmpty()
                    ? "Choose a mode and enter the arena."
                    : QString("%1 mode selected. %2").arg(modeShortTagFor(selectedModeName_), description));
            }
        } else {
            previewHintLabel_->setText(QString("%1 is coming soon. The current ready combat theme is Save the Kings.")
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
        if (isPlayableMode(selectedModeName_)) {
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
        } else {
            hint = QString("%1 is coming soon. Select Save the Kings to enter the current combat experience.")
                       .arg(selectedModeName_);
        }
        actionHintLabel_->setText(hint);
    }

    if (enterArenaButton_) {
        if (!isPlayableMode(selectedModeName_)) {
            enterArenaButton_->setText("COMING SOON");
        } else if (selectedModeName_.compare(kExhibitionLobbyMode, Qt::CaseInsensitive) == 0) {
            enterArenaButton_->setText("OPEN DUEL SETUP");
        } else {
            enterArenaButton_->setText("ENTER ARENA");
        }
    }
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

    struct LobbyAnimationSpec {
        QString idlePath;
        int idleFrameCount;
        QString attackPath;
        int attackFrameCount;
    };

    const QString characterName = selectedCharacter_.name.trimmed().toLower();
    const LobbyAnimationSpec spec = [&]() -> LobbyAnimationSpec {
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
    }();

    const QString resolvedImagePath = resolveAssetPath(selectedCharacter_.imagePath);
    const QString resolvedPortraitPath = profilePortraitPathForCharacter(selectedCharacter_.name, selectedCharacter_.imagePath);
    const QFileInfo imageInfo(resolvedImagePath);
    const QDir baseDir = imageInfo.dir();
    const QString idleSpritePath = spec.idlePath.isEmpty() ? QString() : baseDir.filePath(spec.idlePath);
    const QString attackSpritePath = spec.attackPath.isEmpty() ? QString() : baseDir.filePath(spec.attackPath);

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
                statusLabel->setText(isPlayableMode(it.key()) ? "Selected" : "Coming soon");
            } else {
                statusLabel->setText(isPlayableMode(it.key()) ? "Select mode" : "Coming soon");
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
                           sceneRect.height() - scaled.height() - sceneRect.height() * 0.03);

    if (fallbackTextItem_) {
        fallbackTextItem_->setPos((sceneRect.width() - fallbackTextItem_->boundingRect().width()) * 0.5,
                                  sceneRect.height() * 0.85);
    }

    const QRectF focusRect(sceneRect.width() * 0.08,
                           sceneRect.height() * 0.02,
                           sceneRect.width() * 0.84,
                           sceneRect.height() * 0.94);
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

void ProfileLobbyWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updatePreviewScale();
}

bool ProfileLobbyWidget::eventFilter(QObject* watched, QEvent* event) {
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

    if (actionHintLabel_) {
        const QString baseText = actionHintLabel_->text();
        actionHintLabel_->setText(QString("%1  |  Rating %2 / 5  |  %3W-%4L")
                                      .arg(baseText)
                                      .arg(stats.currentRating, 0, 'f', 1)
                                      .arg(stats.wins)
                                      .arg(stats.losses));
    }
}

