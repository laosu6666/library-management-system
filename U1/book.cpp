// src/book.cpp
#include "book.h"
#include "database.h"

Book::Book(const QString &isbn, const QString &title, const QString &author,
           int totalCopies, const QString &publisher, const QDate &publishDate,
           double price, const QString &introduction, QObject *parent)
    : QObject(parent), m_isbn(isbn), m_title(title), m_author(author),
      m_publisher(publisher), m_publishDate(publishDate), m_price(price),
      m_introduction(introduction), m_totalCopies(totalCopies),
      m_availableCopies(totalCopies)
{
}

bool Book::borrow()
{
    if(m_availableCopies <= 0) return false;

    m_availableCopies--;

    // 更新数据库
    Database::instance()->execute(
        QString("UPDATE Books SET AvailableCopies = %1 WHERE ISBN = '%2'")
        .arg(m_availableCopies).arg(m_isbn)
    );

    return true;
}

bool Book::returnBook()
{
    if(m_availableCopies >= m_totalCopies) return false;

    m_availableCopies++;

    // 更新数据库
    Database::instance()->execute(
        QString("UPDATE Books SET AvailableCopies = %1 WHERE ISBN = '%2'")
        .arg(m_availableCopies).arg(m_isbn)
    );

    return true;
}

double Book::calculateAverageRating(const QString &isbn)
{
    QSqlQuery q = Database::instance()->executeQuery(
        QString("SELECT AVG(Rating) FROM Comments WHERE ISBN = '%1'").arg(isbn)
    );

    if(q.next()) {
        return q.value(0).toDouble();
    }
    return 0.0;
}
