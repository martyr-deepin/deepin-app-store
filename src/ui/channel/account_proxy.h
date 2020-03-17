#pragma once

#include <QObject>
#include <QVariantMap>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <DNotifySender>

namespace dstore
{
class AccountManager;
class AccountProxy: public QObject
{
Q_OBJECT
public:
    explicit AccountProxy(QObject *parent = Q_NULLPTR);

Q_SIGNALS:
    // info of deepinid changed
    void userInfoChanged(const QVariantMap &info);

    // requestLogin from titlebar
    void requestLogin();

    // on oauth2 authorized flow finish
    void onAuthorized(const QString &code, const QString &state);

public Q_SLOTS:
    QVariantMap getUserInfo() const;

    void authorize(const QString &clientID,
                   const QStringList &scopes,
                   const QString &callback,
                   const QString &state);
    void logout();
    void authorizationNotify(const int& timeOut);

private:
    AccountManager *manager_;
};

} // namespace dstore
