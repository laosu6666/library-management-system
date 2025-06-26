// src/library.cpp
#include "library.h"
#include "database.h"
#include <QSqlQuery>
#include <QDate>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>

Library::Library(QObject *parent) : QObject(parent)
{
    // 启动定时检查逾期图书
    QTimer::singleShot(0, this, &Library::checkOverdueBooks);
}

User* Library::registerUser(const QString &email, const QString &password,
                           const QString &name)
{
    QString userId = User::generateUserId();

    QString query = QString(
        "INSERT INTO Users (UserID, Email, Password, Name) "
        "VALUES ('%1', '%2', '%3', '%4')"
    ).arg(userId, email, password, name);

    if(Database::instance()->execute(query)) {
        return new User(userId, email, password, name);
    }
    return nullptr;
}

User* Library::authenticateUser(const QString &identifier, const QString &password)
{
    QString query;
    if(identifier.contains('@')) {
        // 使用邮箱登录
        query = QString(
            "SELECT * FROM Users WHERE Email = '%1' AND Password = '%2'"
        ).arg(Database::instance()->escapeString(identifier), password);
    } else {
        // 使用用户ID登录
        query = QString(
            "SELECT * FROM Users WHERE UserID = '%1' AND Password = '%2'"
        ).arg(identifier, password);
    }

    QSqlQuery q = Database::instance()->executeQuery(query);
    if(q.next()) {
        return new User(
            q.value("UserID").toString(),
            q.value("Email").toString(),
            q.value("Password").toString(),
            q.value("Name").toString(),
            q.value("Type").toString() == "Super" ? User::Super : User::Normal,
            q.value("TotalReadingHours").toFloat(),
            q.value("Fines").toDouble(),
            q.value("CreditScore").toInt(),
            q.value("HadLowCredit").toBool()
        );
    }
    return nullptr;
}

bool Library::borrowBook(const QString &userId, const QString &isbn)
{
    // 检查用户和图书是否存在
    User* user = findUserById(userId);
    Book* book = findBookByIsbn(isbn);
    if(!user || !book) return false;

    // 检查用户信用分
    if(!user->canBorrow()) {
        QMessageBox::warning(nullptr, "借阅失败",
            "您的信用分低于90分，暂时无法借书\n"
            "请通过缴费提升信用分");
        return false;
    }

    // 检查用户当前借阅数量
    int currentBorrowCount = getCurrentBorrowCount(userId);
    if(currentBorrowCount >= user->maxBorrowCount()) {
        QMessageBox::warning(nullptr, "借阅失败",
            QString("您已达到最大借阅数量 (%1 本)").arg(user->maxBorrowCount()));
        return false;
    }

    // 检查是否有可借副本
    if(book->availableCopies() <= 0) {
        QMessageBox::warning(nullptr, "借阅失败", "该图书已无可用副本");
        return false;
    }

    // 借阅图书
    if(!book->borrow()) return false;

    // 创建借阅记录
    QDate borrowDate = QDate::currentDate();
    QDate dueDate = borrowDate.addDays(user->borrowDays()); // 根据用户类型设置借阅期限

    QString query = QString(
        "INSERT INTO BorrowRecords (UserID, ISBN, BorrowDate, DueDate) "
        "VALUES ('%1', '%2', '%3', '%4')"
    ).arg(userId, isbn, borrowDate.toString("yyyy-MM-dd"), dueDate.toString("yyyy-MM-dd"));

    if(Database::instance()->execute(query)) {
        QMessageBox::information(nullptr, "借阅成功",
            QString("借阅成功！请于 %1 前归还").arg(dueDate.toString("yyyy年MM月dd日")));
        return true;
    }
    return false;
}

bool Library::returnBook(const QString &userId, const QString &isbn)
{
    // 获取借阅记录
    BorrowRecord record = getBorrowRecord(userId, isbn);
    if(record.recordId == -1) {
        return false;
    }

    // 更新归还日期
    QDate returnDate = QDate::currentDate();
    QString query = QString(
        "UPDATE BorrowRecords SET ReturnDate = '%1' "
        "WHERE RecordID = %2"
    ).arg(returnDate.toString("yyyy-MM-dd")).arg(record.recordId);

    if(!Database::instance()->execute(query)) {
        return false;
    }

    // 增加图书可用副本
    Book* book = findBookByIsbn(isbn);
    if(book) {
        book->returnBook();
    }

    // 计算逾期罚款和信用分扣除
    if(returnDate > record.dueDate) {
        int daysOverdue = record.dueDate.daysTo(returnDate);
        double fine = daysOverdue * 0.5; // 每天0.5元罚款

        // 更新罚款
        query = QString(
            "UPDATE BorrowRecords SET Fine = %1 "
            "WHERE RecordID = %2"
        ).arg(fine).arg(record.recordId);
        Database::instance()->execute(query);

        // 更新用户罚款总额
        User* user = findUserById(userId);
        if(user) {
            user->addFine(fine);
        }

        // 计算信用分扣除
        calculateCreditDeduction(userId, record.dueDate);
    }

    return true;
}

void Library::calculateCreditDeduction(const QString &userId, const QDate &dueDate)
{
    QDate returnDate = QDate::currentDate();
    int daysOverdue = dueDate.daysTo(returnDate);
    int deduction = 0;

    if(daysOverdue > 0 && daysOverdue <= 7) {
        deduction = 2; // 一周内扣2分
    } else if(daysOverdue > 7) {
        deduction = 5; // 超过一周扣5分
    }

    if(deduction > 0) {
        // 更新借阅记录的信用分扣除
        QString query = QString(
            "UPDATE BorrowRecords SET CreditDeduction = %1 "
            "WHERE UserID = '%2' AND DueDate = '%3' AND ReturnDate IS NOT NULL"
        ).arg(deduction).arg(userId).arg(dueDate.toString("yyyy-MM-dd"));
        Database::instance()->execute(query);

        // 更新用户信用分
        User* user = findUserById(userId);
        if(user) {
            user->deductCreditScore(deduction);
            QMessageBox::warning(nullptr, "信用分扣除",
                QString("逾期归还，信用分扣除 %1 分\n当前信用分: %2")
                .arg(deduction).arg(user->creditScore()));
        }
    }
}

void Library::checkOverdueBooks()
{
    // 每天检查一次逾期图书
    QTimer::singleShot(24 * 3600 * 1000, this, &Library::checkOverdueBooks);

    QString query = QString(
        "SELECT * FROM BorrowRecords "
        "WHERE ReturnDate IS NULL AND DueDate < '%1'"
    ).arg(QDate::currentDate().toString("yyyy-MM-dd"));

    QSqlQuery q = Database::instance()->executeQuery(query);
    while(q.next()) {
        QString userId = q.value("UserID").toString();
        QDate dueDate = q.value("DueDate").toDate();

        // 计算逾期天数
        int daysOverdue = dueDate.daysTo(QDate::currentDate());

        // 超过30天未还，额外扣信用分
        if(daysOverdue > 30) {
            User* user = findUserById(userId);
            if(user) {
                user->deductCreditScore(5);
                QMessageBox::warning(nullptr, "严重逾期",
                    QString("图书严重逾期超过30天，信用分扣除5分\n当前信用分: %1")
                    .arg(user->creditScore()));
            }
        }
    }
}

int Library::getCurrentBorrowCount(const QString &userId)
{
    QString query = QString(
        "SELECT COUNT(*) FROM BorrowRecords "
        "WHERE UserID = '%1' AND ReturnDate IS NULL"
    ).arg(userId);

    QSqlQuery q = Database::instance()->executeQuery(query);
    if(q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

int Library::getUserCreditScore(const QString &userId)
{
    QString query = QString(
        "SELECT CreditScore FROM Users WHERE UserID = '%1'"
    ).arg(userId);

    QSqlQuery q = Database::instance()->executeQuery(query);
    if(q.next()) {
        return q.value(0).toInt();
    }
    return 100; // 默认100分
}
