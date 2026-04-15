#include "LoginPage.h"
#include "ui_LoginPage.h"
#include "registration.h"

Loginpage::LoginPage(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::loginepage)
{
    ui->setupUi(this);
    this->setStyleSheet("background-color: #4E8D9C;");

}

Loginpage::~loginePage()
{
    delete ui;
}



void LoginPage::on_SignUpButton_clicked()
{
    hide();
    Registration* RW = new Registration(this);
    RW -> show();

}

