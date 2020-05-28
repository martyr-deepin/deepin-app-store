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

#include "pkgmanagerservice.h"
#include <QtDBus>

PkgManagerService::PkgManagerService(QObject *parent) : QObject(parent)
{
    lastoreDaemon = new QDBusInterface("com.deepin.lastore",
                                   "/com/deepin/lastore",
                                   "com.deepin.lastore.Manager",
                                   QDBusConnection::systemBus());

    m_pMetaDataManager = new MetaDataManager(lastoreDaemon,this);
    connect(m_pMetaDataManager,SIGNAL(jobListChanged()),this,SIGNAL(jobListChanged()));

    //注册自定义dbus数据类型
    AppVersion::registerMetaType();
    InstalledAppInfo::registerMetaType();
    InstalledAppTimestamp::registerMetaType();

    //初始化获取任务列表缓存
//    QDBusMessage myDBusMessage;
//    QVariant reply =  lastoreDaemon->property("JobList");
//    QList<QVariant> list ;
//    list.append(reply);
//    qDebug()<<reply;
//    myDBusMessage.setArguments(list);
}

void PkgManagerService::CleanArchives()
{
    QDBusMessage reply = lastoreDaemon->call("CleanArchives");
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "D-Bus Error CleanArchives:" << reply.errorMessage();
    }
}

QDBusObjectPath PkgManagerService::FixError(const QString &errType)
{
    QList<QVariant> argumentList;
    argumentList << errType;

    const QDBusPendingReply<QDBusObjectPath> reply = lastoreDaemon->callWithArgumentList(QDBus::AutoDetect,
                         "FixError", argumentList);
    if (reply.isError()) {
        qDebug() << reply.error();
        return QDBusObjectPath();
    }

    return m_pMetaDataManager->addJob(reply.value());
}

QDBusObjectPath PkgManagerService::Install(const QString &localName, const QString &id)
{
    QList<QVariant> argumentList;
    argumentList << localName << id;

    const QDBusPendingReply<QDBusObjectPath> reply = lastoreDaemon->callWithArgumentList(QDBus::AutoDetect,
                         "InstallPackage", argumentList);
    if (reply.isError()) {
        qDebug() << reply.error();
        return QDBusObjectPath();
    }

    return m_pMetaDataManager->addJob(reply.value());
}

// ListInstalled will list all install package
InstalledAppInfoList PkgManagerService::ListInstalled()
{
    return m_pMetaDataManager->listInstalled();
}

qlonglong PkgManagerService::QueryDownloadSize(const QString &id)
{
    qlonglong size = m_pMetaDataManager->queryDownloadSize(id);
    if(size == 0) {
        QStringList partsList;
        partsList.append(id);
        QList<QVariant> argumentList;
        argumentList << partsList;
        qDebug()<<partsList;

        const QDBusPendingReply<qlonglong> reply = lastoreDaemon->callWithArgumentList(QDBus::AutoDetect,
                             "PackagesDownloadSize", argumentList);
        if (reply.isError()) {
            qDebug() << reply.error();
            return 0;
        }
        qDebug()<<reply.value();
        return reply.value();
    }else {
        return size;
    }
}

InstalledAppTimestampList PkgManagerService::QueryInstallationTime(const QStringList &idList)
{
    return  m_pMetaDataManager->getInstallationTimes(idList);
}

AppVersionList PkgManagerService::QueryVersion(const QStringList &idList)
{
    return  m_pMetaDataManager->queryVersion(idList);
}

QDBusObjectPath PkgManagerService::Remove(const QString &localName, const QString &id)
{
    QList<QVariant> argumentList;
    argumentList << localName << id;

    const QDBusPendingReply<QDBusObjectPath> reply = lastoreDaemon->callWithArgumentList(QDBus::AutoDetect,
                         "RemovePackage", argumentList);
    if (reply.isError()) {
        qDebug() << reply.error();
        return QDBusObjectPath();
    }

    return m_pMetaDataManager->addJob(reply.value());
}

void PkgManagerService::updateCacheList()
{
    m_pMetaDataManager->updateCacheList();
}

QList<QDBusObjectPath> PkgManagerService::jobList()
{
    qDebug()<<"get JobList....";
    return m_pMetaDataManager->getJobList();
}
