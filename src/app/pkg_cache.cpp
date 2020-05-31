#include <QCoreApplication>
#include <QMap>
#include <QDebug>
#include <QProcess>
#include <QVariantMap>
#include <QFile>
#include <QDir>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QCommandLineParser>

#include <QLocalSocket>

bool getInstalledList(QMap<QString,QString> &versionList)
{
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    qputenv("LC_ALL", "C");
    process.setProcessEnvironment(env);

    process.start("/usr/bin/dpkg-query",
                  QStringList()<<"--show"<<"-f"
                  <<"${Package}\\t${db:Status-Abbrev}\\t${Version}\\n");
    QString dpkgQuery;
    if(process.waitForFinished()) {
        dpkgQuery = QString(process.readAll());
    }

    //accountsservice\tii \t0.6.45-2\t455\nacl\tii \t2.2.53-4\t206\n
    QStringList installedList = dpkgQuery.split('\n');
    foreach (QString line, installedList) {
        QStringList installedDetialList = line.split('\t');

        if(installedDetialList.value(1).startsWith("ii")) {
            QString id = installedDetialList.value(0);
            QStringList fullPackageName = id.split(':');
            QString fuzzyPackageName = fullPackageName[0];
            versionList.insert(fuzzyPackageName,installedDetialList.value(2));
        }
    }

    return true;
}


bool getStoreList(QVariantMap &storeList,QMap<QString,QString> &versionList)
{
    //get app store url
    QString appstore = QString("/etc/apt/sources.list.d/appstore.list");
    QFile storeFile(appstore);
    if(!storeFile.exists())
        return false;
    if (!storeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString storeUrl;
    while (!storeFile.atEnd()) {
      QString line = storeFile.readLine();
      if(line.startsWith("deb"))
      {
          storeUrl = line.section('/',2,2);
      }
    }
    storeFile.close();

    //get all package list
    QDir dir("/var/lib/apt/lists/");
    if(!dir.exists()) {
        qDebug()<<dir.absolutePath()<<"is not exist";
        return false;
    }
    QStringList filters;
    filters << storeUrl+"_appstore*_Packages";
    dir.setNameFilters(filters);
    QFileInfoList infoList = dir.entryInfoList();
    for (int i=0;i<infoList.size();i++) {
        QString cacheFile = infoList.value(i).absoluteFilePath();

        QFile file(cacheFile);
        if(!file.open(QIODevice::ReadOnly))
            qDebug()<<"open fail";
        QString data = file.readAll();
        file.close();
        QStringList packageList = data.split("\n\n", QString::SkipEmptyParts);

        for (int i = 0; i < packageList.size(); ++i) {
            QStringList list = packageList.value(i).split('\n');
            QMap<QString,QVariant> appMap;
            appMap.clear();
            for(int i=0;i<list.size();i++) {
                QString str = list.value(i);
                if(str.startsWith("Package: ")) {
                    QString appId = str.section("Package: ",1,1);
                    appMap.insert("appID",appId);
                    appMap.insert("localVer",versionList.value(appId));
                }
                else if(str.startsWith("Version: ")) {
                    appMap.insert("remoteVer",str.section("Version: ",1,1));
                }
                else if(str.startsWith("Architecture: ")) {
                    appMap.insert("appArch",str.section("Architecture: ",1,1));
                }
                else if(str.startsWith("Installed-Size: ")) {
                    appMap.insert("installSize",str.section("Installed-Size: ",1,1));
                }
                else if(str.startsWith("Size: ")) {
                    appMap.insert("debSize",str.section("Size: ",1,1));
                }
            }
            storeList.insert(appMap.value("appID").toString(),QVariant(appMap));
        }
    }
    return true;
}

bool createDatabase(QVariantMap &storeList)
{
    QString dbPath = QString("/usr/share/deepin-app-store/");

    QDir dir(dbPath);
    if(!dir.exists())
        dir.mkpath(dbPath);
    dir.remove("cache.db");

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","create");
    if (QSqlDatabase::contains("create")) {
        db = QSqlDatabase::database("create");
    } else {
        db = QSqlDatabase::addDatabase("QMYSQL", "create");
    }
    db.setDatabaseName(dbPath+"/cache.db");
    db.setUserName("root");
    db.setPassword("deepin-app-store");

    bool isOk = db.open();
    if(!isOk) {
        qDebug()<<db.lastError();
        return false;
    }
    else {
        QSqlQuery query(db);
        QString creatTableStr = "CREATE TABLE Packages  \
                (                                       \
                  pkg_id      char(50)  NOT NULL ,      \
                  pkg_localver    char(20)  NULL ,  \
                  pkg_remotever char(20) NOT NULL ,        \
                  pkg_arch    char(10)  NULL ,          \
                  pkg_installsize   char(20)   NULL ,   \
                  pkg_size     char(20)  NULL ,         \
                  pkg_binpath char(100)  NULL  ,         \
                  PRIMARY KEY (pkg_id)                    \
                );";
        query.prepare(creatTableStr);
        if(!query.exec()) {
            qDebug()<<query.lastError();
            return false;
        }
        query.finish();

        db.transaction();
        QVariantMap::iterator i;
        for (i = storeList.begin(); i != storeList.end(); ++i) {
           QMap<QString,QVariant> map = i.value().toMap();
           QSqlQuery query(db);
           query.prepare("INSERT INTO Packages (pkg_id, pkg_localver, pkg_remotever,pkg_arch,pkg_installsize,pkg_size,pkg_binpath) "
                         "VALUES (?,?,?,?,?,?,?)");
           query.bindValue(0, map.value("appID").toString());
           query.bindValue(1, map.value("localVer").toString());
           query.bindValue(2, map.value("remoteVer").toString());
           query.bindValue(3, map.value("appArch").toString());
           query.bindValue(4, map.value("installSize").toString());
           query.bindValue(5, map.value("debSize").toString());
           query.bindValue(6, "");
           query.exec();
           query.finish();
        }
        db.commit();
        db.close();
    }

    return true;
}

bool updateDatabase(QVariantMap &storeList)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","update");

    if (QSqlDatabase::contains("update")) {
        db = QSqlDatabase::database("update");
    } else {
        db = QSqlDatabase::addDatabase("QMYSQL", "update");
    }
    db.setDatabaseName("/usr/share/deepin-app-store/cache.db");
    db.setUserName("root");
    db.setPassword("deepin-app-store");

    bool isOk = db.open();
    if(!isOk) {
        qDebug()<<db.lastError();
        return false;
    }
    else {
        db.transaction();
        QVariantMap::iterator i;
        for (i = storeList.begin(); i != storeList.end(); ++i) {
           QMap<QString,QVariant> map = i.value().toMap();
           QSqlQuery query(db);
           QString sql = QString("INSERT INTO Packages(pkg_id, pkg_localver, pkg_remotever,pkg_arch,pkg_installsize,pkg_size,pkg_binpath) "
                                 "VALUES(%1,%2,%3,%4,%5,%6,%7) "
                                 "ON DUPLICATE KEY "
                                 "UPDATE pkg_localver=%2, pkg_remotever=%3,pkg_arch=%4,pkg_installsize=%5,pkg_size=%6,pkg_binpath=%7")
                   .arg(map.value("appID").toString()).arg(map.value("localVer").toString()).arg(map.value("remoteVer").toString())
                   .arg(map.value("appArch").toString()).arg(map.value("installSize").toString()).arg(map.value("debSize").toString())
                   .arg("");
           query.prepare(sql);
           query.exec();
           query.finish();
        }
        db.commit();
        db.close();
    }

    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QMap<QString,QString> versionList;
    getInstalledList(versionList);
    QVariantMap storeList;
    getStoreList(storeList,versionList);
    QCommandLineParser parser;
    QCommandLineOption update({"u", "update"}, QObject::tr("update package cache."));
    parser.addOption(update);

    QCommandLineOption create({"c", "create"}, QObject::tr("create package cache."));
    parser.addOption(create);

    parser.process(app);

    if(parser.isSet(update)) {
        updateDatabase(storeList);
    }
    else {
        createDatabase(storeList);
    }

    QLocalSocket localSocket;
    localSocket.connectToServer("ServerName");

    return 0;
}
