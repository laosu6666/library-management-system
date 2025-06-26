// src/database.cpp
#include "database.h"

Database* Database::m_instance = nullptr;

Database::Database(QObject *parent) : QObject(parent)
{
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("library_system");
    db.setUserName("root");
    db.setPassword("password");
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

    // 确保新字段存在
    execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS CreditScore INT DEFAULT 100");
    execute("ALTER TABLE Users ADD COLUMN IF NOT EXISTS HadLowCredit BOOLEAN DEFAULT FALSE");
    execute("ALTER TABLE BorrowRecords ADD COLUMN IF NOT EXISTS CreditDeduction INT DEFAULT 0");

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

QString Database::escapeString(const QString &input)
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
