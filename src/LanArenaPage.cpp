#include "LanArenaPage.h"

#include "InputHandler.h"
#include "LanSessionManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace {

struct ArenaTheme {
    QString name;
    QString imagePath;
    QString description;
};

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

QPixmap circularPortrait(const QPixmap& source, int side, const QColor& rimColor) {
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
    painter.setPen(QPen(rimColor, qMax(2.0, side / 24.0)));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(contentRect.adjusted(0.5, 0.5, -0.5, -0.5));

    painter.setPen(QPen(QColor(255, 242, 209, 104), qMax(1.0, side / 90.0)));
    painter.drawEllipse(contentRect.adjusted(4.0, 4.0, -4.0, -4.0));
    painter.end();

    return result;
}

QString portraitKey(const QString& path, int side, const QColor& rimColor) {
    return QStringLiteral("%1|%2|%3|%4|%5").arg(path).arg(side).arg(rimColor.red()).arg(rimColor.green()).arg(rimColor.blue());
}

QPixmap cachedCircularPortrait(const QString& resolvedPath, int side, const QColor& rimColor) {
    static QHash<QString, QPixmap> cache;
    const QString key = portraitKey(resolvedPath, side, rimColor);
    const auto it = cache.constFind(key);
    if (it != cache.constEnd()) {
        return it.value();
    }

    const QPixmap result = circularPortrait(QPixmap(resolvedPath), side, rimColor);
    cache.insert(key, result);
    return result;
}

QString initialsFromText(const QString& text) {
    const QStringList parts = text.simplified().split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return QStringLiteral("?");
    }

    QString initials = parts.first().left(1).toUpper();
    if (parts.size() > 1) {
        initials += parts.last().left(1).toUpper();
    }
    return initials.left(2);
}

QPixmap placeholderPortrait(const QString& text, int side, const QColor& accentColor) {
    if (side <= 0) {
        return QPixmap();
    }

    QPixmap result(side, side);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF outerRect(0, 0, side, side);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(17, 14, 12, 240));
    painter.drawEllipse(outerRect.adjusted(3, 3, -3, -3));

    QRadialGradient glow(QPointF(side / 2.0, side / 2.0), side * 0.6);
    glow.setColorAt(0.0, QColor(accentColor.red(), accentColor.green(), accentColor.blue(), 220));
    glow.setColorAt(0.8, QColor(accentColor.red(), accentColor.green(), accentColor.blue(), 58));
    glow.setColorAt(1.0, QColor(10, 10, 10, 0));
    painter.setBrush(glow);
    painter.drawEllipse(outerRect.adjusted(10, 10, -10, -10));

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(accentColor, qMax(2.0, side / 24.0)));
    painter.drawEllipse(outerRect.adjusted(4.5, 4.5, -4.5, -4.5));

    QFont font(QStringLiteral("Segoe UI"));
    font.setBold(true);
    font.setPixelSize(qMax(18, side / 3));
    painter.setFont(font);
    painter.setPen(QColor("#FFF3D2"));
    painter.drawText(outerRect, Qt::AlignCenter, initialsFromText(text));
    painter.end();

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

QVector<ArenaTheme> arenaThemes() {
    return {
        {QStringLiteral("Colosseum"), QStringLiteral("assets/backgrounds/Level_1.png"), QStringLiteral("Classic stone arena with open sightlines and a grounded duel pace.")},
        {QStringLiteral("Ember Court"), QStringLiteral("assets/backgrounds/Level_2.png"), QStringLiteral("A hotter battleground where every clash feels more aggressive.")},
        {QStringLiteral("Forest Temple"), QStringLiteral("assets/backgrounds/Level_3.png"), QStringLiteral("Mystic ruins wrapped in green light and cleaner contrast.")},
        {QStringLiteral("Night Fortress"), QStringLiteral("assets/backgrounds/Level_4.png"), QStringLiteral("A darker keep built for pressure-heavy late-night duels.")},
        {QStringLiteral("Lava Pit"), QStringLiteral("assets/backgrounds/Level_5.png"), QStringLiteral("Volcanic chaos with a sharp, dramatic mood for explosive matches.")},
        {QStringLiteral("Sky Ruins"), QStringLiteral("assets/backgrounds/Level_6.png"), QStringLiteral("High ruins above the clouds for a more heroic final-stage feel.")}
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
    return QStringLiteral("Pick the battleground that matches the mood of your duel.");
}

QPixmap framedArenaPreview(const QString& resolvedPath, const QSize& size) {
    const QPixmap source(resolvedPath);
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
    painter.drawRoundedRect(outerRect, 24, 24);

    const QRectF imageRect(8, 8, size.width() - 16, size.height() - 16);
    QPainterPath clip;
    clip.addRoundedRect(imageRect, 18, 18);
    painter.setClipPath(clip);

    const QPixmap scaled = source.scaled(imageRect.size().toSize(),
                                         Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation);
    const QRect sourceRect((scaled.width() - imageRect.width()) / 2,
                           (scaled.height() - imageRect.height()) / 2,
                           static_cast<int>(imageRect.width()),
                           static_cast<int>(imageRect.height()));
    painter.drawPixmap(imageRect.toRect(), scaled, sourceRect);

    painter.setClipping(false);
    painter.setPen(QPen(QColor("#D4A017"), 2.1));
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
    list->setSpacing(10);
    list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    list->setFixedHeight(fixedHeight);
}

QString itemName(QListWidgetItem* item) {
    return item ? item->data(Qt::UserRole + 1).toString() : QString();
}

bool isKnownPlayerType(int rawValue) {
    switch (rawValue) {
        case static_cast<int>(PlayerType::ARCEN):
        case static_cast<int>(PlayerType::DEMON_SLAYER):
        case static_cast<int>(PlayerType::FANTASY_WARRIOR):
        case static_cast<int>(PlayerType::HUNTRESS):
        case static_cast<int>(PlayerType::KNIGHT):
        case static_cast<int>(PlayerType::MARTIAL):
        case static_cast<int>(PlayerType::MARTIAL_HERO):
        case static_cast<int>(PlayerType::MEDIEVAL_WARRIOR):
        case static_cast<int>(PlayerType::WIZARD):
            return true;
        default:
            return false;
    }
}

PlayerType playerTypeFromStoredValue(int rawValue) {
    if (!isKnownPlayerType(rawValue)) {
        return PlayerType::KNIGHT;
    }
    return static_cast<PlayerType>(rawValue);
}

QPixmap portraitForPlayerInfo(const LanPlayerInfo& player, int side, const QColor& rimColor) {
    if (isKnownPlayerType(player.fighterType)) {
        const QString portraitPath = playerProfilePath(playerTypeFromStoredValue(player.fighterType));
        const QPixmap portrait = cachedCircularPortrait(portraitPath, side, rimColor);
        if (!portrait.isNull()) {
            return portrait;
        }
    }

    const QString fallbackText = !player.fighterName.trimmed().isEmpty()
        ? player.fighterName
        : (!player.username.trimmed().isEmpty() ? player.username : QStringLiteral("?"));
    return placeholderPortrait(fallbackText, side, rimColor);
}

QString sectionFrameStyle() {
    return QStringLiteral(
        "QFrame#lanSectionFrame {"
        " background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgba(31,22,17,0.95), stop:1 rgba(14,10,8,0.98));"
        " border: 1px solid rgba(212,160,23,0.12);"
        " border-radius: 16px;"
        "}"
    );
}

QString insetPanelStyle() {
    return QStringLiteral(
        "QFrame#lanInsetPanel {"
        " background: rgba(64,43,27,0.30);"
        " border: 1px solid rgba(212,160,23,0.09);"
        " border-radius: 12px;"
        "}"
    );
}

QString previewCardStyle() {
    return QStringLiteral(
        "QFrame#lanPlayerCard {"
        " background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(66,46,28,0.54), stop:1 rgba(27,18,13,0.96));"
        " border: 1px solid rgba(212,160,23,0.12);"
        " border-radius: 14px;"
        "}"
    );
}

QString secondaryButtonStyle() {
    return QStringLiteral(
        "QPushButton {"
        " background: rgba(70,49,32,0.88);"
        " border: 1px solid rgba(212,160,23,0.18);"
        " border-radius: 16px;"
        " color: #F3DEB4;"
        " font: 700 14px 'Segoe UI';"
        " padding: 12px 18px;"
        "}"
        "QPushButton:hover { background: rgba(88,61,36,0.95); border-color: rgba(255,214,128,0.30); }"
        "QPushButton:checked { background: rgba(120,82,31,0.94); border-color: rgba(224,178,74,0.34); color: #FFF2CE; }"
        "QPushButton:disabled { color: rgba(240,222,178,0.42); background: rgba(58,42,27,0.30); border-color: rgba(212,160,23,0.08); }"
    );
}

QString primaryButtonStyle() {
    return QStringLiteral(
        "QPushButton {"
        " background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #7D1712, stop:0.48 #B82D1B, stop:1 #E14E23);"
        " border: 1px solid rgba(255,198,165,0.40);"
        " border-radius: 16px;"
        " color: #FFF1E8;"
        " font: 800 15px 'Segoe UI';"
        " padding: 13px 22px;"
        "}"
        "QPushButton:hover { border-color: rgba(255,215,179,0.66); }"
        "QPushButton:disabled { color: rgba(255,230,230,0.44); background: rgba(110,13,13,0.34); border-color: rgba(216,106,85,0.18); }"
    );
}

QString chipStyle(const QColor& fill, const QColor& border, const QColor& textColor = QColor("#FFF0C6")) {
    return QStringLiteral(
        "color:%1; font:800 11px 'Segoe UI'; padding:7px 12px; border-radius:12px;"
        "background:%2; border:1px solid %3;")
        .arg(textColor.name(),
             fill.name(QColor::HexArgb),
             border.name(QColor::HexArgb));
}

void applyConnectionChip(QLabel* label, const QString& text, bool connected) {
    if (!label) {
        return;
    }

    label->setText(text);
    if (connected) {
        label->setStyleSheet(chipStyle(QColor(39, 174, 96, 52), QColor("#2ECC71")));
    } else {
        label->setStyleSheet(chipStyle(QColor(108, 122, 137, 48), QColor("#7F8C8D")));
    }
}

void applyReadyChip(QLabel* label, bool ready, bool connected) {
    if (!label) {
        return;
    }

    if (ready) {
        label->setText(QStringLiteral("Ready"));
        label->setStyleSheet(chipStyle(QColor(212, 160, 23, 56), QColor("#D4A017")));
        return;
    }

    label->setText(connected ? QStringLiteral("Choosing") : QStringLiteral("Waiting"));
    label->setStyleSheet(chipStyle(QColor(84, 67, 53, 96), QColor(212, 160, 23, 40), QColor("#DCC89F")));
}

} // namespace

LanArenaPage::LanArenaPage(QWidget* parent)
    : QWidget(parent),
      sessionManager_(nullptr),
      syncingUi_(false),
      username_(QStringLiteral("Player_01")),
      fighterName_(playerLabel(PlayerType::KNIGHT)),
      fighterType_(PlayerType::KNIGHT),
      selectedArena_(QStringLiteral("Colosseum")),
      addressHintLabel_(nullptr),
      localNameLabel_(nullptr),
      localFighterLabel_(nullptr),
      localPortraitLabel_(nullptr),
      localConnectionLabel_(nullptr),
      localReadyLabel_(nullptr),
      remoteNameLabel_(nullptr),
      remoteFighterLabel_(nullptr),
      remotePortraitLabel_(nullptr),
      remoteConnectionLabel_(nullptr),
      remoteReadyLabel_(nullptr),
      linkStatusBadgeLabel_(nullptr),
      linkStatusTextLabel_(nullptr),
      linkStatusMetaLabel_(nullptr),
      duelHintLabel_(nullptr),
      arenaNameLabel_(nullptr),
      arenaDescriptionLabel_(nullptr),
      arenaPreviewLabel_(nullptr),
      arenaOwnerHintLabel_(nullptr),
      hostAddressEdit_(nullptr),
      portEdit_(nullptr),
      fighterList_(nullptr),
      arenaList_(nullptr),
      hostButton_(nullptr),
      joinButton_(nullptr),
      disconnectButton_(nullptr),
      readyButton_(nullptr),
      primeButton_(nullptr) {
    const std::vector<PlayerType> sortedTypes = InputHandler::getCharactersSortedByFeatures();
    for (PlayerType type : sortedTypes) {
        availablePlayerTypes_.append(type);
    }
    if (availablePlayerTypes_.isEmpty()) {
        availablePlayerTypes_.append(PlayerType::KNIGHT);
    }

    currentSnapshot_.arenaName = selectedArena_;
    currentSnapshot_.localPlayer.username = username_;
    currentSnapshot_.localPlayer.fighterName = fighterName_;
    currentSnapshot_.localPlayer.fighterType = static_cast<int>(fighterType_);

    setupUi();
}

void LanArenaPage::setSessionManager(LanSessionManager* manager) {
    if (sessionManager_ == manager) {
        return;
    }

    if (sessionManager_) {
        QObject::disconnect(sessionManager_, nullptr, this, nullptr);
    }

    sessionManager_ = manager;
    refreshAddressHints();

    if (!sessionManager_) {
        return;
    }

    connect(sessionManager_, &LanSessionManager::snapshotChanged, this, &LanArenaPage::applySnapshot);
    connect(sessionManager_, &LanSessionManager::eventLogged, this, &LanArenaPage::appendLog);
    connect(sessionManager_, &LanSessionManager::errorRaised, this, &LanArenaPage::appendLog);

    sessionManager_->setLocalProfile(username_, fighterName_, static_cast<int>(fighterType_));
    sessionManager_->setArenaName(selectedArena_);
    applySnapshot(sessionManager_->snapshot());
}

void LanArenaPage::setIdentity(const QString& username, const QString& fighterName, PlayerType fighterType) {
    const QString normalizedUser = username.trimmed().isEmpty() ? QStringLiteral("Player_01") : username.trimmed();
    const QString normalizedFighter = fighterName.trimmed().isEmpty() ? playerLabel(fighterType) : fighterName.trimmed();
    const bool fighterChanged = fighterType_ != fighterType || fighterName_ != normalizedFighter;
    const bool resetReady = fighterChanged && currentSnapshot_.localPlayer.ready;

    username_ = normalizedUser;
    fighterName_ = normalizedFighter;
    fighterType_ = fighterType;

    if (!availablePlayerTypes_.contains(fighterType_)) {
        availablePlayerTypes_.append(fighterType_);
        rebuildFighterEntries();
    }

    currentSnapshot_.localPlayer.username = username_;
    currentSnapshot_.localPlayer.fighterName = fighterName_;
    currentSnapshot_.localPlayer.fighterType = static_cast<int>(fighterType_);
    if (resetReady) {
        currentSnapshot_.localPlayer.ready = false;
    }

    syncFighterSelection();
    applySnapshot(currentSnapshot_);

    if (sessionManager_) {
        sessionManager_->setLocalProfile(username_, fighterName_, static_cast<int>(fighterType_));
        if (resetReady) {
            sessionManager_->setLocalReady(false);
        }
    }
}

void LanArenaPage::hostSession() {
    if (!sessionManager_) {
        return;
    }

    sessionManager_->setLocalProfile(username_, fighterName_, static_cast<int>(fighterType_));
    sessionManager_->setArenaName(selectedArena_);
    sessionManager_->startHosting(username_, fighterName_, static_cast<int>(fighterType_), selectedPort());
}

void LanArenaPage::joinSession() {
    if (!sessionManager_) {
        return;
    }

    sessionManager_->setLocalProfile(username_, fighterName_, static_cast<int>(fighterType_));
    sessionManager_->setArenaName(selectedArena_);
    sessionManager_->joinSession(hostAddressEdit_->text(), username_, fighterName_, static_cast<int>(fighterType_), selectedPort());
}

void LanArenaPage::disconnectSession() {
    if (sessionManager_) {
        sessionManager_->disconnectSession();
    }
}

void LanArenaPage::readyToggled(bool checked) {
    currentSnapshot_.localPlayer.ready = checked;
    if (sessionManager_) {
        sessionManager_->setLocalReady(checked);
    }
}

void LanArenaPage::primeMatch() {
    if (sessionManager_) {
        sessionManager_->primeMatch();
    }
}

void LanArenaPage::applySnapshot(const LanSessionSnapshot& snapshot) {
    currentSnapshot_ = snapshot;

    if (!snapshot.arenaName.trimmed().isEmpty()) {
        selectedArena_ = snapshot.arenaName.trimmed();
    }

    if (!snapshot.localPlayer.username.trimmed().isEmpty()) {
        username_ = snapshot.localPlayer.username.trimmed();
    }
    if (!snapshot.localPlayer.fighterName.trimmed().isEmpty()) {
        fighterName_ = snapshot.localPlayer.fighterName.trimmed();
    } else if (isKnownPlayerType(snapshot.localPlayer.fighterType)) {
        fighterType_ = playerTypeFromStoredValue(snapshot.localPlayer.fighterType);
        fighterName_ = playerLabel(fighterType_);
    }
    if (isKnownPlayerType(snapshot.localPlayer.fighterType)) {
        fighterType_ = playerTypeFromStoredValue(snapshot.localPlayer.fighterType);
    }

    syncFighterSelection();
    syncArenaSelection();
    refreshArenaControlState();

    if (linkStatusBadgeLabel_) {
        QColor statusFill(108, 122, 137, 52);
        QColor statusBorder("#7F8C8D");
        switch (snapshot.state) {
            case LanSessionState::HOSTING:
                statusFill = QColor(52, 152, 219, 54);
                statusBorder = QColor("#2E86DE");
                break;
            case LanSessionState::CONNECTING:
                statusFill = QColor(142, 68, 173, 54);
                statusBorder = QColor("#8E44AD");
                break;
            case LanSessionState::LINKED:
            case LanSessionState::READY_CHECK:
                statusFill = QColor(39, 174, 96, 58);
                statusBorder = QColor("#2ECC71");
                break;
            case LanSessionState::MATCH_PRIMED:
                statusFill = QColor(230, 126, 34, 58);
                statusBorder = QColor("#E67E22");
                break;
            case LanSessionState::ERROR:
                statusFill = QColor(192, 57, 43, 66);
                statusBorder = QColor("#E74C3C");
                break;
            case LanSessionState::IDLE:
            default:
                break;
        }
        linkStatusBadgeLabel_->setText(lanSessionStateDisplayName(snapshot.state).toUpper());
        linkStatusBadgeLabel_->setStyleSheet(chipStyle(statusFill, statusBorder));
    }
    if (linkStatusTextLabel_) {
        linkStatusTextLabel_->setText(snapshot.statusLine);
    }
    if (linkStatusMetaLabel_) {
        const QString pingText = snapshot.lastPingMs >= 0
            ? QStringLiteral("%1 ms").arg(snapshot.lastPingMs)
            : QStringLiteral("--");
        const QString rivalText = snapshot.remoteConnected
            ? QStringLiteral("Rival connected")
            : QStringLiteral("Waiting for rival");
        linkStatusMetaLabel_->setText(QStringLiteral("You are %1  |  %2  |  Ping %3")
                                          .arg(lanRoleDisplayName(snapshot.localRole), rivalText, pingText));
    }

    localNameLabel_->setText(snapshot.localPlayer.username.trimmed().isEmpty()
                                 ? QStringLiteral("You")
                                 : snapshot.localPlayer.username.trimmed());
    localFighterLabel_->setText(snapshot.localPlayer.fighterName.trimmed().isEmpty()
                                    ? playerLabel(fighterType_)
                                    : snapshot.localPlayer.fighterName.trimmed());
    localPortraitLabel_->setPixmap(portraitForPlayerInfo(snapshot.localPlayer, 104, QColor("#42D9A5")));
    applyConnectionChip(localConnectionLabel_, QStringLiteral("Local"), true);
    applyReadyChip(localReadyLabel_, snapshot.localPlayer.ready, true);

    if (snapshot.remoteConnected) {
        remoteNameLabel_->setText(snapshot.remotePlayer.username.trimmed().isEmpty()
                                      ? QStringLiteral("Linked Rival")
                                      : snapshot.remotePlayer.username.trimmed());
        remoteFighterLabel_->setText(snapshot.remotePlayer.fighterName.trimmed().isEmpty()
                                         ? QStringLiteral("Choosing a fighter...")
                                         : snapshot.remotePlayer.fighterName.trimmed());
        remotePortraitLabel_->setPixmap(portraitForPlayerInfo(snapshot.remotePlayer, 104, QColor("#D86B4A")));
    } else {
        remoteNameLabel_->setText(QStringLiteral("Waiting for rival"));
        remoteFighterLabel_->setText(QStringLiteral("Their fighter appears here as soon as they connect."));
        remotePortraitLabel_->setPixmap(placeholderPortrait(QStringLiteral("?"), 104, QColor("#7F8C8D")));
    }
    applyConnectionChip(remoteConnectionLabel_,
                        snapshot.remoteConnected ? QStringLiteral("Connected") : QStringLiteral("Waiting"),
                        snapshot.remoteConnected);
    applyReadyChip(remoteReadyLabel_, snapshot.remotePlayer.ready, snapshot.remoteConnected);

    arenaPreviewLabel_->setPixmap(cachedArenaPreview(arenaImagePath(selectedArena_), QSize(380, 136)));
    arenaNameLabel_->setText(selectedArena_);
    arenaDescriptionLabel_->setText(arenaDescription(selectedArena_));

    const bool busy = snapshot.state == LanSessionState::HOSTING
        || snapshot.state == LanSessionState::CONNECTING
        || snapshot.state == LanSessionState::LINKED
        || snapshot.state == LanSessionState::READY_CHECK
        || snapshot.state == LanSessionState::MATCH_PRIMED;

    hostButton_->setEnabled(!busy || snapshot.localRole == LanRole::NONE || snapshot.state == LanSessionState::ERROR);
    joinButton_->setEnabled(!busy || snapshot.localRole == LanRole::NONE || snapshot.state == LanSessionState::ERROR);
    disconnectButton_->setEnabled(busy || snapshot.state == LanSessionState::ERROR);
    readyButton_->setEnabled(snapshot.remoteConnected);
    {
        const QSignalBlocker blocker(readyButton_);
        readyButton_->setChecked(snapshot.localPlayer.ready);
    }
    readyButton_->setText(snapshot.localPlayer.ready ? QStringLiteral("Ready Locked") : QStringLiteral("I'm Ready"));

    primeButton_->setEnabled(snapshot.canStartMatch && snapshot.localRole == LanRole::HOST);
    if (snapshot.localRole == LanRole::GUEST) {
        primeButton_->setText(snapshot.canStartMatch ? QStringLiteral("Waiting for Host") : QStringLiteral("Host Starts the Duel"));
    } else {
        primeButton_->setText(QStringLiteral("Start Duel"));
    }

    if (snapshot.canStartMatch && snapshot.localRole == LanRole::HOST) {
        duelHintLabel_->setText(QStringLiteral("Both fighters are locked in. Start the duel when both players are ready to go."));
    } else if (snapshot.canStartMatch) {
        duelHintLabel_->setText(QStringLiteral("Both fighters are ready. Stay here while the host starts the duel."));
    } else if (snapshot.remoteConnected) {
        duelHintLabel_->setText(QStringLiteral("You are linked. Watch each fighter update live, then get both players ready."));
    } else if (snapshot.localRole == LanRole::HOST) {
        duelHintLabel_->setText(QStringLiteral("Your room is open. As soon as someone joins, their fighter and ready state will appear here."));
    } else if (snapshot.localRole == LanRole::GUEST) {
        duelHintLabel_->setText(QStringLiteral("Trying to join the room. Keep this page open until the waiting room appears."));
    } else {
        duelHintLabel_->setText(QStringLiteral("Choose your fighter, pick the battleground, then host a room or join a nearby one."));
    }
}

void LanArenaPage::appendLog(const QString& message) {
    Q_UNUSED(message);
}

void LanArenaPage::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName(QStringLiteral("lanArenaPage"));
    setStyleSheet(
        "QWidget#lanArenaPage { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #120D0B, stop:0.38 #1D1410, stop:0.74 #14100D, stop:1 #090807); color: #F5E6D3; }"
        "QLabel { background: transparent; border: none; }"
        "QLabel#eyebrow { color: rgba(245,213,143,0.64); font: 700 11px 'Segoe UI'; letter-spacing: 2px; }"
        "QLabel#headline { color:#FFF0C6; font: 900 28px 'Segoe UI'; }"
        "QLabel#subheadline { color: rgba(245,230,184,0.78); font: 13px 'Segoe UI'; }"
        "QLabel#sectionTitle { color:#FFF0C6; font: 800 18px 'Segoe UI'; }"
        "QLabel#microLabel { color: rgba(245,213,143,0.70); font: 700 10px 'Segoe UI'; letter-spacing: 1px; }"
        "QLabel#sectionBody { color: rgba(245,230,184,0.74); font: 12px 'Segoe UI'; }"
        "QLabel#playerName { color:#FFF2CF; font:800 20px 'Segoe UI'; }"
        "QLabel#fighterLine { color:#F4D68F; font:700 14px 'Segoe UI'; }"
        "QLineEdit {"
        " background-color: rgba(49,35,23,0.94);"
        " color: #F5E6D3;"
        " border: 1px solid rgba(124,90,36,0.32);"
        " border-radius: 14px;"
        " padding: 11px 12px;"
        " font: 13px 'Segoe UI';"
        "}"
        "QLineEdit:focus { border-color: rgba(212,160,23,0.38); background: rgba(58,40,24,0.98); }"
        "QListWidget {"
        " background: rgba(14,11,9,0.58);"
        " border:1px solid rgba(212,160,23,0.16);"
        " border-radius:18px;"
        " color:#F1DEC0;"
        " outline:none;"
        " padding:10px 10px 4px 10px;"
        "}"
        "QListWidget::item {"
        " background: rgba(255,255,255,0.025);"
        " border:1px solid rgba(212,160,23,0.14);"
        " border-radius:16px;"
        " padding:10px 8px;"
        " margin:4px;"
        "}"
        "QListWidget::item:selected {"
        " background: rgba(212,160,23,0.16);"
        " border:2px solid #D4A017;"
        " color:#FFF2CF;"
        "}"
        "QScrollBar:horizontal, QScrollBar:vertical { background: transparent; border:none; }"
        "QScrollBar::handle:horizontal, QScrollBar::handle:vertical { background: rgba(212,160,23,0.34); border-radius: 5px; }"
        "QScrollBar:horizontal { height: 10px; margin: 8px 12px 0 12px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal, QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width:0px; height:0px; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal, QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(28, 18, 28, 18);
    rootLayout->setSpacing(12);

    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(18);

    auto* titleWrap = new QVBoxLayout();
    titleWrap->setSpacing(4);

    auto* eyebrow = new QLabel(QStringLiteral("LOCAL DUEL"), this);
    eyebrow->setObjectName(QStringLiteral("eyebrow"));
    titleWrap->addWidget(eyebrow);

    auto* title = new QLabel(QStringLiteral("ARENA LINK"), this);
    title->setObjectName(QStringLiteral("headline"));
    titleWrap->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("Pick fighters, sync the battleground, and start when both players are ready."), this);
    subtitle->setObjectName(QStringLiteral("subheadline"));
    subtitle->setWordWrap(true);
    titleWrap->addWidget(subtitle);

    headerRow->addLayout(titleWrap, 1);

    auto* backButton = new QPushButton(QStringLiteral("Back to Lobby"), this);
    backButton->setStyleSheet(secondaryButtonStyle());
    connect(backButton, &QPushButton::clicked, this, &LanArenaPage::backRequested);
    headerRow->addWidget(backButton, 0, Qt::AlignTop);
    rootLayout->addLayout(headerRow);

    auto* bodyRow = new QHBoxLayout();
    bodyRow->setSpacing(18);

    auto* leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(14);

    auto* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(14);

    auto* roomFrame = new QFrame(this);
    roomFrame->setObjectName(QStringLiteral("lanSectionFrame"));
    roomFrame->setStyleSheet(sectionFrameStyle());
    roomFrame->setMinimumHeight(220);
    roomFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* roomLayout = new QVBoxLayout(roomFrame);
    roomLayout->setContentsMargins(18, 14, 18, 14);
    roomLayout->setSpacing(9);

    auto* roomTitle = new QLabel(QStringLiteral("Open or Join a Room"), roomFrame);
    roomTitle->setObjectName(QStringLiteral("sectionTitle"));
    roomLayout->addWidget(roomTitle);

    auto* roomBody = new QLabel(QStringLiteral("Host opens the room. Guest enters the host IP and joins."), roomFrame);
    roomBody->setObjectName(QStringLiteral("sectionBody"));
    roomBody->setWordWrap(true);
    roomLayout->addWidget(roomBody);

    addressHintLabel_ = new QLabel(roomFrame);
    addressHintLabel_->setObjectName(QStringLiteral("sectionBody"));
    addressHintLabel_->setWordWrap(true);
    roomLayout->addWidget(addressHintLabel_);

    auto* roomInputPanel = new QFrame(roomFrame);
    roomInputPanel->setObjectName(QStringLiteral("lanInsetPanel"));
    roomInputPanel->setStyleSheet(insetPanelStyle());
    auto* roomInputGrid = new QGridLayout(roomInputPanel);
    roomInputGrid->setContentsMargins(14, 10, 14, 10);
    roomInputGrid->setHorizontalSpacing(12);
    roomInputGrid->setVerticalSpacing(8);

    auto* hostLabel = new QLabel(QStringLiteral("HOST IP"), roomInputPanel);
    hostLabel->setObjectName(QStringLiteral("microLabel"));
    roomInputGrid->addWidget(hostLabel, 0, 0);

    auto* portLabel = new QLabel(QStringLiteral("PORT"), roomInputPanel);
    portLabel->setObjectName(QStringLiteral("microLabel"));
    roomInputGrid->addWidget(portLabel, 0, 1);

    hostAddressEdit_ = new QLineEdit(roomInputPanel);
    hostAddressEdit_->setPlaceholderText(QStringLiteral("Enter the host IP from the other laptop"));
    roomInputGrid->addWidget(hostAddressEdit_, 1, 0);

    portEdit_ = new QLineEdit(QString::number(kDefaultLanPort), roomInputPanel);
    portEdit_->setValidator(new QIntValidator(1024, 65535, portEdit_));
    roomInputGrid->addWidget(portEdit_, 1, 1);

    roomInputGrid->setColumnStretch(0, 3);
    roomInputGrid->setColumnStretch(1, 1);
    roomLayout->addWidget(roomInputPanel);

    hostButton_ = new QPushButton(QStringLiteral("Open Room"), roomFrame);
    hostButton_->setStyleSheet(primaryButtonStyle());
    connect(hostButton_, &QPushButton::clicked, this, &LanArenaPage::hostSession);

    joinButton_ = new QPushButton(QStringLiteral("Join Room"), roomFrame);
    joinButton_->setStyleSheet(secondaryButtonStyle());
    connect(joinButton_, &QPushButton::clicked, this, &LanArenaPage::joinSession);

    disconnectButton_ = new QPushButton(QStringLiteral("Leave Room"), roomFrame);
    disconnectButton_->setStyleSheet(secondaryButtonStyle());
    connect(disconnectButton_, &QPushButton::clicked, this, &LanArenaPage::disconnectSession);

    auto* roomActionGrid = new QGridLayout();
    roomActionGrid->setHorizontalSpacing(12);
    roomActionGrid->setVerticalSpacing(8);
    roomActionGrid->addWidget(hostButton_, 0, 0);
    roomActionGrid->addWidget(joinButton_, 0, 1);
    roomActionGrid->addWidget(disconnectButton_, 1, 0, 1, 2);
    roomLayout->addLayout(roomActionGrid);

    leftColumn->addWidget(roomFrame, 2);

    auto* fighterFrame = new QFrame(this);
    fighterFrame->setObjectName(QStringLiteral("lanSectionFrame"));
    fighterFrame->setStyleSheet(sectionFrameStyle());
    fighterFrame->setMinimumHeight(250);
    fighterFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* fighterLayout = new QVBoxLayout(fighterFrame);
    fighterLayout->setContentsMargins(18, 14, 18, 14);
    fighterLayout->setSpacing(8);

    auto* fighterTitle = new QLabel(QStringLiteral("Choose Your Fighter"), fighterFrame);
    fighterTitle->setObjectName(QStringLiteral("sectionTitle"));
    fighterLayout->addWidget(fighterTitle);

    auto* fighterBody = new QLabel(QStringLiteral("Your choice updates live for the other player."), fighterFrame);
    fighterBody->setObjectName(QStringLiteral("sectionBody"));
    fighterBody->setWordWrap(true);
    fighterLayout->addWidget(fighterBody);

    fighterList_ = new QListWidget(fighterFrame);
    configureBrowserList(fighterList_, QSize(72, 72), QSize(122, 116), 138);
    connect(fighterList_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
        if (!current || syncingUi_) {
            return;
        }

        const bool wasReady = currentSnapshot_.localPlayer.ready;
        fighterType_ = static_cast<PlayerType>(current->data(Qt::UserRole).toInt());
        fighterName_ = itemName(current);
        currentSnapshot_.localPlayer.username = username_;
        currentSnapshot_.localPlayer.fighterType = static_cast<int>(fighterType_);
        currentSnapshot_.localPlayer.fighterName = fighterName_;
        if (wasReady) {
            currentSnapshot_.localPlayer.ready = false;
        }

        applySnapshot(currentSnapshot_);

        if (sessionManager_) {
            sessionManager_->setLocalProfile(username_, fighterName_, static_cast<int>(fighterType_));
            if (wasReady) {
                sessionManager_->setLocalReady(false);
            }
        }

        emit localFighterChanged(fighterType_, fighterName_);
    });
    fighterLayout->addWidget(fighterList_);
    rightColumn->addWidget(fighterFrame, 3);

    auto* arenaFrame = new QFrame(this);
    arenaFrame->setObjectName(QStringLiteral("lanSectionFrame"));
    arenaFrame->setStyleSheet(sectionFrameStyle());
    arenaFrame->setMinimumHeight(455);
    arenaFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* arenaLayout = new QVBoxLayout(arenaFrame);
    arenaLayout->setContentsMargins(18, 14, 18, 14);
    arenaLayout->setSpacing(8);

    auto* arenaTitle = new QLabel(QStringLiteral("Choose the Battleground"), arenaFrame);
    arenaTitle->setObjectName(QStringLiteral("sectionTitle"));
    arenaLayout->addWidget(arenaTitle);

    arenaOwnerHintLabel_ = new QLabel(arenaFrame);
    arenaOwnerHintLabel_->setObjectName(QStringLiteral("sectionBody"));
    arenaOwnerHintLabel_->setWordWrap(true);
    arenaLayout->addWidget(arenaOwnerHintLabel_);

    arenaPreviewLabel_ = new QLabel(arenaFrame);
    arenaPreviewLabel_->setAlignment(Qt::AlignCenter);
    arenaPreviewLabel_->setFixedSize(380, 136);
    arenaLayout->addWidget(arenaPreviewLabel_, 0, Qt::AlignHCenter);

    arenaNameLabel_ = new QLabel(arenaFrame);
    arenaNameLabel_->setStyleSheet(QStringLiteral("color:#FFF2CF; font:800 18px 'Segoe UI';"));
    arenaNameLabel_->setAlignment(Qt::AlignHCenter);
    arenaLayout->addWidget(arenaNameLabel_);

    arenaDescriptionLabel_ = new QLabel(arenaFrame);
    arenaDescriptionLabel_->setObjectName(QStringLiteral("sectionBody"));
    arenaDescriptionLabel_->setWordWrap(true);
    arenaDescriptionLabel_->setAlignment(Qt::AlignHCenter);
    arenaDescriptionLabel_->hide();
    arenaLayout->addWidget(arenaDescriptionLabel_);

    arenaList_ = new QListWidget(arenaFrame);
    configureBrowserList(arenaList_, QSize(112, 64), QSize(148, 108), 136);
    connect(arenaList_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
        if (!current || syncingUi_) {
            return;
        }

        const bool wasReady = currentSnapshot_.localPlayer.ready;
        selectedArena_ = itemName(current);
        currentSnapshot_.arenaName = selectedArena_;
        if (wasReady) {
            currentSnapshot_.localPlayer.ready = false;
        }

        applySnapshot(currentSnapshot_);

        if (sessionManager_) {
            sessionManager_->setArenaName(selectedArena_);
            if (wasReady) {
                sessionManager_->setLocalReady(false);
            }
        }
    });
    arenaLayout->addWidget(arenaList_);
    leftColumn->addWidget(arenaFrame, 4);

    bodyRow->addLayout(leftColumn, 5);

    auto* waitingFrame = new QFrame(this);
    waitingFrame->setObjectName(QStringLiteral("lanSectionFrame"));
    waitingFrame->setStyleSheet(sectionFrameStyle());
    waitingFrame->setMinimumHeight(390);
    waitingFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* waitingLayout = new QVBoxLayout(waitingFrame);
    waitingLayout->setContentsMargins(22, 18, 22, 18);
    waitingLayout->setSpacing(14);

    auto* waitingTitle = new QLabel(QStringLiteral("Live Waiting Room"), waitingFrame);
    waitingTitle->setObjectName(QStringLiteral("sectionTitle"));

    linkStatusBadgeLabel_ = new QLabel(QStringLiteral("IDLE"), waitingFrame);
    linkStatusBadgeLabel_->setAlignment(Qt::AlignCenter);

    auto* waitingHeaderRow = new QHBoxLayout();
    waitingHeaderRow->setSpacing(12);
    waitingHeaderRow->addWidget(waitingTitle, 1);
    waitingHeaderRow->addWidget(linkStatusBadgeLabel_, 0, Qt::AlignRight | Qt::AlignVCenter);
    waitingLayout->addLayout(waitingHeaderRow);

    auto* linkStatusPanel = new QFrame(waitingFrame);
    linkStatusPanel->setObjectName(QStringLiteral("lanInsetPanel"));
    linkStatusPanel->setStyleSheet(insetPanelStyle());
    auto* linkStatusLayout = new QVBoxLayout(linkStatusPanel);
    linkStatusLayout->setContentsMargins(14, 10, 14, 10);
    linkStatusLayout->setSpacing(5);

    linkStatusTextLabel_ = new QLabel(linkStatusPanel);
    linkStatusTextLabel_->setWordWrap(true);
    linkStatusTextLabel_->setStyleSheet(QStringLiteral("color:#FFF0C6; font:700 13px 'Segoe UI';"));
    linkStatusLayout->addWidget(linkStatusTextLabel_);

    linkStatusMetaLabel_ = new QLabel(linkStatusPanel);
    linkStatusMetaLabel_->setWordWrap(true);
    linkStatusMetaLabel_->setStyleSheet(QStringLiteral("color:rgba(245,230,184,0.68); font:12px 'Segoe UI';"));
    linkStatusLayout->addWidget(linkStatusMetaLabel_);

    waitingLayout->addWidget(linkStatusPanel);

    auto* playerRow = new QHBoxLayout();
    playerRow->setSpacing(12);

    auto makePlayerCard = [&](const QString& eyebrowText,
                              QLabel** portraitLabel,
                              QLabel** nameLabel,
                              QLabel** fighterLabel,
                              QLabel** connectionLabel,
                              QLabel** readyLabel) {
        auto* card = new QFrame(waitingFrame);
        card->setObjectName(QStringLiteral("lanPlayerCard"));
        card->setStyleSheet(previewCardStyle());
        card->setMinimumHeight(205);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 14, 16, 14);
        cardLayout->setSpacing(8);

        auto* eyebrowLabel = new QLabel(eyebrowText, card);
        eyebrowLabel->setObjectName(QStringLiteral("eyebrow"));
        cardLayout->addWidget(eyebrowLabel, 0, Qt::AlignHCenter);

        *portraitLabel = new QLabel(card);
        (*portraitLabel)->setFixedSize(104, 104);
        (*portraitLabel)->setAlignment(Qt::AlignCenter);
        cardLayout->addWidget(*portraitLabel, 0, Qt::AlignHCenter);

        *nameLabel = new QLabel(card);
        (*nameLabel)->setObjectName(QStringLiteral("playerName"));
        (*nameLabel)->setAlignment(Qt::AlignCenter);
        (*nameLabel)->setWordWrap(true);
        cardLayout->addWidget(*nameLabel);

        *fighterLabel = new QLabel(card);
        (*fighterLabel)->setObjectName(QStringLiteral("fighterLine"));
        (*fighterLabel)->setAlignment(Qt::AlignCenter);
        (*fighterLabel)->setWordWrap(true);
        cardLayout->addWidget(*fighterLabel);

        auto* chipRow = new QHBoxLayout();
        chipRow->setSpacing(8);
        chipRow->setContentsMargins(0, 4, 0, 0);

        *connectionLabel = new QLabel(card);
        (*connectionLabel)->setAlignment(Qt::AlignCenter);
        chipRow->addWidget(*connectionLabel, 1);

        *readyLabel = new QLabel(card);
        (*readyLabel)->setAlignment(Qt::AlignCenter);
        chipRow->addWidget(*readyLabel, 1);

        cardLayout->addLayout(chipRow);
        return card;
    };

    playerRow->addWidget(makePlayerCard(QStringLiteral("YOU"),
                                        &localPortraitLabel_,
                                        &localNameLabel_,
                                        &localFighterLabel_,
                                        &localConnectionLabel_,
                                        &localReadyLabel_), 1);
    playerRow->addWidget(makePlayerCard(QStringLiteral("LINKED GLADIATOR"),
                                        &remotePortraitLabel_,
                                        &remoteNameLabel_,
                                        &remoteFighterLabel_,
                                        &remoteConnectionLabel_,
                                        &remoteReadyLabel_), 1);
    waitingLayout->addLayout(playerRow, 1);

    auto* actionFrame = new QFrame(this);
    actionFrame->setObjectName(QStringLiteral("lanSectionFrame"));
    actionFrame->setStyleSheet(sectionFrameStyle());
    actionFrame->setMinimumHeight(142);
    actionFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* actionFrameLayout = new QVBoxLayout(actionFrame);
    actionFrameLayout->setContentsMargins(18, 14, 18, 14);
    actionFrameLayout->setSpacing(10);

    duelHintLabel_ = new QLabel(actionFrame);
    duelHintLabel_->setWordWrap(true);
    duelHintLabel_->setStyleSheet(
        "background: rgba(80,56,34,0.42);"
        "border: 1px solid rgba(212,160,23,0.10);"
        "border-radius: 12px;"
        "padding: 10px 12px;"
        "color: rgba(245,230,184,0.82);"
        "font: 12px 'Segoe UI';");
    actionFrameLayout->addWidget(duelHintLabel_);

    readyButton_ = new QPushButton(QStringLiteral("I'm Ready"), actionFrame);
    readyButton_->setCheckable(true);
    readyButton_->setStyleSheet(secondaryButtonStyle());
    connect(readyButton_, &QPushButton::toggled, this, &LanArenaPage::readyToggled);

    primeButton_ = new QPushButton(QStringLiteral("Start Duel"), actionFrame);
    primeButton_->setStyleSheet(primaryButtonStyle());
    connect(primeButton_, &QPushButton::clicked, this, &LanArenaPage::primeMatch);

    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(12);
    actionRow->addWidget(readyButton_, 1);
    actionRow->addWidget(primeButton_, 1);
    actionFrameLayout->addLayout(actionRow);

    rightColumn->insertWidget(0, waitingFrame, 4);
    rightColumn->addWidget(actionFrame, 1);
    bodyRow->addLayout(rightColumn, 6);
    rootLayout->addLayout(bodyRow, 1);

    rebuildFighterEntries();
    rebuildArenaEntries();
    refreshAddressHints();
    applySnapshot(currentSnapshot_);
}

void LanArenaPage::rebuildFighterEntries() {
    if (!fighterList_) {
        return;
    }

    syncingUi_ = true;
    fighterList_->clear();
    for (PlayerType type : availablePlayerTypes_) {
        const QString label = playerLabel(type);
        auto* item = new QListWidgetItem(QIcon(cachedCircularPortrait(playerProfilePath(type), 84, QColor("#D4A017"))),
                                         label,
                                         fighterList_);
        item->setData(Qt::UserRole, static_cast<int>(type));
        item->setData(Qt::UserRole + 1, label);
        item->setSizeHint(fighterList_->gridSize());
        fighterList_->addItem(item);
        if (type == fighterType_) {
            fighterList_->setCurrentItem(item);
        }
    }
    syncingUi_ = false;
}

void LanArenaPage::rebuildArenaEntries() {
    if (!arenaList_) {
        return;
    }

    syncingUi_ = true;
    arenaList_->clear();
    for (const ArenaTheme& theme : arenaThemes()) {
        auto* item = new QListWidgetItem(QIcon(cachedArenaPreview(arenaImagePath(theme.name), QSize(120, 68))),
                                         theme.name,
                                         arenaList_);
        item->setData(Qt::UserRole + 1, theme.name);
        item->setSizeHint(arenaList_->gridSize());
        arenaList_->addItem(item);
        if (theme.name.compare(selectedArena_, Qt::CaseInsensitive) == 0) {
            arenaList_->setCurrentItem(item);
        }
    }
    syncingUi_ = false;
}

void LanArenaPage::syncFighterSelection() {
    if (!fighterList_) {
        return;
    }

    const QSignalBlocker blocker(fighterList_);
    for (int i = 0; i < fighterList_->count(); ++i) {
        auto* item = fighterList_->item(i);
        if (item && item->data(Qt::UserRole).toInt() == static_cast<int>(fighterType_)) {
            fighterList_->setCurrentRow(i);
            return;
        }
    }
}

void LanArenaPage::syncArenaSelection() {
    if (!arenaList_) {
        return;
    }

    const QSignalBlocker blocker(arenaList_);
    for (int i = 0; i < arenaList_->count(); ++i) {
        auto* item = arenaList_->item(i);
        if (itemName(item).compare(selectedArena_, Qt::CaseInsensitive) == 0) {
            arenaList_->setCurrentRow(i);
            return;
        }
    }
}

void LanArenaPage::refreshAddressHints() {
    if (!addressHintLabel_) {
        return;
    }

    QStringList hints;
    if (sessionManager_) {
        hints = sessionManager_->localAddressHints();
    }
    if (hints.isEmpty()) {
        hints << QStringLiteral("127.0.0.1");
    }

    addressHintLabel_->setText(QStringLiteral("If you host, nearby players can join using: %1. For same-machine practice, use 127.0.0.1.")
                                   .arg(hints.join(QStringLiteral("  |  "))));
}

void LanArenaPage::refreshArenaControlState() {
    if (!arenaList_ || !arenaOwnerHintLabel_) {
        return;
    }

    const bool hostOwnsArena = currentSnapshot_.localRole != LanRole::GUEST;
    const bool allowArenaSelection = currentSnapshot_.localRole == LanRole::NONE || hostOwnsArena;
    arenaList_->setEnabled(allowArenaSelection);

    if (currentSnapshot_.localRole == LanRole::GUEST) {
        arenaOwnerHintLabel_->setText(QStringLiteral("The host chooses the battleground. Your screen updates live when it changes."));
    } else if (currentSnapshot_.remoteConnected) {
        arenaOwnerHintLabel_->setText(QStringLiteral("You control the shared battleground for both players."));
    } else {
        arenaOwnerHintLabel_->setText(QStringLiteral("Pick the battleground now so the room opens with the right mood."));
    }
}

quint16 LanArenaPage::selectedPort() const {
    bool ok = false;
    const int portValue = portEdit_->text().toInt(&ok);
    if (!ok || portValue < 1024 || portValue > 65535) {
        return kDefaultLanPort;
    }
    return static_cast<quint16>(portValue);
}
