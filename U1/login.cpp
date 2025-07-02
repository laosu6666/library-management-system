#include "login.h"
#include "ui_login.h" // 必须包含自动生成的UI头文件
#include <QMessageBox>
#include "library.h"  // 包含Library类的定义
#include "user.h"     // 包含User类的定义

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::LoginDialog), m_user(nullptr),  m_library (new Library(this))
{
    ui->setupUi(this);
    setWindowTitle("图书管理系统 - 登录");

    // 创建图书馆实例


    // 连接信号槽
    connect(ui->btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(ui->btnRegister, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
}
LoginDialog::~LoginDialog()
{
    delete ui; // 删除UI对象
    // 不要删除 m_library，因为它由父对象管理
}

User* LoginDialog::getAuthenticatedUser() const {
    return m_user; // 返回原始指针，调用方负责管理
}

void LoginDialog::onLoginClicked()
{
    QString identifier = ui->txtIdentifier->text();
    QString password = ui->txtPassword->text();
    QString role = ui->cmbRole->currentText(); // 新增

    if(identifier.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "请输入用户名/邮箱和密码");
        return;
    }

    m_user = m_library->authenticateUser(identifier, password);
    if(m_user) {
        // 判断身份
        if(role == "图书管理员" && m_user->type() != User::Super) {
            QMessageBox::warning(this, "登录失败", "该账号不是管理员账号");
            delete m_user;
            m_user = nullptr;
            return;
        }
        if(role == "读者" && m_user->type() != User::Normal) {
            QMessageBox::warning(this, "登录失败", "请用读者账号登录");
            delete m_user;
            m_user = nullptr;
            return;
        }
        // 信用分校验只针对读者
        if(m_user->type() == User::Normal && !m_user->canBorrow()) {
            QMessageBox::warning(this, "登录失败",
                QString("您的信用分低于90分 (%1分)，暂时无法借书\n"
                        "请通过缴费提升信用分").arg(m_user->creditScore()));
            delete m_user;
            m_user = nullptr;
            return;
        }
        accept();
    } else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
    }
}

void LoginDialog::onRegisterClicked()
{
    QString email = ui->txtIdentifier->text();
    QString password = ui->txtPassword->text();
    QString name = ui->txtName->text();

    if(email.isEmpty() || password.isEmpty() || name.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "请填写所有必填字段");
        return;
    }

    if(!email.contains('@')) {
        QMessageBox::warning(this, "注册失败", "请输入有效的邮箱地址");
        return;
    }

    User *newUser = m_library->registerUser(email, password, name);
    if(newUser) {
        QMessageBox::information(this, "注册成功",
            QString("注册成功！您的用户ID是：%1\n请使用邮箱或用户ID登录").arg(newUser->id()));
        delete newUser;
    } else {
        QMessageBox::warning(this, "注册失败", "注册失败，邮箱可能已被使用");
    }
}
