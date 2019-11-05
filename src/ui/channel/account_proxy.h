#pragma once

#include <QObject>
#include <QVariantMap>

namespace dstore
{
class AccountManager;
class AccountProxy: public QObject
{
Q_OBJECT
public:
    explicit AccountProxy(QObject *parent = Q_NULLPTR);

Q_SIGNALS:
    void userInfoChanged(const QVariantMap &info);
    void onAuthorized(const QString &code, const QString &state);

public Q_SLOTS:
    QVariantMap getUserInfo() const;

    QString getToken() const;

    void login();

    void logout();

    void authorize(const QString &clientID,
                   const QStringList &scopes,
                   const QString &callback,
                   const QString &state) {

    }

private:
    AccountManager *manager_;
};

} // namespace dstore
