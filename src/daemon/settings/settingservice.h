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
#include <QVariant>
#include <QSettings>
#include <QString>
#include <QProcessEnvironment>
#include <QFile>
#include <QVariantMap>
#include <QDBusVariant>
#include <QDebug>
#include <QBitArray>
#include <QMetaEnum>
#include <DSysInfo>

#include "../dbus/daemondbus.h"
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
const char lowAutoInstall[]          = "autoInstall";
const char lowThemeName[]            = "themeName";
const char lowWindowState[]          = "windowState";
const char lowAllowShowPackageName[] = "allowShowPackageName";
const char lowCurrentRegion[]        = "currentRegion";

//upper case key
const char upAutoInstall[]          = "AutoInstall";
const char upThemeName[]            = "ThemeName";
const char upWindowState[]          = "WindowState";
const char upAllowShowPackageName[] = "AllowShowPackageName";

const char upServer[]             = "Server";
const char upMetadataServer[]     = "MetadataServer";
const char upSupportAot[]         = "SupportAot";
const char upSupportSignIn[]      = "SupportSignIn";
const char upAllowSwitchRegion[]  = "AllowSwitchRegion";
const char upDefaultRegion[]      = "DefaultRegion";
const char upUpyunBannerVisible[] = "UpyunBannerVisible";

}

class SettingService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", STOREDAEMON_SETTINGS_INTERFACE)

public:
    explicit SettingService(QObject *parent = nullptr);
    enum SettingKey{
        AutoInstall,
        ThemeName,
        WindowState,
        Server,
        MetadataServer,
        OperationServerMap,
        DefaultRegion,
        SupportSignIn,
        UpyunBannerVisible,
        AllowSwitchRegion,
        AllowShowPackageName,
        SupportAot
    };
    Q_ENUM(SettingKey)

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
