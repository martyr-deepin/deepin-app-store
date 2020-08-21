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


#include "pkg_cache_implement.h"
#include <chrono>
#include <fstream>

#define GET_STR(x) #x

QMap<QString,QString> pkg_cache_implement::getSystemInstalledList()
{
    QMap<QString,QString> versionList;

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

    return versionList;
}

QVariantMap pkg_cache_implement::getStoreDistributedList(QMap<QString,QString>& versionList)
{
    QVariantMap storeList;

    QString appstore = QString("/etc/apt/sources.list.d/appstore.list");
    QFile storeFile(appstore);
    if(!storeFile.exists())
        return QVariantMap{};
    if (!storeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QVariantMap{};
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

    //get all package list
    QDir dir("/var/lib/apt/lists/");
    if(!dir.exists()) {
        qDebug()<<dir.absolutePath()<<"is not exist";
        return QVariantMap{};
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

        for (int i = 0; i < packageList.size(); ++i) {
            QStringList list = packageList.value(i).split('\n');
            QMap<QString,QVariant> appMap;
            appMap.clear();

            QRegExp rx("[?\\ \\:]");
            for(int i=0;i<list.size();i++) {
                QString str = list.value(i);

                QStringList localList = str.split(rx, QString::SkipEmptyParts);
                QString type = localList.value(0);
                QString value = localList.value(1);

                if(!appMap.contains("appID") && type == "Package") {
                    appMap.insert("appID",value);
                    appMap.insert("localVer",versionList.value(value));
                }
                else if(!appMap.contains("remoteVer") && type == "Version") {
                    appMap.insert("remoteVer",value);
                }
                else if(!appMap.contains("appArch") && type == "Architecture") {
                    appMap.insert("appArch",value);
                }
                else if(!appMap.contains("installSize") && type == "Installed-Size") {
                    appMap.insert("installSize",value);
                }
                else if(!appMap.contains("debSize") && type == "Size") {
                    appMap.insert("debSize",value);
                }
            }
            storeList.insert(appMap.value("appID").toString(),QVariant(appMap));
        }
    }
    return storeList;
}

void pkg_cache_implement::connectToDataBase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");

    m_db.setDatabaseName("/usr/share/deepin-app-store/cache.db");
    m_db.setUserName("root");
    m_db.setPassword("deepin-app-store");

    m_bSqlIsOpen = m_db.open();
}

bool pkg_cache_implement::createPackageTable()
{
    if(!m_bSqlIsOpen)
        return false;

    QSqlQuery query(m_db);
    bool bRet = query.exec("create table if not exists Packages "
                           "(id char(50) primary key not null, "
                           "localver char(20), "
                           "remotever char(20), "
                           "arch    char(10), "
                           "installsize   char(20), "
                           "debsize     char(20), "
                           "binpath char(100))");

    return bRet;
}

bool pkg_cache_implement::insertPackageTable(QVariantMap& storeList)
{
    if(!m_bSqlIsOpen)
        return false;

    QSqlQuery query(m_db);

    m_db.transaction();

    std::ostringstream oss;
    QString sqlCmd;
    for(const auto& var: storeList)
    {
        auto mapRow = var.toMap();
        oss << "replace into Packages (id, localver, remotever, arch, installsize, debsize, binpath) values ('" <<
               mapRow.value("appID").toString().toStdString() << "' , '" <<
               mapRow.value("localVer").toString().toStdString() << "', '" <<
               mapRow.value("remoteVer").toString().toStdString() << "', '" <<
               mapRow.value("appArch").toString().toStdString() << "', '" <<
               mapRow.value("installSize").toString().toStdString() << "', '" <<
               mapRow.value("debSize").toString().toStdString() << "', '" <<
               "/opt/apps/" <<"')";

        QString sqlCmd = oss.str().c_str();
        oss.str("");

        query.exec(sqlCmd);
    }

    m_db.commit();

    return true;
}

bool pkg_cache_implement::updatePackageTable(QVariantMap& storeList)
{
    return insertPackageTable(storeList);
}

std::map<std::string,std::string>  pkg_cache_implement::getDesktopPackage()
{
    std::map<std::string,std::string> map_{};

    auto shellCmd = "dpkg --search '*.desktop'";

    FILE *pp = nullptr;
    if((pp = popen(shellCmd, "r")) == nullptr )
    {
        perror("popen() error!");
        return map_;
    }

    char buf[1024] = {0};
    std::string szTmp{""},szResult{""},szPackageName{""},szDesktopName{""};
    while(fgets(buf, sizeof buf, pp))
    {
        szTmp = buf;
        szTmp.erase(std::remove(szTmp.begin(), szTmp.end(), '\n'), szTmp.end());
        size_t nIndex = szTmp.find_last_of(':');
        szPackageName = szTmp.substr(0,nIndex);
        szDesktopName = szTmp.substr(nIndex+2,szTmp.length()- nIndex - 2);

        map_[szDesktopName] = szPackageName;

        memset(buf,'\0',sizeof buf);
    }

    pclose(pp);

    return map_;
}

bool pkg_cache_implement::checkLinkedPath(const char *linkedPath)
{
    if(linkedPath == nullptr)
        return false;

    std::string szFileInfo = linkedPath;
    if (szFileInfo.find("->") != std::string::npos)
        return true;

    return false;
}



void pkg_cache_implement::getSymbolicDesktopPackage(std::map<std::string,std::string>& desktopPackages)
{
    auto shellCmd = "ls -l /usr/share/applications/";

    FILE *pp = nullptr;
    if((pp = popen(shellCmd, "r")) == nullptr )
    {
        perror("popen() error!");
        return;
    }

    char buf[1024] = {0};
    std::string szTmp{""},szSymbolic{""},szDestLink{""};
    std::set<std::string> setSymbols;
    while(fgets(buf, sizeof buf, pp))
    {
        szTmp = buf;
        szTmp.erase(std::remove(szTmp.begin(), szTmp.end(), '\n'), szTmp.end());

        if(checkLinkedPath(szTmp.c_str()))
            setSymbols.insert(szTmp);

        memset(buf,'\0',sizeof buf);
    }

    pclose(pp);

    for(const auto& var: setSymbols)
    {
        size_t nPos = var.find("->");
        if(nPos > 0)
        {
            szSymbolic = var.substr(0,nPos - 1);
            size_t spaceLast = szSymbolic.find_last_of(" ");
            szSymbolic = szSymbolic.substr(spaceLast+1,nPos - spaceLast - 2);
            szSymbolic = "/usr/share/applications/" + szSymbolic;
            szDestLink = var.substr(nPos+3,var.length() - nPos - 3);

            desktopPackages[szSymbolic] = desktopPackages[szDestLink];

            szSymbolic = szDestLink = "";
        }
    }
}

std::set<std::string> pkg_cache_implement::getStoreHasInstalledList(QVariantMap& storeList)
{
    std::set<std::string> setStoreInstalled;
    for(const auto& var:storeList)
    {
        auto kvl = var.toMap();

        QString appLocalVersion = kvl.value("localVer").toString();

        if(!appLocalVersion.isNull())
        {
            setStoreInstalled.insert(kvl.value("appID").toString().toStdString());
        }
    }

    return setStoreInstalled;
}

std::set<std::string> pkg_cache_implement::queryPackageInfo(const std::string &szPackageName)
{
    if(szPackageName == "")
        return std::set<std::string>{};

    char shellCmd[260] = {0};
    strcpy( shellCmd, "dpkg -L " );
    strcat( shellCmd, szPackageName.c_str());
    strcat( shellCmd, " | grep \"/usr/bin\\|/usr/local/bin\\|/usr/share\\|/usr/sbin\\|/opt/apps\"");

    FILE *pp = nullptr;
    if((pp = popen(shellCmd, "r")) == nullptr )
    {
        perror("popen() error!");
        return std::set<std::string>{};
    }

    std::set<std::string> setPaths;

    char buf[1024] = {0};
    std::string szTmp = "";

    while(fgets(buf, sizeof buf, pp))
    {
        szTmp = buf;
        szTmp.erase(std::remove(szTmp.begin(), szTmp.end(), '\n'), szTmp.end());
        setPaths.insert(szTmp);
        memset(buf,'\0',sizeof buf);
    }

    pclose(pp);

    return setPaths;
}



bool pkg_cache_implement::createPackageLinkTable()
{
    if(!m_bSqlIsOpen)
        return false;

    QSqlQuery query(m_db);
    bool bRet = query.exec("create table if not exists package_binary_file "
                           "(binaryPath varchar(260) primary key, "
                           "packageName varchar(40))");
    return bRet;
}

bool pkg_cache_implement::initLinkTable(const std::map<std::string,std::string>& desktopPackages, std::set<std::string>& installedPackages)
{
    if(!m_bSqlIsOpen)
        return false;

    bool bRet = createPackageLinkTable();

    m_db.transaction();

    QSqlQuery query(m_db);

    std::ostringstream oss;

    for(const auto& var: desktopPackages)
    {
        installedPackages.insert(var.second);
        oss << "replace into package_binary_file (binaryPath, packageName) values ('" << var.first << "', '" << var.second <<"')";
        QString sqlCmd = oss.str().c_str();
        oss.str("");

        query.exec(sqlCmd);
    }

    for(const auto& packageName: installedPackages)
    {
        auto setPackageLinkPaths = queryPackageInfo(packageName);
        std::ostringstream oss;
        for(const auto& var: setPackageLinkPaths)
        {
            oss << "replace into package_binary_file (binaryPath, packageName) values ('" << var << "', '" << packageName <<"')";
            QString sqlCmd = oss.str().c_str();
            oss.str("");

            query.exec(sqlCmd);
        }
    }

    m_db.commit();

    return bRet;
}

bool pkg_cache_implement::checkDaemonState()
{
    std::string shellCmd = "ps -ef |grep 'deepin-app-store-daemon' |grep -v \"grep\" |wc -l";

    FILE *pp = nullptr;
    if((pp = popen(shellCmd.c_str(), "r")) == nullptr )
    {
        perror("popen() error!");
        return false;
    }

    char buf[1024] = {0};
    std::string szTmp = "";

    auto ptr = fgets(buf, sizeof buf, pp);
    (void)ptr;
    pclose(pp);

    szTmp = buf;
    szTmp.erase(std::remove(szTmp.begin(), szTmp.end(), '\n'), szTmp.end());

    if(std::stoi(szTmp) > 0)
        return true;

    return false;
}

std::map<std::string, std::string> pkg_cache_implement::getDesktopPackage(const std::string &packageName)
{
    std::map<std::string,std::string> map_{};

    std::ostringstream oss;
    oss << "dpkg --search '*.desktop' | grep '" << packageName << "'";
    std::string shellCmd = oss.str();

    FILE *pp = nullptr;
    if((pp = popen(shellCmd.c_str(), "r")) == nullptr )
    {
        perror("popen() error!");
        return map_;
    }

    char buf[1024] = {0};
    std::string szTmp{""},szResult{""},szPackageName{""},szDesktopName{""};
    while(fgets(buf, sizeof buf, pp))
    {
        szTmp = buf;
        szTmp.erase(std::remove(szTmp.begin(), szTmp.end(), '\n'), szTmp.end());
        size_t nIndex = szTmp.find_last_of(':');
        szPackageName = szTmp.substr(0,nIndex);
        szDesktopName = szTmp.substr(nIndex+2,szTmp.length()- nIndex - 2);

        if(szPackageName == packageName)
            map_[szDesktopName] = szPackageName;

        memset(buf,'\0',sizeof buf);
    }

    pclose(pp);

    return map_;
}

void pkg_cache_implement::getSymbolicDesktopPackage(std::map<std::string, std::string> &desktopPackages, const std::string &packageName)
{
    auto shellCmd = "ls -l /usr/share/applications/";

    FILE *pp = nullptr;
    if((pp = popen(shellCmd, "r")) == nullptr )
    {
        perror("popen() error!");
        return;
    }

    char buf[1024] = {0};
    std::string szTmp{""},szSymbolic{""},szDestLink{""};
    std::set<std::string> setSymbols;
    while(fgets(buf, sizeof buf, pp))
    {
        szTmp = buf;
        szTmp.erase(std::remove(szTmp.begin(), szTmp.end(), '\n'), szTmp.end());

        if(checkLinkedPath(szTmp.c_str()))
            setSymbols.insert(szTmp);

        memset(buf,'\0',sizeof buf);
    }

    pclose(pp);

    for(const auto& var: setSymbols)
    {
        size_t nPos = var.find("->");
        if(nPos > 0)
        {
            szSymbolic = var.substr(0,nPos - 1);
            size_t spaceLast = szSymbolic.find_last_of(" ");
            szSymbolic = szSymbolic.substr(spaceLast+1,nPos - spaceLast - 2);
            szSymbolic = "/usr/share/applications/" + szSymbolic;
            szDestLink = var.substr(nPos+3,var.length() - nPos - 3);

            if(desktopPackages[szDestLink] == packageName)
                desktopPackages[szSymbolic] = desktopPackages[szDestLink];

            szSymbolic = szDestLink = "";
        }
    }
}


void pkg_cache_implement::updateStoreTableByInstall(const std::string& packageName,const std::string& packageVersion)
{/*仅维护所有商店内应用，而不是所有应用，此处先查询再确定是否更新表*/
    if(!m_bSqlIsOpen)
        return;

    QSqlQuery query(m_db);

    std::ostringstream oss;
    oss << "select count(*) from Packages where id='" << packageName << "'";
    query.exec(oss.str().c_str());
    oss.str("");
    int nCount = query.record().count();

    if(nCount == 1)
    {//存在该包名的相关信息
        oss << "update Packages set localVer='" << packageVersion << "' where id='" << packageName << "'";
        query.exec(oss.str().c_str());
    }
}

void pkg_cache_implement::updateAppPathTableByInstall(const std::string& packageName)
{/*replace into app bin path和desktop path*/
    if(!m_bSqlIsOpen)
        return;

    auto mapDesktop = getDesktopPackage(packageName);
    //getSymbolicDesktopPackage(mapDesktop,packageName);

    m_db.transaction();

    QSqlQuery query(m_db);
    std::ostringstream oss;
    for(const auto& var: mapDesktop)
    {
        oss << "replace into package_binary_file (binaryPath, packageName) values ('" << var.first << "', '" << var.second <<"')";
        QString sqlCmd = oss.str().c_str();
        oss.str("");

        query.exec(sqlCmd);
    }

    auto setPackageLinkPaths = queryPackageInfo(packageName);//查询二进制路径并写入数据库

    for(const auto& var: setPackageLinkPaths)
    {
        oss << "replace into package_binary_file (binaryPath, packageName) values ('" << var << "', '" << packageName <<"')";
        QString sqlCmd = oss.str().c_str();
        oss.str("");

        query.exec(sqlCmd);
    }

    //fix dpkg -i command trigger invalid package
    if(!setPackageLinkPaths.size())
    {
        updateStoreTableByRemove(packageName);
    }

    m_db.commit();

}

void pkg_cache_implement::updateStoreTableByRemove(const std::string& packageName)
{/*仅维护所有商店内应用，而不是所有应用，此处先查询再确定是否更新表*/
    if(!m_bSqlIsOpen)
        return;

    QSqlQuery query(m_db);

    std::ostringstream oss;
    oss << "select count(*) from Packages where id='" << packageName << "'";
    query.exec(oss.str().c_str());
    oss.str("");
    int nCount = query.record().count();

    if(nCount == 1)
    {//存在该包名的相关信息，清空版本信息
        oss << "update Packages set localVer=null where id='" << packageName << "'";
        query.exec(oss.str().c_str());
    }
}

void pkg_cache_implement::updateAppPathTableByRemove(const std::string& packageName)
{/*delete from table:bin path和desktop path*/
    if(!m_bSqlIsOpen)
        return;

    QSqlQuery query(m_db);

    std::ostringstream oss;
    oss << "delete from package_binary_file where packageName='" << packageName << "'";
    query.exec(oss.str().c_str());
}

pkg_cache_implement::pkg_cache_implement() noexcept :
    m_bSqlIsOpen(false)
{
    sem = sem_open("/cache_sem", O_CREAT, 0666, 1);
}

pkg_cache_implement::~pkg_cache_implement()
{
    m_db.close();
    m_bSqlIsOpen = false;
}

pkg_cache_implement *pkg_cache_implement::instance()
{
    static pkg_cache_implement s_pkgCache;
    return &s_pkgCache;
}

void pkg_cache_implement::createDataBase()
{
    while (sem_wait(sem) == -1 && errno == EINTR);

    system("rm -rf /usr/share/deepin-app-store/cache.db");

    auto versionList = getSystemInstalledList();
    auto storeList = getStoreDistributedList(versionList);
    auto installedList =  getStoreHasInstalledList(storeList);

    connectToDataBase();
    createPackageTable();
    insertPackageTable(storeList);

    auto mapDesktop_ = getDesktopPackage();
    getSymbolicDesktopPackage(mapDesktop_);
    initLinkTable(mapDesktop_,installedList);

    sem_post(sem);
}

void pkg_cache_implement::updateDataBase()
{
    while (sem_wait(sem) == -1 && errno == EINTR);

    auto versionList = getSystemInstalledList();
    auto storeList = getStoreDistributedList(versionList);
    auto installedList =  getStoreHasInstalledList(storeList);

    connectToDataBase();
    createPackageTable();
    insertPackageTable(storeList);

    sem_post(sem);
}

void pkg_cache_implement::updateDataBaseByInstall(const std::string& packageName,const std::string& packageVersion)
{
    while (sem_wait(sem) == -1 && errno == EINTR);

    connectToDataBase();
    updateStoreTableByInstall(packageName,packageVersion);
    updateAppPathTableByInstall(packageName);

    sem_post(sem);
}

void pkg_cache_implement::updateDataBaseByRemove(const std::string& packageName)
{
    while (sem_wait(sem) == -1 && errno == EINTR);

    connectToDataBase();
    updateStoreTableByRemove(packageName);
    updateAppPathTableByRemove(packageName);

    sem_post(sem);
}
