#include "PausePage.h"

#include <QEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

PausePage::PausePage(QWidget *parent)
    : QWidget(parent),
      hostWidget_(parent),
      panel_(nullptr),
      titleLabel_(nullptr),
      subtitleLabel_(nullptr) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, false);
    setFocusPolicy(Qt::StrongFocus);
    hide();

    if (hostWidget_) {
        hostWidget_->installEventFilter(this);
    }

    setupUI();
    syncToParent();
}

PausePage::~PausePage() {}

void PausePage::showPauseOverlay() {
    syncToParent();
    show();
    raise();
    setFocus(Qt::OtherFocusReason);
}

void PausePage::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), QColor(5, 4, 3, 118));

    QLinearGradient topFade(rect().topLeft(), rect().bottomLeft());
    topFade.setColorAt(0.0, QColor(34, 20, 13, 110));
    topFade.setColorAt(0.45, QColor(16, 11, 8, 58));
    topFade.setColorAt(1.0, QColor(9, 6, 5, 138));
    painter.fillRect(rect(), topFade);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 188, 92, 14));
    painter.drawEllipse(QRectF(width() * 0.18, height() * 0.18, width() * 0.28, height() * 0.18));
    painter.drawEllipse(QRectF(width() * 0.58, height() * 0.50, width() * 0.24, height() * 0.16));
}

void PausePage::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        emit resumeClicked();
        return;
    }

    QWidget::keyPressEvent(event);
}

bool PausePage::eventFilter(QObject *watched, QEvent *event) {
    if (watched == hostWidget_ && (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
        syncToParent();
    }

    return QWidget::eventFilter(watched, event);
}

void PausePage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 36, 36, 36);
    mainLayout->setAlignment(Qt::AlignCenter);

    panel_ = new QFrame(this);
    panel_->setObjectName("pausePanel");
    panel_->setMinimumWidth(380);
    panel_->setMaximumWidth(430);
    panel_->setStyleSheet(
        "QFrame#pausePanel {"
        " background: rgba(26, 17, 12, 214);"
        " border: 1px solid rgba(226, 170, 81, 0.62);"
        " border-radius: 28px;"
        "}"
        "QLabel#pauseTitle { color:#FFF0C6; font:900 34px 'Segoe UI'; letter-spacing:2px; }"
        "QLabel#pauseBody { color:rgba(245,230,184,0.84); font:13px 'Segoe UI'; }"
        "QPushButton#primaryPauseAction {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #8C1111, stop:1 #E1502C);"
        " color:#FFF2E8;"
        " border: 2px solid rgba(255, 187, 140, 0.90);"
        " border-radius: 14px;"
        " padding: 12px 18px;"
        " font:800 15px 'Segoe UI';"
        "}"
        "QPushButton#primaryPauseAction:hover { border-color:#FFD4B7; }"
        "QPushButton#secondaryPauseAction {"
        " background: rgba(70, 48, 31, 0.92);"
        " color:#F4DEC2;"
        " border: 1px solid rgba(212, 160, 23, 0.82);"
        " border-radius: 14px;"
        " padding: 12px 18px;"
        " font:700 14px 'Segoe UI';"
        "}"
        "QPushButton#secondaryPauseAction:hover { background: rgba(98, 68, 41, 0.98); }");

    auto *shadow = new QGraphicsDropShadowEffect(panel_);
    shadow->setBlurRadius(34.0);
    shadow->setOffset(0.0, 16.0);
    shadow->setColor(QColor(0, 0, 0, 150));
    panel_->setGraphicsEffect(shadow);

    QVBoxLayout *panelLayout = new QVBoxLayout(panel_);
    panelLayout->setContentsMargins(30, 28, 30, 28);
    panelLayout->setSpacing(14);

    titleLabel_ = new QLabel("BATTLE PAUSED", panel_);
    titleLabel_->setObjectName("pauseTitle");
    titleLabel_->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(titleLabel_);

    subtitleLabel_ = new QLabel("Catch your breath, adjust your plan, or press ESC to jump right back in.", panel_);
    subtitleLabel_->setObjectName("pauseBody");
    subtitleLabel_->setWordWrap(true);
    subtitleLabel_->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(subtitleLabel_);

    panelLayout->addSpacing(6);

    QPushButton *resumeButton = new QPushButton("Resume Battle", panel_);
    resumeButton->setObjectName("primaryPauseAction");
    connect(resumeButton, &QPushButton::clicked, this, &PausePage::resumeClicked);
    panelLayout->addWidget(resumeButton);

    QPushButton *settingsButton = new QPushButton("Settings", panel_);
    settingsButton->setObjectName("secondaryPauseAction");
    connect(settingsButton, &QPushButton::clicked, this, &PausePage::settingsClicked);
    panelLayout->addWidget(settingsButton);

    QPushButton *menuButton = new QPushButton("Back to Menu", panel_);
    menuButton->setObjectName("secondaryPauseAction");
    connect(menuButton, &QPushButton::clicked, this, &PausePage::menuClicked);
    panelLayout->addWidget(menuButton);

    mainLayout->addWidget(panel_, 0, Qt::AlignCenter);
}

void PausePage::syncToParent() {
    if (!hostWidget_) {
        return;
    }

    setGeometry(hostWidget_->rect());
}
