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
        .arg(m_fines).arg(m_id)
    );
}

void User::payFineWithCredit(double amount)
{
    // 1元补1信用分
    int creditToAdd = static_cast<int>(amount);
    if(creditToAdd > 0) {
        addCreditScore(creditToAdd);
        payFine(amount);
    }
}

void User::setCreditScore(int score)
{
    if(score < 0) score = 0;
    if(score > 150) score = 150;

    m_creditScore = score;

    // 更新数据库
    Database::instance()->execute(
        QString("UPDATE Users SET CreditScore = %1 WHERE UserID = '%2'")
        .arg(score).arg(m_id)
    );

    // 检查是否首次低于90分
    if(m_creditScore < 90 && !m_hadLowCredit) {
        m_hadLowCredit = true;
        Database::instance()->execute(
            QString("UPDATE Users SET HadLowCredit = TRUE WHERE UserID = '%1'")
            .arg(m_id)
        );
    }
}

void User::setHadLowCredit(bool had)
{
    m_hadLowCredit = had;
}

void User::addCreditScore(int points)
{
    setCreditScore(m_creditScore + points);
}

void User::deductCreditScore(int points)
{
    setCreditScore(m_creditScore - points);
}

bool User::canBorrow() const
{
    // 信用分低于90分不能借书
    return m_creditScore >= 90;
}

bool User::canUpgrade() const
{
    // 曾经信用分低于90分不能升级
    return !m_hadLowCredit;
}

QString User::generateUserId()
{
    static int counter = 1;
    return QString("119%1").arg(counter++, 3, 10, QChar('0'));
}
QString User::id() const {
    return m_id;
}

QString User::email() const {
    return m_email;
}

QString User::password() const {
    return m_password;
}

QString User::name() const {
    return m_name;
}

User::UserType User::type() const {
    return m_type;
}

float User::readingHours() const {
    return m_readingHours;
}

double User::fines() const {
    return m_fines;
}

int User::creditScore() const {
    return m_creditScore;
}

bool User::hadLowCredit() const {
    return m_hadLowCredit;
}
