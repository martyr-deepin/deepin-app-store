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

#include "services/dbus_manager.h"

#include <QCommandLineParser>
#include <QDebug>
#include <QtDBus>
#include <QUuid>

#include "dbus/app_store_dbus_adapter.h"
#include "dbus/app_store_dbus_interface.h"
#include "dbus/dbus_consts.h"

namespace dstore
{


DBusManager::DBusManager(QObject *parent)
    : QObject(parent)
{
}

DBusManager::~DBusManager() = default;

bool DBusManager::registerDBus()
{
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.parse(qApp->arguments());

    const QStringList args = parser.positionalArguments();
    auto showAppName = args.value(0);

    // Register dbus service.
    QDBusConnection session_bus = QDBusConnection::sessionBus();

    if (session_bus.registerService(kAppStoreDbusService) &&
        session_bus.registerObject(kAppStoreDbusPath,
                                   this,
                                   QDBusConnection::ExportScriptableContents)) {
        if (!showAppName.isEmpty()){
            this->ShowAppDetail(showAppName);
        }
        return true;
    }

    qInfo() << "Failed to register dbus object" << session_bus.lastError();

    // Open app with dbus interface.
    if (!showAppName.isEmpty()) {
        auto *interface = new AppStoreDBusInterface(
            kAppStoreDbusService,
            kAppStoreDbusPath,
            session_bus,
            this
        );

        if (interface->isValid()) {
            // Only pass the first positional argument.
            interface->ShowAppDetail(showAppName);
        } else {
            interface->Raise();
        }
    }

    return false;
}

void DBusManager::Raise()
{
    Q_EMIT this->raiseRequested();
}

void DBusManager::ShowAppDetail(const QString &app_name)
{
    Q_EMIT this->showDetailRequested(app_name);
}

QVariantMap DBusManager::Install(const QString &appID)
{
    auto req = newRequest();
    qDebug() << "Install" << req->id << appID;
    req->data["app_id"] = appID;
    Q_EMIT this->requestInstallApp(req->id, appID);
    return QVariantMap();
}

QVariantMap DBusManager::Update(const QString &appID)
{
    auto req = newRequest();
    qDebug() << "Update" << req->id << appID;
    req->data["app_id"] = appID;
    Q_EMIT this->requestUpdateApp(req->id, appID);
    return QVariantMap();
}

QVariantMap DBusManager::UpdateAll()
{
    auto req = newRequest();
    qDebug() << "UpdateAll" << req->id;
    Q_EMIT this->requestUpdateAllApp(req->id);
    return QVariantMap();
}

QVariantMap DBusManager::Uninstall(const QString &appID)
{
    auto req = newRequest();
    qDebug() << "UninstallApp" << req->id << appID;
    req->data["app_id"] = appID;
    Q_EMIT this->requestUninstallApp(req->id, appID);
    return QVariantMap();
}

void DBusManager::onRequestFinished(const QString &reqID, const QVariantMap &result)
{
    qDebug() << "onRequestFinished" << reqID << result;

    // find request
    if (!requests.contains(reqID)) {
        qWarning() << "unknown request id";
        return;
    }

    auto req = requests[reqID];

    auto reply = req->msg.createReply();
    reply << result;

    QDBusConnection::sessionBus().send(reply);
}

RequestData *DBusManager::newRequest()
{
    auto reqID = QUuid::createUuid().toString();
    auto *req = new RequestData;
    req->id = reqID;
    req->msg = message();
    req->msg.setDelayedReply(true);
    requests[reqID] = req;
    return req;
}

void DBusManager::OnAuthorized(const QString &code, const QString &state)
{
    Q_EMIT this->requestAuthorized(code, state);
}

void DBusManager::OnCancel()
{
    qDebug() << "OnCancel";
}

}  // namespace dstore