#include "sysinfo.h"

#include <DSysInfo>

#include <sys/utsname.h>

SysInfo::SysInfo(QObject *parent) : QObject(parent)
{

}

QString SysInfo::arch() const
{
    struct utsname name;

    if (uname(&name) == -1) {
        return  "";
    }

    return QString::fromLatin1(name.machine);
}


QString SysInfo::product() const
{
    const auto defaultProduct = "community";
    switch (Dtk::Core::DSysInfo::deepinType()) {
    case Dtk::Core::DSysInfo::DeepinDesktop:
        return "community";
    case Dtk::Core::DSysInfo::DeepinProfessional:
        return "professional";
    default:
        return defaultProduct;
//        return "unknown";'
    }
    return defaultProduct;
}

// desktop/tablet
QString SysInfo::desktopMode() const
{
    return "desktop";
}
