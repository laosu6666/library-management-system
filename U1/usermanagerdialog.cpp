#include "usermanagerdialog.h"
#include "ui_usermanagerdialog.h"

UserManagerDialog::UserManagerDialog(Library *library, QWidget *parent)
    : QDialog(parent), ui(new Ui::UserManagerDialog), m_library(library)
{
    ui->setupUi(this);
    loadUsers();

    // 添加删除按钮连接
    connect(ui->btnDelete, &QPushButton::clicked, this, &UserManagerDialog::on_btnDelete_clicked);
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
void UserManagerDialog::on_btnDelete_clicked()
{
    int row = ui->tblUsers->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择要删除的用户！");
        return;
    }
    QString userId = ui->tblUsers->item(row, 0)->text();

    // 防止删除管理员
    if (ui->tblUsers->item(row, 3)->text() == "管理员") {
        QMessageBox::warning(this, "提示", "不能删除管理员账号！");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定要删除该用户吗？") == QMessageBox::Yes) {
        if (m_library->deleteUser(userId)) {
            QMessageBox::information(this, "成功", "用户已删除！");
            loadUsers(); // 刷新列表
        } else {
            QMessageBox::warning(this, "失败", "删除失败，用户可能还有未归还的图书！");
        }
    }
}
