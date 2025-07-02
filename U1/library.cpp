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
    // 检查邮箱是否已存在
    QString emailCheck = QString("SELECT COUNT(*) FROM Users WHERE Email = '%1'").arg(email);
    QSqlQuery q = Database::instance()->executeQuery(emailCheck);
    if(q.next() && q.value(0).toInt() > 0) {
        return nullptr; // 邮箱已存在
    }

    QString userId;
    int tryCount = 0;
    do {
        userId = User::generateUserId();
        QString checkQuery = QString("SELECT COUNT(*) FROM Users WHERE UserID = '%1'").arg(userId);
        QSqlQuery q = Database::instance()->executeQuery(checkQuery);
        if(q.next() && q.value(0).toInt() == 0) break;
        tryCount++;
        if(tryCount > 10) return nullptr; // 防止死循环
    } while(true);

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
// 查找用户
User* Library::findUserById(const QString &userId)
{
    QString query = QString("SELECT * FROM Users WHERE UserID = '%1'").arg(userId);
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

// 查找图书
Book* Library::findBookByIsbn(const QString &isbn)
{
    QString query = QString("SELECT * FROM Books WHERE ISBN = '%1'").arg(isbn);
    QSqlQuery q = Database::instance()->executeQuery(query);
    if(q.next()) {
        return new Book(
            q.value("ISBN").toString(),
            q.value("Title").toString(),
            q.value("Author").toString(),
            q.value("TotalCopies").toInt(),
            q.value("Publisher").toString(),
            q.value("PublishDate").toDate(),
            q.value("Price").toDouble(),
            q.value("Introduction").toString()
        );
    }
    return nullptr;
}

// 删除用户
bool Library::deleteUser(const QString &userId)
{
    QString query = QString("DELETE FROM Users WHERE UserID = '%1'").arg(userId);
    return Database::instance()->execute(query);
}

// 添加图书
bool Library::addBook(const QString &isbn, const QString &title, const QString &author, int totalCopies,
                      const QString &publisher, const QDate &publishDate, double price, const QString &introduction)
{
    // 1. 检查Books表是否已存在该ISBN
    QString checkSql = QString("SELECT COUNT(*) FROM Books WHERE ISBN='%1'").arg(isbn);
    QSqlQuery q = Database::instance()->executeQuery(checkSql);
    bool exists = (q.next() && q.value(0).toInt() > 0);

    if (!exists) {
        // 新书，插入Books表
        QString sql = QString("INSERT INTO Books (ISBN, Title, Author, TotalCopies, AvailableCopies, Publisher, PublishDate, Price, Introduction) "
                              "VALUES ('%1', '%2', '%3', %4, %4, '%5', '%6', %7, '%8')")
                .arg(isbn, title, author)
                .arg(totalCopies)
                .arg(publisher)
                .arg(publishDate.toString("yyyy-MM-dd"))
                .arg(price)
                .arg(introduction);
        if (!Database::instance()->execute(sql)) return false;
    } else {
        // 已有该书，更新总数和可借数
        QString updateSql = QString("UPDATE Books SET TotalCopies = TotalCopies + %1, AvailableCopies = AvailableCopies + %1 WHERE ISBN = '%2'")
                .arg(totalCopies).arg(isbn);
        if (!Database::instance()->execute(updateSql)) return false;
    }

    // 2. 为每个副本生成唯一编号并插入BookCopies表
    // 查询当前已有多少副本
    QString countSql = QString("SELECT COUNT(*) FROM BookCopies WHERE ISBN='%1'").arg(isbn);
    QSqlQuery countQ = Database::instance()->executeQuery(countSql);
    int start = 0;
    if (countQ.next()) start = countQ.value(0).toInt();

    for (int i = 1; i <= totalCopies; ++i) {
        QString copyNum = QString("%1").arg(start + i, 3, 10, QChar('0')); // 001, 002, ...
        QString copyID = QString("%1-%2").arg(isbn).arg(copyNum);
        QString insertCopy = QString("INSERT INTO BookCopies (CopyID, ISBN, Status) VALUES ('%1', '%2', 'Available')")
                .arg(copyID, isbn);
        Database::instance()->execute(insertCopy);
    }

    return true;
}

// 移除图书
bool Library::removeBook(const QString &isbn)
{
    // 先检查是否有未归还的借阅记录
    QString check = QString("SELECT COUNT(*) FROM BorrowRecords WHERE ISBN='%1' AND ReturnDate IS NULL").arg(isbn);
    QSqlQuery q = Database::instance()->executeQuery(check);
    if(q.next() && q.value(0).toInt() > 0) {
        return false; // 有未归还，不能删除
    }
    // 删除图书
    QString del = QString("DELETE FROM Books WHERE ISBN='%1'").arg(isbn);
    return Database::instance()->execute(del);
}

// 续借图书
bool Library::renewBook(const QString &userId, const QString &isbn)
{
    BorrowRecord record = getBorrowRecord(userId, isbn);
    if(record.recordId == -1) return false;

    QDate newDueDate = record.dueDate.addDays(14); // 默认续借2周
    QString query = QString(
        "UPDATE BorrowRecords SET DueDate = '%1' WHERE RecordID = %2"
    ).arg(newDueDate.toString("yyyy-MM-dd")).arg(record.recordId);

    return Database::instance()->execute(query);
}

// 预约图书
bool Library::reserveBook(const QString &userId, const QString &isbn)
{
    QString query = QString(
        "INSERT INTO Reservations (UserID, ISBN, ReserveDate) VALUES ('%1', '%2', '%3')"
    ).arg(userId, isbn, QDate::currentDate().toString("yyyy-MM-dd"));
    return Database::instance()->execute(query);
}

// 取消预约
bool Library::cancelReservation(const QString &userId, const QString &isbn)
{
    QString query = QString(
        "DELETE FROM Reservations WHERE UserID = '%1' AND ISBN = '%2'"
    ).arg(userId, isbn);
    return Database::instance()->execute(query);
}

// 添加评论
bool Library::addComment(const QString &userId, const QString &isbn,
                        const QString &comment, int rating)
{
    QString query = QString(
        "INSERT INTO Comments (UserID, ISBN, Comment, Rating, Date) VALUES ('%1', '%2', '%3', %4, '%5')"
    ).arg(userId, isbn, comment).arg(rating).arg(QDate::currentDate().toString("yyyy-MM-dd"));
    return Database::instance()->execute(query);
}

// 搜索图书
QList<Book*> Library::searchBooks(const QString &keyword)
{
    QList<Book*> books;
    QString query = QString(
        "SELECT * FROM Books WHERE Title LIKE '%%1%' OR Author LIKE '%%1%' OR ISBN LIKE '%%1%'"
    ).arg(keyword);

    QSqlQuery q = Database::instance()->executeQuery(query);
    while(q.next()) {
        books.append(new Book(
            q.value("ISBN").toString(),
            q.value("Title").toString(),
            q.value("Author").toString(),
            q.value("TotalCopies").toInt(),
            q.value("Publisher").toString(),
            q.value("PublishDate").toDate(),
            q.value("Price").toDouble(),
            q.value("Introduction").toString()
        ));
    }
    return books;
}

// 获取高评分图书
QList<Book*> Library::getTopRatedBooks(int limit)
{
    QList<Book*> books;
    QString query = QString(
        "SELECT ISBN, AVG(Rating) as AvgRating FROM Comments GROUP BY ISBN ORDER BY AvgRating DESC LIMIT %1"
    ).arg(limit);

    QSqlQuery q = Database::instance()->executeQuery(query);
    while(q.next()) {
        Book* book = findBookByIsbn(q.value("ISBN").toString());
        if(book) books.append(book);
    }
    return books;
}

// 获取用户借阅的所有图书
QList<Book*> Library::getBooksBorrowedByUser(const QString &userId)
{
    QList<Book*> books;
    QString query = QString(
        "SELECT ISBN FROM BorrowRecords WHERE UserID = '%1' AND ReturnDate IS NULL"
    ).arg(userId);

    QSqlQuery q = Database::instance()->executeQuery(query);
    while(q.next()) {
        Book* book = findBookByIsbn(q.value("ISBN").toString());
        if(book) books.append(book);
    }
    return books;
}

// 获取用户罚款
double Library::getUserFines(const QString &userId)
{
    QString query = QString(
        "SELECT Fines FROM Users WHERE UserID = '%1'"
    ).arg(userId);

    QSqlQuery q = Database::instance()->executeQuery(query);
    if(q.next()) {
        return q.value(0).toDouble();
    }
    return 0.0;
}

// 支付罚款
bool Library::payFines(const QString &userId, double amount)
{
    QString query = QString(
        "UPDATE Users SET Fines = Fines - %1 WHERE UserID = '%2'"
    ).arg(amount).arg(userId);
    return Database::instance()->execute(query);
}

// 获取所有用户
QList<User*> Library::getAllUsers()
{
    QList<User*> users;
    QSqlQuery q = Database::instance()->executeQuery("SELECT * FROM Users");
    while(q.next()) {
        users.append(new User(
            q.value("UserID").toString(),
            q.value("Email").toString(),
            q.value("Password").toString(),
            q.value("Name").toString(),
            q.value("Type").toString() == "Super" ? User::Super : User::Normal,
            q.value("CreditScore").toInt(),
            q.value("HadLowCredit").toBool(),
            q.value("TotalReadingHours").toFloat(),
            q.value("Fines").toDouble()
        ));
    }
    return users;
}

// 获取借阅记录
QList<Library::BorrowRecord> Library::getBorrowRecords(const QString &isbn)
{
    QList<BorrowRecord> records;
    QString query;
    if(isbn.isEmpty()) {
        query = "SELECT * FROM BorrowRecords";
    } else {
        query = QString("SELECT * FROM BorrowRecords WHERE ISBN = '%1'").arg(isbn);
    }
    QSqlQuery q = Database::instance()->executeQuery(query);
    while(q.next()) {
        BorrowRecord record;
        record.recordId = q.value("RecordID").toInt();
        record.userId = q.value("UserID").toString();
        record.isbn = q.value("ISBN").toString();
        record.borrowDate = q.value("BorrowDate").toDate();
        record.dueDate = q.value("DueDate").toDate();
        record.returnDate = q.value("ReturnDate").toDate();
        record.fine = q.value("Fine").toDouble();
        record.creditDeduction = q.value("CreditDeduction").toInt();
        records.append(record);
    }
    return records;
}

// 更新用户信用分
bool Library::updateCreditScore(const QString &userId, int score)
{
    QString query = QString(
        "UPDATE Users SET CreditScore = %1 WHERE UserID = '%2'"
    ).arg(score).arg(userId);
    return Database::instance()->execute(query);
}



// 获取单条借阅记录
Library::BorrowRecord Library::getBorrowRecord(const QString &userId, const QString &isbn)
{
    BorrowRecord record;
    record.recordId = -1;
    QString query = QString(
        "SELECT * FROM BorrowRecords WHERE UserID = '%1' AND ISBN = '%2' AND ReturnDate IS NULL"
    ).arg(userId, isbn);
    QSqlQuery q = Database::instance()->executeQuery(query);
    if(q.next()) {
        record.recordId = q.value("RecordID").toInt();
        record.userId = q.value("UserID").toString();
        record.isbn = q.value("ISBN").toString();
        record.borrowDate = q.value("BorrowDate").toDate();
        record.dueDate = q.value("DueDate").toDate();
        record.returnDate = q.value("ReturnDate").toDate();
        record.fine = q.value("Fine").toDouble();
        record.creditDeduction = q.value("CreditDeduction").toInt();
    }
    return record;
}


