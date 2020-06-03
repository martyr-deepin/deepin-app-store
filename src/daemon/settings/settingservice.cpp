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
#include "settingservice.h"

SettingService::SettingService(QObject *parent) : QObject(parent)
{
    QString dirPath = QProcessEnvironment::systemEnvironment().value("XDG_CONFIG_HOME");
    QString configFolder;

    if(dirPath != ""){
        configFolder = dirPath;
    }else {
        dirPath =  QProcessEnvironment::systemEnvironment().value("HOME");
        configFolder = dirPath + "/.config";
    }

    configFolder += "/deepin/deepin-app-store";

    sysCfg = new QSettings(appStoreConfPath, QSettings::IniFormat);
    qDebug()<<appStoreConfPath;
    userCfg = new QSettings(configFolder + "/settings.ini", QSettings::IniFormat);
}

QVariant SettingService::getServer(){
//    QSettings setting("/etc/deepin-version", QSettings::IniFormat);
//    setting.beginGroup("Release");
//    QString version = setting.value("Type").toString();
//    setting.endGroup();
    qint16 version =  Dtk::Core::DSysInfo::deepinType();
    QString storeServer;

    switch (version) {
    case 0:
        storeServer = "https://store.chinauos.com";
        break;
    case 1:
        storeServer = "http://community-store.deepin.com";
        break;
    case 2:
        storeServer = "https://professional-store.chinauos.com";
        break;
    case 3:
        storeServer = "https://enterprise-store.chinauos.com";
        break;
    case 4:
        storeServer = "https://home-store.chinauos.com";
        break;
    default:
        storeServer = "https://store.chinauos.com";
        break;
    }

    return sysCfg->value(keyServer,storeServer).toString();
}

QVariant SettingService::getMetadataServer(){
    return sysCfg->value(keyMetadataServer).toString();
}

QVariantMap SettingService::getOperationServerMap(){
    QVariantMap servers;
    sysCfg->beginGroup(gOperationServer);
    QStringList keyList = sysCfg->allKeys();
    QString key;

    foreach (key, keyList) {
        QString groupKey = QString("%1/%2").arg(gOperationServer).arg(key);
        servers[key] = sysCfg->value(groupKey);
    }

    sysCfg->endGroup();
    return servers;
}

QVariant SettingService::getSupportSignIn(){
    return sysCfg->value(keySupportSignIn,true).toBool();
}

QVariant SettingService::getUpyunBannerVisible(){
    return sysCfg->value(keyUpyunBannerVisible,true).toBool();
}

QVariant SettingService::getAllowSwitchRegion(){
    return sysCfg->value(keyAllowSwitchRegion,false).toBool();
}

QVariant SettingService::getWindowState(){
    userCfg->beginGroup(gWebWindow);
    QString windowState =  userCfg->value(keyWindowState).toString();
    userCfg->endGroup();
    return windowState;
}

QVariant SettingService::getAutoInstall(){
    return userCfg->value(keyAutoInstall).toBool();
}

QVariant SettingService::getDefaultRegion(){
    return sysCfg->value(keyDefaultRegion,"CN").toString();
}

QVariant SettingService::getThemeName(){
    return userCfg->value(keyThemeName,"light").toString();
}

QVariant SettingService::getAllowShowPackageName(){
    return userCfg->value(keyAllowShowPackageName).toBool();
}

QVariant SettingService::getSupportAot(){
    return sysCfg->value(keySupportAot,false).toBool();
}

// GetInterfaceName return dbus interface name
QString SettingService::GetInterfaceName(){
    return dbusSettingsInterface;
}

//TODO: SetSettings update dstore settings
void SettingService::SetSettings(QString key, QString value)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<settingKey>();
    int enumValue = metaEnum.keyToValue(key.toStdString().c_str());

    switch (enumValue) {
    case 0:
        userCfg->setValue(keyAutoInstall,value);
        break;
    case 1:
        userCfg->setValue(keyThemeName,value);
        break;
    case 2:
        userCfg->beginGroup(gWebWindow);
        userCfg->setValue(keyWindowState,value);
        userCfg->endGroup();
        break;
    default:
        qDebug() << "Non-existent key value";
        break;
    }
}

//TODO: GetSettings read setting of system and user
QDBusVariant SettingService::GetSettings(QString key)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<settingKey>();
    int enumValue = metaEnum.keyToValue(key.toStdString().c_str());
    QVariant value;

    switch (enumValue) {
    case 0:
        value = getAutoInstall();
        break;
    case 1:
        value = getThemeName();
        break;
    case 2:
        value = getWindowState();
        break;
    case 3:
        value = getServer();
        break;
    case 4:
        value = getMetadataServer();//Not tested
        break;
    case 5:
        value = getOperationServerMap();//Not tested
        break;
    case 6:
        value = getDefaultRegion();
        break;
    case 7:
        value = getSupportSignIn();
        break;
    case 8:
        value = getUpyunBannerVisible();
        break;
    case 9:
        value = getAllowSwitchRegion();
        break;
    case 10:
        value = getAllowShowPackageName();
        break;
    case 11:
        value = getSupportAot();
        break;
    default:
        qDebug() << "Non-existent key value";
        break;
    }
    qDebug() << key << value;
    return QDBusVariant(value);
}

