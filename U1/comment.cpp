// src/comment.cpp
#include "comment.h"
#include "database.h"
#include "user.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

Comment::Comment(int commentId, const QString &userId, const QString &isbn,
                 const QString &content, int rating,
                 const QDateTime &commentDate, QObject *parent)
    : QObject(parent), m_commentId(commentId), m_userId(userId), m_isbn(isbn),
      m_content(content), m_rating(rating), m_commentDate(commentDate)
{
}

bool Comment::addCommentToDatabase(const QString &userId, const QString &isbn,
                                  const QString &content, int rating)
{
    // 验证评分范围
    if(rating < 1 || rating > 5) {
        qDebug() << "Invalid rating value:" << rating;
        return false;
    }

    // 获取当前时间
    QDateTime now = QDateTime::currentDateTime();

    // 构建SQL查询
    QString query = QString(
        "INSERT INTO Comments (UserID, ISBN, Comment, Rating, CommentDate) "
        "VALUES ('%1', '%2', '%3', %4, '%5')"
    ).arg(
        Database::instance()->escapeString(userId),
        Database::instance()->escapeString(isbn),
        Database::instance()->escapeString(content),
        QString::number(rating),
        now.toString("yyyy-MM-dd hh:mm:ss")
    );

    // 执行查询
    return Database::instance()->execute(query);
}

QList<Comment*> Comment::getCommentsForBook(const QString &isbn)
{
    QList<Comment*> comments;

    QString query = QString(
        "SELECT c.*, u.Name, u.Type "
        "FROM Comments c "
        "JOIN Users u ON c.UserID = u.UserID "
        "WHERE c.ISBN = '%1' "
        "ORDER BY c.CommentDate DESC"
    ).arg(Database::instance()->escapeString(isbn));

    QSqlQuery q = Database::instance()->executeQuery(query);
    while(q.next()) {
        Comment* comment = new Comment(
            q.value("CommentID").toInt(),
            q.value("UserID").toString(),
            q.value("ISBN").toString(),
            q.value("Comment").toString(),
            q.value("Rating").toInt(),
            q.value("CommentDate").toDateTime()
        );

        // 存储用户信息
        comment->setProperty("userName", q.value("Name").toString());
        comment->setProperty("userType", q.value("Type").toString());

        comments.append(comment);
    }
    return comments;
}

double Comment::getAverageRatingForBook(const QString &isbn)
{
    QString query = QString(
        "SELECT AVG(Rating) as AvgRating FROM Comments WHERE ISBN = '%1'"
    ).arg(Database::instance()->escapeString(isbn));

    QSqlQuery q = Database::instance()->executeQuery(query);
    if(q.next()) {
        return q.value("AvgRating").toDouble();
    }
    return 0.0;
}

QList<Comment*> Comment::getAdminCommentsForBook(const QString &isbn)
{
    QList<Comment*> adminComments;

    QString query = QString(
        "SELECT c.*, u.Name "
        "FROM Comments c "
        "JOIN Users u ON c.UserID = u.UserID "
        "WHERE c.ISBN = '%1' AND u.Type = 'Super' "
        "ORDER BY c.CommentDate DESC"
    ).arg(Database::instance()->escapeString(isbn));

    QSqlQuery q = Database::instance()->executeQuery(query);
    while(q.next()) {
        Comment* comment = new Comment(
            q.value("CommentID").toInt(),
            q.value("UserID").toString(),
            q.value("ISBN").toString(),
            q.value("Comment").toString(),
            q.value("Rating").toInt(),
            q.value("CommentDate").toDateTime()
        );

        // 存储管理员名称
        comment->setProperty("adminName", q.value("Name").toString());

        adminComments.append(comment);
    }
    return adminComments;
}
