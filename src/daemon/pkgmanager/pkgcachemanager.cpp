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



#include "pkgcachemanager.h"
#include <QtDBus>
#include <QDBusError>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <sstream>

PkgCacheManager::PkgCacheManager()
{

}

QString PkgCacheManager::RequestPackageName(QString path)
{
    QSqlDatabase db;

    if (QSqlDatabase::contains("PkgCacheManager")) {
        db = QSqlDatabase::database("PkgCacheManager");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", "PkgCacheManager");
    }

    db.setDatabaseName("/usr/share/deepin-app-store/cache.db");

    db.open();

    QString strPackageName = "";

    std::ostringstream oss;
    oss <<  "select * from package_binary_file where binaryPath = '" << path.toStdString() << "'";
    QString sqlCmd = oss.str().c_str();

    QSqlQuery query(db);
    bool bRet = query.exec(sqlCmd);
    if(bRet)
    {
        while(query.next())
        {
            strPackageName = query.value("packageName").toString();
        }
    }

    db.close();

    return strPackageName;
}
