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

#ifndef SETTINGSERVICE_H
#define SETTINGSERVICE_H

#include <QObject>
#include "../dbus/daemondbus.h"
#include <QVariant>
#include <QSettings>
#include <QString>
#include <QProcessEnvironment>
#include <QFile>
#include <QVariantMap>
#include <QDBusVariant>
#include <QDebug>
#include <QBitArray>

namespace
{
//settings file path
const char appStoreConfPath[]        = "/usr/share/deepin-app-store/settings.ini";
const char appStoreConfPathDefault[] = "/usr/share/deepin-app-store/settings.ini.default";

//dbus interface and path
const char dbusSettingsInterface[] = "com.deepin.AppStore.Settings";
const char dbusSettingsPath[]      = "/com/deepin/AppStore/Settings";

//group
const char gGeneral[]         = "General";
const char gOperationServer[] = "OperationServer";
const char gWebWindow[]       = "WebWindow";

//lower case key
const char keyAutoInstall[]          = "autoInstall";
const char keyCurrentRegion[]        = "currentRegion";
const char keyThemeName[]            = "themeName";
const char keyWindowState[]          = "windowState";
const char keyAllowShowPackageName[] = "allowShowPackageName";

const char keyServer[]             = "Server";
const char keyMetadataServer[]     = "MetadataServer";
const char keySupportAot[]         = "SupportAot";
const char keySupportSignIn[]      = "SupportSignIn";
const char keyAllowSwitchRegion[]  = "AllowSwitchRegion";
const char keyDefaultRegion[]      = "DefaultRegion";
const char keyUpyunBannerVisible[] = "UpyunBannerVisible";

//upper case key
const char AutoInstall[]          = "AutoInstall";
const char ThemeName[]            = "ThemeName";
const char WindowState[]          = "WindowState";
const char AllowShowPackageName[] = "AllowShowPackageName";

const char Server[]              = "Server";
const char MetadataServer[]      = "MetadataServer";
const char OperationServerMap[]  = "OperationServerMap";
const char DefaultRegion[]       = "DefaultRegion";
const char AllowSwitchRegion[]   = "AllowSwitchRegion";
const char SupportSignIn[]       = "SupportSignIn";
const char SupportAot[]          = "SupportAot";
const char UpyunBannerVisible[]  = "UpyunBannerVisible";
}

class SettingService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", STOREDAEMON_SETTINGS_INTERFACE)

public:
    explicit SettingService(QObject *parent = nullptr);

private:
    void setUserSettings(QString group, QString key, QString value);

    QVariant getServer();

    QVariant getMetadataServer();

    QVariantMap getOperationServerMap();

    QVariant getSupportSignIn();

    QVariant getUpyunBannerVisible();

    QVariant getAllowSwitchRegion();

    QVariant getWindowState();

    QVariant getAutoInstall();

    QVariant getDefaultRegion();

    QVariant getThemeName();

    QVariant getAllowShowPackageName();

    QVariant getSupportAot();

    QSettings *sysCfg;

    QSettings *userCfg;
signals:

public slots:
    QString GetInterfaceName();

    QDBusVariant GetSettings(QString key);

    void SetSettings(QString key,QString value);
};


#endif // SETTINGSERVICE_H
