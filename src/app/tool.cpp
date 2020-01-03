#include <QCoreApplication>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QDirIterator>
#include <QProcess>

#include <unistd.h>

QList<QJsonObject> enumAppInfoList()
{
    QList<QJsonObject> appInfoList;
    QDir apps("/opt/apps");
    auto list = apps.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (auto &appID: list) {
        auto infoPath = apps.absoluteFilePath(appID + "/info");
        QFile infoFile(infoPath);
        if (!infoFile.open(QIODevice::ReadOnly)) {
           continue;
        }
        auto doc = QJsonDocument::fromJson(infoFile.readAll());
        appInfoList.push_back(doc.object());
    }
    return appInfoList;
}

void linkDir(const QString &source, const QString &target)
{
    auto ensureTargetDir = [](const QString &targetFile)
    {
        QFileInfo t(targetFile);
        QDir tDir(t.dir());
        tDir.mkpath(".");
    };

    QDir sourceDir(source);
    QDir targetDir(target);
    QDirIterator iter(source, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        auto sourceFile = iter.next();
        auto targetFile = targetDir.absoluteFilePath(sourceDir.relativeFilePath(sourceFile));

        QFileInfo tfi(targetFile);
        if (tfi.isSymLink() && (tfi.canonicalFilePath() == sourceFile)) {
            continue;
        }
        ensureTargetDir(targetFile);
        auto ret = symlink(sourceFile.toStdString().c_str(), targetFile.toStdString().c_str());
        if (0 != ret) {
            qDebug() << "link failed" << sourceFile << "=>" << targetFile << ret;
        }
    }
}

void linkApp(const QJsonObject &app)
{
    auto appID = app.value("appid").toString();
    auto appEntriesDir = QDir("/opt/apps/" + appID + "/entries");
    auto autoStartDir = QDir(appEntriesDir.absoluteFilePath("autostart"));

    bool autoStart = app.value("permissions").toObject().value("autostart").toBool();
    if (autoStart) {
        linkDir(appEntriesDir.absoluteFilePath("autostart"), "/etc/xdg/autostart");
    }

    // link application
    auto sysShareDir = QDir("/usr/share");
    linkDir(appEntriesDir.absoluteFilePath("applications"), sysShareDir.absoluteFilePath("applications"));
    linkDir(appEntriesDir.absoluteFilePath("icons"), sysShareDir.absoluteFilePath("icons"));
    linkDir(appEntriesDir.absoluteFilePath("mime"), sysShareDir.absoluteFilePath("mime"));
    linkDir(appEntriesDir.absoluteFilePath("glib-2.0"), sysShareDir.absoluteFilePath("glib-2.0"));
    linkDir(appEntriesDir.absoluteFilePath("services"), sysShareDir.absoluteFilePath("dbus-1/services"));
    linkDir(appEntriesDir.absoluteFilePath("GConf"), sysShareDir.absoluteFilePath("GConf"));
}

void cleanLink()
{
    auto cleanDirBrokenLink = [](const QString &dir)
    {
        QProcess p;
        auto cmd = "find " + dir + " -xtype l -delete";
        p.start("bash", QStringList{"-c", cmd});
        p.waitForFinished();
    };

    auto sysShareDir = QDir("/usr/share");
    cleanDirBrokenLink(sysShareDir.absoluteFilePath("applications"));
    cleanDirBrokenLink(sysShareDir.absoluteFilePath("icons"));
    cleanDirBrokenLink(sysShareDir.absoluteFilePath("mime"));
    cleanDirBrokenLink(sysShareDir.absoluteFilePath("glib-2.0"));
    cleanDirBrokenLink(sysShareDir.absoluteFilePath("dbus-1/services"));
    cleanDirBrokenLink(sysShareDir.absoluteFilePath("/etc/xdg/autostart"));
}

void update()
{
    QProcess p;
    auto cmd = "glib-compile-schemas /usr/share/glib-2.0/schemas/";
    p.start("bash", QStringList{"-c", cmd});
    p.waitForFinished();

    cmd = "update-icon-caches /usr/share/icons/*";
    p.start("bash", QStringList{"-c", cmd});
    p.waitForFinished();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    for (auto &a:enumAppInfoList()) {
        linkApp(a);
    }
    // clean
    cleanLink();

    // trigger
    update();

    return 0;
}