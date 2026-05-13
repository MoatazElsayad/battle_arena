/********************************************************************************
** Form generated from reading UI file 'GameOverPage.ui'
**
** Created by: Qt User Interface Compiler version 6.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GAMEOVERPAGE_H
#define UI_GAMEOVERPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GameOverPage
{
public:
    QLabel *overLabel;
    QLabel *winlose;
    QLabel *scorePoints;
    QPushButton *backButton;
    QPushButton *restartButton;
    QPushButton *exitButton;

    void setupUi(QWidget *GameOverPage)
    {
        if (GameOverPage->objectName().isEmpty())
            GameOverPage->setObjectName("GameOverPage");
        GameOverPage->resize(673, 350);
        GameOverPage->setStyleSheet(QString::fromUtf8("QWidget#GameOverPage {\n"
"	background-color: #2A1810;\n"
"	color: #F5E6D3;\n"
"}\n"
"QLabel {\n"
"	background: transparent;\n"
"	color: #F5E6D3;\n"
"}\n"
"QLabel#overLabel {\n"
"	color: #D4A017;\n"
"}\n"
"QLabel#winlose {\n"
"	color: #F5E6D3;\n"
"}\n"
"QLabel#scorePoints {\n"
"	color: #D4A017;\n"
"}\n"
"QPushButton {\n"
"	background-color: #5C4033;\n"
"	color: #F5E6D3;\n"
"	border: 2px solid #D4A017;\n"
"	border-radius: 10px;\n"
"}\n"
"QPushButton:hover {\n"
"	background-color: #6B4D3D;\n"
"}"));
        overLabel = new QLabel(GameOverPage);
        overLabel->setObjectName("overLabel");
        overLabel->setGeometry(QRect(120, 10, 451, 81));
        QFont font;
        font.setFamilies({QString::fromUtf8("Showcard Gothic")});
        font.setPointSize(48);
        overLabel->setFont(font);
        overLabel->setAlignment(Qt::AlignCenter);
        winlose = new QLabel(GameOverPage);
        winlose->setObjectName("winlose");
        winlose->setGeometry(QRect(130, 80, 421, 81));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Showcard Gothic")});
        font1.setPointSize(36);
        winlose->setFont(font1);
        winlose->setAlignment(Qt::AlignCenter);
        scorePoints = new QLabel(GameOverPage);
        scorePoints->setObjectName("scorePoints");
        scorePoints->setGeometry(QRect(20, 140, 421, 81));
        scorePoints->setFont(font1);
        scorePoints->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        backButton = new QPushButton(GameOverPage);
        backButton->setObjectName("backButton");
        backButton->setGeometry(QRect(10, 290, 181, 41));
        QFont font2;
        font2.setFamilies({QString::fromUtf8("Showcard Gothic")});
        font2.setPointSize(24);
        backButton->setFont(font2);
        restartButton = new QPushButton(GameOverPage);
        restartButton->setObjectName("restartButton");
        restartButton->setGeometry(QRect(250, 290, 181, 41));
        restartButton->setFont(font2);
        exitButton = new QPushButton(GameOverPage);
        exitButton->setObjectName("exitButton");
        exitButton->setGeometry(QRect(470, 290, 181, 41));
        exitButton->setFont(font2);

        retranslateUi(GameOverPage);

        QMetaObject::connectSlotsByName(GameOverPage);
    } // setupUi

    void retranslateUi(QWidget *GameOverPage)
    {
        GameOverPage->setWindowTitle(QCoreApplication::translate("GameOverPage", "Game Over", nullptr));
        overLabel->setText(QCoreApplication::translate("GameOverPage", "GAME OVER", nullptr));
        winlose->setText(QCoreApplication::translate("GameOverPage", "XXX", nullptr));
        scorePoints->setText(QCoreApplication::translate("GameOverPage", "XXX", nullptr));
        backButton->setText(QCoreApplication::translate("GameOverPage", "Back", nullptr));
        restartButton->setText(QCoreApplication::translate("GameOverPage", "Restart", nullptr));
        exitButton->setText(QCoreApplication::translate("GameOverPage", "Exit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class GameOverPage: public Ui_GameOverPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GAMEOVERPAGE_H
