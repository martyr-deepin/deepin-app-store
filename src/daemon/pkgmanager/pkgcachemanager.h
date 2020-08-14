/*
 * Copyright (C) 2019 ~ 2020 Union Tech Co., Ltd.
 *
 * Author:     zhangxudong <zhangxudong@uniontech.com>
 *
 * Maintainer: zhangxudong <zhangxudong@uniontech.com>
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

#pragma once
#include <QObject>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>

#include "../dbus/daemondbus.h"

class QDBusInterface;
class PkgCacheManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", LASTOREDAEMON_CACHE_INTERFACE)

public:
    explicit PkgCacheManager();
    ~PkgCacheManager(){}

signals:
    void PackageUpdated(QString CmdType,QString PackageName ,QString PackageVersion);

public slots:
    QString RequestPackageName(QString path);//通过二进制文件查询包名

};

