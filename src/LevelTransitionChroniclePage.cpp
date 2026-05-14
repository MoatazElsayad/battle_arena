#include "LevelTransitionChroniclePage.h"

#include "ChronicleAiAdvisor.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QRegularExpression>
#include <QTextDocument>
#include <QWheelEvent>
#include <QImage>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Anonymous helpers
// ---------------------------------------------------------------------------
namespace {

QString resolveAssetPath(const QString& relativePath) {
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

qreal easeOutCubic(qreal value) {
    const qreal t = clamp01(value);
    return 1.0 - std::pow(1.0 - t, 3.0);
}

QString highlightEnemyName(QString text, const QString& enemyName, const QString& accentColor) {
    QString safe = text.toHtmlEscaped();
    const QString trimmedEnemyName = enemyName.trimmed();
    if (trimmedEnemyName.isEmpty()) {
        return safe;
    }

    const QRegularExpression enemyRegex(
        QStringLiteral("(%1)").arg(QRegularExpression::escape(trimmedEnemyName.toHtmlEscaped())),
        QRegularExpression::CaseInsensitiveOption);
    safe.replace(enemyRegex,
                 QStringLiteral("<span style=\"color:%1; font-weight:700; font-style:italic;\">\\1</span>")
                     .arg(accentColor));
    return safe;
}

QString styledSummaryBlock(const QString& label,
                           const QString& textHtml,
                           const QString& labelColor,
                           const QString& bodyColor,
                           int labelPx,
                           int bodyPx,
                           bool bodyBold = false,
                           bool bodyItalic = false) {
    return QStringLiteral(
               "<div style=\"margin:0 0 12px 0;\">"
               "<div style=\"color:%1; font-weight:800; font-size:%2px; letter-spacing:1px; margin-bottom:3px;\">%3</div>"
               "<div style=\"color:%4; font-size:%5px; line-height:1.18; font-weight:%6; font-style:%7;\">%8</div>"
               "</div>")
        .arg(labelColor,
             QString::number(labelPx),
             label.toHtmlEscaped(),
             bodyColor,
             QString::number(bodyPx),
             bodyBold ? QStringLiteral("700") : QStringLiteral("500"),
             bodyItalic ? QStringLiteral("italic") : QStringLiteral("normal"),
             textHtml);
}

QString buildAiSummaryHtml(const ChronicleSummary& summary,
                           bool loading,
                           const ChronicleBattleReport& battleReport,
                           int labelPx,
                           int bodyPx) {
    if (loading) {
        return QStringLiteral(
                   "<div style=\"margin:0 0 12px 0;\">"
                   "<div style=\"color:#F2D17A; font-weight:800; font-size:%1px; letter-spacing:1px; margin-bottom:3px;\">READING</div>"
                   "<div style=\"color:#FFF1CC; font-size:%2px; line-height:1.18;\">Reading the battle record and preparing tactical counsel.</div>"
                   "</div>"
                   "<div style=\"margin:0;\">"
                   "<div style=\"color:#79DA87; font-weight:800; font-size:%1px; letter-spacing:1px; margin-bottom:3px;\">STATUS</div>"
                   "<div style=\"color:#DDF6C8; font-size:%2px; line-height:1.18; font-style:italic;\">Live summary incoming...</div>"
                   "</div>")
            .arg(labelPx)
            .arg(bodyPx);
    }

    QString html;
    html += styledSummaryBlock(QStringLiteral("BATTLE"),
                               summary.lastLevelSummary.toHtmlEscaped(),
                               QStringLiteral("#F6C25B"),
                               QStringLiteral("#FFF2D8"),
                               labelPx,
                               bodyPx,
                               true,
                               false);
    html += styledSummaryBlock(QStringLiteral("THREAT"),
                               highlightEnemyName(summary.nextLevelPreview, battleReport.nextEnemyName, QStringLiteral("#FF8A66")),
                               QStringLiteral("#FFD166"),
                               QStringLiteral("#FFD7A3"),
                               labelPx,
                               bodyPx,
                               false,
                               true);
    html += styledSummaryBlock(QStringLiteral("TACTIC"),
                               summary.recommendation.toHtmlEscaped(),
                               QStringLiteral("#79DA87"),
                               QStringLiteral("#DDF6C8"),
                               labelPx,
                               bodyPx,
                               false,
                               false);

    return html;
}

// Returns the tightest bounding rect that contains all opaque pixels.
// Result is cached by the caller — do NOT call this every frame.
QRect opaqueBounds(const QPixmap& pixmap) {
    if (pixmap.isNull()) {
        return QRect();
    }
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int minX = image.width(), minY = image.height();
    int maxX = -1,            maxY = -1;
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

QString playerRunRelativePath(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN:            return "assets/players/Arcen/Sprites/Character/Run.png";
        case PlayerType::DEMON_SLAYER:     return "assets/players/Demon_Slayer/Sprites/Run.png";
        case PlayerType::FANTASY_WARRIOR:  return "assets/players/Fantasy_Warrior/Sprites/Run.png";
        case PlayerType::HUNTRESS:         return "assets/players/Huntress/Sprites/Run.png";
        case PlayerType::KNIGHT:           return "assets/players/Knight/Sprites/RUN.png";
        case PlayerType::MARTIAL:          return "assets/players/Martial/Sprite/Run.png";
        case PlayerType::MARTIAL_HERO:     return "assets/players/Martial_Hero/Sprites/Run.png";
        case PlayerType::MEDIEVAL_WARRIOR: return "assets/players/Medieval_Warrior/Sprites/Run.png";
        case PlayerType::WIZARD:           return "assets/players/Wizard/Sprites/Run.png";
        default:                           return "assets/players/Knight/Sprites/RUN.png";
    }
}

int playerRunFrameCount(PlayerType type) {
    switch (type) {
        case PlayerType::MEDIEVAL_WARRIOR: return 6;
        default:                           return 8;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
LevelTransitionChroniclePage::LevelTransitionChroniclePage(QWidget *parent)
    : QWidget(parent),
      continueButton_(nullptr),
      replayButton_(nullptr),
      aiAdvisor_(new ChronicleAiAdvisor(this)),
      cachedPlayerType_(PlayerType::KNIGHT),
      completedLevel_(1),
      totalLevels_(6),
      playerType_(PlayerType::KNIGHT),
      playerName_(QStringLiteral("Gladiator")),
      battleReport_(),
      aiSummary_(),
      aiSummaryScrollOffset_(0.0),
      aiSummaryLoading_(false),
      continueEmitted_(false)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background:#140D09;");
    setupButtons();

    frameTimer_.setInterval(16); // ~60 fps
    connect(&frameTimer_, &QTimer::timeout,
            this, &LevelTransitionChroniclePage::advanceAnimation);
    connect(aiAdvisor_, &ChronicleAiAdvisor::summaryReady,
            this, &LevelTransitionChroniclePage::handleAiSummaryReady);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void LevelTransitionChroniclePage::configure(int completedLevel, int totalLevels,
                                              PlayerType playerType, const QString &playerName,
                                              const ChronicleBattleReport& report)
{
    totalLevels_    = qMax(1, totalLevels);
    completedLevel_ = qBound(1, completedLevel, totalLevels_);
    playerType_     = playerType;
    playerName_     = playerName.trimmed().isEmpty()
                          ? QStringLiteral("Gladiator")
                          : playerName.trimmed();
    battleReport_ = report;
    battleReport_.completedLevel = completedLevel_;
    battleReport_.totalLevels = totalLevels_;
    battleReport_.playerName = playerName_;
    aiSummary_ = ChronicleSummary();
    aiSummaryScrollOffset_ = 0.0;
    aiSummaryLoading_ = true;

    // Invalidate caches whenever configuration changes
    spriteSheetCache_ = QPixmap();
    backgroundCache_.clear();
    opaqueBoundsCache_.clear();
    cachedPlayerType_ = playerType_;

    if (replayButton_) {
        replayButton_->setText(isZombieChronicle()
                                   ? QString("Replay Outbreak %1").arg(completedLevel_)
                                   : QString("Replay Realm %1").arg(completedLevel_));
    }
}

void LevelTransitionChroniclePage::startChronicle() {
    continueEmitted_ = false;

    // Always call start() — restart() is fine too but start() is clearer
    // for a fresh run. Either way, isValid() becomes true after this call.
    sceneClock_.start();

    if (continueButton_) {
        continueButton_->setEnabled(false);
        continueButton_->setText("Chronicle opening...");
    }
    if (replayButton_) {
        replayButton_->setEnabled(true);
    }

    frameTimer_.start();
    setFocus();
    update();

    if (isZombieChronicle()) {
        aiSummaryLoading_ = false;
    } else if (aiAdvisor_) {
        aiAdvisor_->requestSummary(battleReport_);
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
void LevelTransitionChroniclePage::setupButtons() {
    continueButton_ = new QPushButton("Continue the Journey", this);
    continueButton_->setObjectName("chronicleContinue");
    continueButton_->setCursor(Qt::PointingHandCursor);
    continueButton_->setStyleSheet(
        "QPushButton#chronicleContinue {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "   stop:0 #7A1010, stop:0.55 #D43B24, stop:1 #8A4F0F);"
        " color:#FFF1D0; border:2px solid #E47C60; border-radius:20px;"
        " padding:14px 34px; font:900 18px 'Segoe UI'; letter-spacing:1px;"
        "}"
        "QPushButton#chronicleContinue:disabled { background:#3A2418; color:#9F8754; border-color:#6A3D22; }"
        "QPushButton#chronicleContinue:hover    { border-color:#FFC19D; }");
    connect(continueButton_, &QPushButton::clicked,
            this, &LevelTransitionChroniclePage::handleContinueClicked);

    replayButton_ = new QPushButton("Replay This Realm", this);
    replayButton_->setObjectName("chronicleReplay");
    replayButton_->setCursor(Qt::PointingHandCursor);
    replayButton_->setEnabled(false); // enabled in startChronicle()
    replayButton_->setStyleSheet(
        "QPushButton#chronicleReplay {"
        " background:rgba(53,30,18,0.88); color:#D4AF37;"
        " border:1px solid rgba(212,160,23,0.60); border-radius:15px;"
        " padding:9px 22px; font:800 13px 'Segoe UI';"
        "}"
        "QPushButton#chronicleReplay:hover    { background:rgba(80,45,24,0.95); }"
        "QPushButton#chronicleReplay:disabled { color:#9F8754; border-color:rgba(154,122,58,0.45); }");
    connect(replayButton_, &QPushButton::clicked, this, [this]() {
        emit replayRequested(completedLevel_);
    });
}

QVector<LevelTransitionChroniclePage::RealmInfo> LevelTransitionChroniclePage::realms() const {
    if (isZombieChronicle()) {
        return {
            {1, "Infected Streets", "Four zombies cleared from the first street.", "assets/backgrounds/zombie_lvl1.png", 1},
            {2, "Advanced Outbreak", "Advanced infected still haunt the city.",    "assets/backgrounds/zombie_lvl2.png", 2}
        };
    }

    return {
        {1, "Ember Gate",      "The first flame was driven back.",  "assets/backgrounds/Level_1.png", 1},
        {2, "Ashen Court",     "The court walls still stand.",      "assets/backgrounds/Level_2.png", 1},
        {3, "Demon Rampart",   "The sky fiend was cast down.",      "assets/backgrounds/Level_3.png", 2},
        {4, "Night Veil",      "The shadow path was broken.",       "assets/backgrounds/Level_4.png", 2},
        {5, "Wizard Keep",     "The dark spell was shattered.",     "assets/backgrounds/Level_5.png", 3},
        {6, "Wolf King's Gate","The final beasts guard the crown.", "assets/backgrounds/Level_6.png", 3}
    };
}

// Cached background loader — reads from disk only once per path.
QPixmap LevelTransitionChroniclePage::loadPixmap(const QString &relativePath) const {
    auto it = backgroundCache_.find(relativePath);
    if (it != backgroundCache_.end()) {
        return it.value();
    }
    const QString path = resolveAssetPath(relativePath);
    QPixmap px = path.isEmpty() ? QPixmap() : QPixmap(path);
    backgroundCache_.insert(relativePath, px);
    return px;
}

// Returns the current animation frame for the player sprite.
// The sprite sheet is loaded once and cached; only the crop changes each frame.
QPixmap LevelTransitionChroniclePage::playerPreviewPixmap() const {
    const qreal seconds = sceneClock_.isValid() ? sceneClock_.elapsed() / 1000.0 : 0.0;

    // Reload sheet only when player type changes (configure() clears the cache)
    if (spriteSheetCache_.isNull() || cachedPlayerType_ != playerType_) {
        const QString path = resolveAssetPath(playerRunRelativePath(playerType_));
        spriteSheetCache_ = path.isEmpty() ? QPixmap() : QPixmap(path);
        cachedPlayerType_ = playerType_;
    }

    if (spriteSheetCache_.isNull()) {
        return QPixmap();
    }

    const int frameCount = qMax(1, playerRunFrameCount(playerType_));
    const int frameWidth = qMax(1, spriteSheetCache_.width() / frameCount);
    const int frameIndex = static_cast<int>(seconds / 0.075) % frameCount;
    return spriteSheetCache_.copy(frameIndex * frameWidth, 0, frameWidth, spriteSheetCache_.height());
}

// ---------------------------------------------------------------------------
// Timer slot
// ---------------------------------------------------------------------------
void LevelTransitionChroniclePage::advanceAnimation() {
    const qreal seconds = sceneClock_.isValid() ? sceneClock_.elapsed() / 1000.0 : 0.0;

    if (continueButton_) {
        const bool ready = seconds >= 4.0;
        continueButton_->setEnabled(ready);
        continueButton_->setText(ready ? "Press SPACE to Continue" : "Chronicle opening...");
    }

    update();
}

void LevelTransitionChroniclePage::handleContinueClicked() {
    if (continueEmitted_) return;
    continueEmitted_ = true;
    frameTimer_.stop();
    emit continueRequested();
}

void LevelTransitionChroniclePage::handleAiSummaryReady(const ChronicleSummary& summary) {
    aiSummary_ = summary;
    aiSummaryScrollOffset_ = 0.0;
    aiSummaryLoading_ = false;
    update();
}

// ---------------------------------------------------------------------------
// Event overrides
// ---------------------------------------------------------------------------
void LevelTransitionChroniclePage::keyPressEvent(QKeyEvent *event) {
    const int key = event->key();
    if (key == Qt::Key_Space  || key == Qt::Key_Return ||
        key == Qt::Key_Enter  || key == Qt::Key_A      || key == Qt::Key_X) {
        if (!continueButton_ || continueButton_->isEnabled()) {
            handleContinueClicked();
        }
        return;
    }
    QWidget::keyPressEvent(event);
}

void LevelTransitionChroniclePage::wheelEvent(QWheelEvent *event) {
    if (isZombieChronicle()) {
        QWidget::wheelEvent(event);
        return;
    }

    if (!aiSummaryPanelRect().contains(event->position())) {
        QWidget::wheelEvent(event);
        return;
    }

    const qreal delta = event->angleDelta().y() / 4.0;
    aiSummaryScrollOffset_ = qMax<qreal>(0.0, aiSummaryScrollOffset_ - delta);
    update();
    event->accept();
}

QRectF LevelTransitionChroniclePage::aiSummaryPanelRect() const {
    return QRectF(width() * 0.735, height() * 0.045, width() * 0.225, height() * 0.255);
}

bool LevelTransitionChroniclePage::isZombieChronicle() const {
    return battleReport_.nextEnemyType.compare(QStringLiteral("Zombie Outbreak"), Qt::CaseInsensitive) == 0
        || battleReport_.nextEnemyName.contains(QStringLiteral("zombie"), Qt::CaseInsensitive)
        || totalLevels_ == 2;
}

void LevelTransitionChroniclePage::drawAiSummaryPanel(QPainter &painter, qreal seconds) {
    if (isZombieChronicle()) {
        return;
    }

    const qreal w = width();
    const qreal h = height();
    const QRectF panelRect = aiSummaryPanelRect();
    const qreal glow = 0.5 + 0.5 * std::sin(seconds * 2.2);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath panelPath;
    panelPath.addRoundedRect(panelRect, 18.0, 18.0);

    QRadialGradient halo(panelRect.center(), panelRect.width() * 0.70);
    halo.setColorAt(0.0, QColor(212, 59, 36, static_cast<int>(12 + glow * 10)));
    halo.setColorAt(0.70, QColor(212, 160, 23, 6));
    halo.setColorAt(1.0, QColor(212, 160, 23, 0));
    painter.fillRect(panelRect.adjusted(-14, -14, 14, 14), halo);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(212, 160, 23, 190), 1));
    painter.drawPath(panelPath);

    const QRectF accent(panelRect.left() + 10, panelRect.top() + 12, 5, panelRect.height() - 24);
    QLinearGradient accentGradient(accent.topLeft(), accent.bottomLeft());
    accentGradient.setColorAt(0.0, QColor("#D4AF37"));
    accentGradient.setColorAt(0.55, QColor("#D43B24"));
    accentGradient.setColorAt(1.0, QColor("#7A1010"));
    painter.setPen(Qt::NoPen);
    painter.setBrush(accentGradient);
    painter.drawRoundedRect(accent, 2.5, 2.5);

    painter.setPen(QColor("#FFDFA0"));
    QFont titleFont("Segoe UI", qMax(10, static_cast<int>(h * 0.017)), QFont::Black);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    painter.setFont(titleFont);
    painter.drawText(panelRect.adjusted(25, 12, -14, -panelRect.height() * 0.62),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     "AI BATTLE COUNSEL");

    const QString sourceLabel = aiSummaryLoading_
        ? QStringLiteral("CHECKING")
        : (aiSummary_.fromCache
               ? QStringLiteral("CACHED AI")
               : (aiSummary_.fromFallback ? QStringLiteral("TACTICAL BRIEF") : QStringLiteral("LIVE AI")));
    const QColor sourceColor = aiSummaryLoading_
        ? QColor(120, 86, 28, 125)
        : (aiSummary_.fromCache
               ? QColor(46, 88, 132, 150)
               : (aiSummary_.fromFallback ? QColor(142, 104, 28, 150) : QColor(28, 122, 66, 150)));
    const QRectF sourceRect(panelRect.right() - 138, panelRect.top() + 12, 118, 20);
    painter.setPen(QPen(QColor(255, 230, 166, 125), 1));
    painter.setBrush(sourceColor);
    painter.drawRoundedRect(sourceRect, 10, 10);
    painter.setPen(QColor("#FFF1CC"));
    QFont sourceFont("Segoe UI", qMax(7, static_cast<int>(h * 0.010)), QFont::Black);
    sourceFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    painter.setFont(sourceFont);
    painter.drawText(sourceRect, Qt::AlignCenter, sourceLabel);

    const qreal tagReservedHeight = 42.0;
    const QRectF textViewport = panelRect.adjusted(25, panelRect.height() * 0.27, -18, -(tagReservedHeight + 14.0));
    const int labelPx = qMax(8, static_cast<int>(h * 0.0105));
    const int bodyPx = qMax(9, static_cast<int>(h * 0.0126));

    QTextDocument bodyDocument;
    bodyDocument.setDocumentMargin(0.0);
    bodyDocument.setTextWidth(textViewport.width());
    bodyDocument.setHtml(buildAiSummaryHtml(aiSummary_, aiSummaryLoading_, battleReport_, labelPx, bodyPx));

    const qreal maxScroll = qMax<qreal>(0.0, bodyDocument.size().height() - textViewport.height());
    aiSummaryScrollOffset_ = qBound<qreal>(0.0, aiSummaryScrollOffset_, maxScroll);

    painter.save();
    painter.setClipRect(textViewport);
    painter.translate(textViewport.left(), textViewport.top() - aiSummaryScrollOffset_);
    bodyDocument.drawContents(&painter, QRectF(0.0, 0.0, textViewport.width(), bodyDocument.size().height()));
    painter.restore();

    const QStringList tags = aiSummaryLoading_
        ? QStringList{"ANALYZING", "TACTICS", "NEXT"}
        : aiSummary_.focusTags.mid(0, 3);
    QFont tagFont("Segoe UI", qMax(7, static_cast<int>(h * 0.010)), QFont::Black);
    painter.setFont(tagFont);
    qreal tagX = panelRect.left() + 25;
    const qreal tagY = panelRect.bottom() - 24;
    for (const QString& tag : tags) {
        const QRectF tagRect(tagX, tagY, qMin<qreal>(72.0, tag.size() * 7.0 + 18.0), 18.0);
        painter.setPen(QPen(QColor(212, 160, 23, 120), 1));
        painter.setBrush(QColor(122, 16, 16, 95));
        painter.drawRoundedRect(tagRect, 9.0, 9.0);
        painter.setPen(QColor("#FFE6A6"));
        painter.drawText(tagRect, Qt::AlignCenter, tag);
        tagX += tagRect.width() + 6.0;
    }

    if (maxScroll > 1.0) {
        const QRectF track(panelRect.right() - 10, textViewport.top(), 3, textViewport.height());
        const qreal thumbHeight = qMax<qreal>(18.0, track.height() * textViewport.height() / (bodyDocument.size().height() + 1.0));
        const qreal thumbY = track.top() + (track.height() - thumbHeight) * (aiSummaryScrollOffset_ / maxScroll);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(212, 160, 23, 55));
        painter.drawRoundedRect(track, 1.5, 1.5);
        painter.setBrush(QColor(255, 230, 166, 170));
        painter.drawRoundedRect(QRectF(track.left() - 1, thumbY, 5, thumbHeight), 2.5, 2.5);
    }

    painter.restore();
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void LevelTransitionChroniclePage::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing,        true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal seconds = sceneClock_.isValid() ? sceneClock_.elapsed() / 1000.0 : 0.0;
    const qreal w       = width();
    const qreal h       = height();
    const qreal intro   = easeOutCubic(seconds / 1.25);
    const bool zombieChronicle = isZombieChronicle();

    // ── Background gradient ──────────────────────────────────────────────────
    QLinearGradient bg(0, 0, w, h);
    bg.setColorAt(0.00, zombieChronicle ? QColor("#07140E") : QColor("#120B08"));
    bg.setColorAt(0.32, zombieChronicle ? QColor("#10291A") : QColor("#2A1810"));
    bg.setColorAt(0.70, zombieChronicle ? QColor("#173E27") : QColor("#4C2F1B"));
    bg.setColorAt(1.00, zombieChronicle ? QColor("#07100B") : QColor("#160C08"));
    painter.fillRect(rect(), bg);

    // ── Floating ember particles ─────────────────────────────────────────────
    painter.save();
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < 54; ++i) {
        const qreal seed   = i * 31.0;
        const qreal x      = std::fmod(seed * 17.0 + seconds * (10.0 + (i % 5) * 3.5), w + 80.0) - 40.0;
        const qreal y      = std::fmod(seed * 9.0  + std::sin(seconds * 0.7 + i) * 24.0, h);
        const qreal radius = 1.8 + (i % 4);
        painter.setBrush(zombieChronicle ? QColor(70, 220, 132, 46 + (i % 5) * 12)
                                         : QColor(212, 160, 23, 58 + (i % 5) * 14));
        painter.drawEllipse(QPointF(x, y), radius, radius);
    }
    painter.restore();

    // ── Title ────────────────────────────────────────────────────────────────
    QFont titleFont("Showcard Gothic", qMax(28, static_cast<int>(h * 0.052)));
    if (titleFont.family() != "Showcard Gothic") {
        titleFont = painter.font();
        titleFont.setPointSize(qMax(28, static_cast<int>(h * 0.052)));
        titleFont.setBold(true);
    }
    painter.setFont(titleFont);
    const QString pageTitle = zombieChronicle
        ? QStringLiteral("OUTBREAK CHRONICLE")
        : QStringLiteral("THE KINGDOM CHRONICLE");
    painter.setPen(zombieChronicle ? QColor(6, 31, 17, 220) : QColor(70, 16, 12, 210));
    const QRectF titleRect(w * 0.045, h * 0.035, w * 0.64, h * 0.08);
    painter.drawText(titleRect.translated(3, 3), Qt::AlignLeft | Qt::AlignVCenter, pageTitle);
    painter.setPen(zombieChronicle ? QColor("#B7FFD1") : QColor("#FFD700"));
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, pageTitle);

    // ── Progress bar ─────────────────────────────────────────────────────────
    const qreal safety = 100.0 * completedLevel_ / qMax(1, totalLevels_);
    const QRectF progressShell(w * 0.052, h * 0.125, w * 0.48, 24);
    painter.setPen(QPen(zombieChronicle ? QColor("#46D77F") : QColor("#D4AF37"), 2));
    painter.setBrush(zombieChronicle ? QColor(8, 28, 18, 230) : QColor(34, 20, 13, 230));
    painter.drawRoundedRect(progressShell, 12, 12);

    QRectF progressFill = progressShell.adjusted(4, 4, -4, -4);
    progressFill.setWidth(progressFill.width() * safety / 100.0);
    if (progressFill.width() > 0) {
        QLinearGradient progressGradient(progressFill.topLeft(), progressFill.topRight());
        progressGradient.setColorAt(0.00, zombieChronicle ? QColor("#125C32") : QColor("#7A1010"));
        progressGradient.setColorAt(0.55, zombieChronicle ? QColor("#22C55E") : QColor("#D43B24"));
        progressGradient.setColorAt(1.00, zombieChronicle ? QColor("#B7FFD1") : QColor("#D4AF37"));
        painter.setPen(Qt::NoPen);
        painter.setBrush(progressGradient);
        painter.drawRoundedRect(progressFill, 8, 8);
    }

    painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
    painter.setPen(zombieChronicle ? QColor("#DDFCE7") : QColor("#F3D38C"));
    painter.drawText(progressShell, Qt::AlignCenter,
                     zombieChronicle
                         ? QString("CITY CLEANUP: %1%     Outbreaks Cleared: %2 / %3")
                               .arg(static_cast<int>(std::round(safety)))
                               .arg(completedLevel_)
                               .arg(totalLevels_)
                         : QString("THE KING'S SAFETY: %1%     Realms Saved: %2 / %3")
                         .arg(static_cast<int>(std::round(safety)))
                         .arg(completedLevel_)
                         .arg(totalLevels_));

    if (!zombieChronicle) {
        drawAiSummaryPanel(painter, seconds);
    }

    // ── Scroll / main panel (animates in) ────────────────────────────────────
    const QRectF scrollRect(w * 0.055, h * 0.22, w * 0.89 * intro, h * 0.54);
    QPainterPath scrollPath;
    scrollPath.addRoundedRect(scrollRect, 32, 32);

    // ── Button layout — always position them so Qt doesn't flash them at (0,0)
    const int buttonW = 300;
    const int buttonH = 56;
    if (continueButton_) {
        continueButton_->setGeometry((width() - buttonW) / 2,
                                     static_cast<int>(h * 0.895), buttonW, buttonH);
    }
    if (replayButton_) {
        replayButton_->setGeometry(static_cast<int>(w * 0.055),
                                   static_cast<int>(h * 0.895) + 6, 210, 42);
    }

    // Wait for the panel reveal to finish before drawing interior content
    if (intro < 0.98) {
        return;
    }

    // ── Path line ────────────────────────────────────────────────────────────
    painter.setClipPath(scrollPath);

    const QVector<RealmInfo> realmList = realms();
    const qreal pathY  = scrollRect.center().y() - h * 0.015;
    const qreal startX = scrollRect.left()  + scrollRect.width() * 0.08;
    const qreal endX   = scrollRect.right() - scrollRect.width() * 0.08;

    painter.setPen(QPen(zombieChronicle ? QColor("#0F5A32") : QColor("#7A1010"), 13, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(startX, pathY), QPointF(endX, pathY));
    painter.setPen(QPen(zombieChronicle ? QColor("#46D77F") : QColor("#D4AF37"),  5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(startX, pathY), QPointF(endX, pathY));

    // ── Portal positions ─────────────────────────────────────────────────────
    const qreal portalRadius = qMin(w * 0.055, h * 0.084);
    QVector<QPointF> portalCenters;
    portalCenters.reserve(realmList.size());
    for (int i = 0; i < realmList.size(); ++i) {
        const qreal t = realmList.size() == 1
                            ? 0.0
                            : static_cast<qreal>(i) / (realmList.size() - 1);
        portalCenters.append(QPointF(startX + (endX - startX) * t, pathY));
    }

    // ── Portals ──────────────────────────────────────────────────────────────
    for (int i = 0; i < realmList.size(); ++i) {
        const RealmInfo &realm = realmList[i];
        const QPointF    center = portalCenters[i];
        const bool completed    = realm.level <  completedLevel_;
        const bool justCompleted= realm.level == completedLevel_;
        const bool locked       = realm.level >  completedLevel_;
        const qreal pulse       = justCompleted ? (1.0 + 0.08 * std::sin(seconds * 5.5)) : 1.0;
        const qreal radius      = portalRadius * pulse;
        const QRectF portalRect(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0);

        // Clipped background image
        QPainterPath clip;
        clip.addEllipse(portalRect);
        painter.save();
        painter.setClipPath(clip, Qt::IntersectClip);
        QPixmap bgPx = loadPixmap(realm.backgroundPath); // cached
        if (!bgPx.isNull()) {
            QRectF imageRect = portalRect.adjusted(-radius * 0.25, -radius * 0.25, radius * 0.25, radius * 0.25);
            imageRect.translate(std::sin(seconds * 0.5 + i) * 5.0,
                                std::cos(seconds * 0.4 + i) * 3.0);
            painter.drawPixmap(imageRect.toRect(), bgPx);
        } else {
            painter.fillRect(portalRect, QColor("#3C2A1A"));
        }
        if (locked) {
            painter.fillRect(portalRect, QColor(26, 15, 10, qMin(220, 130 + i * 16)));
        }
        painter.restore();

        // Portal ring
        QColor frameColor = locked
            ? QColor("#7C6B58")
            : (justCompleted
                   ? (zombieChronicle ? QColor("#22C55E") : QColor("#D43B24"))
                   : (zombieChronicle ? QColor("#46D77F") : QColor("#D4AF37")));
        painter.setPen(QPen(frameColor, justCompleted ? 7 : 5));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(portalRect);

        // Glow aura on just-completed portal
        if (justCompleted) {
            QRadialGradient aura(center, radius * 1.7);
            aura.setColorAt(0.0,  zombieChronicle ? QColor(34, 197, 94, 105) : QColor(212, 59,  36,  95));
            aura.setColorAt(0.55, zombieChronicle ? QColor(183, 255, 209, 55) : QColor(212, 175, 55,  55));
            aura.setColorAt(1.0,  zombieChronicle ? QColor(34, 197, 94, 0) : QColor(212, 59,  36,   0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(aura);
            painter.drawEllipse(center, radius * 1.7, radius * 1.7);
        }

        // Labels below portal
        painter.setFont(QFont("Segoe UI", 8, QFont::Bold));
        painter.setPen(QColor("#F3D38C"));
        const QRectF textRect(center.x() - radius * 1.3, center.y() + radius + 8, radius * 2.6, 54);
        painter.drawText(textRect, Qt::AlignHCenter | Qt::TextWordWrap,
                         QString("%1 %2 - %3\n%4")
                             .arg(zombieChronicle ? QStringLiteral("Outbreak") : QStringLiteral("Level"))
                             .arg(realm.level)
                             .arg(realm.name)
                             .arg(locked
                                      ? (zombieChronicle ? QStringLiteral("Next infected wave") : QStringLiteral("Next Realm"))
                                      : realm.blurb));

        // Overlay text inside portal
        if (completed) {
            painter.setFont(QFont("Segoe UI", 12, QFont::Black));
            painter.setPen(zombieChronicle ? QColor("#B7FFD1") : QColor("#D4AF37"));
            painter.drawText(portalRect, Qt::AlignCenter, zombieChronicle ? "STREET\nCLEAR" : "KING\nSAVED");
        } else if (justCompleted) {
            painter.setFont(QFont("Segoe UI", 11, QFont::Black));
            painter.setPen(QColor("#FFF1D0"));
            painter.drawText(portalRect.adjusted(0, radius * 0.35, 0, 0),
                             Qt::AlignCenter,
                             zombieChronicle ? "WAVE\nCLEARED!" : "JUST\nSAVED!");
        } else {
            painter.setFont(QFont("Segoe UI", 20, QFont::Black));
            painter.setPen(QColor("#E3E7F0"));
            painter.drawText(portalRect, Qt::AlignCenter, zombieChronicle ? "INFESTED" : "LOCKED");
            painter.setFont(QFont("Segoe UI", 9, QFont::Bold));
            painter.drawText(portalRect.adjusted(0, radius * 0.54, 0, 0), Qt::AlignCenter,
                             QString(realm.difficulty, QChar('*')));
        }
    }

    // ── Player character walking along the path ──────────────────────────────
    //
    // FIX: playerEndIndex must always differ from playerStartIndex so the
    // character actually walks.  When completedLevel_ == totalLevels_ the
    // player has just finished the last realm and should walk ONTO that last
    // portal (index == completedLevel_ - 1), arriving from the second-to-last
    // one.  In all other cases the character walks forward to the next portal.
    //
    const int playerStartIndex = qBound(0, completedLevel_ - 1, portalCenters.size() - 1);

    int playerEndIndex;
    if (completedLevel_ >= totalLevels_) {
        // Last level done: walk FROM second-to-last TO last portal.
        // Use max(0, ...) so a single-level game doesn't underflow.
        playerEndIndex = playerStartIndex;
        const int fromIndex = qMax(0, playerStartIndex - 1);
        // Reverse: start at fromIndex, end at playerStartIndex
        const qreal walkT   = easeOutCubic(clamp01((seconds - 0.8) / 6.6));
        const QPointF knightPos = portalCenters[fromIndex]
                                  + (portalCenters[playerStartIndex] - portalCenters[fromIndex]) * walkT;
        // Draw player at knightPos (we handle drawing inline here and skip the
        // general case below by jumping to the narrative text).
        QPixmap playerPx = playerPreviewPixmap();
        if (!playerPx.isNull()) {
            // Speed trail
            QLinearGradient trail(knightPos.x() - portalRadius * 0.8,  pathY,
                                  knightPos.x() + portalRadius * 0.35, pathY);
            trail.setColorAt(0.0,  zombieChronicle ? QColor(34, 197, 94, 0) : QColor(212, 59,  36,   0));
            trail.setColorAt(0.45, zombieChronicle ? QColor(34, 197, 94, 100) : QColor(212, 59,  36, 105));
            trail.setColorAt(1.0,  zombieChronicle ? QColor(183, 255, 209, 0) : QColor(212, 175, 55,   0));
            painter.setPen(QPen(QBrush(trail), 5, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(knightPos.x() - portalRadius * 0.85, pathY - 2),
                             QPointF(knightPos.x() + portalRadius * 0.15, pathY - 2));

            // Shadow
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 90));
            painter.drawEllipse(QPointF(knightPos.x(), pathY + portalRadius * 0.54),
                                portalRadius * 0.38, portalRadius * 0.09);

            // Sprite — use cached opaqueBounds
            const qreal previewH = portalRadius * 1.65;
            const QPixmap scaled = playerPx.scaledToHeight(static_cast<int>(previewH),
                                                            Qt::FastTransformation);
            const QString boundsKey = QString::number(scaled.cacheKey());
            if (!opaqueBoundsCache_.contains(boundsKey)) {
                opaqueBoundsCache_.insert(boundsKey, opaqueBounds(scaled));
            }
            const QRect visible = opaqueBoundsCache_.value(boundsKey);
            const QPointF topLeft(knightPos.x() - visible.center().x(),
                                  pathY + portalRadius * 0.50 - visible.bottom());
            painter.drawPixmap(topLeft, scaled);
        }
    } else {
        // Normal case: walk from completed portal toward the next one
        playerEndIndex = qBound(0, completedLevel_, portalCenters.size() - 1);
        const qreal walkT   = easeOutCubic(clamp01((seconds - 0.8) / 6.6));
        const QPointF knightPos = portalCenters[playerStartIndex]
                                  + (portalCenters[playerEndIndex] - portalCenters[playerStartIndex]) * walkT;

        QPixmap playerPx = playerPreviewPixmap();
        if (!playerPx.isNull()) {
            // Speed trail
            QLinearGradient trail(knightPos.x() - portalRadius * 0.8,  pathY,
                                  knightPos.x() + portalRadius * 0.35, pathY);
            trail.setColorAt(0.0,  zombieChronicle ? QColor(34, 197, 94, 0) : QColor(212, 59,  36,   0));
            trail.setColorAt(0.45, zombieChronicle ? QColor(34, 197, 94, 100) : QColor(212, 59,  36, 105));
            trail.setColorAt(1.0,  zombieChronicle ? QColor(183, 255, 209, 0) : QColor(212, 175, 55,   0));
            painter.setPen(QPen(QBrush(trail), 5, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(knightPos.x() - portalRadius * 0.85, pathY - 2),
                             QPointF(knightPos.x() + portalRadius * 0.15, pathY - 2));

            // Shadow
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 90));
            painter.drawEllipse(QPointF(knightPos.x(), pathY + portalRadius * 0.54),
                                portalRadius * 0.38, portalRadius * 0.09);

            // Sprite — use cached opaqueBounds
            const qreal previewH = portalRadius * 1.65;
            const QPixmap scaled = playerPx.scaledToHeight(static_cast<int>(previewH),
                                                            Qt::FastTransformation);
            const QString boundsKey = QString::number(scaled.cacheKey());
            if (!opaqueBoundsCache_.contains(boundsKey)) {
                opaqueBoundsCache_.insert(boundsKey, opaqueBounds(scaled));
            }
            const QRect visible = opaqueBoundsCache_.value(boundsKey);
            const QPointF topLeft(knightPos.x() - visible.center().x(),
                                  pathY + portalRadius * 0.50 - visible.bottom());
            painter.drawPixmap(topLeft, scaled);
        }
    }

    // ── Narrative text ───────────────────────────────────────────────────────
    painter.setClipping(false);

    painter.setFont(QFont("Georgia", 13, QFont::DemiBold));
    painter.setPen(QColor("#F3D38C"));
    painter.drawText(QRectF(w * 0.18, h * 0.79, w * 0.64, 42), Qt::AlignCenter,
                     zombieChronicle
                         ? "The first street is cleared. Advanced infected are moving deeper inside the city."
                         : "Another realm saved... The King's light grows stronger. Yet greater trials await...");
}
