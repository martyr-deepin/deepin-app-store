/*
 * Copyright (C) 2018 Deepin Technology Co., Ltd.
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

#include "services/store_daemon_manager.h"

#include <QThread>
#include <QCoreApplication>

#include "base/launcher.h"

#include "dbus/dbus_consts.h"
#include "dbus/dbus_variant/app_version.h"
#include "dbus/dbus_variant/installed_app_info.h"
#include "dbus/dbus_variant/installed_app_timestamp.h"
#include "dbus/lastore_deb_interface.h"
#include "dbus/lastore_job_interface.h"
#include "dbus/sys_lastore_job_interface.h"

#include "package/package_manager.h"
#include "package/apt_package_manager.h"

namespace dstore
{

namespace
{
const char kResultOk[] = "ok";

const char kResultErrName[] = "errorName";

const char kResultErrMsg[] = "errorMsg";

const char kResult[] = "result";

const char kResultName[] = "name";

bool ReadJobInfo(LastoreJobInterface &job_interface,
                 const QString &job,
                 QVariantMap &result)
{
    result.insert("id", job_interface.id());
    result.insert("job", job);
    result.insert("status", job_interface.status());
    result.insert("type", job_interface.type());
    result.insert("speed", job_interface.speed());
    result.insert("progress", job_interface.progress());
    result.insert("description", job_interface.description());
    result.insert("packages", job_interface.packages());
    result.insert("cancelable", job_interface.cancelable());
    result.insert("downloadSize", job_interface.downloadSize());
    result.insert("createTime", job_interface.createTime());
    result.insert("name", job_interface.name());
    QStringList app_names;

    // Package list may container additional language related packages.
    const QStringList pkgs = job_interface.packages();
    if (pkgs.length() >= 1) {
        const QString &package_name = pkgs.at(0);
        auto packageID = package_name.split(":").first();
        app_names.append(packageID);
    }

    result.insert("names", app_names);

    return (!app_names.isEmpty());
}

bool ReadJobStatus(SysLastoreJobInterface &job_interface,
                 const QString &job,
                 QVariantMap &result)
{
    result.insert("id", job_interface.id());
    result.insert("job", job);
    result.insert("status", job_interface.status());
    result.insert("type", job_interface.type());
    result.insert("speed", job_interface.speed());
    result.insert("progress", job_interface.progress());
    result.insert("description", job_interface.description());
    result.insert("packages", job_interface.packages());
    result.insert("cancelable", job_interface.cancelable());
    result.insert("downloadSize", job_interface.downloadSize());
    result.insert("createTime", job_interface.createTime());
    result.insert("name", job_interface.name());
    return (!job_interface.status().isEmpty());
}

}

class StoreDaemonManagerPrivate
{
public:
    StoreDaemonManagerPrivate(StoreDaemonManager *parent)
        :
        deb_interface_(new LastoreDebInterface(
            kLastoreDebDbusService,
            kLastoreDebDbusPath,
            QDBusConnection::sessionBus(),
            parent)),
        q_ptr(parent)
    {
        aptPM = new AptPackageManager(parent);
        pm = new PackageManager(parent);
        pm->registerDpk("deb", aptPM);
    }

    void initConnections();

    PackageManager *pm = nullptr;
    AptPackageManager *aptPM = nullptr;
    LastoreDebInterface *deb_interface_ = nullptr;

    QMap<QString, QString> apps;

    QVariantList jobs_info;
    QStringList paths;
    QTimer *m_pTimer;
    QMutex mutex;

    StoreDaemonManager *q_ptr;
    Q_DECLARE_PUBLIC(StoreDaemonManager)
};

void StoreDaemonManagerPrivate::initConnections()
{
    Q_Q(StoreDaemonManager);
    q->connect(deb_interface_, &LastoreDebInterface::jobListChanged,
               q, &StoreDaemonManager::onJobListChanged);
}

StoreDaemonManager::StoreDaemonManager(QObject *parent)
    : QObject(parent),
      dd_ptr(new StoreDaemonManagerPrivate(this))
{
    Q_D(StoreDaemonManager);
    this->setObjectName("StoreDaemonManager");

    AppVersion::registerMetaType();
    InstalledAppInfo::registerMetaType();
    InstalledAppTimestamp::registerMetaType();

    d->m_pTimer = new QTimer(this);
    d->m_pTimer->setInterval(1000);
    connect(d->m_pTimer,&QTimer::timeout,this,&StoreDaemonManager::fetchJobsInfo);
    d->m_pTimer->start();

    d->initConnections();
}

StoreDaemonManager::~StoreDaemonManager()
{
}

void StoreDaemonManager::clearArchives()
{
    Q_D(StoreDaemonManager);
    d->deb_interface_->CleanArchives();
}

QVariantMap StoreDaemonManager::updateSource()
{
    Q_D(StoreDaemonManager);
    const QDBusPendingReply<QDBusObjectPath> reply = d->deb_interface_->UpdateSource();
    return QVariantMap{
        {kResultOk, (!reply.value().path().isEmpty())},
        {kResultErrName, ""},
        {kResultErrMsg, ""},
        {kResult, reply.value().path()},
    };
}

void StoreDaemonManager::openApp(const QVariant &app)
{
    Q_D(StoreDaemonManager);
    d->aptPM->Open(app.toString());
}

void StoreDaemonManager::updateAppList(const SearchMetaList &app_list)
{
    Q_D(StoreDaemonManager);
    for (auto &app : app_list) {
        d->apps.insert(app.name, app.name);
    }
}

bool StoreDaemonManager::isDBusConnected()
{
    Q_D(StoreDaemonManager);
    return d->deb_interface_->isValid();
}

QVariantMap StoreDaemonManager::cleanJob(const QString &job)
{
    LastoreJobInterface job_interface(kLastoreDebJobService,
                                      job,
                                      QDBusConnection::sessionBus(),
                                      this);
    if (job_interface.isValid()) {
        const QDBusPendingReply<> reply = job_interface.Clean();
        if (reply.isError()) {
            return QVariantMap{
                {kResultOk, false},
                {kResultErrName, reply.error().name()},
                {kResultErrMsg, reply.error().message()},
                {kResult, job}
            };
        }
        else {
            return QVariantMap{
                {kResultOk, true},
                {kResultErrName, ""},
                {kResultErrMsg, ""},
                {kResult, job}
            };
        }
    }
    else {
        return QVariantMap{
            {kResultOk, false},
            {kResultErrName, job_interface.lastError().name()},
            {kResultErrMsg, job_interface.lastError().message()},
        };
    }
}

QVariantMap StoreDaemonManager::pauseJob(const QString &job)
{
    LastoreJobInterface job_interface(kLastoreDebJobService,
                                      job,
                                      QDBusConnection::sessionBus(),
                                      this);
    if (job_interface.isValid()) {
        const QDBusPendingReply<> reply = job_interface.Pause();
        if (reply.isError()) {
            return QVariantMap{
                {kResultOk, false},
                {kResultErrName, reply.error().name()},
                {kResultErrMsg, reply.error().message()},
                {kResult, job}
            };
        }
        else {
            return QVariantMap{
                {kResultOk, true},
                {kResultErrName, ""},
                {kResultErrMsg, ""},
                {kResult, job}
            };
        }
    }
    else {
        return QVariantMap{
            {kResultOk, false},
            {kResultErrName, job_interface.lastError().name()},
            {kResultErrMsg, job_interface.lastError().message()},
            {
                kResult, QVariantMap{
                {kResultName, job},
            }
            }
        };
    }
}

QVariantMap StoreDaemonManager::startJob(const QString &job)
{
    LastoreJobInterface job_interface(kLastoreDebJobService,
                                      job,
                                      QDBusConnection::sessionBus(),
                                      this);
    if (job_interface.isValid()) {
        const QDBusPendingReply<> reply = job_interface.Start();
        if (reply.isError()) {
            return QVariantMap{
                {kResultOk, false},
                {kResultErrName, reply.error().name()},
                {kResultErrMsg, reply.error().message()},
                {kResult, job}
            };
        }
        else {
            return QVariantMap{
                {kResultOk, true},
                {kResultErrName, ""},
                {kResultErrMsg, ""},
                {kResult, job}
            };
        }
    }
    else {
        return QVariantMap{
            {kResultOk, false},
            {kResultErrName, job_interface.lastError().name()},
            {kResultErrMsg, job_interface.lastError().message()},
            {kResult, job}
        };
    }
}

QVariantMap StoreDaemonManager::installedPackages()
{
    // TODO: filter install list
    Q_D(StoreDaemonManager);
    const QDBusPendingReply<InstalledAppInfoList> reply =
        d->deb_interface_->ListInstalled();
    while (!reply.isFinished()) {
        qApp->processEvents();
    }
    if (reply.isError()) {
        qDebug() << reply.error();
        return QVariantMap{
            {kResultOk, false},
            {kResultErrName, ""},
            {kResultErrMsg, ""},
            {kResult, reply.error().name()},
        };
    }

    const InstalledAppInfoList list = reply.value();
    QVariantList result;
    for (const InstalledAppInfo &info : list) {
        Package pkg;
        pkg.packageName = info.packageName;
        pkg.appName = info.appName;
        auto packageID =   pkg.packageName.split(":").first();
        pkg.localVersion = info.version;
        pkg.desktop = info.desktop;
        pkg.size = info.size;
        pkg.packageURI = "dpk://deb/" + packageID;
        for (const auto& k : info.localeNames.keys()) {
            pkg.allLocalName[k] = info.localeNames[k];
        }
        pkg.icon = dstore::GetThemeIconData(dstore::GetIconFromDesktop(pkg.desktop),48);
        pkg.installedTime = info.installationTime;
        result.append(pkg.toVariantMap());
    }

    // qDebug() << result.data;
    return QVariantMap{
        {kResultOk, true},
        {kResultErrName, ""},
        {kResultErrMsg, ""},
        {kResult, result},
    };
}

QVariantMap StoreDaemonManager::installPackage(const QVariantList &apps)
{
    Q_D(StoreDaemonManager);

    for (auto &package : apps) {
        QString localName = package.toMap().value("name").toString();
        QString id = package.toMap().value("packageName").toString();
        const QDBusPendingReply<QDBusObjectPath> reply =
            d->deb_interface_->Install(localName, id);

        while (!reply.isFinished()) {
            qApp->processEvents();
        }

        if (reply.isError()) {
            qDebug() << reply.error();
        }
    }

    return QVariantMap();
}

QVariantMap StoreDaemonManager::updatePackage(const QVariantList &apps)
{
    return this->installPackage(apps);
}

QVariantMap StoreDaemonManager::removePackage(const QVariantList &apps)
{
    Q_D(StoreDaemonManager);
    for (auto &package : apps) {
        QString localName = package.toMap().value("name").toString();
        QString id = package.toMap().value("packageName").toString();
        const QDBusPendingReply<QDBusObjectPath> reply =
            d->deb_interface_->Remove(localName, id);

        while (!reply.isFinished()) {
            qApp->processEvents();
        }

        if (reply.isError()) {
            qDebug() << reply.error();
        }
    }
     return QVariantMap();
}

QVariantMap StoreDaemonManager::jobList()
{
    // TODO(Shaohua): List flatpak jobs.
    Q_D(StoreDaemonManager);
    const QList<QDBusObjectPath> jobs = d->deb_interface_->jobList();
    QStringList paths;
    for (const QDBusObjectPath &job : jobs) {
        paths.append(job.path());
    }
    return QVariantMap{
        {kResultOk, true},
        {kResultErrName, ""},
        {kResultErrMsg, ""},
        {kResult, paths}
    };
}

QVariantMap StoreDaemonManager::queryVersions(const QStringList &apps)
{
    Q_D(StoreDaemonManager);
    auto result = d->pm->QueryVersion(apps);
    return QVariantMap{
        {kResultOk, result.success},
        {kResultErrName, result.errName},
        {kResultErrMsg, result.errMsg},
        {kResult, result.data},
    };
}

QVariantMap StoreDaemonManager::query(const QVariantList &apps)
{
    Q_D(StoreDaemonManager);

    QStringList list;
    for (auto v : apps) {
        list.append(v.toString());
    }
    const QDBusPendingReply<AppVersionList> reply =
        d->deb_interface_->QueryVersion(list);

    while (!reply.isFinished()) {
        qApp->processEvents();
    }

    if (reply.isError()) {
        qWarning() << reply.error();
        return QVariantMap{
            {kResultOk, false},
            {kResultErrName, ""},
            {kResultErrMsg, ""},
            {kResult, reply.error().name()},
        };
    }

    const AppVersionList version_list = reply.value();
    QMap<QString, Package> result;
    for (const AppVersion &version : version_list) {
        auto package_name = version.pkg_name;
        auto packageID =  package_name.split(":").first();
        // TODO: remove name
        Package pkg;
        pkg.packageURI = /*"dpk://deb/" + */packageID;
        pkg.packageName = package_name;
        pkg.localVersion = version.installed_version;
        pkg.remoteVersion = version.remote_version;
        pkg.upgradable = version.upgradable;
        pkg.appName = packageID;
        result.insert(packageID, pkg);
    }

    /*const QDBusPendingReply<InstalledAppTimestampList> installTimeReply =
        d->deb_interface_->QueryInstallationTime(list);

    while (!installTimeReply.isFinished()) {
        qApp->processEvents();
    }

    if (installTimeReply.isError()) {
        qDebug() << installTimeReply.error();
    }

//    installTimeReply.waitForFinished();

    const InstalledAppTimestampList timestamp_list = installTimeReply.value();

    for (const InstalledAppTimestamp &timestamp : timestamp_list) {
        auto package_name = timestamp.pkg_name;
        auto packageID =  package_name.split(":").first();
        auto pkg = result.value(package_name);
        pkg.installedTime =  timestamp.timestamp;
        result.insert(package_name, pkg);
    }*/

    QVariantMap data;
    for (auto &p : result) {
        data.insert(p.packageURI, p.toVariantMap());
    }

    return QVariantMap{
        {kResultOk, true},
        {kResultErrName,""},
        {kResultErrMsg,""},
        {kResult, data},
    };
}

QVariantMap StoreDaemonManager::queryDownloadSize(const QVariantList &apps)
{
    Q_D(StoreDaemonManager);

    QMap<QString, Package> result;
    QString packageName = apps.first().toString();
    const QDBusPendingReply<qlonglong> sizeReply =
        d->deb_interface_->QueryDownloadSize(packageName);

    while (!sizeReply.isFinished()) {
        qApp->processEvents();
    }

    if (sizeReply.isError()) {
        qDebug() << sizeReply.error();
        return QVariantMap{
            {kResultOk, false},
            {kResultErrName, ""},
            {kResultErrMsg, ""},
            {kResult, sizeReply.error().name()},
        };
    }

    Package pkg;
    auto packageID =  packageName.split(":").first();
    pkg.packageURI = "dpk://deb/" + packageID;
    pkg.packageName = packageName;
    const qlonglong size = sizeReply.value();
    pkg.size = 0;
    pkg.downloadSize = size;

    result.insert(packageName, pkg);

    QVariantMap data;
    for (auto &p : result) {
        data.insert(p.packageURI, p.toVariantMap());
    }

    return QVariantMap{
        {kResultOk, true},
        {kResultErrName, ""},
        {kResultErrMsg, ""},
        {kResult, data},
    };
}

QVariantMap StoreDaemonManager::getJobInfo(const QString &job)
{
    QVariantMap result;
    LastoreJobInterface job_interface(kLastoreDebJobService,
                                      job,
                                      QDBusConnection::sessionBus(),
                                      this);
    if (job_interface.isValid()) {
        if (ReadJobInfo(job_interface, job, result)) {
            return QVariantMap{
                {kResultOk, true},
                {kResultErrName, ""},
                {kResultErrMsg, ""},
                {kResult, result},
            };
        }
        else {
            return QVariantMap{
                {kResultOk, false},
                {kResultErrName, "app name list is empty"},
                {kResultErrMsg, ""},
                {kResult, job},
            };
        }
    }
    else {
        return QVariantMap{
            {kResultOk, false},
            {kResultErrName, "Invalid job interface"},
            {kResultErrMsg, job_interface.lastError().message()},
            {
                kResult, QVariantMap{
                {kResultName, job},
            }
            },
        };
    }
}

QVariantMap StoreDaemonManager::getJobStatus(const QString &job)
{
    QVariantMap result;
    SysLastoreJobInterface job_interface("com.deepin.lastore",
                                      job,
                                      QDBusConnection::systemBus(),
                                      this);
    if (job_interface.isValid()) {
        if (ReadJobStatus(job_interface, job, result)) {
            return QVariantMap{
                {kResultOk, true},
                {kResultErrName, ""},
                {kResultErrMsg, ""},
                {kResult, result},
            };
        }
        else {
            return QVariantMap{
                {kResultOk, false},
                {kResultErrName, "Invalid job"},
                {kResultErrMsg, ""},
                {kResult, job},
            };
        }
    }
    else {
        return QVariantMap{
            {kResultOk, false},
            {kResultErrName, "Invalid job interface"},
            {kResultErrMsg, job_interface.lastError().message()},
            {
                kResult, QVariantMap{
                {kResultName, job},
            }
            },
        };
    }
}


QVariantMap StoreDaemonManager::getJobsInfo(const QStringList &jobs)
{
    Q_D(StoreDaemonManager);
    QMutexLocker locker(&d->mutex);
    return QVariantMap{
        {kResultOk, true},
        {kResultErrName, ""},
        {kResultErrMsg, ""},
        {kResult, d->jobs_info},
    };
}

void StoreDaemonManager::fetchJobsInfo()
{
    Q_D(StoreDaemonManager);
    QMutexLocker locker(&d->mutex);
    d->jobs_info.clear();
    if(d->paths.isEmpty())
        return;

    for (const QString &jobPath : d->paths) {
        QVariantMap job_info;
        LastoreJobInterface job_interface(kLastoreDebJobService,
                                          jobPath,
                                          QDBusConnection::sessionBus());
        if (job_interface.isValid()) {
            if (ReadJobInfo(job_interface, jobPath, job_info)) {
                d->jobs_info.append(job_info);
            }
            else {
                qWarning() << "Invalid app_names for job:" << jobPath;
            }
        }
    }
}

void StoreDaemonManager::onJobListChanged()
{
    Q_D(StoreDaemonManager);
    d->paths.clear();
    const QList<QDBusObjectPath> jobs = d->deb_interface_->jobList();
//    QStringList paths;
    for (const QDBusObjectPath &job : jobs) {
        d->paths.append(job.path());
    }
    fetchJobsInfo();
    emit this->jobListChanged(d->paths);
}

QVariantMap StoreDaemonManager::fixError(const QString &error_type)
{
    Q_D(StoreDaemonManager);
    const QDBusObjectPath path = d->deb_interface_->FixError(error_type);
    const QString job_path = path.path();
    return QVariantMap{
        {kResultOk, (!job_path.isEmpty())},
        {kResultErrName, ""},
        {kResultErrMsg, ""},
        {kResult, job_path},
    };
}

void StoreDaemonManager::onFinish(const QVariantMap &result)
{

}

}  // namespace dstore
