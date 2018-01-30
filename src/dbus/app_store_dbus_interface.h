/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp com.deepin.AppStore.xml -p app_store_dbus_interface -c AppstoreDBusInterface
 *
 * qdbusxml2cpp is Copyright (C) 2016 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef APP_STORE_DBUS_INTERFACE_H
#define APP_STORE_DBUS_INTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface com.deepin.AppStore
 */
class AppstoreDBusInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.deepin.AppStore"; }

public:
    AppstoreDBusInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~AppstoreDBusInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> OpenApp(const QString &in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("OpenApp"), argumentList);
    }

    inline QDBusPendingReply<> Raise()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Raise"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace com {
  namespace deepin {
    typedef ::AppstoreDBusInterface AppStore;
  }
}
#endif
