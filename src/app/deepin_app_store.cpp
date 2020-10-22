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

#include <DApplication>
#include <DLog>
#include <QIcon>
#include <QDBusError>
#include <QDBusConnection>
#include <DGuiApplicationHelper>
#include <DPlatformWindowHandle>
#include <DApplicationSettings>

#include "base/consts.h"
#include "resources/images.h"
#include "resources/theme.h"
#include "services/dbus_manager.h"
#include "services/settings_manager.h"
#include "services/rcc_scheme_handler.h"
#include "ui/web_window.h"
#include "base/sysinfo.h"

int main(int argc, char **argv)
{
    //Disable function: Qt::AA_ForceRasterWidgets, solve the display problem of domestic platform (loongson mips)
    qputenv("DTK_FORCE_RASTER_WIDGETS", "FALSE");
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-web-security");
#ifndef DSTORE_NO_DXCB
    Dtk::Widget::DApplication::loadDXcbPlugin();
#endif

    Dtk::Widget::DApplication app(argc, argv);
    if (!Dtk::Widget::DPlatformWindowHandle::pluginVersion().isEmpty()) {
        app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    }

    app.setAttribute(Qt::AA_UseSoftwareOpenGL);

    auto themName = dstore::SettingsManager::instance()->themeName();
    app.setAttribute(Qt::AA_EnableHighDpiScaling, true);
    app.setWindowIcon(QIcon(dstore::kImageDeepinAppStore));
    app.setProductIcon(QIcon(dstore::kImageDeepinAppStore));
    app.setOrganizationName("deepin");
    app.setOrganizationDomain("deepin.com");
    app.setApplicationVersion(Dtk::Widget::DApplication::buildVersion("5.6.0.0"));
    app.setApplicationName(dstore::kAppName);
    app.loadTranslator();
    app.setApplicationDisplayName(QObject::tr("App Store"));
    app.setApplicationDescription(QObject::tr(
                                      "App Store is a software center with diverse and quality applications, supporting installation and uninstallation with one click."
                                      ));
    app.setApplicationAcknowledgementPage(
                "https://www.deepin.org/acknowledgments/deepin-app-store/");

    SysInfo systeminfo;
    if(systeminfo.product() == "professional" || systeminfo.product() == "personal") {
        if (QLocale::system().name() == "zh_CN") {
            app.setApplicationLicense("<a href = https://www.uniontech.com/agreement/privacy-cn>" + QObject::tr("UnionTech Software Privacy Policy") + "</a>");
        }else {
            app.setApplicationLicense("<a href = https://www.uniontech.com/agreement/privacy-en> <i>" + QObject::tr("UnionTech Software Privacy Policy") + "</i> </a>");
        }
    }
    else if (systeminfo.product() == "community") {
//        if (QLocale::system().name() == "zh_CN") {
//            app.setApplicationLicense("<a href = https://www.uniontech.com/agreement/privacy-cn>《统信软件隐私政策》</a>");
//        }else {
//            app.setApplicationLicense("<a href = https://www.uniontech.com/agreement/privacy-en>《UnionTech Software Privacy Policy》</a>");
//        }
    }


    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();

    if (!DGuiApplicationHelper::setSingleInstance("com.deepin.AppStore")) {
        qWarning() << "another store is running";
        return 0;
    }
    app.setAutoActivateWindows(true);

    // fix error for cutelogger
    // No appenders associated with category js
    auto category = "js";
    auto fileAppender = new Dtk::Core::RollingFileAppender(Dtk::Core::DLogManager::getlogFilePath());
    static Dtk::Core::Logger customLoggerInstance(category);
    customLoggerInstance.logToGlobalInstance(category, true);
    customLoggerInstance.registerAppender(fileAppender);

    dstore::DBusManager dbus_manager;
    if (!dbus_manager.registerDBus()) {
        // register failed, another store running
        // Exit process after 1000ms.
        QTimer::singleShot(1000, [&]()
        {
            app.quit();
        });
        return app.exec();
    }

    Dtk::Widget::DApplicationSettings savetheme;

    dstore::WebWindow window;

    window.registerWnd();
    window.setupDaemon(&dbus_manager);

    app.installEventFilter(&window);

    window.loadPage();
    window.showWindow();

    return app.exec();
}
