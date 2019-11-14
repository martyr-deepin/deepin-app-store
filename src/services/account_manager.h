#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QVariantMap>

namespace dstore
{

class AccountManagerPrivate;
class AccountManager: public QObject
{
Q_OBJECT
public:
    explicit AccountManager(QObject *parent = Q_NULLPTR);
    ~AccountManager() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void userInfoChanged(const QVariantMap &info);

public Q_SLOTS:
    QVariantMap getUserInfo() const;

    void authorize(const QString &clientID,
                   const QStringList &scopes,
                   const QString &callback,
                   const QString &state);

    void logout();

private:
    QScopedPointer<AccountManagerPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), AccountManager)
};

} // namespace dstore
