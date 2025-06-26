// src/user.cpp
#include "user.h"
#include "database.h"
#include <QSqlQuery>
#include <QDateTime>
#include <random>
#include <QDebug>

User::User(const QString &id, const QString &email, const QString &password,
           const QString &name, UserType type, float readingHours,
           double fines, int creditScore, bool hadLowCredit, QObject *parent)
    : QObject(parent), m_id(id), m_email(email), m_password(password),
      m_name(name), m_type(type), m_readingHours(readingHours), m_fines(fines),
      m_creditScore(creditScore), m_hadLowCredit(hadLowCredit)
{
    // 根据用户类型设置借阅规则
    if(m_type == Super) {
        m_maxBorrow = 8;      // 超级读者最多借8本
        m_borrowDays = 28;    // 借阅期限4周

        // 超级读者初始信用分120
        if(m_creditScore < 120) {
            m_creditScore = 120;
        }
    } else {
        m_maxBorrow = 5;      // 普通读者最多借5本
        m_borrowDays = 14;    // 借阅期限2周

        // 普通读者初始信用分100
        if(m_creditScore < 100) {
            m_creditScore = 100;
        }
    }
}

int User::maxBorrowCount() const {
    return m_maxBorrow;
}

int User::borrowDays() const {
    return m_borrowDays;
}

bool User::upgradeToSuper()
{
    if(m_type == Normal && m_readingHours >= 200.0 && canUpgrade()) {
        m_type = Super;
        m_maxBorrow = 8;
        m_borrowDays = 28;
        m_creditScore = 120; // 升级后信用分设为120

        // 更新数据库
        QString query = QString(
            "UPDATE Users SET Type = 'Super', "
            "MaxBorrow = 8, BorrowDays = 28, "
            "CreditScore = 120 "
            "WHERE UserID = '%1'"
        ).arg(m_id);

        if(Database::instance()->execute(query)) {
            QMessageBox::information(nullptr, "升级成功",
                "恭喜您已升级为超级读者！\n"
                "新的借阅权限：最多可借8本书，借期4周\n"
                "信用分已提升至120");
            return true;
        }
    }
    return false;
}

void User::addReadingHours(float hours)
{
    m_readingHours += hours;

    // 更新数据库
    QSqlQuery q = Database::instance()->executeQuery(
        QString("UPDATE Users SET TotalReadingHours = %1 WHERE UserID = '%2'")
        .arg(m_readingHours).arg(m_id)
    );

    // 检查是否需要升级
    if(m_type == Normal && m_readingHours >= 200.0) {
        upgradeToSuper();
    }
}

void User::addFine(double amount)
{
    m_fines += amount;

    // 更新数据库
    Database::instance()->execute(
        QString("UPDATE Users SET Fines = %1 WHERE UserID = '%2'")
        .arg(m_fines).arg(m_id)
    );
}

void User::payFine(double amount)
{
    if(amount > m_fines) amount = m_fines;
    m_fines -= amount;

    // 更新数据库
    Database::instance()->execute(
        QString("UPDATE Users SET Fines = %1 WHERE UserID = '%2'")
