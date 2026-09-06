#pragma once

#include <QString>
#include <QtGlobal>

struct DdsInfo
{
    bool valid = false;
    int width = 0;
    int height = 0;
    int mipCount = 0;
    QString format;
    QString detail;
    bool hasAlphaCapability = false;
    bool blockCompressed = false;
    bool dx10Header = false;
    quint64 fileSize = 0;
};

DdsInfo readDdsInfo(const QString& path);
