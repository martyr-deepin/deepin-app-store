#include <QCoreApplication>
#include "pkg_cache_implement.h"
#include "tcp_client.h"
#include <memory>
#include <unistd.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("deepin-app-store-pkgcache");

    //Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();

    QCommandLineParser parser;
    QCommandLineOption create({"c", "create"}, "create package cache.");
    QCommandLineOption update({"u", "update"}, "update package cache.");
    QCommandLineOption install({"i", "install"}, "install package cache.");
    QCommandLineOption remove({"r", "remove"}, "remove package cache.");
    parser.addOption(create);
    parser.addOption(update);
    parser.addOption(install);
    parser.addOption(remove);

    parser.process(app);

    //判断文件是否存在
    if (access("/usr/share/deepin-app-store/cache.db", F_OK|R_OK) != 0){
        pkg_cache_implement::instance()->createDataBase();
        return 0;
    }

    if(parser.isSet(create)) {//创建
        pkg_cache_implement::instance()->createDataBase();
    }
    else if(parser.isSet(update)){//更新
        pkg_cache_implement::instance()->updateDataBase();
    }
    else if(parser.isSet(install) && argc == 4){//指定包名安装
        pkg_cache_implement::instance()->updateDataBaseByInstall(argv[2],argv[3]);
    }
    else if(parser.isSet(remove) && argc == 4){//指定包名卸载
        pkg_cache_implement::instance()->updateDataBaseByRemove(argv[2]);
    }

    if(pkg_cache_implement::instance()->checkDaemonState())
    {//仅安装卸载时，才通知daemon
        if(argc == 4){
            std::string packageName{argv[2]};
            if(packageName.substr(0,3) == "lib" && packageName.find('.') == std::string::npos){
                return 0;
            }
        }


        std::unique_ptr<tcp_session> tcp(new tcp_session("0.0.0.0",29898));

        if(tcp->connect() == 0)
        {
            if(argc == 4)
            {
                std::ostringstream oss;
                oss << argv[1] << "##" << argv[2] << "##" << argv[3];
                std::string szData = oss.str();

                const char* buf = szData.c_str();
                ssize_t nDataLen = static_cast<qint64>(szData.length());

                ssize_t nSendSize = 0;
                while (nSendSize != nDataLen)
                {
                    ssize_t len_t = 0;
                    len_t = tcp->send(&buf[nSendSize], static_cast<size_t>(nDataLen-nSendSize));

                    if (len_t == -1)
                        return -1;

                    nSendSize += len_t;
                }
            }
            else if(argc == 2)
            {
                std::string szData = argv[1];
                const char* buf = szData.c_str();
                ssize_t nDataLen = static_cast<qint64>(szData.length());

                ssize_t nSendSize = 0;
                while (nSendSize != nDataLen)
                {
                    ssize_t len_t = 0;
                    len_t = tcp->send(&buf[nSendSize], static_cast<size_t>(nDataLen-nSendSize));

                    if (len_t == -1)
                        return -1;

                    nSendSize += len_t;
                }
            }

        }
    }


    return 0;
}

