#ifndef PROFILELOBBYWIDGET_H
#define PROFILELOBBYWIDGET_H

#include <QMap>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "Enums.h"
#include "ProgressionTypes.h"

class QLabel;
class QToolButton;
class QPushButton;
class QProgressBar;
class QMenu;
class QScrollArea;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class QGraphicsTextItem;
class QGraphicsDropShadowEffect;
class QVariantAnimation;
class QLayout;
class QBoxLayout;
class QTimer;
class QVBoxLayout;
class QComboBox;
class QFrame;
class QKeyEvent;

class ProfileLobbyWidget : public QWidget {
    Q_OBJECT

public:
    // LAN teammate:
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
    struct DuelSetup {
        QString opponentMode;      // "Random" / "Manual"
        QString opponentCategory;   // "Player" / "Enemy"
        QString selectedOpponent;   // optional identifier
        QString selectedArena;
    };

    DuelSetup duelConfig() const { return duelSetup_; }

    explicit ProfileLobbyWidget(QWidget* parent = nullptr);
    ~ProfileLobbyWidget() override;

    UserProfile userProfile() const { return userProfile_; }
    Character selectedCharacter() const { return selectedCharacter_; }
    QString selectedMode() const { return selectedModeName_; }

public slots:
    void setUserProfile(const UserProfile& profile);
    void setSelectedCharacter(const Character& character);
    void setSelectedMode(const QString& modeName);
    void setDuelSetup(const DuelSetup& setup);
    void updateProgression(const PlayerProgression& stats);
    void showRankUpgradePopup(const QString& previousRank, const QString& newRank);
    void showCharacterUnlockPopup(const QString& characterName,
                                  const QString& rankName,
                                  const QString& imagePath);

signals:
    void enterArenaClicked(const QString& modeName);
    void changeCharacterClicked();
    void settingsActionTriggered(const QString& actionName);
    void usernameEditRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // LAN teammate:
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
    void setupDuelSetupPanel(QBoxLayout* rootLayout);
    void setupRankUpgradeOverlay();

    QWidget* createModeCard(const GameMode& mode);
    QString badgeColorFor(const QString& badge) const;
    void prepareCharacterUnlockReveal(const QString& characterName, const QString& imagePath);
    void updateCharacterUnlockReveal(qreal progress);
    void displayCharacterUnlockPopup(const QString& characterName,
                                     const QString& rankName,
                                     const QString& imagePath);
    void claimRankUpgradePopup();
    void syncRankUpgradeOverlay();
    void refreshProfileUi();
    void refreshCharacterLockState();
    void refreshCharacterPreview();
    void refreshModeSelectionUi();
    void refreshLobbyContext();
    void refreshPreviewTitle();
    void updatePreviewScale();
    void updateModeCardHover(QWidget* card, bool hovered);
    bool isPlayableMode(const QString& modeName) const;
    bool isZombieModeUnlocked() const;
    QString modeAccentFor(const QString& modeName) const;
    QString modeShortTagFor(const QString& modeName) const;
    QString modeDescriptionFor(const QString& modeName) const;
    QStringList duelPlayerOpponentChoices() const;
    QStringList duelEnemyOpponentChoices() const;
    void refreshDuelSetupControls();
    QString resolveIdleSpriteSheet(const QString& imagePath) const;
    QVector<QPixmap> extractIdleFrames(const QPixmap& spriteSheet) const;
    int estimateFrameCount(const QSize& spriteSheetSize) const;
    void advanceIdleAnimation();

    UserProfile userProfile_;
    Character selectedCharacter_;
    QString selectedModeName_;
    DuelSetup duelSetup_;

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
    QLabel* lobbySummaryCharacterLabel_;
    QLabel* lobbySummaryRankLabel_;
    QLabel* lobbySummaryScoreLabel_;
    QLabel* lobbySummaryBadgeLabel_;
    QWidget* rankUpgradeOverlay_;
    QFrame* rankUpgradePanel_;
    QLabel* rankUpgradeEyebrowLabel_;
    QLabel* rankUpgradeTitleLabel_;
    QLabel* rankUpgradeBodyLabel_;
    QWidget* rankUpgradeBadgeRowWidget_;
    QLabel* rankUpgradeArrowLabel_;
    QLabel* rankUpgradeOldRankLabel_;
    QLabel* rankUpgradeNewRankLabel_;
    QLabel* rankUpgradeOldBadgeLabel_;
    QLabel* rankUpgradeNewBadgeLabel_;
    QLabel* rankUpgradeHintLabel_;
    QGraphicsView* characterUnlockView_;
    QGraphicsScene* characterUnlockScene_;
    QGraphicsPixmapItem* characterUnlockGlowItem_;
    QGraphicsPixmapItem* characterUnlockPortraitItem_;
    QGraphicsPixmapItem* characterUnlockFighterItem_;
    QLabel* previewPortraitLabel_;
    QLabel* previewDescriptionLabel_;
    QLabel* previewMoveLabel_;
    QLabel* previewAbilitiesLabel_;
    QLabel* previewRoleChipLabel_;
    QLabel* previewAttacksChipLabel_;
    QLabel* previewRangeChipLabel_;
    QLabel* previewProjectileChipLabel_;
    QLabel* previewUnlockChipLabel_;
    QLabel* previewHintLabel_;
    QProgressBar* attackPowerBar_;
    QProgressBar* healPowerBar_;
    QProgressBar* mobilityPowerBar_;
    QProgressBar* controlPowerBar_;
    QGraphicsView* characterView_;
    QGraphicsScene* previewScene_;
    QGraphicsPixmapItem* sceneBackgroundItem_;
    QGraphicsRectItem* sceneLockDimItem_;
    QGraphicsPixmapItem* characterItem_;
    QGraphicsPixmapItem* glowItem_;
    QGraphicsPixmapItem* characterLockItem_;
    QGraphicsTextItem* characterLockTextItem_;
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
    QFrame* duelSetupPanel_;
    QComboBox* opponentModePicker_;
    QComboBox* opponentCategoryPicker_;
    QComboBox* opponentPicker_;
    QComboBox* arenaPicker_;
    QLabel* duelSetupLabel_;

    QVariantAnimation* hoverGlowAnimation_;
    QVariantAnimation* rankUpgradeOverlayAnimation_;
    QVariantAnimation* characterUnlockRevealAnimation_;
    QGraphicsDropShadowEffect* enterArenaGlowEffect_;
    QTimer* idleAnimationTimer_;
    QVector<QPixmap> idleFrames_;
    QVector<QPixmap> attackFrames_;
    QVector<QPixmap> characterUnlockIdleFrames_;
    int idleFrameIndex_;
    bool showcasingAttack_;
    int idleShowcaseElapsedMs_;
    bool rankUpgradeOverlayClosing_;
    bool showingCharacterUnlockPopup_;
    bool characterUnlockClaimReady_;
    QString pendingCharacterUnlockName_;
    QString pendingCharacterUnlockRank_;
    QString pendingCharacterUnlockImagePath_;
};

#endif // PROFILELOBBYWIDGET_H
