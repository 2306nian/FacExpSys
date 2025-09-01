// userdao.h
#ifndef USERDAO_H
#define USERDAO_H

#include <QObject>
#include <QString>
#include <QDateTime>

struct LoginResult {
    bool success;
    QString userType;
    QString message;
};

class UserDAO : public QObject
{
    Q_OBJECT

public:
    static UserDAO *instance();

    // 注册新用户
    bool registerUser(const QString &username,
                      const QString &password,
                      const QString &userType,
                      const QDateTime &createdAt);

    // 查询用户是否存在
    bool userExists(const QString &username);

    // 验证登录（用户名 + 密码）
    bool verifyUser(const QString &username, const QString &password);

    // 登陆逻辑函数
    LoginResult login(const QString &username, const QString &password);

    // 获取用户类型
    QString getUserType(const QString &username);

private:
    explicit UserDAO(QObject *parent = nullptr);
    static UserDAO *m_instance;

    // 内部：密码哈希函数
    QString hashPassword(const QString &password) const;
};

#endif // USERDAO_H
