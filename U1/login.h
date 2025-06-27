#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>

#include "ui_login.h"

// 前向声明
namespace Ui {
class LoginDialog;
}

class Library; // 前向声明
class User;    // 前向声明

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
     User* getAuthenticatedUser() const;
    ~LoginDialog(); // 必须声明析构函数
private slots:  // 添加槽函数声明
    void onLoginClicked();  // 登录按钮点击处理
    // 如果有注册按钮，也可以添加
    void onRegisterClicked();

private:
    Ui::LoginDialog *ui;   // UI 指针
    User *m_user;  // 用户对象指针
     User* m_authenticatedUser;
    Library *m_library;    // 图书馆对象指针
};

#endif // LOGIN_H
