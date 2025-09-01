// userdao.cpp
#include "userdao.h"
#include "database.h"
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QVariant>
#include <QDebug>
#include <QSqlError>

UserDAO *UserDAO::m_instance = nullptr;

UserDAO *UserDAO::instance()
{
    if (!m_instance) {
        m_instance = new UserDAO;
    }
    return m_instance;
}

UserDAO::UserDAO(QObject *parent)
    : QObject(parent)
{
}

QString UserDAO::hashPassword(const QString &password) const
{
    // 使用 SHA-256 哈希（简单安全）
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

bool UserDAO::registerUser(const QString &username,
                           const QString &password,
                           const QString &userType,
                           const QDateTime &createdAt)
{
    if (username.isEmpty() || password.isEmpty()) {
        qWarning() << "Username or password is empty";
        return false;
    }

    if (userType != "client" && userType != "expert") {
        qWarning() << "Invalid user type:" << userType;
        return false;
    }

    QSqlDatabase db = Database::instance()->db();
    QSqlQuery query(db);

    // 检查用户是否已存在
    if (userExists(username)) {
        qWarning() << "User already exists:" << username;
        return false;
    }

    query.prepare(R"(
        INSERT INTO users (username, password_hash, user_type, created_at)
        VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(username);
    query.addBindValue(hashPassword(password));
    query.addBindValue(userType);
    query.addBindValue(createdAt);

    if (!query.exec()) {
        qWarning() << "Failed to insert user:" << query.lastError().text();
        return false;
    }

    qDebug() << "User registered:" << username << "(" << userType << ")";
    return true;
}

bool UserDAO::userExists(const QString &username)
{
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT 1 FROM users WHERE username = ?");
    query.addBindValue(username);
    return query.exec() && query.next();
}

bool UserDAO::verifyUser(const QString &username, const QString &password)
{
    QSqlDatabase db = Database::instance()->db();
    QSqlQuery query(db);

    query.prepare("SELECT password_hash FROM users WHERE username = ?");
    query.addBindValue(username);

    if (query.exec() && query.next()) {
        QString storedHash = query.value(0).toString();
        QString inputHash = hashPassword(password);
        if (storedHash == inputHash) {
            // 密码正确，更新最后登录时间
            QSqlQuery updateQuery(db);
            updateQuery.prepare("UPDATE users SET last_login = ? WHERE username = ?");
            updateQuery.addBindValue(QDateTime::currentDateTime());
            updateQuery.addBindValue(username);
            updateQuery.exec();  // 不严格检查是否成功

            return true;
        }
    }
    return false;
}

QString UserDAO::getUserType(const QString &username)
{
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT user_type FROM users WHERE username = ?");
    query.addBindValue(username);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

LoginResult UserDAO::login(const QString &username, const QString &password)
{
    if (username.isEmpty() || password.isEmpty())
        return {false, "", "Username or password cannot be empty"};

    if (!userExists(username))
        return {false, "", "User does not exist"};

    if (!verifyUser(username, password))
        return {false, "", "Incorrect password"};

    QString userType = getUserType(username);
    return {true, userType, "Login successful"};
}
