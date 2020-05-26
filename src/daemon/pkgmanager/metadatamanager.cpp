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

#include "metadatamanager.h"
#include <QProcess>
#include <QFileInfo>
#include <QDateTime>
#include <QtDBus>
#include "lastorejobservice.h"
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QFileSystemWatcher>

#include <QMutex>

class MetaDataManagerPrivate
{
public:
    MetaDataManagerPrivate(MetaDataManager *parent) :
        q_ptr(parent)
    {
    }

    qint64     lastRepoUpdated;
    QMap<QString,CacheAppInfo> repoApps;
    QVariantMap listApps;
    InstalledAppInfoList listInstalledInfo;

    QMutex mutex;

//    QMap<QString,CacheAppInfo> listStorePackages();
    QString getPackageDesktop(QString packageName);
    QDBusObjectPath addJob(QDBusObjectPath);

    QList<QDBusObjectPath> m_jobList;//joblist
    //使用QString为了方便比较
    QMap<QString,LastoreJobService *> m_jobServiceList;
    QFileSystemWatcher *m_fileSystemWatcher;

    MetaDataManager *q_ptr;
    Q_DECLARE_PUBLIC(MetaDataManager)
};

QString MetaDataManagerPrivate::getPackageDesktop(QString packageName)
{
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    qputenv("LC_ALL", "C");
    process.setProcessEnvironment(env);
    process.start("dpkg",QStringList()<<"-L"<<packageName);
    QString dpkgDesktopQuery;
    if(process.waitForFinished()) {
        dpkgDesktopQuery = QString(process.readAll());
    }
    if(dpkgDesktopQuery.isEmpty())
        return QString("");

    QStringList pathList = dpkgDesktopQuery.split('\n');
    foreach (QString line, pathList) {
        if(line.endsWith(".desktop"))
            return line;
    }
    return QString("");
}

MetaDataManager::MetaDataManager(QDBusInterface *lastoreDaemon, QObject *parent) :
    QObject(parent),
    d_ptr(new MetaDataManagerPrivate(this)),
    m_pLastoreDaemon(lastoreDaemon)
{
    Q_D(MetaDataManager);
    d->lastRepoUpdated = 0;
    updateCacheList();

    d->m_fileSystemWatcher = new QFileSystemWatcher(this);
    d->m_fileSystemWatcher->addPath("/usr/share/deepin-app-store/update");
    connect(d->m_fileSystemWatcher,&QFileSystemWatcher::fileChanged,this,[=](){
        this->updateCacheList();
    });

    connect(this,&MetaDataManager::signUpdateJobList,this,&MetaDataManager::updateJobList,Qt::QueuedConnection);
}

MetaDataManager::~MetaDataManager()
{
    Q_D(MetaDataManager);
    d->m_fileSystemWatcher->deleteLater();
}

AppVersionList MetaDataManager::queryVersion(const QStringList &idList)
{
    Q_D(MetaDataManager);
    AppVersionList listInstallInfo;
    if(idList.isEmpty())
        return listInstallInfo;

    for(int i=0;i<idList.size();i++) {
        AppVersion versionInfo;
        if(d->listApps.contains(idList.value(i)))
        {
            QMap<QString,QVariant> map = d->listApps.value(idList.value(i)).toMap();
            versionInfo.pkg_name = idList.value(i);
            versionInfo.installed_version = map.value("appLocalVer").toString();
            versionInfo.remote_version = map.value("appRemoteVer").toString();
            if(versionInfo.installed_version.isEmpty()) {
                versionInfo.upgradable = false;
            }
            else {
                versionInfo.upgradable = (QString::compare(versionInfo.installed_version,versionInfo.remote_version)<0);
            }
        }

        listInstallInfo.append(versionInfo);
    }
    return  listInstallInfo;
}

InstalledAppInfoList MetaDataManager::listInstalled()
{
    Q_D(MetaDataManager);
    return  d->listInstalledInfo;
}


InstalledAppTimestampList MetaDataManager::getInstallationTimes(const QStringList &idList)
{
    InstalledAppTimestampList listInstallationTimeInfo;
    foreach (QString id, idList) {
        qlonglong installTime = getAppInstalledTime(id);
        if(installTime != 0) {
            InstalledAppTimestamp installedTimeInfo;
            installedTimeInfo.pkg_name = id;
            installedTimeInfo.timestamp = installTime;
            listInstallationTimeInfo.append(installedTimeInfo);
        }
    }

    return listInstallationTimeInfo;
}

qlonglong MetaDataManager::getAppInstalledTime(QString id)
{
    QFileInfo info("/var/lib/dpkg/info/"+id+".md5sums");
    if (info.exists()) {
        return info.birthTime().toSecsSinceEpoch();
    }
    else {
        return  0;
    }
}

qlonglong MetaDataManager::queryDownloadSize(const QString &id)
{
    Q_D(MetaDataManager);
    return  d->listApps.value(id).toMap().value("appSize").toLongLong();
}

QDBusObjectPath MetaDataManager::addJob(QDBusObjectPath path)
{
    Q_D(MetaDataManager);
    qDebug()<<"addJob "<<path.path();
    QString jobId = path.path().section('/',4,4).remove(0,3);
    QString servicePath = QString("/com/deepin/AppStore/Backend/Job") + jobId;
    //dbus has registed
    if(d->m_jobServiceList.contains(servicePath)) {
        return QDBusObjectPath(QString("/com/deepin/AppStore/Backend/Job") + jobId);
    }

    LastoreJobService *lastoreJob = new  LastoreJobService(jobId);
    //delete service
    connect(lastoreJob, &LastoreJobService::destroyService, this, &MetaDataManager::cleanService);
    //start、pause、clean job
    connect(lastoreJob, &LastoreJobService::jobController, this,&MetaDataManager::jobController);
    qDebug()<<"servicePath "<<servicePath<<lastoreJob->id();
    //registerObject
    if (!QDBusConnection::sessionBus().registerObject(servicePath,lastoreJob,
            QDBusConnection::ExportAllSlots |
            QDBusConnection::ExportAllProperties)) {
        qDebug() << "registerObject setting dbus Error" << QDBusConnection::sessionBus().lastError();
    }
    d->m_jobServiceList.insert(servicePath,lastoreJob);
//    updateJobList();//update JobList
    emit signUpdateJobList();
    return QDBusObjectPath(servicePath);
}

QList<QDBusObjectPath> MetaDataManager::getJobList()
{
    Q_D(MetaDataManager);
    return d->m_jobList;
}

void MetaDataManager::updateCacheList()
{
    Q_D(MetaDataManager);
    d->lastRepoUpdated = 0;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","read");

    if (QSqlDatabase::contains("read")) {
        db = QSqlDatabase::database("read");
    } else {
        db = QSqlDatabase::addDatabase("QMYSQL", "read");
    }

    QString dbPath = "/usr/share/deepin-app-store/cache.db";
    db.setDatabaseName(dbPath);
    db.setUserName("root");
    db.setPassword("deepin-app-store");

    bool isOk = db.open();
    if(!isOk) {
        qDebug()<<"error info :"<<db.lastError();
    }
    else {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM Packages;");
        query.exec();

        d->listInstalledInfo.clear();
        while(query.next())
        {
            QMap<QString,QVariant> map;
//            map.insert("appID",query.value(0).toString());
            map.insert("appLocalVer",query.value(1).toString());
            map.insert("appRemoteVer",query.value(2).toString());
            map.insert("appArch",query.value(3).toString());
            map.insert("appInstallSize",query.value(4).toString());
            map.insert("appSize",query.value(5).toString());
            map.insert("appBin",query.value(6).toString());
            d->listApps.insert(query.value(0).toString(),QVariant(map));
            if(!query.value(1).toString().isEmpty())
            {
                InstalledAppInfo installInfo;
                installInfo.packageName = query.value(0).toString();
                installInfo.appName = query.value(0).toString();
                installInfo.version = query.value(1).toString();
                installInfo.size = query.value(5).toLongLong();
        //            installInfo.localeNames = installInfo.appName;
                installInfo.desktop = d->getPackageDesktop(installInfo.packageName);
                installInfo.installationTime = getAppInstalledTime(installInfo.packageName);
                d->listInstalledInfo.append(installInfo);
            }
        }
    }
    db.close();
}

//清理lastore接口对象，注销服务和dbus接口，更新job列表
void MetaDataManager::cleanService()
{
    Q_D(MetaDataManager);
    LastoreJobService *lastoreJob = static_cast<LastoreJobService *> (sender());

    QString servicePath = QString("/com/deepin/AppStore/Backend/Job") + lastoreJob->id();
    //unregisterObject
    QDBusConnection::sessionBus().unregisterObject(servicePath);

    d->m_jobServiceList.remove(servicePath);
//    updateJobList();//update JobList
    emit signUpdateJobList();
    lastoreJob->deleteLater();
    qDebug() << "unregisterObject dbus " << servicePath;
}

void MetaDataManager::jobController(QString cmd, QString jobId)
{
    QList<QVariant> argumentList;
    argumentList << jobId;

    QDBusMessage reply = m_pLastoreDaemon->callWithArgumentList(QDBus::AutoDetect,cmd, argumentList);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "D-Bus Error:" << reply.errorMessage();
    }
}

void MetaDataManager::updateJobList()
{
    Q_D(MetaDataManager);
    QList<QDBusObjectPath> jobList;

    QString servicePath;
    foreach (LastoreJobService* lastoreJob, d->m_jobServiceList) {
        servicePath = QString("/com/deepin/AppStore/Backend/Job") + lastoreJob->id();
        jobList.append(QDBusObjectPath(servicePath));
    }
    d->m_jobList = jobList;
    emit jobListChanged();//通知界面更新
}
