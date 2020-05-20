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
#ifndef LASTOREJOBSERVICE_H
#define LASTOREJOBSERVICE_H

#include <QObject>
#include "lastorejobmanager.h"
#include <QDebug>

class LastoreJobService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.AppStore.Backend.Deb.Job")
    Q_PROPERTY(bool Cancelable READ cancelable)
    Q_PROPERTY(qlonglong CreateTime READ createTime)
    Q_PROPERTY(QString Description READ description)
    Q_PROPERTY(qlonglong DownloadSize READ downloadSize)
    Q_PROPERTY(QString Id READ id)
    Q_PROPERTY(QString Name READ name)
    Q_PROPERTY(QStringList Packages READ packages)
    Q_PROPERTY(double Progress READ progress)
    Q_PROPERTY(qlonglong Speed READ speed)
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(QString Type READ type)

public:
    explicit LastoreJobService(QString path,QObject *parent = nullptr);
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

private:
    LastoreJobManager *m_pLastoreJob = nullptr;
    QString m_JobPath;

signals:
    void destroyService();
    void jobController(QString cmd,QString jobId);

public slots:
    void Clean();
    void Pause();
    void Start();
    void lastoreJobListChanged(QString,QMap<QString,QVariant> ,QStringList);
};

#endif // LASTOREJOBSERVICE_H
