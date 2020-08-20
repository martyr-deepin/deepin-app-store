#pragma once
#include <QStringList>

class IAppPayCall
{
public:
    virtual void setPayStatus(const QString &appId,const int& status) = 0;
};

