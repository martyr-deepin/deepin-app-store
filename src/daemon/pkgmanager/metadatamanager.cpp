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

    void resetListAppsSize(QHash<QString,qlonglong> appSize);
    void insertListAppsSize(QString key,qlonglong size);
    qlonglong getAppSize(QString id);

    QMutex mutex;
    QMutex mutexAppInstalled;
    QMutex mutexAppSize;
    QMutex mutexAppAll;

//    AppVersion queryAppVersion(const QString &id);
    QDBusObjectPath addJob(QDBusObjectPath);
    QHash<QString,CacheAppInfo> listStorePackages();

    QHash<QString,LastoreJobService *> getJobServiceList();
    void insertJobServiceList(QString key, LastoreJobService *job);
    void removeJobServiceList(QString key);

    QList<QDBusObjectPath> m_jobList;//joblist
    //使用QString为了方便比较
    QHash<QString,LastoreJobService *> m_jobServiceList;

    MetaDataManager *q_ptr;
    Q_DECLARE_PUBLIC(MetaDataManager)


    QTimer *m_pTimer;
    QProcess *m_pProcess;
    QString updateEtag;
};

QHash<QString,CacheAppInfo> MetaDataManagerPrivate::listStorePackages()
{
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    qputenv("LC_ALL", "C");
    process.setProcessEnvironment(env);
    process.start("/bin/bash");

    process.write("/usr/bin/aptitude search '?Label(Uos_eagle)'");
    process.closeWriteChannel();

    QString dpkgQuery;
    if(process.waitForFinished()) {
        dpkgQuery = QString(process.readAllStandardOutput());
    }
    else {
        return repoApps;
    }

    QHash<QString,CacheAppInfo> apps;
    //cut line
    QString packageName;
    QString type;//"i" 系统app;"p" 第三方app
    QStringList lineList = dpkgQuery.split('\n');
    foreach (QString line, lineList) {
        line = line.simplified();
        packageName = line.split(' ').value(1,"");
        type = line.split(' ').value(0,"");

        if(!packageName.isEmpty()) {
            cacheAppInfo appInfo;
            appInfo.PackageName = packageName;
            apps.insert(packageName,appInfo);

            //":"
            QStringList packageNameList = packageName.split(':');
            QString fuzzyPackageName = packageNameList.value(0,packageName);

            cacheAppInfo appInfoFuzzy;
            appInfoFuzzy.PackageName = packageName;
            apps.insert(fuzzyPackageName,appInfoFuzzy);
        }
    }

    repoApps = apps;
    return  apps;
}

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

/*AppVersion MetaDataManagerPrivate::queryAppVersion(const QString &id)
{
    QHash<QString,CacheAppInfo> appInfoList = listStorePackages();

    qDebug()<<"IDList "<<id;
    AppVersion versionInfo;

    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    qputenv("LC_ALL", "C");
    process.setProcessEnvironment(env);
    process.start("/usr/bin/apt-cache",QStringList()<<"policy"<<"--"<<id);

    QString dpkgInstalledQuery;
    if(process.waitForFinished())
    {
        dpkgInstalledQuery = QString(process.readAll());
    }
    if(dpkgInstalledQuery.isEmpty())
        return versionInfo;

    QString  packageName;
    QString  localVersion;
    QString  remoteVersion;
    //uos-browser-stable:
    //  Installed: 5.1.1001.7-1
    //  Candidate: 5.1.1001.7-1
    QStringList parseInstallList = dpkgInstalledQuery.split('\n');

    foreach (QString line, parseInstallList) {
        if(!line.isEmpty()) {
            // is package name line,if find space
            if(! line.contains(" ")) {
                packageName = line.chopped(1);
            }
            // get local version
            if(line.contains("Installed: ")) {
                localVersion = line.section(QString("Installed: "),1,1);
                if(QString::compare(localVersion,QString("(none)")) == 0)
                        localVersion = "";
            }
            // get remote version
            if(line.contains("Candidate: ")) {
                remoteVersion = line.section(QString("Candidate: "),1,1);
                versionInfo.pkg_name = packageName;
                versionInfo.installed_version = localVersion;
                versionInfo.remote_version = remoteVersion;
                if(localVersion.isEmpty()) {
                    versionInfo.upgradable = false;
                }
                else {
                    versionInfo.upgradable = (QString::compare(localVersion,remoteVersion)<0);
                }
            }
        }
    }
    return  versionInfo;
}*/

MetaDataManager::MetaDataManager(QDBusInterface *lastoreDaemon, QObject *parent) :
    QObject(parent),
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
    });

    //update source
    d->m_pProcess = new QProcess(this);
    d->m_pProcess->setReadChannel(QProcess::StandardOutput);
    d->m_pProcess->setEnvironment(QStringList() <<"LC_ALL"<<"C");

    QObject::connect(d->m_pProcess, &QProcess::readyReadStandardOutput,[=](){
        QByteArray byte = d->m_pProcess->readAllStandardOutput();
        QTextStream text(&byte,QIODevice::ReadOnly);
        while (!text.atEnd()) {
            QString line = text.readLine();
            if(line.startsWith("etag") && line != d->updateEtag) {
                qDebug()<<"updateSource ";
                updateSource();
                d->updateEtag = line;
            }
        }
    });

    d->m_pTimer = new QTimer(this);
    d->m_pTimer->start(1000*60*5);
    QObject::connect(d->m_pTimer, &QTimer::timeout,[=](){
        //get app store url
        QString appstore = QString("/etc/apt/sources.list.d/appstore.list");
        QFile storeFile(appstore);
        if(!storeFile.exists())
            return;
        if (!storeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QString storeUrl;
        while (!storeFile.atEnd()) {
          QString line = storeFile.readLine();
          if(line.startsWith("deb"))
          {
              storeUrl = line.section(' ',1,1);
          }
        }
        storeFile.close();

        QString url = QString("curl -I %1/dists/eagle/InRelease").arg(storeUrl);
        d->m_pProcess->start("/bin/sh",QStringList() << "-c" <<url);
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
//        if(d->listAllApps.contains(appID)) {
//            versionInfo = d->getAppsInfo(appID);
//        }
//        else {
//            versionInfo = d->queryAppVersion(appID);
//            d->insertListAllApps(appID,versionInfo);
//        }
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
              versionInfo.upgradable = (QString::compare(versionInfo.installed_version,versionInfo.remote_version)<0);
          }

          listAppsSize.insert(appID,appSize);
          listAllApps.insert(appID,versionInfo);
          if(!appLocalVer.isEmpty())
          {
              InstalledAppInfo installInfo;
              installInfo.packageName = appID;
              installInfo.appName = appLocalVer;
              installInfo.version = appRemoteVer;
              installInfo.size = appSize;
      //            installInfo.localeNames = installInfo.appName;
              installInfo.desktop = "";
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
