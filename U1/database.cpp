// src/database.cpp
#include "database.h"

Database* Database::m_instance = nullptr;

Database::Database(QObject *parent) : QObject(parent)
{
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("library_system");
    db.setUserName("root");
    db.setPassword("123456");
}

Database::~Database()
{
    if(db.isOpen()) db.close();
}

bool Database::initialize()
{
    if(!db.open()) {
        qDebug() << "Database connection error:" << db.lastError().text();
        return false;
    }

    // 创建表结构
    QStringList tables = {
        "CREATE TABLE IF NOT EXISTS Users ("
        "   UserID VARCHAR(6) PRIMARY KEY,"
        "   Email VARCHAR(50) UNIQUE NOT NULL,"
        "   Password VARCHAR(255) NOT NULL,"
        "   Name VARCHAR(50) NOT NULL,"
        "   Type ENUM('Normal', 'Super') DEFAULT 'Normal',"
        "   TotalReadingHours FLOAT DEFAULT 0.0,"
        "   Fines DECIMAL(10,2) DEFAULT 0.0,"
        "   CreditScore INT DEFAULT 100,"  // 信用分
        "   HadLowCredit BOOLEAN DEFAULT FALSE" // 是否曾低于90分
        ");",

        "CREATE TABLE IF NOT EXISTS Books ("
        "   ISBN VARCHAR(20) PRIMARY KEY,"
        "   Title VARCHAR(255) NOT NULL,"
        "   Author VARCHAR(100) NOT NULL,"
        "   Publisher VARCHAR(100),"
        "   PublishDate DATE,"
        "   Price DECIMAL(10,2),"
        "   Introduction TEXT,"
        "   TotalCopies INT NOT NULL,"
        "   AvailableCopies INT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS BorrowRecords ("
        "   RecordID INT AUTO_INCREMENT PRIMARY KEY,"
        "   UserID VARCHAR(6) NOT NULL,"
        "   ISBN VARCHAR(20) NOT NULL,"
        "   BorrowDate DATE NOT NULL,"
        "   DueDate DATE NOT NULL,"
        "   ReturnDate DATE,"
        "   Fine DECIMAL(10,2) DEFAULT 0.0,"
        "   CreditDeduction INT DEFAULT 0,"  // 信用分扣除
        "   FOREIGN KEY (UserID) REFERENCES Users(UserID),"
        "   FOREIGN KEY (ISBN) REFERENCES Books(ISBN)"
        ");",

        "CREATE TABLE IF NOT EXISTS Comments ("
        "   CommentID INT AUTO_INCREMENT PRIMARY KEY,"
        "   UserID VARCHAR(6) NOT NULL,"
        "   ISBN VARCHAR(20) NOT NULL,"
        "   Comment TEXT,"
        "   Rating INT CHECK (Rating BETWEEN 1 AND 5),"
        "   CommentDate DATETIME NOT NULL,"
        "   FOREIGN KEY (UserID) REFERENCES Users(UserID),"
        "   FOREIGN KEY (ISBN) REFERENCES Books(ISBN)"
        ");",

        "CREATE TABLE IF NOT EXISTS Reservations ("
        "   ReservationID INT AUTO_INCREMENT PRIMARY KEY,"
        "   UserID VARCHAR(6) NOT NULL,"
        "   ISBN VARCHAR(20) NOT NULL,"
        "   ReserveDate DATETIME NOT NULL,"
        "   Status ENUM('Pending', 'Fulfilled', 'Cancelled') DEFAULT 'Pending',"
        "   FOREIGN KEY (UserID) REFERENCES Users(UserID),"
        "   FOREIGN KEY (ISBN) REFERENCES Books(ISBN)"
        ");"
    };

    foreach (const QString &tableSql, tables) {
        if(!execute(tableSql)) {
            return false;
        }
    }

    // 确保新字段存在 - 使用更兼容的方式
    QStringList columnsToAdd = {
        "ALTER TABLE Users ADD COLUMN CreditScore INT DEFAULT 100",
        "ALTER TABLE Users ADD COLUMN HadLowCredit BOOLEAN DEFAULT FALSE",
        "ALTER TABLE Users ADD COLUMN Type VARCHAR(16) DEFAULT 'Normal'", // 新增
        "ALTER TABLE BorrowRecords ADD COLUMN CreditDeduction INT DEFAULT 0"
    };

    foreach (const QString &alterSql, columnsToAdd) {
        QSqlQuery q(db);
        if (!q.exec(alterSql)) {
            // 检查是否是"列已存在"的错误
            if (q.lastError().text().contains("Duplicate column name")) {
                qDebug() << "Column already exists, skipping:" << alterSql;
            } else {
                qDebug() << "Error adding column:" << q.lastError().text();
                qDebug() << "Query:" << alterSql;
                // 对于非重复列错误，可以选择记录但继续
            }
        }
    }

    // 插入管理员账号（如果不存在）
    QString adminEmail = "123456@163.com";
    QString adminCheck = QString("SELECT COUNT(*) FROM Users WHERE Email = '%1'").arg(adminEmail);
    QSqlQuery q = executeQuery(adminCheck);
    if(q.next() && q.value(0).toInt() == 0) {
        QString insertAdmin = QString(
            "INSERT INTO Users (UserID, Email, Password, Name, Type, TotalReadingHours, Fines, CreditScore, HadLowCredit) "
            "VALUES ('001', '%1', 'LLL123456', '001', 'Super', 0.0, 0.0, 120, FALSE)"
        ).arg(adminEmail);
        execute(insertAdmin);
    }

    return true;
}


bool Database::execute(const QString &query)
{
    QSqlQuery q(db);
    if(!q.exec(query)) {
        qDebug() << "Query error:" << q.lastError().text();
        qDebug() << "Query:" << query;
        return false;
    }
    return true;
}

QSqlQuery Database::executeQuery(const QString &query)
{
    QSqlQuery q(db);
    q.exec(query);
    return q;
}

QString Database::escapeString( QString input)
{
    return input.replace("'", "''");
}

Database* Database::instance()
{
    if(!m_instance) {
        m_instance = new Database();
    }
    return m_instance;
}
