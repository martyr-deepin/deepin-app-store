/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
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

#include "dbus/dbus_consts.h"

namespace dstore
{

const char kAppStoreDbusService[] = "com.deepin.AppStore";
const char kAppStoreDbusPath[] = "/com/deepin/AppStore";
const char kAppStoreDbusInterface[] = "com.deepin.AppStore";

const char kAppstoreDaemonService[] = "com.deepin.AppStore.Daemon";
const char kAppstoreDaemonPath[] = "/com/deepin/AppStore/Metadata";
const char kAppstoreDaemonInterface[] = "com.deepin.AppStore.Metadata";

const char kAppstoreDaemonSettingsPath[] = "/com/deepin/AppStore/Settings";
const char kAppstoreDaemonSettingsInterface[] = "com.deepin.AppStore.Settings";

const char kLastoreDebDbusPath[] = "/com/deepin/AppStore/Backend";
const char kLastoreDebDbusService[] = "com.deepin.AppStore.Daemon";
const char kLastoreDebJobService[] = "com.deepin.AppStore.Daemon";

const char kLicenseService[] = "com.deepin.license";
const char kLicenseInfoPath[] = "/com/deepin/license/Info";
const char kLicenseInfoInterface[] = "com.deepin.license.Info";

const char kAppearanceService[] = "com.deepin.daemon.Appearance";
const char kAppearancePath[] = "/com/deepin/daemon/Appearance";
const char kAppearanceInterface[] = "com.deepin.daemon.Appearance";

const char kNetworkService[] = "com.deepin.daemon.Network";
const char kLauncherPath[] = "/com/deepin/dde/daemon/Launcher";
const char kLauncherInterface[] = "com.deepin.dde.daemon.Launcher";
const char kNetworkProxyChainsPath[] = "/com/deepin/daemon/Network/ProxyChains";
const char kNetworkProxyChainsInterface[] = "com.deepin.daemon.Network.ProxyChains";
}  // namespace dstore
