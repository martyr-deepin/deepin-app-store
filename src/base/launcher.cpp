/*
 * Copyright (C) 2018 Deepin Technology Co., Ltd.
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

#include "launcher.h"

#include <QDebug>
#include <QProcess>
#include <QRegExp>
#include <QSettings>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatforminputcontext.h>
#include <private/qguiapplication_p.h>
#include <QGuiApplication>
#include <QPixmap>
#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <DHiDPIHelper>
#include <QIconEngine>
#include <QBuffer>

namespace dstore
{

using namespace Dtk::Widget;

QPixmap loadSvg(const QString &fileName, const int size)
{
    QPixmap pixmap(size, size);
    QSvgRenderer renderer(fileName);
    pixmap.fill(Qt::transparent);

    QPainter painter;
    painter.begin(&pixmap);
    renderer.render(&painter);
    painter.end();

    return pixmap;
}

QString GetThemeIconData(const QString &iconName, const int size)
{
    auto p = GetThemeIcon(iconName, size);
    QBuffer b;
    b.open(QIODevice::WriteOnly);
    p.save(&b, "png");
    return "data:image/png;base64," + b.data().toBase64();
}

QPixmap GetThemeIcon(const QString &iconName, const int size)
{
    const auto ratio = qApp->devicePixelRatio();
    const int s = (size);
    QPlatformTheme *const platformTheme = QGuiApplicationPrivate::platformTheme();

    QPixmap pixmap;
    do {
        if (QFile::exists(iconName)) {
            if (iconName.endsWith(".svg"))
                pixmap = loadSvg(iconName, s * ratio);
            else
                pixmap = DHiDPIHelper::loadNxPixmap(iconName);

            if (!pixmap.isNull())
                break;
        }

        QScopedPointer<QIconEngine> engine(platformTheme->createIconEngine(iconName));
        QIcon icon;

        if (!engine.isNull()) {
            qDebug() << engine;
            if (engine->isNull()) {
                icon = QIcon::fromTheme("application-x-desktop");
            }
            else {
                icon = QIcon::fromTheme(iconName, QIcon::fromTheme("application-x-desktop"));
            }
        }

        pixmap = icon.pixmap(QSize(s, s));
        if (!pixmap.isNull())
            break;

    }
    while (false);

    if (qFuzzyCompare(pixmap.devicePixelRatioF(), 1.)) {
        pixmap = pixmap.scaled(QSize(s, s) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        pixmap.setDevicePixelRatio(ratio);
    }

    return pixmap;
}

QString GetExecFromDesktop(const QString &filepath)
{
    QSettings settings(filepath, QSettings::IniFormat);
    settings.beginGroup("Desktop Entry");
    if (settings.contains("Exec")) {
        QString exec = settings.value("Exec").toString();
        exec.remove(QRegExp("%."));
        exec.remove(QRegExp("^\""));
        exec.remove(QRegExp(" *$"));
        return exec;
    }
    return QString();
}

QString GetIconFromDesktop(const QString &filepath)
{
    QString icon;
    QSettings settings(filepath, QSettings::IniFormat);
    settings.beginGroup("Desktop Entry");
    if (settings.contains("Icon")) {
         icon = settings.value("Icon").toString();
    }
    return icon;
}

bool ExecuteDesktopFile(const QString &filepath)
{
    const QString exec = GetExecFromDesktop(filepath);
    if (exec.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Failed to parse " << filepath;
        return false;
    }
    return QProcess::startDetached(exec);
}

}  // namespace