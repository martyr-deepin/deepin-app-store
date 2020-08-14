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

#include <QProcess>
#include <QFileInfo>
#include <QDateTime>
#include <QtDBus>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QLocalServer>
#include <QLocalSocket>

#include "metadatamanager.h"
#include "lastorejobservice.h"

class MetaDataManagerPrivate
{
public:
    MetaDataManagerPrivate(MetaDataManager *parent) :
        q_ptr(parent)
    {
    }

    QHash<QString,CacheAppInfo> repoApps;

    QHash<QString,AppVersion> listAllApps;
    QHash<QString,qlonglong> listAppsSize;
    InstalledAppInfoList listInstalledInfo;

    void resetListAllApps(QHash<QString,AppVersion> appsList);
    void insertListAllApps(QString key,AppVersion appVersion);
    AppVersion getAppsInfo(QString key);

    void resetInstalledInfoList(InstalledAppInfoList installedList);
    InstalledAppInfoList getInstalledInfoList();

    bool compareVersion(QString localVersion,QString remoteVersion);

    void resetListAppsSize(QHash<QString,qlonglong> appSize);
    void insertListAppsSize(QString key,qlonglong size);
    qlonglong getAppSize(QString id);

    QMutex mutex;
    QMutex mutexAppInstalled;
    QMutex mutexAppSize;
    QMutex mutexAppAll;

    QDBusObjectPath addJob(QDBusObjectPath);

    QHash<QString,LastoreJobService *> getJobServiceList();
    void insertJobServiceList(QString key, LastoreJobService *job);
    void removeJobServiceList(QString key);

    QList<QDBusObjectPath> m_jobList;//joblist
    //使用QString为了方便比较
    QHash<QString,LastoreJobService *> m_jobServiceList;

    MetaDataManager *q_ptr;
    Q_DECLARE_PUBLIC(MetaDataManager)
};

QHash<QString, LastoreJobService *> MetaDataManagerPrivate::getJobServiceList()
{
    QMutexLocker locker(&mutex);
    return m_jobServiceList;
}

void MetaDataManagerPrivate::insertJobServiceList(QString key, LastoreJobService *job)
{
    QMutexLocker locker(&mutex);
    m_jobServiceList.insert(key,job);
}

void MetaDataManagerPrivate::removeJobServiceList(QString key/*, LastoreJobService *job*/)
{
    QMutexLocker locker(&mutex);
    m_jobServiceList.remove(key);
}

void MetaDataManagerPrivate::resetListAllApps(QHash<QString, AppVersion> appsList)
{
    QMutexLocker locker(&mutexAppAll);
    listAllApps.clear();
    listAllApps = appsList;
}

void MetaDataManagerPrivate::insertListAllApps(QString key, AppVersion appVersion)
{
    QMutexLocker locker(&mutexAppAll);
    listAllApps.insert(key,appVersion);
}

AppVersion MetaDataManagerPrivate::getAppsInfo(QString key)
{
    QMutexLocker locker(&mutexAppAll);
    return listAllApps.value(key);
}

void MetaDataManagerPrivate::resetInstalledInfoList(InstalledAppInfoList installedList)
{
    QMutexLocker locker(&mutexAppInstalled);
    listInstalledInfo.clear();
    listInstalledInfo = installedList;
}

InstalledAppInfoList MetaDataManagerPrivate::getInstalledInfoList()
{
    QMutexLocker locker(&mutexAppInstalled);
    return listInstalledInfo;
}

void MetaDataManagerPrivate::resetListAppsSize(QHash<QString, qlonglong> appSize)
{
    QMutexLocker locker(&mutexAppSize);
    listAppsSize.clear();
    listAppsSize = appSize;
}

void MetaDataManagerPrivate::insertListAppsSize(QString key, qlonglong size)
{
    QMutexLocker locker(&mutexAppSize);
    listAppsSize.insert(key,size);
}

qlonglong MetaDataManagerPrivate::getAppSize(QString id)
{
    QMutexLocker locker(&mutexAppSize);
    return listAppsSize.value(id);
}

MetaDataManager::MetaDataManager(QDBusInterface *lastoreDaemon, QObject *parent) :
    QObject(parent),server(29898,this),
    d_ptr(new MetaDataManagerPrivate(this)),
    m_pLastoreDaemon(lastoreDaemon)
{
    qRegisterMetaType<QHash<QString,InstalledAppInfo>>("InstalledAppInfomap");
    qRegisterMetaType<QHash<QString,qlonglong>>("AppSizeMap");
    qRegisterMetaType<QHash<QString,AppVersion>>("AppVersionMap");

    Q_D(MetaDataManager);
    worker = new WorkerDataBase();
    worker->moveToThread(&thread);
    connect(worker, &WorkerDataBase::resultReady, this, [=](InstalledAppInfoList listInstalledInfo,QHash<QString,qlonglong> listAppsSize,QHash<QString,AppVersion> listAllApps){
        d->resetListAllApps(listAllApps);
        d->resetListAppsSize(listAppsSize);
        d->resetInstalledInfoList(listInstalledInfo);
        Q_EMIT jobListChanged();//通知界面更新
    });
    connect(this, &MetaDataManager::updateCache, worker, &WorkerDataBase::updateCache);
    thread.start();
    Q_EMIT updateCache();

    QLocalServer *m_server = new QLocalServer(this);
    QLocalServer::removeServer("ServerName");
    bool ok = m_server->listen("ServerName");
    if (!ok)
    {
        // TODO:
    }
    connect(m_server,&QLocalServer::newConnection, this,[=](){
        Q_EMIT updateCache();
        QLocalSocket *localSocket = m_server->nextPendingConnection();
        localSocket->abort();
    });
}

MetaDataManager::~MetaDataManager()
{
    worker->deleteLater();
}

AppVersionList MetaDataManager::queryVersion(const QStringList &idList)
{
    Q_D(MetaDataManager);
    AppVersionList listVersionInfo;

    for(int i=0;i<idList.size();i++) {
        AppVersion versionInfo;
        QString appID = idList.value(i);
        versionInfo = d->getAppsInfo(appID);

        listVersionInfo.append(versionInfo);
    }
    return  listVersionInfo;
}

InstalledAppInfoList MetaDataManager::listInstalled()
{
    Q_D(MetaDataManager);
    return  d->getInstalledInfoList();
}

qlonglong MetaDataManager::queryDownloadSize(const QString &id)
{
    Q_D(MetaDataManager);
    return  d->getAppSize(id);
}

void MetaDataManager::updateSource()
{
    const QDBusPendingReply<QDBusObjectPath> reply = m_pLastoreDaemon->call("UpdateSource");
    if (reply.isError()) {
        qDebug() << reply.error();
    }
}

QDBusObjectPath MetaDataManager::addJob(QDBusObjectPath path)
{
    Q_D(MetaDataManager);
    qDebug()<<"addJob "<<path.path();
    QString jobId = path.path().section('/',4,4).remove(0,3);
    QString servicePath = QString("/com/deepin/AppStore/Backend/Job") + jobId;

    QHash<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();
    //dbus has registed
    if(jobServiceList.contains(servicePath)) {
        return QDBusObjectPath(QString("/com/deepin/AppStore/Backend/Job") + jobId);
    }

    LastoreJobService *lastoreJob = new  LastoreJobService(jobId,this);
    //start、pause、clean job
    connect(lastoreJob, &LastoreJobService::jobController, this,&MetaDataManager::jobController);
    qDebug()<<"servicePath "<<servicePath<<lastoreJob->id();
    //registerObject
    if (!QDBusConnection::sessionBus().registerObject(servicePath,lastoreJob,
            QDBusConnection::ExportAllSlots |
            QDBusConnection::ExportAllProperties)) {
        qDebug() << "registerObject setting dbus Error" << QDBusConnection::sessionBus().lastError();
    }
    d->insertJobServiceList(servicePath,lastoreJob);
    updateJobList();
    return QDBusObjectPath(servicePath);
}

QList<QDBusObjectPath> MetaDataManager::getJobList()
{
    Q_D(MetaDataManager);
    return d->m_jobList;
}

void MetaDataManager::msg_proc(QByteArray &data, QTcpSocket *sock)
{
    Q_EMIT updateCache();
    qDebug() << "socket id = " << sock->socketDescriptor() << ", received data = " << data;

    QString receivedData = QString::fromStdString(data.toStdString());

    QStringList listPara = receivedData.split("##");

    if(listPara.count() == 3)
    {
        qDebug() << "listPara = " << listPara;

        QString CmdType = listPara.at(0) =="-i" ? "install":"remove";
        QString packageName = listPara.at(1);
        QString PackageVersion = listPara.at(2);

        Q_EMIT packageUpdated(CmdType,packageName,PackageVersion);

    }
}

void MetaDataManager::disconnect_proc(QString &str)
{
    (void)str;
}

//清理lastore接口对象，注销服务和dbus接口，更新job列表
void MetaDataManager::cleanService(QStringList jobList)
{
    Q_D(MetaDataManager);
    LastoreJobService *lastoreJob;
    QStringList deleteJob;

    QHash<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();

    foreach (lastoreJob, jobServiceList) {
        QString job = QString("/com/deepin/lastore/Job") + lastoreJob->getJobPath();

        if(!jobList.contains(job)) {
            QString servicePath = QString("/com/deepin/AppStore/Backend/Job") + lastoreJob->getJobPath();
            //unregisterObject
            QDBusConnection::sessionBus().unregisterObject(servicePath);
            deleteJob.append(servicePath);
        }
    }

    while (!deleteJob.isEmpty())
    {
        QHash<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();
        QString servicePath = deleteJob.takeFirst();
        LastoreJobService *lastoreJob = jobServiceList.value(servicePath);
        d->removeJobServiceList(servicePath);
        delete lastoreJob;
        lastoreJob = nullptr;
    }
    updateJobList();
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
    LastoreJobService* lastoreJob;
    QHash<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();
    foreach (lastoreJob, jobServiceList) {
        servicePath = QString("/com/deepin/AppStore/Backend/Job") + lastoreJob->getJobPath();
        jobList.append(QDBusObjectPath(servicePath));
    }
    d->m_jobList = jobList;
    Q_EMIT jobListChanged();//通知界面更新
}

void WorkerDataBase::updateCache()
{
    InstalledAppInfoList listInstalledInfo;
    QHash<QString,qlonglong> listAppsSize;
    QHash<QString,AppVersion> listAllApps;
    bool isOk = db.open();
    if(!isOk) {
        qDebug()<<db.lastError();
        exit(-1);
    } else {
        QString appID;
        QString appLocalVer;
        QString appRemoteVer;
//        QString appArch = ;
//        QString appInstallSize = ;
        qlonglong appSize;
//        QString appBin = ;
        listInstalledInfo.clear();
        QSqlQuery query("SELECT * FROM Packages",db);
        query.exec();
        while (query.next()) {
          appID = query.value(0).toString();
          appLocalVer = query.value(1).toString();
          appRemoteVer = query.value(2).toString();
//          appArch = query.value(3).toString();
//          appInstallSize = query.value(4).toString();
          appSize = query.value(5).toLongLong();
          AppVersion versionInfo;
          versionInfo.pkg_name = appID;
          versionInfo.installed_version = appLocalVer;
          versionInfo.remote_version = appRemoteVer;
          versionInfo.size = appSize;
          if(versionInfo.installed_version.isEmpty()) {
              versionInfo.upgradable = false;
          }
          else {
              versionInfo.upgradable = compareVersion(versionInfo.installed_version,versionInfo.remote_version);
          }

          listAppsSize.insert(appID,appSize);
          listAllApps.insert(appID,versionInfo);
          if(!appLocalVer.isEmpty())
          {
              InstalledAppInfo installInfo;
              installInfo.packageName = appID;
              installInfo.appName = appID;
              installInfo.version = appLocalVer;
              installInfo.size = appSize;
      //            installInfo.localeNames = installInfo.appName;
              installInfo.desktop = getPackageDesktop(appID);
              installInfo.installationTime = getAppInstalledTime(installInfo.packageName);
              listInstalledInfo.append(installInfo);
          }
        }
    }
    db.close();
    qDebug()<<"update cache";
    emit resultReady(listInstalledInfo,listAppsSize,listAllApps);
}

QString WorkerDataBase::getPackageDesktop(QString packageName)
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

qlonglong WorkerDataBase::getAppInstalledTime(QString id)
{
    QDir dir("/var/lib/dpkg/info/");
    if(!dir.exists()) {
        qDebug()<<dir.absolutePath()<<"is not exist";
        return false;
    }
    QStringList filters;
    filters << id+"*.md5sums";
    dir.setNameFilters(filters);
    QFileInfoList infoList = dir.entryInfoList();
    QString cacheFile;
    if(!infoList.isEmpty()) {
        return infoList.takeFirst().birthTime().toSecsSinceEpoch();
    }
    else
        return 0;
}

bool WorkerDataBase::compareVersion(QString localVersion, QString remoteVersion)
{
    QRegExp rx("[?\\.\\+\\-]");
    QStringList localList;
    QStringList remoteList;
    localList = localVersion.split(rx, QString::SkipEmptyParts);
    remoteList = remoteVersion.split(rx, QString::SkipEmptyParts);
    int size = ((localList.size()>remoteList.size())?localList.size():remoteList.size());

    QRegExp rxs("^[0-9]+$");
    for(int i =0 ;i<size;i++) {
        QString localSplit = localList.value(i);
        QString remoteSplit = remoteList.value(i);

        if (rxs.exactMatch(localSplit) && rxs.exactMatch(remoteSplit)) {//均为纯数字
            if(localSplit.toInt() == remoteSplit.toInt())
                continue;
            else
                return (localSplit.toInt() < remoteSplit.toInt());
        }
        else {           //不是纯数字
            if(QString::compare(localSplit,remoteSplit) == 0)
                continue;
            else
                return (QString::compare(localSplit,remoteSplit)<0);
        }
    }
    return false;
}

WorkerDataBase::WorkerDataBase(QObject *parent) :
    QObject(parent)
{
    db = QSqlDatabase::addDatabase("QSQLITE","cache");
    if (QSqlDatabase::contains("cache")) {
        db = QSqlDatabase::database("cache");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", "cache");
    }

    db.setDatabaseName("/usr/share/deepin-app-store/cache.db");
    db.setUserName("root");
    db.setPassword("deepin-app-store");
}

WorkerDataBase::~WorkerDataBase()
{
}
