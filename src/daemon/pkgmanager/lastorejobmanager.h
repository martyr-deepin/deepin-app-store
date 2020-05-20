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
#ifndef LASTOREJOBMANAGER_H
#define LASTOREJOBMANAGER_H

#include <QObject>
#include <QVariantMap>

class QDBusInterface;
class LastoreJobManagerPrivate;
class LastoreJobManager : public QObject
{
    Q_OBJECT
public:
    explicit LastoreJobManager(QString path,QObject *parent = nullptr);
    ~LastoreJobManager();

    bool cancelable();
    qlonglong createTime();
    QString description();
    qlonglong downloadSize();
    QString id();
    QString name();
    QStringList packages();
    double progress();
    qlonglong speed();
    QString status();
    QString type();

signals:

public slots:

private:
    QString m_dbusPath;

    QScopedPointer<LastoreJobManagerPrivate> d_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(d_ptr), LastoreJobManager)
};

#endif // LASTOREJOBMANAGER_H
