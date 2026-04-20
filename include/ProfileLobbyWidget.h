#ifndef PROFILELOBBYWIDGET_H
#define PROFILELOBBYWIDGET_H

#include <QMap>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QLabel;
class QToolButton;
class QPushButton;
class QProgressBar;
class QMenu;
class QScrollArea;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsTextItem;
class QGraphicsDropShadowEffect;
class QVariantAnimation;
class QLayout;
class QBoxLayout;
class QTimer;
class QVBoxLayout;

class ProfileLobbyWidget : public QWidget {
    Q_OBJECT

public:
    // 1v1 teammate:
    // Extend lobby data/UI here for duel setup:
    // selected opponent mode, selected opponent, and selected background.
    // Ranking teammate:
    // This widget should display the player's live progression summary:
    // score, rank, and rating.
    // Lobby data model
    struct UserProfile {
        QString username;
        int score;
        QString badge;
        QString avatarPath;
    };

    struct Character {
        QString name;
        QString imagePath;
        QString specialMoves;
    };

    struct GameMode {
        QString name;
        QString iconPath;
        QString description;
        bool recommended = false;
    };

    explicit ProfileLobbyWidget(QWidget* parent = nullptr);
    ~ProfileLobbyWidget() override;

    UserProfile userProfile() const { return userProfile_; }
    Character selectedCharacter() const { return selectedCharacter_; }
    QString selectedMode() const { return selectedModeName_; }

public slots:
    void setUserProfile(const UserProfile& profile);
    void setSelectedCharacter(const Character& character);
    void setSelectedMode(const QString& modeName);

signals:
    void enterArenaClicked(const QString& modeName);
    void changeCharacterClicked();
    void settingsActionTriggered(const QString& actionName);
    void usernameEditRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // 1v1 teammate:
    // Most duel-theme setup UI belongs in this widget.
    // Main work areas:
    // - setupModeCarousel
    // - setupBottomBar
    // - refreshLobbyContext
    // - mode selection flow
    void setupUi();
    void setupHeader(QBoxLayout* rootLayout);
    void setupCharacterPreview(QBoxLayout* rootLayout);
    void setupModeCarousel(QBoxLayout* rootLayout);
    void setupBottomBar(QBoxLayout* rootLayout);

    QWidget* createModeCard(const GameMode& mode);
    QString badgeColorFor(const QString& badge) const;
    void refreshProfileUi();
    void refreshCharacterPreview();
    void refreshModeSelectionUi();
    void refreshLobbyContext();
    void refreshPreviewTitle();
    void updatePreviewScale();
    void updateModeCardHover(QWidget* card, bool hovered);
    bool isPlayableMode(const QString& modeName) const;
    QString modeAccentFor(const QString& modeName) const;
    QString modeShortTagFor(const QString& modeName) const;
    QString modeDescriptionFor(const QString& modeName) const;
    QString resolveIdleSpriteSheet(const QString& imagePath) const;
    QVector<QPixmap> extractIdleFrames(const QPixmap& spriteSheet) const;
    int estimateFrameCount(const QSize& spriteSheetSize) const;
    void advanceIdleAnimation();

    UserProfile userProfile_;
    Character selectedCharacter_;
    QString selectedModeName_;

    QLabel* logoLabel_;
    QLabel* usernameLabel_;
    QLabel* scoreLabel_;
    QLabel* badgeLabel_;
    QLabel* avatarLabel_;

    QToolButton* settingsButton_;
    QMenu* settingsMenu_;

    QLabel* previewEyebrowLabel_;
    QLabel* previewTitleLabel_;
    QLabel* previewModeChipLabel_;
    QLabel* previewDescriptionLabel_;
    QLabel* previewMoveLabel_;
    QLabel* previewAbilitiesLabel_;
    QLabel* previewHintLabel_;
    QProgressBar* attackPowerBar_;
    QProgressBar* healPowerBar_;
    QProgressBar* mobilityPowerBar_;
    QProgressBar* controlPowerBar_;
    QGraphicsView* characterView_;
    QGraphicsScene* previewScene_;
    QGraphicsPixmapItem* sceneBackgroundItem_;
    QGraphicsPixmapItem* characterItem_;
    QGraphicsPixmapItem* glowItem_;
    QGraphicsTextItem* fallbackTextItem_;

    QScrollArea* modeScrollArea_;
    QWidget* modeContainer_;
    QLayout* modeLayout_;
    QMap<QString, QWidget*> modeCards_;
    QMap<QString, GameMode> availableModes_;
    QMap<QWidget*, QGraphicsDropShadowEffect*> cardGlowEffects_;
    QMap<QWidget*, QSize> cardBaseSizes_;

    QPushButton* changeCharacterButton_;
    QPushButton* enterArenaButton_;
    QLabel* actionSummaryLabel_;
    QLabel* actionHintLabel_;

    QVariantAnimation* hoverGlowAnimation_;
    QGraphicsDropShadowEffect* enterArenaGlowEffect_;
    QTimer* idleAnimationTimer_;
    QVector<QPixmap> idleFrames_;
    QVector<QPixmap> attackFrames_;
    int idleFrameIndex_;
    bool showcasingAttack_;
    int idleShowcaseElapsedMs_;
};

#endif // PROFILELOBBYWIDGET_H
