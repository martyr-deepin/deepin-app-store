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


#pragma once
#include <QMap>
#include <QDebug>
#include <QProcess>
#include <QVariantMap>
#include <QFile>
#include <QDir>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <DLog>
#include <set>
#include <semaphore.h>
#include <fcntl.h>
#include <sstream>
class pkg_cache_implement
{
    QMap<QString,QString> getSystemInstalledList();//获取系统里面所有已经安装的应用
    QVariantMap getStoreDistributedList(QMap<QString,QString>& versionList);//商店里已经上架应用
    std::set<std::string> getStoreHasInstalledList(QVariantMap& storeList);//获取已安装的列表
    std::set<std::string> queryPackageInfo(const std::string& szPackageName);//根据已安装的包名来获取可执行文件路径
    bool checkLinkedPath(const char *linkedPath);

    void connectToDataBase();//连接到数据库引擎
    bool createPackageTable();//创建基础数据库表
    bool insertPackageTable(QVariantMap& storeList);//写入基础数据库
    bool updatePackageTable(QVariantMap& storeList);//更新基础数据库

    std::map<std::string,std::string> getDesktopPackage();//建立desktop file -> package 映射
    void getSymbolicDesktopPackage(std::map<std::string,std::string>& desktopPackages);//建立symbolic desktop file -> package 映射

    bool createPackageLinkTable();//新建(可执行文件->包名)映射数据表
    bool initLinkTable(const std::map<std::string,std::string>& desktopPackages, std::set<std::string>& installedPackages);//初始化可执行文件映射数据表并更新数据


    std::map<std::string,std::string> getDesktopPackage(const std::string& packageName);//跟据包名建立desktop file -> package 映射
    void getSymbolicDesktopPackage(std::map<std::string,std::string>& desktopPackages,const std::string& packageName);//根据包名建立symbolic desktop file -> package 映射
    void updateStoreTableByInstall(const std::string& packageName,const std::string& packageVersion);//更新商店表
    void updateAppPathTableByInstall(const std::string& packageName);//更新package_binary_path表


    void updateStoreTableByRemove(const std::string& packageName);//更新商店表
    void updateAppPathTableByRemove(const std::string& packageName);//更新package_binary_path表


    QSqlDatabase m_db;
    bool m_bSqlIsOpen;

    sem_t *sem;

public:
    explicit pkg_cache_implement() noexcept;
    ~pkg_cache_implement();

    static pkg_cache_implement* instance();

    void createDataBase();
    void updateDataBase();
    void updateDataBaseByInstall(const std::string& packageName,const std::string& packageVersion);
    void updateDataBaseByRemove(const std::string& packageName);
    bool checkDaemonState();


};

