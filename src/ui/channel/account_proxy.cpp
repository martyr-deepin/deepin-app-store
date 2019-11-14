#include "account_proxy.h"

#include "services/account_manager.h"

namespace dstore
{

AccountProxy::AccountProxy(QObject *parent)
    : QObject(parent)
{
    manager_ = new AccountManager(parent);
    connect(manager_, &AccountManager::userInfoChanged,
            this, &AccountProxy::userInfoChanged);
}

QVariantMap dstore::AccountProxy::getUserInfo() const
{
    return manager_->getUserInfo();
}

void AccountProxy::authorize(const QString &clientID,
                             const QStringList &scopes,
                             const QString &callback,
                             const QString &state)
{
    manager_->authorize(clientID, scopes, callback, state);
}

void dstore::AccountProxy::logout()
{
    manager_->logout();
}

} // namespace dstore
