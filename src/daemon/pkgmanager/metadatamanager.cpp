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

class MetaDataManagerPrivate
{
public:
    MetaDataManagerPrivate(MetaDataManager *parent) :
        q_ptr(parent)
    {
    }

    qint64     lastRepoUpdated;
    QMap<QString,CacheAppInfo> repoApps;

    QMap<QString,CacheAppInfo> listStorePackages();
    QString getPackageDesktop(QString packageName);
    QDBusObjectPath addJob(QDBusObjectPath);

    QList<QDBusObjectPath> m_jobList;//joblist
    //使用QString为了方便比较
    QMap<QString,LastoreJobService *> m_jobServiceList;

    MetaDataManager *q_ptr;
    Q_DECLARE_PUBLIC(MetaDataManager)
};

QMap<QString,CacheAppInfo> MetaDataManagerPrivate::listStorePackages()
{
    QFileInfo info("/var/cache/apt/pkgcache.bin");
    if (info.exists()) {
        if(info.lastModified().toSecsSinceEpoch() < lastRepoUpdated){
            return repoApps;
        }
    }
    else {
        if(info.lastModified().toSecsSinceEpoch() > 0){
            return repoApps;
        }
    }

    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    qputenv("LC_ALL", "C");
    process.setProcessEnvironment(env);
    process.start("/bin/bash");

    process.write("/usr/bin/aptitude search '?origin(Uos)'");
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

    d->m_jobServiceList[servicePath] = lastoreJob;
    updateJobList();//update JobList
    return QDBusObjectPath(servicePath);
}

QList<QDBusObjectPath> MetaDataManager::getJobList()
{
    Q_D(MetaDataManager);
    return d->m_jobList;
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
    updateJobList();//update JobList
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
