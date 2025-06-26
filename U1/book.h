// include/book.h
#ifndef BOOK_H
#define BOOK_H

#include <QObject>
#include <QString>
#include <QDate>

class Book : public QObject
{
    Q_OBJECT
public:
    explicit Book(const QString &isbn, const QString &title,
                 const QString &author, int totalCopies,
                 const QString &publisher = "",
                 const QDate &publishDate = QDate(),
                 double price = 0.0,
                 const QString &introduction = "",
                 QObject *parent = nullptr);

    QString isbn() const;
    QString title() const;
    QString author() const;
    QString publisher() const;
    QDate publishDate() const;
    double price() const;
    QString introduction() const;
    int totalCopies() const;
    int availableCopies() const;

    bool borrow();
    bool returnBook();
    bool reserve();
    void cancelReservation();

    static double calculateAverageRating(const QString &isbn);

private:
    QString m_isbn;
    QString m_title;
    QString m_author;
    QString m_publisher;
    QDate m_publishDate;
    double m_price;
    QString m_introduction;
    int m_totalCopies;
    int m_availableCopies;
};

#endif // BOOK_H
