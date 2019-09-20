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

#include <QIcon>
#include <QDBusError>
#include <QDBusConnection>

#include <DApplication>
#include <DLog>
#include <QIcon>
#include <DPlatformWindowHandle>

#include "base/consts.h"
#include "resources/images.h"
#include "resources/theme.h"
#include "services/dbus_manager.h"
#include "services/settings_manager.h"
#include "services/rcc_scheme_handler.h"
#include "ui/web_window.h"

int main(int argc, char **argv)
{

#ifndef DSTORE_NO_DXCB
    Dtk::Widget::DApplication::loadDXcbPlugin();
#endif

    Dtk::Widget::DApplication app(argc, argv);
    if (!Dtk::Widget::DPlatformWindowHandle::pluginVersion().isEmpty()) {
        app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    }

    auto themName = dstore::SettingsManager::instance()->themeName();
    app.setTheme(themName);
    app.setAttribute(Qt::AA_EnableHighDpiScaling, true);
    app.setWindowIcon(QIcon(dstore::kImageDeepinAppStore));
    app.setProductIcon(QIcon(dstore::kImageDeepinAppStore));
    app.setOrganizationName("deepin");
    app.setOrganizationDomain("deepin.org");
    app.setApplicationVersion(Dtk::Widget::DApplication::buildVersion("5.6.0.0"));
    app.setApplicationName(dstore::kAppName);
    app.loadTranslator();
    app.setApplicationDisplayName(QObject::tr("Deepin App Store"));
    app.setApplicationDescription(QObject::tr(
                                      "Deepin App Store is an App Store with diverse and quality applications. "
                                      "It features popular recommendations, newly updated apps and hot topics, and supports one-click installation, "
                                      "updating and uninstalling."));
    app.setApplicationAcknowledgementPage(
        "https://www.deepin.org/acknowledgments/deepin-app-store/");

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();

    // fix error for cutelogger
    // No appenders assotiated with category js
    auto category = "js";
    auto fileAppender = new Dtk::Core::RollingFileAppender(Dtk::Core::DLogManager::getlogFilePath());
    static Dtk::Core::Logger customLoggerInstance(category);
    customLoggerInstance.logToGlobalInstance(category, true);
    customLoggerInstance.registerAppender(fileAppender);

    dstore::DBusManager dbus_manager;
    if (dbus_manager.parseArguments()) {
        // Exit process after 1000ms.
        QTimer::singleShot(1000, [&]() {
            app.quit();
        });
        return app.exec();
    } else {
        dstore::WebWindow window;
        QObject::connect(&dbus_manager, &dstore::DBusManager::raiseRequested,
                         &window, &dstore::WebWindow::raiseWindow);
        QObject::connect(&dbus_manager, &dstore::DBusManager::showDetailRequested,
                         &window, &dstore::WebWindow::showAppDetail);

        app.installEventFilter(&window);

        window.loadPage();
        window.showWindow();

        return app.exec();
    }
}
