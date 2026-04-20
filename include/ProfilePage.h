#ifndef PROFILEPAGE_H
#define PROFILEPAGE_H

#include <QWidget>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class ProfilePage;
}
QT_END_NAMESPACE

class ProfilePage : public QWidget {
    Q_OBJECT

public:
    // Ranking teammate:
    // ProfilePage can show the detailed progression view:
    // score, rank, rating, wins, losses, and matches if needed.
    explicit ProfilePage(QWidget *parent = nullptr);
    ~ProfilePage();

    void setUsername(const QString &username);
    void setScore(int score);
    void setRank(const QString &rank);
    void setCharacterType(const QString &type);
    QString getCharacterType() const;

signals:
    void playClicked();
    void difficultyChanged(const QString &difficulty);
    void characterTypeChanged(const QString &type);

private slots:
    void on_pushButton_clicked();
    void on_diffucultySelector_currentTextChanged(const QString &text);
    void on_characterCombo_currentTextChanged(const QString &text);

private:
    Ui::ProfilePage *ui;
    QString selectedCharacterType_;
};

#endif // PROFILEPAGE_H
