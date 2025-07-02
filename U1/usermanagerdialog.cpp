#include "usermanagerdialog.h"
#include "ui_usermanagerdialog.h"

UserManagerDialog::UserManagerDialog(Library *library, QWidget *parent)
    : QDialog(parent), ui(new Ui::UserManagerDialog), m_library(library)
{
    ui->setupUi(this);
    loadUsers();
}

UserManagerDialog::~UserManagerDialog()
{
    delete ui;
}

void UserManagerDialog::loadUsers()
{
    QList<User*> users = m_library->getAllUsers();
    ui->tblUsers->setRowCount(users.size());
    for(int i=0; i<users.size(); ++i) {
        User *u = users[i];
        ui->tblUsers->setItem(i, 0, new QTableWidgetItem(u->id()));
        ui->tblUsers->setItem(i, 1, new QTableWidgetItem(u->name()));
        ui->tblUsers->setItem(i, 2, new QTableWidgetItem(u->email()));
        ui->tblUsers->setItem(i, 3, new QTableWidgetItem(u->type() == User::Super ? "管理员" : "读者"));
        ui->tblUsers->setItem(i, 4, new QTableWidgetItem(QString::number(u->creditScore())));
        ui->tblUsers->setItem(i, 5, new QTableWidgetItem(QString::number(u->fines(), 'f', 2)));
    }
}
