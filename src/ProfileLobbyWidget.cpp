#include "ProfileLobbyWidget.h"

#include <QAction>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QRegularExpression>
#include <QScrollArea>
#include <QCoreApplication>
#include <QStyle>
#include <QStyleOption>
#include <QToolButton>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace {

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
    grad.setColorAt(0.0, QColor(212, 160, 23, 115));
    grad.setColorAt(0.55, QColor(212, 160, 23, 36));
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
    grad.setColorAt(0.0, QColor(46, 33, 24));
    grad.setColorAt(0.6, QColor(32, 24, 19));
    grad.setColorAt(1.0, QColor(24, 18, 14));
    p.fillRect(bg.rect(), grad);

    // Faint arena-fog bands.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(180, 140, 90, 18));
    p.drawEllipse(QRectF(size.width() * 0.15, size.height() * 0.62, size.width() * 0.70, size.height() * 0.28));
    p.setBrush(QColor(210, 170, 110, 10));
    p.drawEllipse(QRectF(size.width() * 0.05, size.height() * 0.70, size.width() * 0.90, size.height() * 0.22));

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

QPixmap trimTransparent(const QPixmap& source) {
    if (source.isNull()) {
        return source;
    }

    const QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32);
    int minX = image.width();
    int minY = image.height();
    int maxX = -1;
    int maxY = -1;

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 8) {
                minX = qMin(minX, x);
                minY = qMin(minY, y);
                maxX = qMax(maxX, x);
                maxY = qMax(maxY, y);
            }
        }
    }

    if (maxX < minX || maxY < minY) {
        return source;
    }

    QRect bounds(minX, minY, maxX - minX + 1, maxY - minY + 1);
    bounds = bounds.adjusted(-1, -1, 1, 1).intersected(image.rect());
    return QPixmap::fromImage(image.copy(bounds));
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
      previewTitleLabel_(nullptr),
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
      hoverGlowAnimation_(nullptr),
    enterArenaGlowEffect_(nullptr),
    idleAnimationTimer_(new QTimer(this)),
    idleFrameIndex_(0) {
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("profileLobbyRoot");
        setCursor(Qt::ArrowCursor);

    userProfile_ = {"Player_01", 0, "Rookie", QString()};
    selectedCharacter_ = {"Demon Slayer", QString(), "Slash Combo"};
    selectedModeName_ = "1v1 Fight (Duel)";

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
    rootLayout->setContentsMargins(18, 14, 18, 14);
    rootLayout->setSpacing(14);

    setupHeader(rootLayout);
    setupCharacterPreview(rootLayout);
    setupModeCarousel(rootLayout);
    setupBottomBar(rootLayout);

    // Balanced responsive composition
    rootLayout->setStretch(0, 0);
    rootLayout->setStretch(1, 6);
    rootLayout->setStretch(2, 3);
    rootLayout->setStretch(3, 1);

    setLayout(rootLayout);
}

void ProfileLobbyWidget::setupHeader(QVBoxLayout* rootLayout) {
    auto* header = new QFrame(this);
    header->setObjectName("lobbyHeader");
    header->setFixedHeight(82);
    header->setStyleSheet(
        "QFrame#lobbyHeader {"
        " background: rgba(26,20,15,0.84);"
        " border: 1px solid rgba(212,160,23,0.35);"
        " border-radius: 12px;"
        "}"
    );

    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 12, 20, 12);
    headerLayout->setSpacing(16);

    logoLabel_ = new QLabel("Battle Arena", header);
    logoLabel_->setStyleSheet("color:#D4A017; font: 800 28px 'Segoe UI'; letter-spacing: 1px;");
    logoLabel_->setMinimumWidth(220);
    headerLayout->addWidget(logoLabel_, 0, Qt::AlignVCenter | Qt::AlignLeft);

    auto* centerWrap = new QWidget(header);
    auto* centerLayout = new QHBoxLayout(centerWrap);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(14);

    usernameLabel_ = new QLabel(centerWrap);
    usernameLabel_->setCursor(Qt::PointingHandCursor);
    usernameLabel_->setStyleSheet(
        "color:#F5E6B8; font: 700 16px 'Segoe UI';"
        "padding:4px 8px; border-radius:8px; background:rgba(212,160,23,0.08);"
    );
    usernameLabel_->installEventFilter(this);
    centerLayout->addWidget(usernameLabel_);

    auto* scoreIcon = new QLabel(centerWrap);
    scoreIcon->setText(QString::fromUtf8("◉"));
    scoreIcon->setStyleSheet("color:#D4A017; font: 800 15px 'Segoe UI';");
    centerLayout->addWidget(scoreIcon);

    scoreLabel_ = new QLabel(centerWrap);
    scoreLabel_->setStyleSheet("color:#E9D59C; font:700 14px 'Segoe UI';");
    centerLayout->addWidget(scoreLabel_);

    badgeLabel_ = new QLabel(centerWrap);
    badgeLabel_->setStyleSheet(
        "color:#F5E6B8; font:700 13px 'Segoe UI';"
        "padding:4px 10px; border-radius:10px; background:rgba(212,160,23,0.20);"
    );
    centerLayout->addWidget(badgeLabel_);

    headerLayout->addWidget(centerWrap, 1, Qt::AlignCenter);

    auto* rightWrap = new QWidget(header);
    auto* rightLayout = new QHBoxLayout(rightWrap);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    avatarLabel_ = new QLabel(rightWrap);
    avatarLabel_->setFixedSize(40, 40);
    avatarLabel_->setAlignment(Qt::AlignCenter);
    avatarLabel_->setStyleSheet(
        "background:#3B2A1D; border:2px solid #D4A017; border-radius:20px;"
    );
    rightLayout->addWidget(avatarLabel_);

    settingsButton_ = new QToolButton(rightWrap);
    settingsButton_->setText(QString::fromUtf8("⚙"));
    settingsButton_->setCursor(Qt::PointingHandCursor);
    settingsButton_->setFixedSize(34, 34);
    settingsButton_->setStyleSheet(
        "QToolButton {"
        " color:#F0DFAB; font:700 16px 'Segoe UI';"
        " border:1px solid #7C5A24; border-radius:17px; background:rgba(212,160,23,0.10);"
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

void ProfileLobbyWidget::setupCharacterPreview(QVBoxLayout* rootLayout) {
    auto* previewFrame = new QFrame(this);
    previewFrame->setObjectName("characterPreviewFrame");
    previewFrame->setStyleSheet(
        "QFrame#characterPreviewFrame {"
        " background: rgba(26,20,15,0.62);"
        " border:1px solid rgba(212,160,23,0.22);"
        " border-radius:18px;"
        "}"
    );

    auto* previewLayout = new QVBoxLayout(previewFrame);
    previewLayout->setContentsMargins(16, 12, 16, 12);
    previewLayout->setSpacing(10);

    previewTitleLabel_ = new QLabel("Your Fighter", previewFrame);
    previewTitleLabel_->setStyleSheet("color:#EFDDAA; font: 700 18px 'Segoe UI';");
    previewTitleLabel_->setAlignment(Qt::AlignCenter);
    previewLayout->addWidget(previewTitleLabel_);

    characterView_ = new QGraphicsView(previewFrame);
    characterView_->setFrameShape(QFrame::NoFrame);
    characterView_->setRenderHint(QPainter::Antialiasing, true);
    characterView_->setRenderHint(QPainter::SmoothPixmapTransform, true);
    characterView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    characterView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    characterView_->setStyleSheet("background: transparent;");

    // Mouse cursor visibility fix on dark scene.
    characterView_->setCursor(Qt::ArrowCursor);
    characterView_->viewport()->setCursor(Qt::ArrowCursor);

    previewScene_ = new QGraphicsScene(characterView_);
    previewScene_->setSceneRect(0, 0, 1600, 980);
    previewScene_->setBackgroundBrush(QColor(40, 30, 25));
    characterView_->setScene(previewScene_);

    sceneBackgroundItem_ = previewScene_->addPixmap(createCinematicBackdrop(QSize(1600, 980)));
    sceneBackgroundItem_->setZValue(-10.0);

    glowItem_ = previewScene_->addPixmap(createGoldGlowPixmap(760));
    glowItem_->setOpacity(0.52);
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

    // TODO: Replace QGraphicsView with QOpenGLWidget/Qt3D when real 3D fighter assets are integrated.
    // TODO: Add idle animation loop once sprite sheet or skeletal animation data is available.
    // TODO: Load character assets through async cache + streaming manager.

    previewLayout->addWidget(characterView_, 1);
    rootLayout->addWidget(previewFrame, 1);
}

void ProfileLobbyWidget::setupModeCarousel(QVBoxLayout* rootLayout) {
    auto* sectionTitle = new QLabel("Select Game Mode", this);
    sectionTitle->setStyleSheet("color:#EFDDAA; font:700 16px 'Segoe UI'; margin-left:4px;");
    rootLayout->addWidget(sectionTitle);

    modeScrollArea_ = new QScrollArea(this);
    modeScrollArea_->setWidgetResizable(true);
    modeScrollArea_->setFrameShape(QFrame::NoFrame);
    modeScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    modeScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    modeScrollArea_->setStyleSheet(
        "QScrollArea { background: #1A140F; border: 1px solid rgba(212,160,23,0.18); border-radius: 12px; }"
        "QScrollArea > QWidget > QWidget { background: #1A140F; }"
    );
    modeScrollArea_->viewport()->setStyleSheet("background: #1A140F; border: none;");
    modeScrollArea_->setMinimumHeight(190);

    modeContainer_ = new QWidget(modeScrollArea_);
    modeContainer_->setAttribute(Qt::WA_StyledBackground, true);
    modeContainer_->setStyleSheet("background: #1A140F;");
    auto* flow = new FlowLayout(modeContainer_, 2, 14, 14);
    modeLayout_ = flow;

    const QList<GameMode> modes = {
        {"1v1 Fight (Duel)", QString(), "Fast tactical duel against one opponent.", true},
        {"Save the Kings", QString(), "Protect allied rulers across dynamic waves.", false},
        {"Team Deathmatch (4v4)", QString(), "Coordinated squad combat with respawns.", false},
        {"Free-for-All Arena", QString(), "Every fighter for themselves in one arena.", false},
        {"Custom Battle", QString(), "Create private rules and invite your crew.", false}
    };

    for (const GameMode& mode : modes) {
        QWidget* card = createModeCard(mode);
        flow->addWidget(card);
        modeCards_.insert(mode.name, card);
    }

    modeContainer_->setLayout(flow);
    modeScrollArea_->setWidget(modeContainer_);
    rootLayout->addWidget(modeScrollArea_);
}

void ProfileLobbyWidget::setupBottomBar(QVBoxLayout* rootLayout) {
    auto* bottomWrap = new QFrame(this);
    bottomWrap->setStyleSheet("QFrame { background: transparent; }");
    auto* bottomLayout = new QHBoxLayout(bottomWrap);
    bottomLayout->setContentsMargins(2, 0, 2, 0);
    bottomLayout->setSpacing(12);

    changeCharacterButton_ = new QPushButton("Change Character", bottomWrap);
    changeCharacterButton_->setCursor(Qt::PointingHandCursor);
    changeCharacterButton_->setMinimumHeight(52);
    changeCharacterButton_->setStyleSheet(
        "QPushButton {"
        " background: rgba(58,42,27,0.95);"
        " border:1px solid #7B5A2B;"
        " border-radius:12px;"
        " color:#EEDCB0;"
        " font:700 15px 'Segoe UI';"
        " padding:10px 18px;"
        "}"
        "QPushButton:hover { background: rgba(78,55,32,0.95); }"
    );
    connect(changeCharacterButton_, &QPushButton::clicked, this, &ProfileLobbyWidget::changeCharacterClicked);

    enterArenaButton_ = new QPushButton("ENTER ARENA", bottomWrap);
    enterArenaButton_->setCursor(Qt::PointingHandCursor);
    enterArenaButton_->setMinimumHeight(64);
    enterArenaButton_->setMinimumWidth(320);
    enterArenaButton_->setStyleSheet(
        "QPushButton {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #6E0D0D, stop:1 #BC1A1A);"
        " border:2px solid #D14C4C;"
        " border-radius:14px;"
        " color:#FFE6E6;"
        " font:800 21px 'Segoe UI';"
        " letter-spacing: 1px;"
        " padding:14px 34px;"
        "}"
        "QPushButton:hover {"
        " border:2px solid #FF8C8C;"
        "}"
        "QPushButton:pressed {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #4F0A0A, stop:1 #8B1212);"
        "}"
    );

    enterArenaGlowEffect_ = new QGraphicsDropShadowEffect(this);
    enterArenaGlowEffect_->setColor(QColor(255, 45, 45, 210));
    enterArenaGlowEffect_->setOffset(0, 0);
    enterArenaGlowEffect_->setBlurRadius(24);
    enterArenaButton_->setGraphicsEffect(enterArenaGlowEffect_);

    hoverGlowAnimation_ = new QVariantAnimation(this);
    hoverGlowAnimation_->setStartValue(22.0);
    hoverGlowAnimation_->setEndValue(58.0);
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
        emit enterArenaClicked(selectedModeName_);
        // TODO: Trigger matchmaking/network queue start with selected mode and user party data.
    });

    bottomLayout->addWidget(changeCharacterButton_, 0, Qt::AlignLeft);
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(enterArenaButton_, 0, Qt::AlignRight);
    rootLayout->addWidget(bottomWrap);
}

QWidget* ProfileLobbyWidget::createModeCard(const GameMode& mode) {
    auto* card = new QFrame(modeContainer_);
    card->setObjectName("modeCard");
    card->setProperty("modeName", mode.name);
    card->setProperty("selected", false);
    card->setCursor(Qt::PointingHandCursor);
    card->setFixedSize(252, 140);

    QString accent = "#7A7A7A";
    if (mode.name.contains("1v1", Qt::CaseInsensitive)) {
        accent = "#B52A2A";
    } else if (mode.name.contains("Kings", Qt::CaseInsensitive)) {
        accent = "#2A5DA8";
    } else if (mode.name.contains("Deathmatch", Qt::CaseInsensitive)) {
        accent = "#2F8A44";
    } else if (mode.name.contains("Free-for-All", Qt::CaseInsensitive)) {
        accent = "#7A3AA8";
    }

    card->setStyleSheet(QString(
        "QFrame#modeCard {"
        " background: rgba(212,160,23,0.09);"
        " border:1px solid rgba(212,160,23,0.24);"
        " border-left:4px solid %1;"
        " border-radius:12px;"
        "}"
        "QFrame#modeCard[selected='true'] {"
        " border:2px solid #D4A017;"
        " background: rgba(212,160,23,0.18);"
        " border-left:4px solid %1;"
        "}"
    ).arg(accent));

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(5);

    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    QLabel* icon = new QLabel(card);
    icon->setFixedSize(28, 28);
    icon->setAlignment(Qt::AlignCenter);
    bool hasIconPixmap = false;
    if (!mode.iconPath.isEmpty()) {
        QPixmap iconPix(mode.iconPath);
        if (!iconPix.isNull()) {
            icon->setPixmap(iconPix.scaled(22, 22, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            icon->setStyleSheet("background: rgba(212,160,23,0.14); border-radius: 14px;");
            hasIconPixmap = true;
        }
    }
    if (!hasIconPixmap) {
        icon->setText(mode.name.left(1));
        icon->setStyleSheet(QString("background:%1; border-radius:14px; color:white; font:700 13px 'Segoe UI';").arg(accent));
    }
    topRow->addWidget(icon, 0, Qt::AlignTop);

    QLabel* title = new QLabel(mode.name, card);
    title->setWordWrap(true);
    title->setStyleSheet("color:#F0DEB2; font:700 13px 'Segoe UI';");
    topRow->addWidget(title, 1);
    cardLayout->addLayout(topRow);

    QLabel* desc = new QLabel(mode.description, card);
    desc->setWordWrap(true);
    desc->setStyleSheet("color:#CBB68A; font:11px 'Segoe UI';");
    cardLayout->addWidget(desc);

    if (mode.recommended) {
        QLabel* badge = new QLabel("Recommended", card);
        badge->setStyleSheet(
            "background: rgba(212,160,23,0.24);"
            "border: 1px solid #D4A017;"
            "border-radius: 8px;"
            "padding: 3px 8px;"
            "color: #FFE6A6;"
            "font: 700 10px 'Segoe UI';"
        );
        badge->setMaximumWidth(108);
        cardLayout->addWidget(badge, 0, Qt::AlignLeft);
    } else {
        cardLayout->addSpacing(18);
    }

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

void ProfileLobbyWidget::refreshProfileUi() {
    if (usernameLabel_) {
        usernameLabel_->setText(userProfile_.username);
    }
    if (scoreLabel_) {
        scoreLabel_->setText(QString::number(userProfile_.score));
    }
    if (badgeLabel_) {
        const QString color = badgeColorFor(userProfile_.badge);
        badgeLabel_->setText(QString::fromUtf8("● ") + userProfile_.badge);
        badgeLabel_->setStyleSheet(QString(
            "color:#F5E6B8; font:700 13px 'Segoe UI';"
            "padding:4px 10px; border-radius:10px;"
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
            avatar = QPixmap(38, 38);
            avatar.fill(QColor("#5A3E28"));
        }
        avatarLabel_->setPixmap(avatar.scaled(36, 36, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
}

void ProfileLobbyWidget::refreshCharacterPreview() {
    if (!characterItem_) {
        return;
    }

    idleAnimationTimer_->stop();
    idleFrames_.clear();
    idleFrameIndex_ = 0;

    struct IdleSheetSpec {
        QString relativePath;
        int frameCount;
    };

    const QString characterName = selectedCharacter_.name.trimmed().toLower();
    const IdleSheetSpec spec = [&]() -> IdleSheetSpec {
        if (characterName.contains("arcen")) {
            return {"Sprites/Character/Idle.png", 10};
        }
        if (characterName.contains("demon")) {
            return {"Sprites/Idle.png", 4};
        }
        if (characterName.contains("fantasy")) {
            return {"Sprites/Idle.png", 10};
        }
        if (characterName.contains("huntress")) {
            return {"Sprites/Idle.png", 8};
        }
        if (characterName.contains("knight")) {
            return {"Sprites/IDLE.png", 7};
        }
        if (characterName.contains("martial hero")) {
            return {"Sprites/Idle.png", 8};
        }
        if (characterName.contains("martial")) {
            return {"Sprite/Idle.png", 10};
        }
        if (characterName.contains("medieval")) {
            return {"Sprites/Idle.png", 10};
        }
        if (characterName.contains("wizard")) {
            return {"Sprites/Idle.png", 6};
        }
        return {QString(), 1};
    }();

    const QString resolvedImagePath = resolveAssetPath(selectedCharacter_.imagePath);
    const QFileInfo imageInfo(resolvedImagePath);
    const QDir baseDir = imageInfo.dir();
    const QString idleSpritePath = spec.relativePath.isEmpty() ? QString() : baseDir.filePath(spec.relativePath);
    if (!idleSpritePath.isEmpty() && QFileInfo::exists(idleSpritePath)) {
        const QPixmap spriteSheet(idleSpritePath);
        if (!spriteSheet.isNull()) {
            const QImage sheetImage = spriteSheet.toImage().convertToFormat(QImage::Format_ARGB32);
            const int frameCount = qMax(1, spec.frameCount);
            const int frameWidth = spriteSheet.width() / qMax(1, frameCount);
            const int frameHeight = spriteSheet.height();

            // Compute shared visible bounds across all frames to avoid tiny rendering and frame jitter.
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

            QRect cropRect(0, 0, frameWidth, frameHeight);
            if (maxX >= minX && maxY >= minY) {
                cropRect = QRect(minX, minY, maxX - minX + 1, maxY - minY + 1)
                               .adjusted(-1, -1, 1, 1)
                               .intersected(QRect(0, 0, frameWidth, frameHeight));
            }

            idleFrames_.clear();
            for (int i = 0; i < frameCount; ++i) {
                idleFrames_.append(spriteSheet.copy(i * frameWidth + cropRect.x(),
                                                    cropRect.y(),
                                                    cropRect.width(),
                                                    cropRect.height()));
            }
            if (!idleFrames_.isEmpty()) {
                idleFrameIndex_ = 0;
                characterItem_->setPixmap(idleFrames_.first());
                if (fallbackTextItem_) {
                    fallbackTextItem_->setVisible(false);
                }
                idleAnimationTimer_->start(150);
                refreshPreviewTitle();
                updatePreviewScale();
                return;
            }
        }
    }

    QPixmap fallback = createCharacterPlaceholder(QSize(460, 700), selectedCharacter_.name);
    if (fallbackTextItem_) {
        fallbackTextItem_->setVisible(true);
        fallbackTextItem_->setPlainText("Character image missing");
    }

    characterItem_->setPixmap(fallback);
    refreshPreviewTitle();
    updatePreviewScale();
}

void ProfileLobbyWidget::advanceIdleAnimation() {
    if (idleFrames_.isEmpty() || !characterItem_) {
        return;
    }

    idleFrameIndex_ = (idleFrameIndex_ + 1) % idleFrames_.size();
    characterItem_->setPixmap(idleFrames_.at(idleFrameIndex_));
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
    }
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
                      sceneRect.height() * 0.58 - glowItem_->pixmap().height() * 0.5);

    const qreal targetHeight = characterView_->viewport()->height() * 0.96;
    const qreal targetWidth = characterView_->viewport()->width() * 0.74;
    const qreal sx = targetWidth / pix.width();
    const qreal sy = targetHeight / pix.height();
    const qreal scale = qMin(sx, sy) * 3.0;

    characterItem_->setScale(scale);
    const QSizeF scaled(pix.width() * scale, pix.height() * scale);
    characterItem_->setPos((sceneRect.width() - scaled.width()) * 0.5,
                           sceneRect.height() - scaled.height() - sceneRect.height() * 0.02);

    if (fallbackTextItem_) {
        fallbackTextItem_->setPos((sceneRect.width() - fallbackTextItem_->boundingRect().width()) * 0.5,
                                  sceneRect.height() * 0.86);
    }

    characterView_->fitInView(sceneRect, Qt::KeepAspectRatio);
}

void ProfileLobbyWidget::updateModeCardHover(QWidget* card, bool hovered) {
    if (!card || !cardBaseSizes_.contains(card)) {
        return;
    }

    const QSize baseSize = cardBaseSizes_.value(card);
    const QSize hoverSize(static_cast<int>(baseSize.width() * 1.05), static_cast<int>(baseSize.height() * 1.05));
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
    refreshCharacterPreview();
}

void ProfileLobbyWidget::setSelectedMode(const QString& modeName) {
    if (modeCards_.contains(modeName)) {
        selectedModeName_ = modeName;
        refreshModeSelectionUi();
    }
}

void ProfileLobbyWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
    gradient.setColorAt(0.0, QColor("#2C1F14"));
    gradient.setColorAt(1.0, QColor("#1A140F"));
    painter.fillRect(rect(), gradient);

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
                enterArenaGlowEffect_->setBlurRadius(24.0);
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

