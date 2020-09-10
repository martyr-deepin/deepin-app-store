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

#include "services/settings_manager.h"

#include <QDebug>
#include <QFont>
#include <QSettings>
#include <QDBusReply>
#include <QDBusInterface>
#include <QApplication>
#include <QFontInfo>
#include <QCommandLineParser>

#include <DGuiApplicationHelper>

#include "dbus/dbus_consts.h"
#include "base/file_util.h"

namespace dstore
{

namespace
{
const char kAutoInstall[] = "AutoInstall";

const char kThemeName[] = "ThemeName";

const char kWindowState[] = "WindowState";

const char kAllowShowPackageName[] = "AllowShowPackageName";

const char kSupportAot[] = "SupportAot";

const char kSupportSignin[] = "SupportSignIn";

const char kServer[] = "Server";

const char kMetadataServer[] = "MetadataServer";

const char kOperationServerMap[] = "OperationServerMap";

const char kDefaultRegion[] = "DefaultRegion";

const char kAllowSwitchRegion[] = "AllowSwitchRegion";

const char kUpyunBannerVisible[] = "UpyunBannerVisible";

const char kProductName[] = "ProductName";
}

SettingsManager::SettingsManager(QObject *parent)
{
    settings_ifc_ = new QDBusInterface(
                kAppstoreDaemonService,
                kAppstoreDaemonSettingsPath,
                kAppstoreDaemonSettingsInterface,
                QDBusConnection::sessionBus(),
                parent);
    qDebug() << "connect" << kAppstoreDaemonInterface << settings_ifc_->isValid();

    authorizationState_ifc_ = new  QDBusInterface(
                kLicenseService,
                kLicenseInfoPath,
                kLicenseInfoInterface,
                QDBusConnection::systemBus(),
                parent);
    hasActivatorClient = authorizationState_ifc_->isValid();
    qDebug() << "connect" << kLicenseInfoInterface << hasActivatorClient;

    connect(authorizationState_ifc_, SIGNAL(LicenseStateChange()),
            this, SLOT(authStateChange()));

    appearance_ifc_ = new QDBusInterface(
                kAppearanceService,
                kAppearancePath,
                kAppearanceInterface,
                QDBusConnection::sessionBus(),
                parent);
    qDebug() << "connect" << kAppearanceInterface << appearance_ifc_->isValid();

    QDBusConnection::sessionBus().connect(
                kAppearanceService,
                kAppearancePath,
                "org.freedesktop.DBus.Properties",
                QLatin1String("PropertiesChanged"),
                this,
                SLOT(onActiveColor(QString,QMap<QString,QVariant>,QStringList)));
}

SettingsManager::~SettingsManager()
{

}

QString SettingsManager::arch() const
{
    return sysinfo.arch();
}

QString SettingsManager::product() const
{
    return sysinfo.product();
}

QString SettingsManager::desktopMode() const
{
    return sysinfo.desktopMode();
}

quint32 SettingsManager::authorizationState() const
{
    quint32 reply = authorizationState_ifc_->property("AuthorizationState").toUInt();
    if (reply > 4) {
        qWarning() << "query authorization state is not valid";
    }

    SysInfo sysinfo;
    auto systemType = sysinfo.product();

    qDebug() << "systemType = " << systemType;
    qDebug() << "reply = " << reply;

    if(!hasActivatorClient)
    {
        qDebug() << "hasActivatorClient = nullptr";
        return AuthorizationState::Authorized;
    }

    if( (systemType == "professional" && (reply == AuthorizationState::TrialExpired || reply == AuthorizationState::Notauthorized)) ||
            (systemType == "personal" && (reply == AuthorizationState::TrialExpired || reply == AuthorizationState::Notauthorized))){

        qDebug() << "AuthorizationState::Notauthorized";

        return AuthorizationState::Notauthorized;
    }
    else {
        qDebug() << "AuthorizationState::Authorized";

        return AuthorizationState::Authorized;
    }

}

QString SettingsManager::GUIFramework() const
{
    return QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");;
}

QString SettingsManager::productName() const
{
    return getSettings(kProductName).toString();
}

QString SettingsManager::getAppDetailID() const
{
    return  appDetialID;
}

void SettingsManager::setAppDetailID(QString appid)
{
    appDetialID = appid;
}

void SettingsManager::authStateChange()
{
    emit this->authStateChanged();
}

QString SettingsManager::existRecommend(const QString& filePath) const
{
    if(QFile::exists(filePath))
    {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString fileData = QTextStream(&file).readAll();
            file.close();

            return fileData;
        }
    }

    return QString{};
}

bool SettingsManager::remoteDebug()
{
    return true;
}

QString SettingsManager::server() const
{
    return getSettings(kServer).toString();
}

QString SettingsManager::metadataServer() const
{
    return getSettings(kMetadataServer).toString();
}

QVariantMap SettingsManager::operationServerMap() const
{
    return getMapSettings(kOperationServerMap);
}

QString SettingsManager::defaultRegion() const
{
    return getSettings(kDefaultRegion).toString();
}

bool SettingsManager::autoInstall() const
{
    return getSettings(kAutoInstall).toBool();
}

void SettingsManager::setAutoInstall(bool autoinstall)
{
    setSettings(kAutoInstall, autoinstall);
}

QString SettingsManager::getActiveColor() const
{
    return appearance_ifc_->property("QtActiveColor").toString();
}

void SettingsManager::onActiveColor(QString str, QMap<QString, QVariant> map, QStringList list)
{
    Q_UNUSED(str);
    Q_UNUSED(list);
    if(!map.contains("QtActiveColor"))
        return;
    Q_EMIT activeColorChanged(map.value("QtActiveColor").toString());
}

QString SettingsManager::themeName() const
{
    auto themeType = Dtk::Gui::DGuiApplicationHelper::instance()->themeType();

    switch (themeType) {
    case Dtk::Gui::DGuiApplicationHelper::UnknownType:
        return getSettings(kThemeName).toString();
    case Dtk::Gui::DGuiApplicationHelper::DarkType:
        return "dark";
    case Dtk::Gui::DGuiApplicationHelper::LightType:
    default:
        return "light";
    }
}

//get the initial theme name when the program start,avoid DGuiApplicationHelper executing before DApplication
QString SettingsManager::appThemeName() const
{
    return getSettings(kThemeName).toString();
}

void SettingsManager::setThemeName(const QString &themeName) const
{
    setSettings(kThemeName, themeName);
}

QByteArray SettingsManager::windowState() const
{
    auto base64str = getSettings(kWindowState).toString();
    return QByteArray::fromBase64(base64str.toLatin1());
}

void SettingsManager::setWindowState(QByteArray data)
{
    setSettings(kWindowState, QString(data.toBase64()));
}

bool SettingsManager::supportSignIn() const
{
    return getSettings(kSupportSignin).toBool();
}

bool SettingsManager::supportAot() const
{
    return getSettings(kSupportAot).toBool();
}

bool SettingsManager::allowSwitchRegion() const
{
    return getSettings(kAllowSwitchRegion).toBool();
}

bool SettingsManager::allowShowPackageName() const
{
    return getSettings(kAllowShowPackageName).toBool();
}

bool SettingsManager::upyunBannerVisible() const
{
    return getSettings(kUpyunBannerVisible).toBool();
}

typedef QVariantMap OperationInfo;

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                OperationInfo &info)
{
    QString key;
    QString value;
    argument.beginMapEntry();
    argument >> key >> value;
    argument.endMapEntry();
    info[key] = value;
    return argument;
}

// a{ss}
QVariantMap SettingsManager::getMapSettings(const QString &key) const
{
    OperationInfo operationInfo;
    QDBusMessage reply = settings_ifc_->call("GetSettings", key);
    if (!reply.errorName().isEmpty()) {
        qWarning() << reply.errorName() << reply.errorMessage();
        return operationInfo;
    }
    QList<QVariant> outArgs = reply.arguments();
    QVariant first = outArgs.value(0);
    QDBusVariant dbvFirst = first.value<QDBusVariant>();
    QVariant vFirst = dbvFirst.variant();
    QDBusArgument dbusArgs = vFirst.value<QDBusArgument>();

    dbusArgs.beginArray();
    while (!dbusArgs.atEnd()) {
        dbusArgs >> operationInfo;
    }
    dbusArgs.endArray();
    return operationInfo;

}

QVariant SettingsManager::getSettings(const QString &key) const
{
    QDBusReply<QVariant> reply = settings_ifc_->call("GetSettings", key);
    if (reply.error().isValid()) {
        qWarning() << "getSettings failed" << key << reply.error();
    }
    return reply.value();
}

void SettingsManager::setSettings(const QString &key, const QVariant &value) const
{
    QDBusReply<void> reply = settings_ifc_->call("SetSettings", key, value);
    if (reply.error().isValid()) {
        qWarning() << "setSettings failed" << key << reply.error() << value;
    }
}

QString SettingsManager::fontFamily() const
{
    return QApplication::font().family();
}

int SettingsManager::fontPixelSize() const
{
    QFontInfo fontInfo(QApplication::font());
    return fontInfo.pixelSize();
}

}  // namespace dstore
