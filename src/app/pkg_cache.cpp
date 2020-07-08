#include <QCoreApplication>
#include <QMap>
#include <QDebug>
#include <QProcess>
#include <QVariantMap>
#include <QFile>
#include <QDir>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QCommandLineParser>
#include <QLocalSocket>

#include <DLog>

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

            QRegExp rx("[?\\ \\:]");
            for(int i=0;i<list.size();i++) {
                QString str = list.value(i);

                QStringList localList = str.split(rx, QString::SkipEmptyParts);
                QString type = localList.value(0);
                QString value = localList.value(1);

                if(type == "Package") {
                    appMap.insert("appID",value);
                    appMap.insert("localVer",versionList.value(value));
                }
                else if(type == "Version") {
                    appMap.insert("remoteVer",value);
                }
                else if(type == "Architecture") {
                    appMap.insert("appArch",value);
                }
                else if(type == "Installed-Size") {
                    appMap.insert("installSize",value);
                }
                else if(type == "Size") {
                    appMap.insert("debSize",value);
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
        db = QSqlDatabase::addDatabase("QSQLITE", "create");
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
                ( id      char(50)  NOT NULL ,      \
                  localver    char(20)  NULL ,  \
                  remotever char(20)  NULL ,        \
                  arch    char(10)  NULL ,          \
                  installsize   char(20)   NULL ,   \
                  debsize     char(20)  NULL ,         \
                  binpath char(100)  NULL  ,         \
                  PRIMARY KEY (id)                    \
                );";
        query.prepare(creatTableStr);
        if(!query.exec()) {
            qDebug()<<query.lastError();
            return false;
        }
        query.finish();

        QSqlTableModel model(nullptr,db);
        model.setTable("Packages");
        model.setEditStrategy(QSqlTableModel::OnManualSubmit);
        foreach (const QVariant &var, storeList) {
           QMap<QString,QVariant> map = var.toMap();

           model.insertRow(0); //在0行添加1条记录
           model.setData(model.index(0,0),map.value("appID").toString()); //id字段在第0列上
           model.setData(model.index(0,1),map.value("localVer").toString());
           model.setData(model.index(0,2),map.value("remoteVer").toString());
           model.setData(model.index(0,3),map.value("appArch").toString());
           model.setData(model.index(0,4),map.value("installSize").toString());
           model.setData(model.index(0,5),map.value("debSize").toString());
           model.setData(model.index(0,6),"/opt/apps/");

           if(!model.submitAll())
           {
               qDebug()<<model.lastError()<<model.rowCount();
           }
        }

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
        db = QSqlDatabase::addDatabase("QSQLITE", "update");
    }

    db.setDatabaseName("/usr/share/deepin-app-store/cache.db");
    db.setUserName("root");
    db.setPassword("deepin-app-store");

    bool isOk = db.open();
    if(!isOk) {
        qDebug()<<db.lastError();
        exit(-1);
    } else {
        QSqlTableModel model(nullptr,db);
        model.setTable("Packages");
        model.setEditStrategy(QSqlTableModel::OnManualSubmit);
        QVariantMap::iterator i;
        for (i = storeList.begin(); i != storeList.end(); ++i) {
           QMap<QString,QVariant> map = i.value().toMap();

           QString appId = map.value("appID").toString();
           model.setFilter(QString("id='%1'").arg(appId));
           model.select();
           if (model.rowCount() < 1) {
               model.insertRow(0); //在0行添加1条记录
               model.setData(model.index(0,0),appId); //id字段在第0列上
           }
           model.setData(model.index(0,1),map.value("localVer").toString());
           model.setData(model.index(0,2),map.value("remoteVer").toString());
           model.setData(model.index(0,3),map.value("appArch").toString());
           model.setData(model.index(0,4),map.value("installSize").toString());
           model.setData(model.index(0,5),map.value("debSize").toString());
           model.setData(model.index(0,6),"/opt/apps/");
           if(!model.submitAll())
           {
               qDebug()<<model.lastError();
           }
        }
    }
    db.close();
    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("deepin-app-store-pkgcache");

    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();

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

