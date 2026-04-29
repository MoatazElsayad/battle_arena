#include "ExhibitionSetupPage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHash>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "InputHandler.h"

namespace {

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

QPixmap circularPortrait(const QPixmap& source, int side, const QColor& rimColor = QColor("#D4A017")) {
    if (source.isNull() || side <= 0) {
        return QPixmap();
    }

    QPixmap result(side, side);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const int inset = qMax(4, side / 14);
    const QRectF contentRect(inset, inset, side - inset * 2, side - inset * 2);

    QPainterPath clipPath;
    clipPath.addEllipse(contentRect);
    painter.setClipPath(clipPath);

    const QPixmap scaled = source.scaled(contentRect.size().toSize(),
                                         Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation);
    const QRect sourceRect((scaled.width() - contentRect.width()) / 2,
                           (scaled.height() - contentRect.height()) / 2,
                           static_cast<int>(contentRect.width()),
                           static_cast<int>(contentRect.height()));
    painter.drawPixmap(contentRect.toRect(), scaled, sourceRect);

    painter.setClipping(false);
    painter.setPen(QPen(rimColor, qMax(2.0, side / 26.0)));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(contentRect.adjusted(0.5, 0.5, -0.5, -0.5));

    painter.setPen(QPen(QColor(255, 242, 209, 110), qMax(1.0, side / 80.0)));
    painter.drawEllipse(contentRect.adjusted(4.0, 4.0, -4.0, -4.0));
    painter.end();

    return result;
}

QString portraitKey(const QString& path, int side, const QColor& rimColor) {
    return QStringLiteral("%1|%2|%3|%4|%5").arg(path).arg(side).arg(rimColor.red()).arg(rimColor.green()).arg(rimColor.blue());
}

QPixmap cachedCircularPortrait(const QString& resolvedPath, int side, const QColor& rimColor = QColor("#D4A017")) {
    static QHash<QString, QPixmap> cache;
    const QString key = portraitKey(resolvedPath, side, rimColor);
    const auto it = cache.constFind(key);
    if (it != cache.constEnd()) {
        return it.value();
    }

    const QPixmap source(resolvedPath);
    const QPixmap result = circularPortrait(source, side, rimColor);
    cache.insert(key, result);
    return result;
}

QString playerLabel(PlayerType type) {
    return QString::fromStdString(InputHandler::playerTypeToDisplayName(type));
}

QString playerProfilePath(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN: return resolveAssetPath(QStringLiteral("assets/players/Arcen/profile.png"));
        case PlayerType::DEMON_SLAYER: return resolveAssetPath(QStringLiteral("assets/players/Demon_Slayer/profile.png"));
        case PlayerType::FANTASY_WARRIOR: return resolveAssetPath(QStringLiteral("assets/players/Fantasy_Warrior/profile.png"));
        case PlayerType::HUNTRESS: return resolveAssetPath(QStringLiteral("assets/players/Huntress/profile.png"));
        case PlayerType::KNIGHT: return resolveAssetPath(QStringLiteral("assets/players/Knight/profile.png"));
        case PlayerType::MARTIAL: return resolveAssetPath(QStringLiteral("assets/players/Martial/profile.png"));
        case PlayerType::MARTIAL_HERO: return resolveAssetPath(QStringLiteral("assets/players/Martial_Hero/profile.png"));
        case PlayerType::MEDIEVAL_WARRIOR: return resolveAssetPath(QStringLiteral("assets/players/Medieval_Warrior/profile.png"));
        case PlayerType::WIZARD: return resolveAssetPath(QStringLiteral("assets/players/Wizard/profile.png"));
    }
    return QString();
}

QString enemyLabel(EnemyType type) {
    switch (type) {
        case EnemyType::FIRE_WORM: return QStringLiteral("Fire Worm");
        case EnemyType::FIRE_WIZARD: return QStringLiteral("Fire Wizard");
        case EnemyType::FLYING_DEMON: return QStringLiteral("Flying Demon");
        case EnemyType::NIGHTWEAVER: return QStringLiteral("Nightweaver");
        case EnemyType::EVIL_WIZARD: return QStringLiteral("Evil Wizard");
        case EnemyType::BLACK_WEREWOLF: return QStringLiteral("Black Werewolf");
        case EnemyType::RED_WEREWOLF: return QStringLiteral("Red Werewolf");
        case EnemyType::WHITE_WEREWOLF: return QStringLiteral("White Werewolf");
    }
    return QStringLiteral("Enemy");
}

QString enemyProfilePath(EnemyType type) {
    switch (type) {
        case EnemyType::FIRE_WORM: return resolveAssetPath(QStringLiteral("assets/enemies/Fire_Worm/profile.png"));
        case EnemyType::FIRE_WIZARD: return resolveAssetPath(QStringLiteral("assets/enemies/Fire_Wizard/profile.png"));
        case EnemyType::FLYING_DEMON: return resolveAssetPath(QStringLiteral("assets/enemies/Flying_Demon/profile.png"));
        case EnemyType::NIGHTWEAVER: return resolveAssetPath(QStringLiteral("assets/enemies/Nightweaver/profile.png"));
        case EnemyType::EVIL_WIZARD: return resolveAssetPath(QStringLiteral("assets/enemies/Evil_Wizard/profile.png"));
        case EnemyType::BLACK_WEREWOLF:
        case EnemyType::RED_WEREWOLF:
        case EnemyType::WHITE_WEREWOLF:
            return resolveAssetPath(QStringLiteral("assets/beasts/werewolf/profile.png"));
    }
    return QString();
}

struct ArenaTheme {
    QString name;
    QString imagePath;
    QString description;
};

QVector<ArenaTheme> arenaThemes() {
    return {
        {QStringLiteral("Colosseum"), QStringLiteral("assets/backgrounds/Level_1.png"), QStringLiteral("Classic stone arena with open sightlines.")},
        {QStringLiteral("Ember Court"), QStringLiteral("assets/backgrounds/Level_2.png"), QStringLiteral("A hot arena with a sharper, riskier pace.")},
        {QStringLiteral("Forest Temple"), QStringLiteral("assets/backgrounds/Level_3.png"), QStringLiteral("Mystic ruins wrapped in distant forest light.")},
        {QStringLiteral("Night Fortress"), QStringLiteral("assets/backgrounds/Level_4.png"), QStringLiteral("A darker battleground built for pressure duels.")},
        {QStringLiteral("Lava Pit"), QStringLiteral("assets/backgrounds/Level_5.png"), QStringLiteral("A brutal volcanic stage for explosive clashes.")},
        {QStringLiteral("Sky Ruins"), QStringLiteral("assets/backgrounds/Level_6.png"), QStringLiteral("A lofty ruin where every duel feels legendary.")}
    };
}

QString arenaImagePath(const QString& arenaName) {
    const QString lowered = arenaName.trimmed().toLower();
    for (const ArenaTheme& theme : arenaThemes()) {
        if (theme.name.toLower() == lowered) {
            return resolveAssetPath(theme.imagePath);
        }
    }
    return resolveAssetPath(QStringLiteral("assets/backgrounds/Level_1.png"));
}

QString arenaDescription(const QString& arenaName) {
    const QString lowered = arenaName.trimmed().toLower();
    for (const ArenaTheme& theme : arenaThemes()) {
        if (theme.name.toLower() == lowered) {
            return theme.description;
        }
    }
    return QStringLiteral("Choose the stage that fits your duel.");
}

QPixmap framedArenaPreview(const QString& resolvedPath, const QSize& size) {
    QPixmap source(resolvedPath);
    if (source.isNull() || !size.isValid()) {
        return QPixmap();
    }

    QPixmap result(size);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF outerRect(0, 0, size.width(), size.height());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(18, 14, 12, 220));
    painter.drawRoundedRect(outerRect, 22, 22);

    const QRectF imageRect(8, 8, size.width() - 16, size.height() - 16);
    QPainterPath clip;
    clip.addRoundedRect(imageRect, 18, 18);
    painter.setClipPath(clip);
    const QPixmap scaled = source.scaled(imageRect.size().toSize(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QRect sourceRect((scaled.width() - imageRect.width()) / 2,
                           (scaled.height() - imageRect.height()) / 2,
                           static_cast<int>(imageRect.width()),
                           static_cast<int>(imageRect.height()));
    painter.drawPixmap(imageRect.toRect(), scaled, sourceRect);

    painter.setClipping(false);
    painter.setPen(QPen(QColor("#D4A017"), 2.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(imageRect, 18, 18);
    painter.end();

    return result;
}

QPixmap cachedArenaPreview(const QString& resolvedPath, const QSize& size) {
    static QHash<QString, QPixmap> cache;
    const QString key = QStringLiteral("%1|%2|%3").arg(resolvedPath).arg(size.width()).arg(size.height());
    const auto it = cache.constFind(key);
    if (it != cache.constEnd()) {
        return it.value();
    }

    const QPixmap result = framedArenaPreview(resolvedPath, size);
    cache.insert(key, result);
    return result;
}

QString itemName(QListWidgetItem* item) {
    return item ? item->data(Qt::UserRole + 1).toString() : QString();
}

void configureBrowserList(QListWidget* list, const QSize& iconSize, const QSize& gridSize, int fixedHeight) {
    if (!list) {
        return;
    }

    list->setViewMode(QListView::IconMode);
    list->setFlow(QListView::LeftToRight);
    list->setResizeMode(QListView::Adjust);
    list->setLayoutMode(QListView::Batched);
    list->setBatchSize(10);
    list->setMovement(QListView::Static);
    list->setWrapping(false);
    list->setUniformItemSizes(true);
    list->setWordWrap(true);
    list->setTextElideMode(Qt::ElideRight);
    list->setIconSize(iconSize);
    list->setGridSize(gridSize);
    list->setSpacing(12);
    list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    list->setFixedHeight(fixedHeight);
}

}

ExhibitionSetupPage::ExhibitionSetupPage(QWidget* parent)
    : QWidget(parent),
      playerName_(QStringLiteral("Player_01")),
      selectedPlayerType_(PlayerType::KNIGHT),
      syncingUi_(false),
      playerPortraitLabel_(nullptr),
      playerNameLabel_(nullptr),
      opponentPortraitLabel_(nullptr),
      opponentNameLabel_(nullptr),
      summaryLabel_(nullptr),
      arenaPreviewLabel_(nullptr),
      arenaNameLabel_(nullptr),
      arenaDescriptionLabel_(nullptr),
      fighterList_(nullptr),
      randomModeButton_(nullptr),
      manualModeButton_(nullptr),
      playerCategoryButton_(nullptr),
      enemyCategoryButton_(nullptr),
      opponentList_(nullptr),
      arenaList_(nullptr),
      backButton_(nullptr),
      launchButton_(nullptr) {
    duelSetup_.opponentMode = QStringLiteral("Manual");
    duelSetup_.opponentCategory = QStringLiteral("Player");
    duelSetup_.selectedArena = QStringLiteral("Colosseum");
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
}

void ExhibitionSetupPage::applySessionState(const QString& playerName,
                                            const std::vector<PlayerType>& playerTypes,
                                            PlayerType selectedType,
                                            const ProfileLobbyWidget::DuelSetup& setup) {
    QVector<PlayerType> nextTypes;
    for (PlayerType type : playerTypes) {
        nextTypes.append(type);
    }
    if (nextTypes.isEmpty()) {
        nextTypes.append(PlayerType::KNIGHT);
    }

    const QString normalizedName = playerName.trimmed().isEmpty() ? QStringLiteral("Player_01") : playerName.trimmed();
    const PlayerType normalizedSelectedType = nextTypes.contains(selectedType) ? selectedType : nextTypes.first();

    ProfileLobbyWidget::DuelSetup normalizedSetup = setup;
    if (normalizedSetup.opponentMode.trimmed().isEmpty()) {
        normalizedSetup.opponentMode = QStringLiteral("Manual");
    }
    if (normalizedSetup.opponentCategory.trimmed().isEmpty()) {
        normalizedSetup.opponentCategory = QStringLiteral("Player");
    }
    if (normalizedSetup.selectedArena.trimmed().isEmpty()) {
        normalizedSetup.selectedArena = QStringLiteral("Colosseum");
    }

    if (playerName_ == normalizedName
        && availablePlayerTypes_ == nextTypes
        && selectedPlayerType_ == normalizedSelectedType
        && duelSetup_.opponentMode == normalizedSetup.opponentMode
        && duelSetup_.opponentCategory == normalizedSetup.opponentCategory
        && duelSetup_.selectedOpponent == normalizedSetup.selectedOpponent
        && duelSetup_.selectedArena == normalizedSetup.selectedArena) {
        return;
    }

    const bool previousSync = syncingUi_;
    syncingUi_ = true;
    setUpdatesEnabled(false);

    playerName_ = normalizedName;
    availablePlayerTypes_ = nextTypes;
    selectedPlayerType_ = normalizedSelectedType;
    duelSetup_ = normalizedSetup;

    rebuildFighterEntries();
    rebuildOpponentEntries();
    rebuildArenaEntries();
    refreshToggleStates();
    refreshPreview();

    setUpdatesEnabled(true);
    syncingUi_ = previousSync;
    update();
}

void ExhibitionSetupPage::setPlayerName(const QString& playerName) {
    const QString normalized = playerName.trimmed().isEmpty() ? QStringLiteral("Player_01") : playerName.trimmed();
    if (playerName_ == normalized) {
        return;
    }
    playerName_ = normalized;
    refreshPreview();
}

void ExhibitionSetupPage::setAvailablePlayerTypes(const std::vector<PlayerType>& playerTypes) {
    QVector<PlayerType> nextTypes;
    for (PlayerType type : playerTypes) {
        nextTypes.append(type);
    }
    if (nextTypes.isEmpty()) {
        nextTypes.append(PlayerType::KNIGHT);
    }
    if (availablePlayerTypes_ == nextTypes) {
        return;
    }

    availablePlayerTypes_ = nextTypes;
    if (!availablePlayerTypes_.contains(selectedPlayerType_)) {
        selectedPlayerType_ = availablePlayerTypes_.first();
    }

    rebuildFighterEntries();
    rebuildOpponentEntries();
    refreshPreview();
}

void ExhibitionSetupPage::setSelectedPlayerType(PlayerType type) {
    if (selectedPlayerType_ == type) {
        return;
    }
    selectedPlayerType_ = type;
    if (!availablePlayerTypes_.contains(selectedPlayerType_) && !availablePlayerTypes_.isEmpty()) {
        availablePlayerTypes_.append(selectedPlayerType_);
    }
    rebuildFighterEntries();
    rebuildOpponentEntries();
    refreshPreview();
}

PlayerType ExhibitionSetupPage::selectedPlayerType() const {
    return selectedPlayerType_;
}

void ExhibitionSetupPage::setDuelSetup(const ProfileLobbyWidget::DuelSetup& setup) {
    if (duelSetup_.opponentMode == setup.opponentMode
        && duelSetup_.opponentCategory == setup.opponentCategory
        && duelSetup_.selectedOpponent == setup.selectedOpponent
        && duelSetup_.selectedArena == setup.selectedArena) {
        return;
    }

    duelSetup_ = setup;
    if (duelSetup_.opponentMode.trimmed().isEmpty()) {
        duelSetup_.opponentMode = QStringLiteral("Manual");
    }
    if (duelSetup_.opponentCategory.trimmed().isEmpty()) {
        duelSetup_.opponentCategory = QStringLiteral("Player");
    }
    if (duelSetup_.selectedArena.trimmed().isEmpty()) {
        duelSetup_.selectedArena = QStringLiteral("Colosseum");
    }

    refreshToggleStates();
    rebuildOpponentEntries();
    rebuildArenaEntries();
    refreshPreview();
}

ProfileLobbyWidget::DuelSetup ExhibitionSetupPage::duelSetup() const {
    return duelSetup_;
}

void ExhibitionSetupPage::setupUi() {
    setObjectName(QStringLiteral("exhibitionSetupPage"));
    setStyleSheet(
        "QWidget#exhibitionSetupPage, QWidget#pageCanvas, QWidget#pageViewport {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #26160E, stop:0.55 #341D11, stop:1 #160E09);"
        "}"
        "QWidget#sectionCard {"
        " background: rgba(25,19,15,0.94);"
        " border: 1px solid rgba(212,160,23,0.24);"
        " border-radius: 22px;"
        "}"
        "QLabel#eyebrow { color:#E9C978; font:700 11px 'Segoe UI'; letter-spacing:1px; }"
        "QLabel#titleText { color:#FFF0C6; font:800 26px 'Segoe UI'; }"
        "QLabel#bodyText { color:#D9C7A1; font:12px 'Segoe UI'; }"
        "QPushButton#chipButton {"
        " color:#E8D8B4; font:700 12px 'Segoe UI';"
        " padding:10px 16px; border-radius:14px;"
        " background: rgba(255,255,255,0.03); border:1px solid rgba(212,160,23,0.22);"
        "}"
        "QPushButton#chipButton:checked {"
        " color:#FFF2CF; background: rgba(212,160,23,0.24); border:1px solid #D4A017;"
        "}"
        "QPushButton#ctaPrimary {"
        " color:#FFF2CF; font:800 15px 'Segoe UI';"
        " padding:14px 20px; border-radius:18px;"
        " background:#B4381E; border:1px solid rgba(255,220,190,0.22);"
        "}"
        "QPushButton#ctaPrimary:hover { background:#C64524; }"
        "QPushButton#ctaSecondary {"
        " color:#F1DEC0; font:700 14px 'Segoe UI';"
        " padding:12px 18px; border-radius:18px;"
        " background: rgba(255,255,255,0.05); border:1px solid rgba(212,160,23,0.24);"
        "}"
        "QListWidget {"
        " background: rgba(14,11,9,0.56); border:1px solid rgba(212,160,23,0.18);"
        " border-radius:18px; color:#F1DEC0; outline:none; padding:10px 10px 4px 10px;"
        "}"
        "QListWidget::item {"
        " background: rgba(255,255,255,0.025); border:1px solid rgba(212,160,23,0.14);"
        " border-radius:16px; padding:10px 8px; margin:4px;"
        "}"
        "QListWidget::item:selected {"
        " background: rgba(212,160,23,0.16); border:2px solid #D4A017; color:#FFF2CF;"
        "}"
        "QScrollBar:horizontal, QScrollBar:vertical { background: transparent; border:none; }"
        "QScrollBar::handle:horizontal, QScrollBar::handle:vertical { background: rgba(212,160,23,0.34); border-radius: 5px; }"
        "QScrollBar:horizontal { height: 10px; margin: 8px 12px 0 12px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal, QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width:0px; height:0px; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal, QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
    );

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border:none; }"));
    scrollArea->viewport()->setObjectName(QStringLiteral("pageViewport"));
    outerLayout->addWidget(scrollArea);

    auto* content = new QWidget(scrollArea);
    content->setObjectName(QStringLiteral("pageCanvas"));
    content->setAttribute(Qt::WA_StyledBackground, true);
    scrollArea->setWidget(content);

    auto* rootLayout = new QVBoxLayout(content);
    rootLayout->setContentsMargins(26, 22, 26, 22);
    rootLayout->setSpacing(16);

    auto* header = new QWidget(content);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(16);

    auto* titleWrap = new QWidget(header);
    auto* titleLayout = new QVBoxLayout(titleWrap);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(4);

    auto* eyebrow = new QLabel(QStringLiteral("1V1 EXHIBITION"), titleWrap);
    eyebrow->setObjectName(QStringLiteral("eyebrow"));
    titleLayout->addWidget(eyebrow);

    auto* title = new QLabel(QStringLiteral("Choose your rival and build the perfect duel"), titleWrap);
    title->setObjectName(QStringLiteral("titleText"));
    title->setWordWrap(true);
    titleLayout->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("Build the exhibition match your way: choose your fighter, set random or manual rival rules, and lock in the arena theme before the duel begins."), titleWrap);
    subtitle->setObjectName(QStringLiteral("bodyText"));
    subtitle->setWordWrap(true);
    titleLayout->addWidget(subtitle);
    headerLayout->addWidget(titleWrap, 1);

    backButton_ = new QPushButton(QStringLiteral("Back to Lobby"), header);
    backButton_->setObjectName(QStringLiteral("ctaSecondary"));
    connect(backButton_, &QPushButton::clicked, this, &ExhibitionSetupPage::backRequested);
    headerLayout->addWidget(backButton_, 0, Qt::AlignTop);
    rootLayout->addWidget(header);

    auto* heroCard = new QFrame(content);
    heroCard->setObjectName(QStringLiteral("sectionCard"));
    auto* heroLayout = new QHBoxLayout(heroCard);
    heroLayout->setContentsMargins(20, 18, 20, 18);
    heroLayout->setSpacing(18);

    auto* playerWrap = new QWidget(heroCard);
    auto* playerLayout = new QVBoxLayout(playerWrap);
    playerLayout->setContentsMargins(0, 0, 0, 0);
    playerLayout->setSpacing(10);
    auto* playerEyebrow = new QLabel(QStringLiteral("YOUR FIGHTER"), playerWrap);
    playerEyebrow->setObjectName(QStringLiteral("eyebrow"));
    playerLayout->addWidget(playerEyebrow, 0, Qt::AlignHCenter);
    playerPortraitLabel_ = new QLabel(playerWrap);
    playerPortraitLabel_->setFixedSize(118, 118);
    playerPortraitLabel_->setAlignment(Qt::AlignCenter);
    playerLayout->addWidget(playerPortraitLabel_, 0, Qt::AlignHCenter);
    playerNameLabel_ = new QLabel(QStringLiteral("Gladiator"), playerWrap);
    playerNameLabel_->setStyleSheet(QStringLiteral("color:#FFF2CF; font:800 22px 'Segoe UI';"));
    playerLayout->addWidget(playerNameLabel_, 0, Qt::AlignHCenter);
    heroLayout->addWidget(playerWrap, 1);

    auto* centerWrap = new QWidget(heroCard);
    auto* centerLayout = new QVBoxLayout(centerWrap);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(10);
    auto* vsLabel = new QLabel(QStringLiteral("VS"), centerWrap);
    vsLabel->setAlignment(Qt::AlignCenter);
    vsLabel->setStyleSheet(QStringLiteral("color:#FFE6A6; font:900 42px 'Segoe UI';"));
    centerLayout->addStretch(1);
    centerLayout->addWidget(vsLabel);
    summaryLabel_ = new QLabel(QStringLiteral("Select a manual rival or let the arena draw one at random."), centerWrap);
    summaryLabel_->setAlignment(Qt::AlignCenter);
    summaryLabel_->setWordWrap(true);
    summaryLabel_->setObjectName(QStringLiteral("bodyText"));
    centerLayout->addWidget(summaryLabel_);
    centerLayout->addStretch(1);
    heroLayout->addWidget(centerWrap, 1);

    auto* rivalWrap = new QWidget(heroCard);
    auto* rivalLayout = new QVBoxLayout(rivalWrap);
    rivalLayout->setContentsMargins(0, 0, 0, 0);
    rivalLayout->setSpacing(10);
    auto* rivalEyebrow = new QLabel(QStringLiteral("RIVAL PREVIEW"), rivalWrap);
    rivalEyebrow->setObjectName(QStringLiteral("eyebrow"));
    rivalLayout->addWidget(rivalEyebrow, 0, Qt::AlignHCenter);
    opponentPortraitLabel_ = new QLabel(rivalWrap);
    opponentPortraitLabel_->setFixedSize(118, 118);
    opponentPortraitLabel_->setAlignment(Qt::AlignCenter);
    rivalLayout->addWidget(opponentPortraitLabel_, 0, Qt::AlignHCenter);
    opponentNameLabel_ = new QLabel(QStringLiteral("Random Rival"), rivalWrap);
    opponentNameLabel_->setStyleSheet(QStringLiteral("color:#FFF2CF; font:800 22px 'Segoe UI';"));
    opponentNameLabel_->setWordWrap(true);
    opponentNameLabel_->setAlignment(Qt::AlignCenter);
    rivalLayout->addWidget(opponentNameLabel_, 0, Qt::AlignHCenter);
    heroLayout->addWidget(rivalWrap, 1);
    rootLayout->addWidget(heroCard);

    auto* mainGrid = new QGridLayout();
    mainGrid->setHorizontalSpacing(18);
    mainGrid->setVerticalSpacing(18);
    mainGrid->setColumnStretch(0, 1);
    mainGrid->setColumnStretch(1, 1);

    auto* fighterCard = new QFrame(content);
    fighterCard->setObjectName(QStringLiteral("sectionCard"));
    auto* fighterLayout = new QVBoxLayout(fighterCard);
    fighterLayout->setContentsMargins(18, 16, 18, 16);
    fighterLayout->setSpacing(10);
    auto* fighterTitle = new QLabel(QStringLiteral("Choose Your Character"), fighterCard);
    fighterTitle->setObjectName(QStringLiteral("titleText"));
    fighterTitle->setStyleSheet(QStringLiteral("color:#FFF0C6; font:800 18px 'Segoe UI';"));
    fighterLayout->addWidget(fighterTitle);
    auto* fighterBody = new QLabel(QStringLiteral("Browse the gladiators before entering exhibition. This keeps your teammate's lobby choice, but gives duel mode its own full character-select moment."), fighterCard);
    fighterBody->setObjectName(QStringLiteral("bodyText"));
    fighterBody->setWordWrap(true);
    fighterLayout->addWidget(fighterBody);
    fighterList_ = new QListWidget(fighterCard);
    configureBrowserList(fighterList_, QSize(78, 78), QSize(118, 112), 152);
    connect(fighterList_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
        if (!current || syncingUi_) {
            return;
        }
        selectedPlayerType_ = static_cast<PlayerType>(current->data(Qt::UserRole).toInt());
        rebuildOpponentEntries();
        refreshPreview();
        emit selectionChanged();
    });
    fighterLayout->addWidget(fighterList_);
    mainGrid->addWidget(fighterCard, 0, 0);

    auto* rivalCard = new QFrame(content);
    rivalCard->setObjectName(QStringLiteral("sectionCard"));
    auto* rivalCardLayout = new QVBoxLayout(rivalCard);
    rivalCardLayout->setContentsMargins(18, 16, 18, 16);
    rivalCardLayout->setSpacing(10);
    auto* rivalTitle = new QLabel(QStringLiteral("Choose the Rival"), rivalCard);
    rivalTitle->setStyleSheet(QStringLiteral("color:#FFF0C6; font:800 18px 'Segoe UI';"));
    rivalCardLayout->addWidget(rivalTitle);
    auto* rivalBody = new QLabel(QStringLiteral("Pick whether the rival is random or manual, then decide if you want another gladiator archetype or one of the enemy bosses."), rivalCard);
    rivalBody->setObjectName(QStringLiteral("bodyText"));
    rivalBody->setWordWrap(true);
    rivalCardLayout->addWidget(rivalBody);

    auto* modeRow = new QHBoxLayout();
    randomModeButton_ = new QPushButton(QStringLiteral("Random"), rivalCard);
    randomModeButton_->setObjectName(QStringLiteral("chipButton"));
    randomModeButton_->setCheckable(true);
    manualModeButton_ = new QPushButton(QStringLiteral("Manual"), rivalCard);
    manualModeButton_->setObjectName(QStringLiteral("chipButton"));
    manualModeButton_->setCheckable(true);
    modeRow->addWidget(randomModeButton_);
    modeRow->addWidget(manualModeButton_);
    rivalCardLayout->addLayout(modeRow);

    auto* categoryRow = new QHBoxLayout();
    playerCategoryButton_ = new QPushButton(QStringLiteral("Player Rival"), rivalCard);
    playerCategoryButton_->setObjectName(QStringLiteral("chipButton"));
    playerCategoryButton_->setCheckable(true);
    enemyCategoryButton_ = new QPushButton(QStringLiteral("Enemy Rival"), rivalCard);
    enemyCategoryButton_->setObjectName(QStringLiteral("chipButton"));
    enemyCategoryButton_->setCheckable(true);
    categoryRow->addWidget(playerCategoryButton_);
    categoryRow->addWidget(enemyCategoryButton_);
    rivalCardLayout->addLayout(categoryRow);

    opponentList_ = new QListWidget(rivalCard);
    configureBrowserList(opponentList_, QSize(76, 76), QSize(116, 110), 152);
    connect(opponentList_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
        if (!current || syncingUi_) {
            return;
        }
        duelSetup_.selectedOpponent = itemName(current);
        refreshPreview();
        emit selectionChanged();
    });
    rivalCardLayout->addWidget(opponentList_);
    mainGrid->addWidget(rivalCard, 0, 1);

    auto* arenaCard = new QFrame(content);
    arenaCard->setObjectName(QStringLiteral("sectionCard"));
    auto* arenaCardLayout = new QVBoxLayout(arenaCard);
    arenaCardLayout->setContentsMargins(18, 16, 18, 16);
    arenaCardLayout->setSpacing(10);
    auto* arenaTitle = new QLabel(QStringLiteral("Choose the Background Theme"), arenaCard);
    arenaTitle->setStyleSheet(QStringLiteral("color:#FFF0C6; font:800 18px 'Segoe UI';"));
    arenaCardLayout->addWidget(arenaTitle);
    auto* arenaBody = new QLabel(QStringLiteral("Set the visual mood before the duel starts. Exhibition mode uses this theme when the battle loads."), arenaCard);
    arenaBody->setObjectName(QStringLiteral("bodyText"));
    arenaBody->setWordWrap(true);
    arenaCardLayout->addWidget(arenaBody);

    auto* arenaInfoRow = new QHBoxLayout();
    arenaInfoRow->setSpacing(18);

    auto* arenaTextWrap = new QWidget(arenaCard);
    auto* arenaTextLayout = new QVBoxLayout(arenaTextWrap);
    arenaTextLayout->setContentsMargins(0, 0, 0, 0);
    arenaTextLayout->setSpacing(8);

    arenaNameLabel_ = new QLabel(QStringLiteral("Colosseum"), arenaTextWrap);
    arenaNameLabel_->setStyleSheet(QStringLiteral("color:#FFF2CF; font:800 18px 'Segoe UI';"));
    arenaNameLabel_->setWordWrap(true);
    arenaTextLayout->addWidget(arenaNameLabel_);

    arenaDescriptionLabel_ = new QLabel(QStringLiteral("Classic stone arena with open sightlines."), arenaTextWrap);
    arenaDescriptionLabel_->setObjectName(QStringLiteral("bodyText"));
    arenaDescriptionLabel_->setWordWrap(true);
    arenaDescriptionLabel_->setStyleSheet(QStringLiteral("color:#D9C7A1; font:13px 'Segoe UI';"));
    arenaTextLayout->addWidget(arenaDescriptionLabel_);
    arenaTextLayout->addStretch(1);

    arenaPreviewLabel_ = new QLabel(arenaCard);
    arenaPreviewLabel_->setFixedSize(420, 168);
    arenaPreviewLabel_->setAlignment(Qt::AlignCenter);
    arenaInfoRow->addWidget(arenaTextWrap, 1);
    arenaInfoRow->addWidget(arenaPreviewLabel_, 0, Qt::AlignRight | Qt::AlignTop);
    arenaCardLayout->addLayout(arenaInfoRow);

    arenaList_ = new QListWidget(arenaCard);
    configureBrowserList(arenaList_, QSize(104, 58), QSize(140, 94), 116);
    connect(arenaList_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
        if (!current || syncingUi_) {
            return;
        }
        duelSetup_.selectedArena = itemName(current);
        refreshPreview();
        emit selectionChanged();
    });
    arenaCardLayout->addWidget(arenaList_);
    mainGrid->addWidget(arenaCard, 1, 0, 1, 2);

    rootLayout->addLayout(mainGrid, 1);

    auto* footer = new QWidget(content);
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, 0, 0, 0);
    footerLayout->setSpacing(14);

    auto* footerHint = new QLabel(QStringLiteral("Random keeps the duel fresh. Manual lets you stage exact matchups, and the theme card decides the arena backdrop."), footer);
    footerHint->setObjectName(QStringLiteral("bodyText"));
    footerHint->setWordWrap(true);
    footerLayout->addWidget(footerHint, 1);

    launchButton_ = new QPushButton(QStringLiteral("Start Exhibition"), footer);
    launchButton_->setObjectName(QStringLiteral("ctaPrimary"));
    connect(launchButton_, &QPushButton::clicked, this, &ExhibitionSetupPage::launchRequested);
    footerLayout->addWidget(launchButton_, 0, Qt::AlignRight);
    rootLayout->addWidget(footer);

    connect(randomModeButton_, &QPushButton::clicked, this, [this]() {
        duelSetup_.opponentMode = QStringLiteral("Random");
        refreshToggleStates();
        rebuildOpponentEntries();
        refreshPreview();
        emit selectionChanged();
    });
    connect(manualModeButton_, &QPushButton::clicked, this, [this]() {
        duelSetup_.opponentMode = QStringLiteral("Manual");
        refreshToggleStates();
        rebuildOpponentEntries();
        refreshPreview();
        emit selectionChanged();
    });
    connect(playerCategoryButton_, &QPushButton::clicked, this, [this]() {
        duelSetup_.opponentCategory = QStringLiteral("Player");
        rebuildOpponentEntries();
        refreshToggleStates();
        refreshPreview();
        emit selectionChanged();
    });
    connect(enemyCategoryButton_, &QPushButton::clicked, this, [this]() {
        duelSetup_.opponentCategory = QStringLiteral("Enemy");
        rebuildOpponentEntries();
        refreshToggleStates();
        refreshPreview();
        emit selectionChanged();
    });

    rebuildArenaEntries();
    refreshToggleStates();
    refreshPreview();
}

void ExhibitionSetupPage::rebuildFighterEntries() {
    if (!fighterList_) {
        return;
    }

    const bool previousSync = syncingUi_;
    syncingUi_ = true;
    fighterList_->clear();
    for (PlayerType type : availablePlayerTypes_) {
        const QString label = playerLabel(type);
        auto* item = new QListWidgetItem(QIcon(cachedCircularPortrait(playerProfilePath(type), 84)), label, fighterList_);
        item->setData(Qt::UserRole, static_cast<int>(type));
        item->setData(Qt::UserRole + 1, label);
        item->setSizeHint(QSize(118, 106));
        fighterList_->addItem(item);
        if (type == selectedPlayerType_) {
            fighterList_->setCurrentItem(item);
        }
    }
    syncingUi_ = previousSync;
}

void ExhibitionSetupPage::rebuildOpponentEntries() {
    if (!opponentList_) {
        return;
    }

    const bool previousSync = syncingUi_;
    syncingUi_ = true;
    opponentList_->clear();
    if (duelSetup_.opponentCategory.compare(QStringLiteral("Player"), Qt::CaseInsensitive) == 0) {
        for (PlayerType type : availablePlayerTypes_) {
            if (type == selectedPlayerType_) {
                continue;
            }
            const QString label = playerLabel(type);
            auto* item = new QListWidgetItem(QIcon(cachedCircularPortrait(playerProfilePath(type), 82, QColor("#42D9A5"))), label, opponentList_);
            item->setData(Qt::UserRole, static_cast<int>(type));
            item->setData(Qt::UserRole + 1, label);
            item->setSizeHint(QSize(116, 104));
            opponentList_->addItem(item);
        }
    } else {
        const QVector<EnemyType> enemies = {
            EnemyType::FIRE_WORM,
            EnemyType::FIRE_WIZARD,
            EnemyType::FLYING_DEMON,
            EnemyType::NIGHTWEAVER,
            EnemyType::EVIL_WIZARD,
            EnemyType::BLACK_WEREWOLF,
            EnemyType::RED_WEREWOLF,
            EnemyType::WHITE_WEREWOLF
        };
        for (EnemyType type : enemies) {
            const QString label = enemyLabel(type);
            auto* item = new QListWidgetItem(QIcon(cachedCircularPortrait(enemyProfilePath(type), 82, QColor("#D96242"))), label, opponentList_);
            item->setData(Qt::UserRole, static_cast<int>(type));
            item->setData(Qt::UserRole + 1, label);
            item->setSizeHint(QSize(116, 104));
            opponentList_->addItem(item);
        }
    }

    ensureValidOpponentSelection();
    refreshToggleStates();
    syncingUi_ = previousSync;
}

void ExhibitionSetupPage::rebuildArenaEntries() {
    if (!arenaList_) {
        return;
    }

    const bool previousSync = syncingUi_;
    syncingUi_ = true;
    arenaList_->clear();
    for (const ArenaTheme& theme : arenaThemes()) {
        const QString resolvedPath = resolveAssetPath(theme.imagePath);
        auto* item = new QListWidgetItem(QIcon(cachedArenaPreview(resolvedPath, QSize(120, 68))), theme.name, arenaList_);
        item->setData(Qt::UserRole + 1, theme.name);
        item->setSizeHint(QSize(136, 92));
        arenaList_->addItem(item);
    }

    ensureValidArenaSelection();
    syncingUi_ = previousSync;
}

void ExhibitionSetupPage::refreshToggleStates() {
    if (randomModeButton_) {
        randomModeButton_->setChecked(duelSetup_.opponentMode.compare(QStringLiteral("Random"), Qt::CaseInsensitive) == 0);
    }
    if (manualModeButton_) {
        manualModeButton_->setChecked(duelSetup_.opponentMode.compare(QStringLiteral("Manual"), Qt::CaseInsensitive) == 0);
    }
    if (playerCategoryButton_) {
        playerCategoryButton_->setChecked(duelSetup_.opponentCategory.compare(QStringLiteral("Player"), Qt::CaseInsensitive) == 0);
    }
    if (enemyCategoryButton_) {
        enemyCategoryButton_->setChecked(duelSetup_.opponentCategory.compare(QStringLiteral("Enemy"), Qt::CaseInsensitive) == 0);
    }
    if (opponentList_) {
        opponentList_->setEnabled(duelSetup_.opponentMode.compare(QStringLiteral("Manual"), Qt::CaseInsensitive) == 0);
        opponentList_->setStyleSheet(opponentList_->isEnabled()
            ? QString()
            : QStringLiteral("QListWidget { background: rgba(10,8,7,0.20); color: rgba(241,222,192,0.55); }"));
    }
}

void ExhibitionSetupPage::refreshPreview() {
    if (playerPortraitLabel_) {
        playerPortraitLabel_->setPixmap(cachedCircularPortrait(playerProfilePath(selectedPlayerType_), 118, QColor("#42D9A5")));
    }
    if (playerNameLabel_) {
        playerNameLabel_->setText(playerLabel(selectedPlayerType_));
    }

    QString opponentName = QStringLiteral("Random Rival");
    QPixmap opponentPortrait;
    if (duelSetup_.opponentMode.compare(QStringLiteral("Manual"), Qt::CaseInsensitive) == 0) {
        opponentName = duelSetup_.selectedOpponent.trimmed();
        if (duelSetup_.opponentCategory.compare(QStringLiteral("Player"), Qt::CaseInsensitive) == 0) {
            const PlayerType type = static_cast<PlayerType>(opponentList_ && opponentList_->currentItem()
                ? opponentList_->currentItem()->data(Qt::UserRole).toInt()
                : static_cast<int>(selectedPlayerType_));
            opponentPortrait = cachedCircularPortrait(playerProfilePath(type), 118, QColor("#42D9A5"));
        } else {
            const EnemyType type = static_cast<EnemyType>(opponentList_ && opponentList_->currentItem()
                ? opponentList_->currentItem()->data(Qt::UserRole).toInt()
                : static_cast<int>(EnemyType::FIRE_WORM));
            opponentPortrait = cachedCircularPortrait(enemyProfilePath(type), 118, QColor("#D96242"));
        }
    } else {
        opponentName = QStringLiteral("Random %1 Rival").arg(
            duelSetup_.opponentCategory.compare(QStringLiteral("Player"), Qt::CaseInsensitive) == 0
                ? QStringLiteral("Player")
                : QStringLiteral("Enemy"));
        if (opponentList_ && opponentList_->count() > 0) {
            opponentPortrait = qvariant_cast<QIcon>(opponentList_->item(0)->icon()).pixmap(118, 118);
        }
    }

    if (opponentPortraitLabel_) {
        if (!opponentPortrait.isNull()) {
            opponentPortraitLabel_->setPixmap(opponentPortrait);
        } else {
            opponentPortraitLabel_->setPixmap(QPixmap());
        }
    }
    if (opponentNameLabel_) {
        opponentNameLabel_->setText(opponentName);
    }

    if (summaryLabel_) {
        const QString modeLine = duelSetup_.opponentMode.compare(QStringLiteral("Manual"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("Manual %1 rival selected.")
                  .arg(duelSetup_.opponentCategory.compare(QStringLiteral("Player"), Qt::CaseInsensitive) == 0
                           ? QStringLiteral("player")
                           : QStringLiteral("enemy"))
            : QStringLiteral("Random %1 rival enabled.")
                  .arg(duelSetup_.opponentCategory.compare(QStringLiteral("Player"), Qt::CaseInsensitive) == 0
                           ? QStringLiteral("player")
                           : QStringLiteral("enemy"));
        summaryLabel_->setText(QStringLiteral("%1  Theme: %2").arg(modeLine, duelSetup_.selectedArena));
    }

    if (arenaPreviewLabel_) {
        arenaPreviewLabel_->setPixmap(cachedArenaPreview(arenaImagePath(duelSetup_.selectedArena), QSize(420, 168)));
    }
    if (arenaNameLabel_) {
        arenaNameLabel_->setText(duelSetup_.selectedArena);
    }
    if (arenaDescriptionLabel_) {
        arenaDescriptionLabel_->setText(arenaDescription(duelSetup_.selectedArena));
    }
}

void ExhibitionSetupPage::ensureValidOpponentSelection() {
    if (!opponentList_) {
        return;
    }

    if (opponentList_->count() == 0) {
        duelSetup_.selectedOpponent.clear();
        return;
    }

    const bool previousSync = syncingUi_;
    syncingUi_ = true;
    for (int i = 0; i < opponentList_->count(); ++i) {
        if (itemName(opponentList_->item(i)).compare(duelSetup_.selectedOpponent, Qt::CaseInsensitive) == 0) {
            opponentList_->setCurrentRow(i);
            syncingUi_ = previousSync;
            return;
        }
    }

    opponentList_->setCurrentRow(0);
    duelSetup_.selectedOpponent = itemName(opponentList_->currentItem());
    syncingUi_ = previousSync;
}

void ExhibitionSetupPage::ensureValidArenaSelection() {
    if (!arenaList_) {
        return;
    }

    if (duelSetup_.selectedArena.trimmed().isEmpty()) {
        duelSetup_.selectedArena = QStringLiteral("Colosseum");
    }

    const bool previousSync = syncingUi_;
    syncingUi_ = true;
    for (int i = 0; i < arenaList_->count(); ++i) {
        if (itemName(arenaList_->item(i)).compare(duelSetup_.selectedArena, Qt::CaseInsensitive) == 0) {
            arenaList_->setCurrentRow(i);
            syncingUi_ = previousSync;
            return;
        }
    }

    arenaList_->setCurrentRow(0);
    duelSetup_.selectedArena = itemName(arenaList_->currentItem());
    syncingUi_ = previousSync;
}
