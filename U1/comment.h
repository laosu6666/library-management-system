// include/comment.h
#ifndef COMMENT_H
#define COMMENT_H

#include <QObject>
#include <QString>
#include <QDateTime>

class Comment : public QObject
{
    Q_OBJECT
public:
    explicit Comment(int commentId, const QString &userId, const QString &isbn,
                    const QString &content, int rating,
                    const QDateTime &commentDate, QObject *parent = nullptr);

    int commentId() const;
    QString userId() const;
    QString isbn() const;
    QString content() const;
    int rating() const;
    QDateTime commentDate() const;

    static bool addCommentToDatabase(const QString &userId, const QString &isbn,
                                    const QString &content, int rating);
    static QList<Comment*> getCommentsForBook(const QString &isbn);
    static double getAverageRatingForBook(const QString &isbn);
    static QList<Comment*> getAdminCommentsForBook(const QString &isbn);

private:
    int m_commentId;
    QString m_userId;
    QString m_isbn;
    QString m_content;
    int m_rating;
    QDateTime m_commentDate;
};

#endif // COMMENT_HH
