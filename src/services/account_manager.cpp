#include "account_manager.h"

#include "dbus/deepinid_interface.h"
#include "dbus/dbus_consts.h"

namespace dstore
{

namespace
{
const char kDeepinIDDbusService[] = "com.deepin.deepinid";
const char kDeepinIDDbusPath[] = "/com/deepin/deepinid";
}

class AccountManagerPrivate
{
public:
    AccountManagerPrivate(AccountManager *parent) : q_ptr(parent)
    {

    }

    com::deepin::deepinid *deepinid_interface_ = nullptr;

    AccountManager *q_ptr;
    Q_DECLARE_PUBLIC(AccountManager)
};

AccountManager::AccountManager(QObject *parent)
    : QObject(parent), dd_ptr(new AccountManagerPrivate(this))
{
    Q_D(AccountManager);

    d->deepinid_interface_ = new com::deepin::deepinid(
        kDeepinIDDbusService,
        kDeepinIDDbusPath,
        QDBusConnection::sessionBus(),
        this);

    auto userInfo = d->deepinid_interface_->userInfo();
    connect(d->deepinid_interface_, &com::deepin::deepinid::UserInfoChanged,
            this, &AccountManager::userInfoChanged);
//    connect(d->deepinid_interface_, &com::deepin::deepinid::UserInfoChanged,
//    this, [&](DVariantMap userInfo) {
//        qDebug() << userInfo;
//    });
}

AccountManager::~AccountManager() = default;

void AccountManager::authorize(const QString &clientID,
                               const QStringList &scopes,
                               const QString &callback,
                               const QString &state)
{
    QList<QVariant> argumentList;

    static QDBusInterface interface("com.deepin.deepinid.Client",
                                    "/com/deepin/deepinid/Client",
                                    "com.deepin.deepinid.Client");
    argumentList << clientID;
    argumentList << kAppStoreDbusService;
    argumentList << kAppStoreDbusPath;
    argumentList << kAppStoreDbusInterface;
    interface.callWithArgumentList(QDBus::NoBlock, "Register", argumentList);

    argumentList = {};
    argumentList << clientID;
    argumentList << scopes;
    argumentList << callback;
    argumentList << state;
    interface.callWithArgumentList(QDBus::NoBlock, "Authorize", argumentList);
}

QVariantMap AccountManager::getUserInfo() const
{
    Q_D(const AccountManager);
    return d->deepinid_interface_->userInfo();
}

void AccountManager::logout()
{
    Q_D(AccountManager);
    d->deepinid_interface_->Logout();
}

} // namespace dstore
