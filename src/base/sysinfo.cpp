#include "sysinfo.h"

#include <DSysInfo>

#include <sys/utsname.h>

SysInfo::SysInfo(QObject *parent)
    : QObject(parent)
{

}

QString SysInfo::arch() const
{
    struct utsname name{};

    if (uname(&name) == -1) {
        return "";
    }

    auto machine = QString::fromLatin1(name.machine);

    // map arch
    auto archMap = QMap<QString, QString>{
        {"x86_64", "amd64"},
        {"aarch64", "arm64"},
        {"mips64", "mips64"},
    };
    auto arch = archMap[machine];
    if (arch.isEmpty()) {
        arch = machine;
    }
    return arch;
}

QString SysInfo::product() const
{
    const auto defaultProduct = "community";

#ifdef DTK_VERSION
#if DTK_VERSION >= 84017667
    using UosType = Dtk::Core::DSysInfo::UosType;
    using EditionType = Dtk::Core::DSysInfo::UosEdition;

    auto uosType = Dtk::Core::DSysInfo::uosType();
    if(uosType == UosType::UosServer)
    {
        return "server";
    }
    else if(uosType == UosType::UosDesktop)
    {
        auto uosEdition = Dtk::Core::DSysInfo::uosEditionType();

        if(uosEdition == EditionType::UosProfessional)
        {
            return "professional";
        }
        else if(uosEdition == EditionType::UosHome)
        {
            return "personal";
        }
        else if(uosEdition == EditionType::UosCommunity)
        {
            return "community";
        }
    }

#else
    switch (Dtk::Core::DSysInfo::deepinType())
    {
    case Dtk::Core::DSysInfo::DeepinDesktop:
        return "community";
    case Dtk::Core::DSysInfo::DeepinProfessional:
        return "professional";
    case Dtk::Core::DSysInfo::DeepinPersonal:
        return "personal";
    case Dtk::Core::DSysInfo::DeepinServer:
        return "server";
    default:
        return "community";
    }


#endif
#endif


    return defaultProduct;
}

// desktop/tablet
QString SysInfo::desktopMode() const
{
    return "desktop";
}
