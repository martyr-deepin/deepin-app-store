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
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
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

    qint64     lastRepoUpdated;
    QMap<QString,CacheAppInfo> repoApps;

    QMap<QString,AppVersion> listApps;
    QMap<QString,qlonglong> listAppsSize;
    QMap<QString,InstalledAppInfo> listInstalledInfo;
    void insertInstalledInfoList(QString key, InstalledAppInfo info);
    void clearInstalledInfoList();
    InstalledAppInfoList getInstalledInfoList();

    QMutex mutex;
    QMutex mutexAppInfo;

    QString getPackageDesktop(QString packageName);
    AppVersion queryAppVersion(const QString &id,AppVersion &versionInfo);
    QDBusObjectPath addJob(QDBusObjectPath);
    QMap<QString,CacheAppInfo> listStorePackages();

    QMap<QString,LastoreJobService *> getJobServiceList();
    void insertJobServiceList(QString key, LastoreJobService *job);
    void removeJobServiceList(QString key);

    QList<QDBusObjectPath> m_jobList;//joblist
    //使用QString为了方便比较
    QMap<QString,LastoreJobService *> m_jobServiceList;

    MetaDataManager *q_ptr;
    Q_DECLARE_PUBLIC(MetaDataManager)
};

QMap<QString,CacheAppInfo> MetaDataManagerPrivate::listStorePackages()
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

    QMap<QString,CacheAppInfo> apps;
    //cut line
    QString packageName;
    QString type;//"i" 系统app;"p" 第三方app
    QStringList lineList = dpkgQuery.split('\n');
    foreach (QString line, lineList) {
        line = line.simplified();
        packageName = line.split(' ').value(1,"");
        type = line.split(' ').value(0,"");

        if(!packageName.isEmpty() /*&& type.compare("i")==0*/) {
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

    //update time
    lastRepoUpdated = QDateTime::currentSecsSinceEpoch();
    repoApps = apps;
    return  apps;
}

QMap<QString, LastoreJobService *> MetaDataManagerPrivate::getJobServiceList()
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

void MetaDataManagerPrivate::insertInstalledInfoList(QString key, InstalledAppInfo info)
{
    QMutexLocker locker(&mutexAppInfo);
    listInstalledInfo.insert(key,info);
}

void MetaDataManagerPrivate::clearInstalledInfoList()
{
    QMutexLocker locker(&mutexAppInfo);
    listInstalledInfo.clear();
}

InstalledAppInfoList MetaDataManagerPrivate::getInstalledInfoList()
{
    QMutexLocker locker(&mutexAppInfo);
    return  listInstalledInfo.values();
}

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

AppVersion MetaDataManagerPrivate::queryAppVersion(const QString &id,AppVersion &versionInfo)
{
    QMap<QString,CacheAppInfo> appInfoList = listStorePackages();

    qDebug()<<"IDList "<<id;

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
}

MetaDataManager::MetaDataManager(QDBusInterface *lastoreDaemon, QObject *parent) :
    QObject(parent),
    d_ptr(new MetaDataManagerPrivate(this)),
    m_pLastoreDaemon(lastoreDaemon)
{
    Q_D(MetaDataManager);
    d->lastRepoUpdated = 0;
    updateCacheList();
}

MetaDataManager::~MetaDataManager()
{
}

AppVersionList MetaDataManager::queryVersion(const QStringList &idList)
{
    Q_D(MetaDataManager);
    AppVersionList listVersionInfo;

    for(int i=0;i<idList.size();i++) {
        AppVersion versionInfo;
        if(d->listApps.contains(idList.value(i)))
        {
            versionInfo = d->listApps.value(idList.value(i));
        }
        else {
            d->queryAppVersion(idList.value(i),versionInfo);
            QMap<QString,QVariant> map;
            d->listApps.insert(idList.value(i),versionInfo);
        }

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
    return  d->listAppsSize.value(id);
}

QDBusObjectPath MetaDataManager::addJob(QDBusObjectPath path)
{
    Q_D(MetaDataManager);
    qDebug()<<"addJob "<<path.path();
    QString jobId = path.path().section('/',4,4).remove(0,3);
    QString servicePath = QString("/com/deepin/AppStore/Backend/Job") + jobId;

    QMap<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();
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

void MetaDataManager::updateCacheList()
{
    Q_D(MetaDataManager);
    //获取本地应用列表
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    qputenv("LC_ALL", "C");
    process.setProcessEnvironment(env);

    process.start("/usr/bin/dpkg-query",
                  QStringList()<<"--show"<<"-f"
                  <<"${Package}\\t${db:Status-Abbrev}\\t${Version}\\n");
    QString dpkgQuery;
    if(process.waitForFinished()) {
        dpkgQuery = QString(process.readAll());
    }

    QMap<QString,QString> versionList;
    //accountsservice\tii \t0.6.45-2\t455\nacl\tii \t2.2.53-4\t206\n
    QStringList installedList = dpkgQuery.split('\n');
    foreach (QString line, installedList) {
        QStringList installedDetialList = line.split('\t');

        if(installedDetialList.value(1).startsWith("ii")) {
            QString id = installedDetialList.value(0);
            QStringList fullPackageName = id.split(':');
            QString fuzzyPackageName = fullPackageName[0];
            versionList.insert(fuzzyPackageName,installedDetialList.value(2));
        }
    }

    //获取所有应用信息
    //get app store url
    QString appstore = QString("/etc/apt/sources.list.d/appstore.list");
    QFile storeFile(appstore);
    if(!storeFile.exists()) {
        qDebug()<<"storeFile.exists";
    }
    if(!storeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"storeFile.open is error";
    }

    QString storeUrl;
    while (!storeFile.atEnd()) {
      QString line = storeFile.readLine();
      if(line.startsWith("deb"))
      {
          storeUrl = line.section('/',2,2);
      }
    }
    storeFile.close();

    d->clearInstalledInfoList();

    //get all package list
    QDir dir("/var/lib/apt/lists/");
    if(!dir.exists()) {
        qDebug()<<dir.absolutePath()<<"is not exist";
    }
    QStringList filters;
    filters << storeUrl+"_appstore*_Packages";
    dir.setNameFilters(filters);
    QFileInfoList infoList = dir.entryInfoList();
    for (int i=0;i<infoList.size();i++) {
        QString cacheFile = infoList.value(i).absoluteFilePath();

        QFile file(cacheFile);
        if(!file.open(QIODevice::ReadOnly))
            qDebug()<<"open fail";
        QString data = file.readAll();
        file.close();
        QStringList packageList = data.split("\n\n", QString::SkipEmptyParts);

        QString appID;
        QString appLocalVer;
        QString appRemoteVer;
        qlonglong appSize = 0;

        for (int i = 0; i < packageList.size(); ++i) {
            QStringList list = packageList.value(i).split('\n');
            for(int i=0;i<list.size();i++) {
                QString str = list.value(i);
                if(str.startsWith("Package: ")) {
                    appID = str.section("Package: ",1,1);
                    appLocalVer = versionList.value(appID,"");
                }
                else if(str.startsWith("Version: ")) {
                    appRemoteVer = str.section("Version: ",1,1);
                }
                else if(str.startsWith("Size: ")) {
                    appSize = str.section("Size: ",1,1).toLongLong();
                }
            }
            AppVersion versionInfo;
            versionInfo.pkg_name = appID;
            versionInfo.installed_version = appLocalVer;
            versionInfo.remote_version = appRemoteVer;
            if(versionInfo.installed_version.isEmpty()) {
                versionInfo.upgradable = false;
            }
            else {
                versionInfo.upgradable = (QString::compare(versionInfo.installed_version,versionInfo.remote_version)<0);
            }

            if(!appLocalVer.isEmpty())
            {
                InstalledAppInfo installInfo;
                installInfo.packageName = appID;
                installInfo.appName = appLocalVer;
                installInfo.version = appRemoteVer;
                installInfo.size = appSize;
        //            installInfo.localeNames = installInfo.appName;
                installInfo.desktop = d->getPackageDesktop(installInfo.packageName);
                installInfo.installationTime = getAppInstalledTime(installInfo.packageName);
                d->insertInstalledInfoList(appID,installInfo);
            }
            d->listAppsSize.insert(appID,appSize);
            d->listApps.insert(appID,versionInfo);

        }
    }
    qDebug()<<"update";
}

//清理lastore接口对象，注销服务和dbus接口，更新job列表
void MetaDataManager::cleanService(QStringList jobList)
{
    Q_D(MetaDataManager);
    LastoreJobService *lastoreJob;
    QStringList deleteJob;

    QMap<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();

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
        QMap<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();
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
    QMap<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();
    foreach (lastoreJob, jobServiceList) {
        servicePath = QString("/com/deepin/AppStore/Backend/Job") + lastoreJob->getJobPath();
        jobList.append(QDBusObjectPath(servicePath));
    }
    d->m_jobList = jobList;
    emit jobListChanged();//通知界面更新
}
