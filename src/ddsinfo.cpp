#include "ddsinfo.h"

#include <QFile>

namespace
{
quint32 readU32(const QByteArray& data, int offset)
{
    if (offset < 0 || offset + 4 > data.size())
        return 0;
    const auto* p = reinterpret_cast<const unsigned char*>(data.constData() + offset);
    return quint32(p[0]) | (quint32(p[1]) << 8) | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
}

QString fourCcToString(quint32 cc)
{
    char s[5] = {
        char(cc & 0xFF),
        char((cc >> 8) & 0xFF),
        char((cc >> 16) & 0xFF),
        char((cc >> 24) & 0xFF),
        0
    };
    return QString::fromLatin1(s, 4);
}

QString dxgiName(quint32 dxgi)
{
    switch (dxgi)
    {
        case 28: return QStringLiteral("RGBA8");
        case 71: return QStringLiteral("BC1");
        case 72: return QStringLiteral("BC1 sRGB");
        case 74: return QStringLiteral("BC2");
        case 75: return QStringLiteral("BC2 sRGB");
        case 77: return QStringLiteral("BC3");
        case 78: return QStringLiteral("BC3 sRGB");
        case 80: return QStringLiteral("BC4");
        case 83: return QStringLiteral("BC5");
        case 95: return QStringLiteral("BC6H UF16");
        case 96: return QStringLiteral("BC6H SF16");
        case 98: return QStringLiteral("BC7");
        case 99: return QStringLiteral("BC7 sRGB");
        default: return QStringLiteral("DXGI %1").arg(dxgi);
    }
}
}

DdsInfo readDdsInfo(const QString& path)
{
    DdsInfo info;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        info.detail = QStringLiteral("Не удалось открыть файл");
        return info;
    }

    info.fileSize = quint64(f.size());
    const QByteArray data = f.read(148);
    if (data.size() < 128 || data.left(4) != QByteArray("DDS ", 4))
    {
        info.detail = QStringLiteral("Не DDS или повреждённый заголовок");
        return info;
    }

    const quint32 headerSize = readU32(data, 4);
    const quint32 pfSize = readU32(data, 76);
    if (headerSize != 124 || pfSize != 32)
    {
        info.detail = QStringLiteral("Некорректный DDS_HEADER");
        return info;
    }

    info.height = int(readU32(data, 12));
    info.width = int(readU32(data, 16));
    info.mipCount = int(readU32(data, 28));
    if (info.mipCount <= 0)
        info.mipCount = 1;

    const quint32 pfFlags = readU32(data, 80);
    const quint32 fourCC = readU32(data, 84);
    const quint32 rgbBits = readU32(data, 88);
    constexpr quint32 DDPF_ALPHAPIXELS = 0x1;
    constexpr quint32 DDPF_ALPHA = 0x2;
    constexpr quint32 DDPF_FOURCC = 0x4;
    constexpr quint32 DDPF_RGB = 0x40;

    info.hasAlphaCapability = (pfFlags & (DDPF_ALPHAPIXELS | DDPF_ALPHA)) != 0;

    if (pfFlags & DDPF_FOURCC)
    {
        const QString cc = fourCcToString(fourCC);
        if (cc == QLatin1String("DXT1"))
        {
            info.format = QStringLiteral("DXT1 / BC1");
            info.blockCompressed = true;
        }
        else if (cc == QLatin1String("DXT3"))
        {
            info.format = QStringLiteral("DXT3 / BC2");
            info.blockCompressed = true;
            info.hasAlphaCapability = true;
        }
        else if (cc == QLatin1String("DXT5"))
        {
            info.format = QStringLiteral("DXT5 / BC3");
            info.blockCompressed = true;
            info.hasAlphaCapability = true;
        }
        else if (cc == QLatin1String("ATI1") || cc == QLatin1String("BC4U"))
        {
            info.format = QStringLiteral("BC4");
            info.blockCompressed = true;
        }
        else if (cc == QLatin1String("ATI2") || cc == QLatin1String("BC5U"))
        {
            info.format = QStringLiteral("BC5");
            info.blockCompressed = true;
        }
        else if (cc == QLatin1String("DX10"))
        {
            info.dx10Header = true;
            if (data.size() >= 132)
            {
                const quint32 dxgi = readU32(data, 128);
                info.format = dxgiName(dxgi);
                info.blockCompressed = (dxgi >= 70 && dxgi <= 99 && dxgi != 73 && dxgi != 76 && dxgi != 79 && dxgi != 82 && dxgi != 85 && dxgi != 88 && dxgi != 91 && dxgi != 94 && dxgi != 97);
                info.hasAlphaCapability = info.hasAlphaCapability || dxgi == 28 || dxgi == 74 || dxgi == 75 || dxgi == 77 || dxgi == 78 || dxgi == 98 || dxgi == 99;
            }
            else
                info.format = QStringLiteral("DX10");
        }
        else
            info.format = QStringLiteral("FOURCC %1").arg(cc);
    }
    else if (pfFlags & DDPF_RGB)
    {
        info.format = QStringLiteral("RGB%1%2").arg(rgbBits).arg(info.hasAlphaCapability ? QStringLiteral(" + alpha") : QString());
    }
    else
        info.format = QStringLiteral("Неизвестный");

    info.valid = info.width > 0 && info.height > 0;
    if (info.valid)
        info.detail = QStringLiteral("OK");
    return info;
}
