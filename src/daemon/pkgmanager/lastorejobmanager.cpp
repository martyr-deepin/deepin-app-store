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
#include "lastorejobmanager.h"

#include <QDBusConnection>
#include "../dbus/daemondbus.h"
#include <QtDBus>

class LastoreJobManagerPrivate
{
public:
    LastoreJobManagerPrivate(LastoreJobManager *parent) :
        q_ptr(parent)
    {

    }
    QDBusInterface  *jobInterface = nullptr;

    LastoreJobManager *q_ptr;
    Q_DECLARE_PUBLIC(LastoreJobManager)
};

LastoreJobManager::LastoreJobManager(QString path, QObject *parent) :
    QObject(parent),
    m_dbusPath(path),
    d_ptr(new LastoreJobManagerPrivate(this))
{
    Q_D(LastoreJobManager);
    //开启一个新的任务会创建一个新的dbus接口
    d->jobInterface = new QDBusInterface("com.deepin.lastore",
                                         m_dbusPath.prepend("/com/deepin/lastore/Job"),
                                         "com.deepin.lastore.Job",
                                         QDBusConnection::systemBus());
}

LastoreJobManager::~LastoreJobManager()
{

}

bool LastoreJobManager::cancelable()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Cancelable").toBool();
}

qlonglong LastoreJobManager::createTime()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("CreateTime").toLongLong();
}

QString LastoreJobManager::description()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Description").toString();
}

qlonglong LastoreJobManager::downloadSize()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("DownloadSize").toLongLong();
}

QString LastoreJobManager::id()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Id").toString();
}

QString LastoreJobManager::name()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Name").toString();
}

QStringList LastoreJobManager::packages()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Packages").toStringList();
}

double LastoreJobManager::progress()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Progress").toDouble();
}

qlonglong LastoreJobManager::speed()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Speed").toLongLong();
}

QString LastoreJobManager::status()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Status").toString();
}

QString LastoreJobManager::type()
{
    Q_D(LastoreJobManager);
    return d->jobInterface->property("Type").toString();
}

