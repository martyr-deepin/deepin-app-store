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

void AccountProxy::authorizationNotify(const int& timeOut){
    QStringList actions = QStringList() << "_open" << tr("Activate");
    QVariantMap hints;
    hints["x-deepin-action-_open"] = "dbus-send,--print-reply,--dest=com.deepin.license.activator,/com/deepin/license/activator,com.deepin.license.activator.Show";

    QList<QVariant> argumentList;
    argumentList << "uos-activator";
    argumentList << static_cast<uint>(0);
    argumentList << "uos-activator";
    argumentList << "";
    argumentList << tr("Your system is not activated. Please activate as soon as possible for normal use.");
    argumentList << actions;
    argumentList << hints;
    argumentList << static_cast<int>(timeOut);

    static QDBusInterface notifyApp("org.freedesktop.Notifications",
                                    "/org/freedesktop/Notifications",
                                    "org.freedesktop.Notifications");
    notifyApp.asyncCallWithArgumentList("Notify", argumentList);
}

} // namespace dstore
