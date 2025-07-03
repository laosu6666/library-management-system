// include/library.h
#ifndef LIBRARY_H
#define LIBRARY_H

#include <QObject>
#include <QList>
#include <QDate>
#include "user.h"
#include "book.h"
#include "comment.h"

class Library : public QObject
{
    Q_OBJECT
public:
    struct BorrowRecord {
        int recordId;
        QString userId;
        QString isbn;
        QDate borrowDate;
        QDate dueDate;
        QDate returnDate;
        double fine;
        int creditDeduction;
    };

    explicit Library(QObject *parent = nullptr);

    // 用户管理
    User* registerUser(const QString &email, const QString &password,
                      const QString &name);
    User* authenticateUser(const QString &identifier, const QString &password);
    bool deleteUser(const QString &userId);

    // 图书管理
    bool addBook(const QString &isbn, const QString &title, const QString &author, int copies,
             const QString &publisher = "",
             const QDate &publishDate = QDate(),
             double price = 0.0,
             const QString &introduction = "");
    bool removeBook(const QString &isbn);

    // 借阅管理
    bool borrowBook(const QString &userId, const QString &isbn);
    bool returnBook(const QString &userId, const QString &isbn);
    bool renewBook(const QString &userId, const QString &isbn);
    bool reserveBook(const QString &userId, const QString &isbn);
    bool cancelReservation(const QString &userId, const QString &isbn);
    BorrowRecord getBorrowRecord(const QString &userId, const QString &isbn);

    // 评论与评分
    bool addComment(const QString &userId, const QString &isbn,
                   const QString &comment, int rating);

    // 查询功能
    QList<Book*> searchBooks(const QString &keyword);
    QList<Book*> getTopRatedBooks(int limit = 10);
    QList<Book*> getBooksBorrowedByUser(const QString &userId);
    double getUserFines(const QString &userId);
    bool payFines(const QString &userId, double amount);
    int getUserCreditScore(const QString &userId);

    // 管理员功能
    QList<User*> getAllUsers();
    QList<BorrowRecord> getBorrowRecords(const QString &isbn = "");
    bool updateCreditScore(const QString &userId, int score);

    // 信用分管理
    void checkOverdueBooks();
    void calculateCreditDeduction(const QString &userId, const QDate &dueDate);

      int getCurrentBorrowCount(const QString &userId);
      User* findUserById(const QString &userId);
      Book* findBookByIsbn(const QString &isbn);


private:


};

#endif // LIBRARY_H
