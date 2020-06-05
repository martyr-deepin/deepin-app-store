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
    case Dtk::Core::DSysInfo::DeepinType::UnknownDeepin:
        storeServer = "https://store.chinauos.com";
        break;
    case Dtk::Core::DSysInfo::DeepinType::DeepinDesktop:
        storeServer = "http://community-store.deepin.com";
        break;
    case Dtk::Core::DSysInfo::DeepinType::DeepinProfessional:
        storeServer = "https://professional-store.chinauos.com";
        break;
    case Dtk::Core::DSysInfo::DeepinType::DeepinServer:
        storeServer = "https://enterprise-store.chinauos.com";
        break;
    case Dtk::Core::DSysInfo::DeepinType::DeepinPersonal:
        storeServer = "https://home-store.chinauos.com";
        break;
    default:
        storeServer = "https://store.chinauos.com";
        break;
    }

    return sysCfg->value(upServer,storeServer).toString();
}

QVariant SettingService::getMetadataServer(){
    return sysCfg->value(upMetadataServer).toString();
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
    return sysCfg->value(upSupportSignIn,true).toBool();
}

QVariant SettingService::getUpyunBannerVisible(){
    return sysCfg->value(upUpyunBannerVisible,true).toBool();
}

QVariant SettingService::getAllowSwitchRegion(){
    return sysCfg->value(upAllowSwitchRegion,false).toBool();
}

QVariant SettingService::getWindowState(){
    userCfg->beginGroup(gWebWindow);
    QString windowState =  userCfg->value(lowWindowState).toString();
    userCfg->endGroup();
    return windowState;
}

QVariant SettingService::getAutoInstall(){
    return userCfg->value(lowAutoInstall).toBool();
}

QVariant SettingService::getDefaultRegion(){
    return sysCfg->value(upDefaultRegion,"CN").toString();
}

QVariant SettingService::getThemeName(){
    return userCfg->value(lowThemeName,"light").toString();
}

QVariant SettingService::getAllowShowPackageName(){
    return userCfg->value(lowAllowShowPackageName).toBool();
}

QVariant SettingService::getSupportAot(){
    return sysCfg->value(upSupportAot,false).toBool();
}

// GetInterfaceName return dbus interface name
QString SettingService::GetInterfaceName(){
    return dbusSettingsInterface;
}

//TODO: SetSettings update dstore settings
void SettingService::SetSettings(QString key, QString value)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<SettingKey>();
    int enumValue = metaEnum.keyToValue(key.toStdString().c_str());

    switch (enumValue) {
    case SettingKey::AutoInstall:
        userCfg->setValue(lowAutoInstall,value);
        break;
    case SettingKey::ThemeName:
        userCfg->setValue(lowThemeName,value);
        break;
    case SettingKey::WindowState:
        userCfg->beginGroup(gWebWindow);
        userCfg->setValue(lowWindowState,value);
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
    QMetaEnum metaEnum = QMetaEnum::fromType<SettingKey>();
    int enumValue = metaEnum.keyToValue(key.toStdString().c_str());
    QVariant value;

    switch (enumValue) {
    case SettingKey::AutoInstall:
        value = getAutoInstall();
        break;
    case SettingKey::ThemeName:
        value = getThemeName();
        break;
    case SettingKey::WindowState:
        value = getWindowState();
        break;
    case SettingKey::Server:
        value = getServer();
        break;
    case SettingKey::MetadataServer:
        value = getMetadataServer();//Not tested
        break;
    case SettingKey::OperationServerMap:
        value = getOperationServerMap();//Not tested
        break;
    case SettingKey::DefaultRegion:
        value = getDefaultRegion();
        break;
    case SettingKey::SupportSignIn:
        value = getSupportSignIn();
        break;
    case SettingKey::UpyunBannerVisible:
        value = getUpyunBannerVisible();
        break;
    case SettingKey::AllowSwitchRegion:
        value = getAllowSwitchRegion();
        break;
    case SettingKey::AllowShowPackageName:
        value = getAllowShowPackageName();
        break;
    case SettingKey::SupportAot:
        value = getSupportAot();
        break;
    default:
        qDebug() << "Non-existent key value";
        break;
    }
    qDebug() << key << value;
    return QDBusVariant(value);
}

