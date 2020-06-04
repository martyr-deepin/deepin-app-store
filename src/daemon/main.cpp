#include <QDebug>
#include <QCoreApplication>
#include <QtDBus/QtDBus>
#include <DLog>

#include "settings/settingservice.h"
#include "pkgmanager/pkgmanagerservice.h"
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    QCoreApplication app(argc, argv);
    app.setOrganizationName("deepin");

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();

    if (!QDBusConnection::sessionBus().isConnected()) {
        qDebug() << "QDBusConnection::systemBus().isConnected() failed";
        return 0x0001;
    }

    if (!QDBusConnection::sessionBus().registerService(STOREDAEMON_SERVICE)) {
        qDebug() << "registerService Error" << QDBusConnection::sessionBus().lastError();
        exit(0x0002);
    }

    SettingService setting;
    PkgManagerService pkgManager;

    if (!QDBusConnection::sessionBus().registerObject(STOREDAEMON_SETTINGS_PATH,  &setting,
            QDBusConnection::ExportAllSlots/* |
            QDBusConnection::ExportAllSignals |
            QDBusConnection::ExportAllProperties*/)) {
        qDebug() << "registerObject setting dbus Error" << QDBusConnection::sessionBus().lastError();
        exit(0x0003);
    }

    if (!QDBusConnection::sessionBus().registerObject(LASTOREDAEMON_DEB_PATH,  &pkgManager,
            QDBusConnection::ExportAllSlots |
            QDBusConnection::ExportAllSignals |
            QDBusConnection::ExportAllProperties)) {
        qDebug() << "registerObject setting dbus Error" << QDBusConnection::sessionBus().lastError();
        exit(0x0004);
    }
    return app.exec();
}
