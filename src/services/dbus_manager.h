/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEEPIN_APPSTORE_SERVICES_ARGS_PARSER_H
#define DEEPIN_APPSTORE_SERVICES_ARGS_PARSER_H

#include <QObject>
#include <QDBusContext>
#include <QVariantMap>
#include <QDBusMessage>

#include "dbus/dbus_variant/app_metadata.h"

namespace dstore
{

// TODO: move to common file
struct RequestData
{
    QString id;
    QVariantMap data;
    QDBusMessage msg;
};

// DBusManager implements AppStoreDBusInterface.
// Works in background thread.
class DBusManager: public QObject, protected QDBusContext
{
Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.AppStore")
public:
    explicit DBusManager(QObject *parent = nullptr);
    ~DBusManager() override;

    bool registerDBus();

public Q_SLOTS:
    void onRequestFinished(const QString &reqID, const QVariantMap &result);

Q_SIGNALS:
    void raiseRequested();
    void showDetailRequested(const QString &appName);

    void requestInstallApp(const QString &request_id, const QString &appID);
    void requestUpdateApp(const QString &request_id, const QString &appID);
    void requestUpdateAllApp(const QString &request_id);
    void requestUninstallApp(const QString &request_id, const QString &appID);

    void requestAuthorized(const QString &code, const QString &state);

    void requestOpenCategory(const QString &request_id, const QString &category);
    void requestOpenTag(const QString &request_id, const QString &tag);

    void appPayStatus(const QString& appID, const int& status);

public Q_SLOTS:
    // Implement AppStore dbus service.
    Q_SCRIPTABLE void Raise();
    Q_SCRIPTABLE void ShowAppDetail(const QString &appName);

    Q_SCRIPTABLE QVariantMap Install(const QString &appID);
    Q_SCRIPTABLE QVariantMap Update(const QString &appID);
    Q_SCRIPTABLE QVariantMap UpdateAll();
    Q_SCRIPTABLE QVariantMap Uninstall(const QString &appID);

    Q_SCRIPTABLE void OnAuthorized(const QString &code, const QString &state);
    Q_SCRIPTABLE void OnCancel();
    
    Q_SCRIPTABLE QVariantMap OpenCategory(const QString &category);
    Q_SCRIPTABLE QVariantMap OpenTag(const QString &tag);

private:
    RequestData *newRequest();

    QMap<QString, RequestData *> requests;
};

}  // namespace dstore

#endif  // DEEPIN_APPSTORE_SERVICES_ARGS_PARSER_H
