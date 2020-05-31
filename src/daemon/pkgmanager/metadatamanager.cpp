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
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QLocalServer>

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

    QString getPackageDesktop(QString packageName);
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

    QLocalServer *m_server = new QLocalServer(this);
    QLocalServer::removeServer("ServerName");
    bool ok = m_server->listen("ServerName");
    if (!ok)
    {
        // TODO:
    }
    connect(m_server,&QLocalServer::newConnection, this,[=](){
        this->updateCacheList();
    });
}

MetaDataManager::~MetaDataManager()
{
}

AppVersionList MetaDataManager::queryVersion(const QStringList &idList)
{
    Q_D(MetaDataManager);
    AppVersionList listInstallInfo;
    if(idList.isEmpty())
        return listInstallInfo;

    QMap<QString,CacheAppInfo> appInfoList = d->listStorePackages();

    QStringList packageList;
    foreach (QString package, idList) {

        if(appInfoList.keys().contains(package)) {
            packageList.append(package);
        }
    }

    qDebug()<<"IDList "<<packageList;

    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    qputenv("LC_ALL", "C");
    process.setProcessEnvironment(env);

    process.start("/usr/bin/apt-cache",QStringList()<<"policy"<<"--"<<idList);

    QString dpkgInstalledQuery;
    if(process.waitForFinished())
    {
        dpkgInstalledQuery = QString(process.readAll());
    }
    if(dpkgInstalledQuery.isEmpty())
        return listInstallInfo;

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
                AppVersion versionInfo;
                versionInfo.pkg_name = packageName;
                versionInfo.installed_version = localVersion;
                versionInfo.remote_version = remoteVersion;
                if(localVersion.isEmpty()) {
                    versionInfo.upgradable = false;
                }
                else {
                    versionInfo.upgradable = (QString::compare(localVersion,remoteVersion)<0);
                }

                listInstallInfo.append(versionInfo);
            }
        }
    }

    return  listInstallInfo;
}

InstalledAppInfoList MetaDataManager::listInstalled()
{
    Q_D(MetaDataManager);
    InstalledAppInfoList listInstalledInfo;
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    qputenv("LC_ALL", "C");
    process.setProcessEnvironment(env);

    process.start("/usr/bin/dpkg-query",
                  QStringList()<<"--show"<<"-f"
                  <<"${binary:Package}\\t${db:Status-Abbrev}\\t${Version}\\t${Installed-Size}\\n");
    QString dpkgQuery;
    if(process.waitForFinished()) {
        dpkgQuery = QString(process.readAll());
    }
    else {
        return listInstalledInfo;
    }

    QMap<QString,CacheAppInfo> appInfoList = d->listStorePackages();

    //accountsservice\tii \t0.6.45-2\t455\nacl\tii \t2.2.53-4\t206\n
    QStringList installedList = dpkgQuery.split('\n');
    foreach (QString line, installedList) {
        QStringList installedDetialList = line.split('\t');

        if(installedDetialList.value(1).startsWith("ii")) {
            QString id = installedDetialList.value(0);
            QStringList fullPackageName = id.split(':');
            // fuzzyPackageName 就是应用标识，这是有问题的！！！
            QString fuzzyPackageName = fullPackageName[0];
            CacheAppInfo apps;
            if(! appInfoList.keys().contains(fuzzyPackageName)) {
                if (! appInfoList.keys().contains(id)) {
                    continue;
                }
                else {
                    apps = appInfoList.value(id);
                }
            }
            apps = appInfoList.value(fuzzyPackageName);

            // unit of size is KiB, 1KiB = 1024Bytes;
            qint64 size = installedDetialList.value(3).toInt() * 1024;

            InstalledAppInfo installInfo;
            installInfo.packageName = fuzzyPackageName;
            installInfo.appName = fuzzyPackageName;
            installInfo.version = installedDetialList.value(2);
            installInfo.size = size;
            installInfo.localeNames = apps.LocaleName;
            installInfo.desktop = d->getPackageDesktop(fuzzyPackageName);
            installInfo.installationTime = getAppInstalledTime(id);
            listInstalledInfo.append(installInfo);

        }
    }
    return  listInstalledInfo;
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
    qDebug()<<"update";
    db.close();
}

//清理lastore接口对象，注销服务和dbus接口，更新job列表
void MetaDataManager::cleanService(QStringList jobList)
{
    Q_D(MetaDataManager);
    LastoreJobService *lastoreJob;
    QStringList deleteJob;
    QMap<QString,LastoreJobService *> jobServiceList = d->getJobServiceList();
    foreach (lastoreJob, jobServiceList) {
        QString job = QString("/com/deepin/lastore/Job") + lastoreJob->id();

        if(!jobList.contains(job)) {
            QString servicePath = QString("/com/deepin/AppStore/Backend/Job") + lastoreJob->id();
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

    qDebug()<<jobServiceList.keys();
    foreach (lastoreJob, jobServiceList) {
        servicePath = QString("/com/deepin/AppStore/Backend/Job") + lastoreJob->id();
        jobList.append(QDBusObjectPath(servicePath));
    }
    d->m_jobList = jobList;
    emit jobListChanged();//通知界面更新
}
