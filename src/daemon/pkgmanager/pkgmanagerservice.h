/*
 * Copyright (C) 2019 ~ 2020 Union Tech Co., Ltd.
 *
 * Author:     zhoutao <zhoutao@uniontech.com>
 *
 * Maintainer: zhoutao <zhoutao@uniontech.com>
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

#ifndef PKGMANAGERSERVICE_H
#define PKGMANAGERSERVICE_H

#include <QObject>
#include <QDBusObjectPath>
#include "metadatamanager.h"

#include "../../dbus/dbus_variant/app_metadata.h"
#include "../../dbus/dbus_variant/app_version.h"
#include "../../dbus/dbus_variant/installed_app_info.h"
#include "../../dbus/dbus_variant/installed_app_timestamp.h"

class QDBusInterface;
class PkgManagerService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.AppStore.Backend.Deb")
    Q_PROPERTY(QList<QDBusObjectPath> JobList READ jobList NOTIFY jobListChanged)
public:
    explicit PkgManagerService(QObject *parent = nullptr);

signals:
    void jobListChanged();

public slots:
    void CleanArchives();
    QDBusObjectPath FixError(const QString &errType);
    QDBusObjectPath Install(const QString &localName, const QString &id);
    InstalledAppInfoList ListInstalled();
    qlonglong QueryDownloadSize(const QString &id);
    InstalledAppTimestampList QueryInstallationTime(const QStringList &idList);
    AppVersionList QueryVersion(const QStringList &idList);
    QDBusObjectPath Remove(const QString &localName, const QString &id);

    void lastoreJobListChanged(QString,QMap<QString,QVariant> ,QStringList);

    QList<QDBusObjectPath> jobList();

private:
    QDBusInterface  *lastoreDaemon = nullptr;
    MetaDataManager *m_pMetaDataManager;
};

#endif // PKGMANAGERSERVICE_H
