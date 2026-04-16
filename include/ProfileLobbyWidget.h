#ifndef PROFILELOBBYWIDGET_H
#define PROFILELOBBYWIDGET_H

#include <QMap>
#include <QVector>
#include <QWidget>

class QLabel;
class QToolButton;
class QPushButton;
class QMenu;
class QScrollArea;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsTextItem;
class QGraphicsDropShadowEffect;
class QVariantAnimation;
class QLayout;
class QTimer;
class QVBoxLayout;

class ProfileLobbyWidget : public QWidget {
    Q_OBJECT

public:
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
    void setupUi();
    void setupHeader(QVBoxLayout* rootLayout);
    void setupCharacterPreview(QVBoxLayout* rootLayout);
    void setupModeCarousel(QVBoxLayout* rootLayout);
    void setupBottomBar(QVBoxLayout* rootLayout);

    QWidget* createModeCard(const GameMode& mode);
    QString badgeColorFor(const QString& badge) const;
    void refreshProfileUi();
    void refreshCharacterPreview();
    void refreshModeSelectionUi();
    void refreshPreviewTitle();
    void updatePreviewScale();
    void updateModeCardHover(QWidget* card, bool hovered);
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

    QLabel* previewTitleLabel_;
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
    QMap<QWidget*, QGraphicsDropShadowEffect*> cardGlowEffects_;
    QMap<QWidget*, QSize> cardBaseSizes_;

    QPushButton* changeCharacterButton_;
    QPushButton* enterArenaButton_;

    QVariantAnimation* hoverGlowAnimation_;
    QGraphicsDropShadowEffect* enterArenaGlowEffect_;
    QTimer* idleAnimationTimer_;
    QVector<QPixmap> idleFrames_;
    int idleFrameIndex_;
};

#endif // PROFILELOBBYWIDGET_H
